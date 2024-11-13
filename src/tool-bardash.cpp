//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Nov 13 09:08:06 PST 2024
// Last Modified: Wed Nov 13 09:08:09 PST 2024
// Filename:      tool-bardash.cpp
// URL:           https://github.com/craigsapp/humlib/blob/master/src/tool-bardash.cpp
// Syntax:        C++11; humlib
// vim:           syntax=cpp ts=3 noexpandtab nowrap
//
// Description:   Notate hypermeters.
//							*bar:dash=3
//                      solid bar every three barlines, dashes for other two
//							*bar:dash=2
//                      solid bar every two barlines, dashes for other one
//							*bar:dot=2
//                      solid bar every two barlines, dots for other one
//							*bar:invis=2
//                      solid bar every two barlines, invisible for other one
//                   *bar:stop
//                      stop applying given hyper meter.
//
//

#include "tool-bardash.h"
#include "HumRegex.h"

using namespace std;

namespace hum {

// START_MERGE


/////////////////////////////////
//
// Tool_bardash::Tool_bardash -- Set the recognized options for the tool.
//

Tool_bardash::Tool_bardash(void) {
	define("r|remove=b",    "remove any dot/dash/invisible barline stylings");
}



///////////////////////////////
//
// Tool_bardash::run -- Primary interfaces to the tool.
//

bool Tool_bardash::run(HumdrumFileSet& infiles) {
	bool status = true;
	for (int i=0; i<infiles.getCount(); i++) {
		status &= run(infiles[i]);
	}
	return status;
}


bool Tool_bardash::run(const string& indata, ostream& out) {
	HumdrumFile infile(indata);
	return run(infile, out);
}


bool Tool_bardash::run(HumdrumFile& infile, ostream& out) {
	bool status = run(infile);
	out << m_free_text.str();
	return status;
}


bool Tool_bardash::run(HumdrumFile& infile) {
   initialize();
	processFile(infile);
	return true;
}



//////////////////////////////
//
// Tool_bardash::initialize --
//

void Tool_bardash::initialize(void) {
	m_removeQ = getBoolean("remove");
}



//////////////////////////////
//
// Tool_bardash::processFile --
//

void Tool_bardash::processFile(HumdrumFile& infile) {
	if (m_removeQ) {
		removeBarStylings(infile);
	} else {
		applyBarStylings(infile);
	}
	infile.createLinesFromTokens();
	m_humdrum_text << infile;
}



//////////////////////////////
//
// Tool_bardash::removeBarStylings --
//

void Tool_bardash::removeBarStylings(HumdrumFile& infile) {
	vector<HTp> kstarts = infile.getKernSpineStartList();
	for (int i=0; i<(int)kstarts.size(); i++) {
		removeBarStylings(kstarts.at(i));
	}
}



//////////////////////////////
//
// Tool_bardash::removeBarStylings --
//

void Tool_bardash::removeBarStylings(HTp spine) {

	HTp current = spine->getNextToken();
	bool activeQ = false;
	bool dashQ   = false;
	bool dotQ    = false;
	bool invisQ  = false;
	HumRegex hre;
	while (current) {
		if (current->isInterpretation()) {
			if (hre.search(current, "^\\*bar:")) {
				if (hre.search(current, "stop")) {
					activeQ = false;
					dashQ  = false;
					dotQ   = false;
					invisQ = false;
				} else {
					activeQ = true;
					if (hre.search(current, "^\\*bar:.*dash=(\\d+)")) {
						dashQ  = true;
						dotQ   = false;
						invisQ = false;
					} else if (hre.search(current, "^\\*bar:.*dot=(\\d+)")) {
						dashQ  = false;
						dotQ   = true;
						invisQ = false;
					} else if (hre.search(current, "^\\*bar:.*invis=(\\d+)")) {
						dashQ  = false;
						dotQ   = false;
						invisQ = true;
					}
				}
			}
		}
		if (!current->isBarline()) {
			current = current->getNextToken();
			return;
		}
		if (!activeQ) {
			current = current->getNextToken();
			return;
		}
		int track = current->getTrack();
		HTp rcurrent = current;
		while (rcurrent) {
			if (track != rcurrent->getTrack()) {
				break;
			}
			string text = *rcurrent;
			int length = (int)text.size();
			if (dashQ) {
				hre.replaceDestructive(text, "", ":", "g");
			} else if (dotQ) {
				hre.replaceDestructive(text, "", ".", "g");
			} else if (invisQ) {
				hre.replaceDestructive(text, "", "-", "g");
			}
			if (length > (int)text.size()) {
				rcurrent->setText(text);
			}
			rcurrent = rcurrent->getNextFieldToken();
		}
		current = current->getNextToken();
	}

}



//////////////////////////////
//
// Tool_bardash::applyBarStylings --
//

void Tool_bardash::applyBarStylings(HumdrumFile& infile) {
	vector<HTp> kstarts = infile.getKernSpineStartList();
	for (int i=0; i<(int)kstarts.size(); i++) {
		applyBarStylings(kstarts.at(i));
	}
}



//////////////////////////////
//
// Tool_bardash::applyBarStylings --
//

void Tool_bardash::applyBarStylings(HTp spine) {

	HTp current = spine->getNextToken();
	bool activeQ = false;
	bool dashQ   = false;
	bool dotQ    = false;
	bool invisQ  = false;
	int dash  = 0;
	int dot   = 0;
	int invis = 0;
	int counter = 0;
	HumRegex hre;
	while (current) {
		if (current->isInterpretation()) {
			if (hre.search(current, "^\\*bar:")) {
				if (hre.search(current, "stop")) {
					activeQ = false;
					dashQ   = false;
					dotQ    = false;
					invisQ  = false;
					dash    = 0;
					dot     = 0;
					invis   = 0;
				} else {
					activeQ = true;
					if (hre.search(current, "^\\*bar:.*dash=(\\d+)")) {
						dashQ  = true;
						dotQ   = false;
						invisQ = false;
						dash   = hre.getMatchInt(1);
						counter = 0;
					} else if (hre.search(current, "^\\*bar:.*dot=(\\d+)")) {
						dashQ  = false;
						dotQ   = true;
						invisQ = false;
						dot    = hre.getMatchInt(1);
						counter = 0;
					} else if (hre.search(current, "^\\*bar:.*invis=(\\d+)")) {
						dashQ  = false;
						dotQ   = false;
						invisQ = true;
						invis  = hre.getMatchInt(1);
						counter = 0;
					}
				}
			}
			current = current->getNextToken();
			continue;
		}
		if (!current->isBarline()) {
			current = current->getNextToken();
			continue;
		}
		if (!activeQ) {
			current = current->getNextToken();
			continue;
		}

		int track = current->getTrack();
		HTp rcurrent = current;

		if (dashQ && (dash > 0)) {

			if (counter % dash != 0) {
				while (rcurrent) {
					if (track != rcurrent->getTrack()) {
						break;
					}
					string text = *rcurrent;
					if (text.find(":") == string::npos) {
						text += ":";
						rcurrent->setText(text);
					}
					rcurrent = rcurrent->getNextFieldToken();
				}
			}

		} else if (dotQ && (dot > 0)) {

			if (counter % dot != 0) {
				while (rcurrent) {
					if (track != rcurrent->getTrack()) {
						break;
					}
					string text = *rcurrent;
					if (text.find(".") == string::npos) {
						text += ".";
						rcurrent->setText(text);
					}
					rcurrent = rcurrent->getNextFieldToken();
				}
			}

		} else if (invisQ && (invis > 0)) {

			if (counter % invis != 0) {
				while (rcurrent) {
					if (track != rcurrent->getTrack()) {
						break;
					}
					string text = *rcurrent;
					if (text.find("-") == string::npos) {
						text += "-";
						rcurrent->setText(text);
					}
					rcurrent = rcurrent->getNextFieldToken();
				}
			}

		}
		counter++;
		current = current->getNextToken();
	}
}


// END_MERGE

} // end namespace hum



