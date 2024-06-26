//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Apr  4 23:11:06 PDT 2024
// Last Modified: Sun Apr 21 15:35:52 PDT 2024
// Filename:      tool-humbreak.cpp
// URL:           https://github.com/craigsapp/humlib/blob/master/src/tool-humbreak.cpp
// Syntax:        C++11; humlib
// vim:           ts=3 noexpandtab
//
// Description:   Insert line/system breaks and page breaks before input measures.
//
// Options:
//                -m measures == comma-delimited list of measures to add line breaks before
//                -p measures == comma-delimited list of measures to add page breaks before
//                -g label    == use the given label for breaks (defualt "original")
//                -r          == remove line/page breaks
//                -l          == convert page breaks to line breaks
//

#include "tool-humbreak.h"
#include "HumRegex.h"

#include <map>
#include <vector>

using namespace std;

namespace hum {

// START_MERGE


/////////////////////////////////
//
// Tool_humbreak::Tool_humbreak -- Set the recognized options for the tool.
//

Tool_humbreak::Tool_humbreak(void) {
	define("m|measures=s",             "measures numbers to place linebreaks before");
	define("p|page-breaks=s",          "measure numbers to place page breaks before");
	define("g|group=s:original",       "line/page break group");
	define("r|remove|remove-breaks=b", "remove line/page breaks");
	define("l|page-to-line-breaks=b",  "convert page breaks to line breaks");
}



/////////////////////////////////
//
// Tool_humbreak::run -- Do the main work of the tool.
//

bool Tool_humbreak::run(HumdrumFileSet& infiles) {
	bool status = true;
	for (int i=0; i<infiles.getCount(); i++) {
		status &= run(infiles[i]);
	}
	return status;
}


bool Tool_humbreak::run(const string& indata, ostream& out) {
	HumdrumFile infile(indata);
	bool status = run(infile);
	if (hasAnyText()) {
		getAllText(out);
	} else {
		out << infile;
	}
	return status;
}


bool Tool_humbreak::run(HumdrumFile& infile, ostream& out) {
	bool status = run(infile);
	if (hasAnyText()) {
		getAllText(out);
	} else {
		out << infile;
	}
	return status;
}


bool Tool_humbreak::run(HumdrumFile& infile) {
	processFile(infile);
	return true;
}



//////////////////////////////
//
// Tool_humbreak::initialize --  Initializations that only have to be done once
//    for all HumdrumFile segments.
//

void Tool_humbreak::initialize(void) {
	string systemMeasures = getString("measures");
	string pageMeasures = getString("page-breaks");
	m_group = getString("group");
	m_removeQ = getBoolean("remove-breaks");
	m_page2lineQ = getBoolean("page-to-line-breaks");

	vector<string> lbs;
	vector<string> pbs;
	HumRegex hre;
	hre.split(lbs, systemMeasures, "[^\\da-z]+");
	hre.split(pbs, pageMeasures, "[^\\da-z]+");

	for (int i=0; i<(int)lbs.size(); i++) {
		if (hre.search(lbs[i], "^(p?)(\\d+)([a-z]?)")) {
			int number = hre.getMatchInt(2);
			if (!hre.getMatch(1).empty()) {
				m_pageMeasures[number] = 1;
				int offset = 0;
				string letter;
				if (!hre.getMatch(3).empty()) {
					letter = hre.getMatch(3);
					offset = letter.at(0) - 'a';
				}
				m_pageOffset[number] = offset;
			} else {
				m_lineMeasures[number] = 1;
				int offset = 0;
				if (!hre.getMatch(3).empty()) {
					string letter = hre.getMatch(3);
					offset = letter.at(0) - 'a';
				}
				m_lineOffset[number] = offset;
			}
		}
	}

	for (int i=0; i<(int)pbs.size(); i++) {
		if (hre.search(pbs[i], "^(\\d+)([a-z]?)")) {
			int number = hre.getMatchInt(1);
			m_pageMeasures[number] = 1;
			int offset = 0;
			if (!hre.getMatch(2).empty()) {
				string letter = hre.getMatch(2);
				offset = letter.at(0) - 'a';
			}
			m_pageOffset[number] = offset;
		}
	}
}



//////////////////////////////
//
// Tool_humbreak::markLineBreakMeasures --
//

void Tool_humbreak::markLineBreakMeasures(HumdrumFile& infile) {
	vector<HLp> pbreak;
	vector<HLp> lbreak;
	HumRegex hre;
	map<int, int> used;

	for (int i=0; i<infile.getLineCount(); i++) {
		if (infile[i].isCommentGlobal()) {
			HTp token = infile[i].token(0);
			if (hre.search(token, "^!!LO:LB:")) {
				lbreak.push_back(&infile[i]);
			} else if (hre.search(token, "^!!LO:PB:")) {
				pbreak.push_back(&infile[i]);
			}
		}

		if (!infile[i].isBarline()) {
			continue;
		}

		int barnum = infile[i].getBarNumber();
		if (barnum < 0) {
			lbreak.clear();
			pbreak.clear();
			continue;
		}

		int status = m_lineMeasures[barnum];
		if (status) {
			HLp line = &infile[i];
			int offset = m_lineOffset[barnum];
			if (offset && (used[barnum] == 0)) {
				used[barnum] = offset;
				int ocounter = 0;
				lbreak.clear();
				pbreak.clear();
				for (int j=i+1; j<infile.getLineCount(); j++) {
					if (infile[i].isCommentGlobal()) {
						HTp token = infile.token(i, 0);
						if (hre.search(token, "^!!LO:LB:")) {
							lbreak.push_back(&infile[i]);
						}
						if (hre.search(token, "^!!LO:PB:")) {
							pbreak.push_back(&infile[i]);
						}
					}
					if (!infile[j].isBarline()) {
						continue;
					}
					ocounter++;
					if (ocounter == offset) {
						line = &infile[j];
					}
				}
				if (!lbreak.empty()) {
					lbreak.back()->setValue("auto", "barnum", barnum + 1);
				} else {
					line->setValue("auto", "barnum", barnum + 1);
				}
			} else {
				line->setValue("auto", "barnum", barnum + 1);
			}
		}

		status = m_pageMeasures[barnum];
		if (status) {
			HLp line = &infile[i];
			int offset = m_pageOffset[barnum];
			if (offset) {
				int ocounter = 0;
				lbreak.clear();
				pbreak.clear();
				for (int j=i+1; j<infile.getLineCount(); j++) {
					if (infile[i].isCommentGlobal()) {
						HTp token = infile.token(i, 0);
						if (hre.search(token, "^!!LO:LB:")) {
							lbreak.push_back(&infile[i]);
						}
						if (hre.search(token, "^!!LO:PB:")) {
							pbreak.push_back(&infile[i]);
						}
					}
					if (!infile[j].isBarline()) {
						continue;
					}
					ocounter++;
					if (ocounter == offset) {
						line = &infile[j];
					}
				}
				if (!pbreak.empty()) {
					pbreak.back()->setValue("auto", "barnum", barnum + 1);
					pbreak.back()->setValue("auto", "page", 1);
				}
			} else {
				line->setValue("auto", "barnum", barnum + 1);
				line->setValue("auto", "page", 1);
			}
		}
	}
}



//////////////////////////////
//
// Tool_humbreak::addBreaks --
//

void Tool_humbreak::addBreaks(HumdrumFile& infile) {
	markLineBreakMeasures(infile);

	HumRegex hre;
	for (int i=0; i<infile.getLineCount(); i++) {
		if (!(infile[i].isBarline() || infile[i].isComment())) {
			m_humdrum_text << infile[i] << endl;
			continue;
		}

		int barnum = infile[i].getValueInt("auto", "barnum");
		if (barnum < 1) {
			m_humdrum_text << infile[i] << endl;
			continue;
		}
		barnum--;
		int pageQ = infile[i].getValueInt("auto", "page");

		if (pageQ && infile[i].isComment()) {
			HTp token = infile.token(i, 0);
			if (hre.search(token, "^!!LO:PB:")) {
				// Add group to existing LO:PB:
				HTp token = infile.token(i, 0);
				HTp barToken = infile.token(i+1, 0);
				if (barToken->isBarline()) {
					int measure = infile[i+1].getBarNumber();
					int pbStatus = m_pageMeasures[measure];
					if (pbStatus) {
						string query = "\\b" + m_group + "\\b";
						if (!hre.match(token, query)) {
							m_humdrum_text << token << ", " << m_group << endl;
						} else {
							m_humdrum_text << token << endl;
						}
					} else {
						m_humdrum_text << token << endl;
					}
					m_humdrum_text << infile[i+1] << endl;
					i++;
					continue;
				}
			} else if (hre.search(token, "^!!LO:LB:")) {
				// Add group to existing LO:LB:
				HTp token = infile.token(i, 0);
				HTp barToken = infile.token(i+1, 0);
				if (barToken->isBarline()) {
					int measure = infile[i+1].getBarNumber();
					int lbStatus = m_lineMeasures[measure];
					if (lbStatus) {
						string query = "\\b" + m_group + "\\b";
						if (!hre.match(token, query)) {
							m_humdrum_text << token << ", " << m_group << endl;
						} else {
							m_humdrum_text << token << endl;
						}
					} else {
						m_humdrum_text << token << endl;
					}
					m_humdrum_text << infile[i+1] << endl;
					i++;
					continue;
				}
			}
		}

		if (pageQ) {
			m_humdrum_text << "!!LO:PB:g=" << m_group << endl;
		} else {
			m_humdrum_text << "!!LO:LB:g=" << m_group << endl;
		}
		m_humdrum_text << infile[i] << endl;
	}
}



//////////////////////////////
//
// Tool_humbreak::processFile --
//

void Tool_humbreak::processFile(HumdrumFile& infile) {
	initialize();
	if (m_removeQ) {
		removeBreaks(infile);
	} else if (m_page2lineQ) {
		convertPageToLine(infile);
	} else {
		addBreaks(infile);
	}
}



//////////////////////////////
//
// Tool_humbreak::removeBreaks --
//

void Tool_humbreak::removeBreaks(HumdrumFile& infile) {
	for (int i=0; i<infile.getLineCount(); i++) {
		if (infile[i].token(0)->compare(0, 7, "!!LO:LB") == 0) {
			continue;
		}
		if (infile[i].token(0)->compare(0, 7, "!!LO:PB") == 0) {
			continue;
		}
		m_humdrum_text << infile[i] << endl;
	}
}



//////////////////////////////
//
// Tool_humbreak::convertPageToLine --
//

void Tool_humbreak::convertPageToLine(HumdrumFile& infile) {
	HumRegex hre;
	for (int i=0; i<infile.getLineCount(); i++) {
		if (infile[i].token(0)->compare(0, 7, "!!LO:PB") == 0) {
			string text = *infile[i].token(0);
			hre.replaceDestructive(text, "!!LO:LB", "!!LO:PB");
			m_humdrum_text << text << endl;
			continue;
		}
		m_humdrum_text << infile[i] << endl;
	}
}


// END_MERGE

} // end namespace hum



