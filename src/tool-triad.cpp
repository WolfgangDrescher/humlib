//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue May  5 05:36:55 PDT 2026
// Last Modified: Sat May  9 21:02:00 PDT 2026
// Filename:      tool-triad.cpp
// URL:           https://github.com/craigsapp/humlib/blob/master/src/tool-triad.cpp
// Syntax:        C++11; humlib
// vim:           ts=3 noexpandtab
//
// Description:   Analyze triad encoding and corrected encoding.
//

#include "tool-triad.h"
#include "Convert.h"
#include "HumRegex.h"

using namespace std;

namespace hum {

// START_MERGE


/////////////////////////////////
//
// Tool_triad::Tool_triad -- Set the recognized options for the tool.
//

Tool_triad::Tool_triad(void) {
	define("A=s:darkviolet",           "Color of triads with diatonic A root");
	define("B=s:darkorange",           "Color of triads with diatonic B root");
	define("C=s:limegreen",            "Color of triads with diatonic C root");
	define("D=s:royalblue",            "Color of triads with diatonic D root");
	define("E=s:crimson",              "Color of triads with diatonic E root");
	define("F=s:goldenrod",            "Color of triads with diatonic F root");
	define("G=s:skyblue",              "Color of triads with diatonic G root");
	define("a|append=b",               "Add analysis at the end of the line");
	define("c|pc|class|pitch-class=b", "Display pitch classes");
	define("p|pitches=b",              "Display pitches");
	define("R|rest=b",                 "Display rest rather than null token");
	define("s|summary=b",              "Display summary table");
	define("r|root=b",                 "Display root only");
	define("I|no-inversion=b",         "Do not giave inversion number");
	define("q|quality=b",              "Display quality only");
	define("U|no-unison=b",            "No Unison quality");
	define("P|no-partial=b",           "No Unisons/missing 3rds/missing 5ths");
	define("l|low=b",                  "Sort pitches from low to high");
	define("ascii=b",                  "Don't use unicode interval subscripts");
	define("no-color|root-color=b",    "Turn off Colorize by root");
	define("analysis-color=s:salmon",  "Set analysis color");
}



/////////////////////////////////
//
// Tool_triad::run -- Do the main work of the tool.
//

bool Tool_triad::run(HumdrumFileSet& infiles) {
	bool status = true;
	for (int i=0; i<infiles.getCount(); i++) {
		status &= run(infiles[i]);
	}
	return status;
}


bool Tool_triad::run(const string& indata, ostream& out) {
	HumdrumFile infile(indata);
	bool status = run(infile);
	if (hasAnyText()) {
		getAllText(out);
	} else {
		out << infile;
	}
	return status;
}


bool Tool_triad::run(HumdrumFile& infile, ostream& out) {
	bool status = run(infile);
	if (hasAnyText()) {
		getAllText(out);
	} else {
		out << infile;
	}
	return status;
}



bool Tool_triad::run(HumdrumFile& infile) {
	initialize();
	processFile(infile);
	return true;
}



//////////////////////////////
//
// Tool_triad::initialize --  Initializations that only have to be done once
//    for all HumdrumFile segments.
//

void Tool_triad::initialize(void) {
	m_pcColor.resize(7);
	m_pcColor[0] = getString("C");
	m_pcColor[1] = getString("D");
	m_pcColor[2] = getString("E");
	m_pcColor[3] = getString("F");
	m_pcColor[4] = getString("G");
	m_pcColor[5] = getString("A");
	m_pcColor[6] = getString("B");

	m_appendQ      = getBoolean("append");
	m_summaryQ     = getBoolean("summary");
	m_classQ       = getBoolean("pitch-class");
	m_pitchesQ     = getBoolean("pitches");
	m_restQ        = getBoolean("rest");
	m_rootQ        = getBoolean("root");
	m_rootQ        = true;
	m_qualityQ     = getBoolean("quality");
	m_unisonQ      = !getBoolean("no-unison");
	m_lowQ         = !getBoolean("low");
	m_asciiQ       = getBoolean("ascii");
	m_partialQ     = !getBoolean("no-partial");
	m_noInversionQ = getBoolean("no-inversion");
	m_rootColorQ   = !getBoolean("no-color");
	m_color        = getString("analysis-color");
}



/////////////////////////////
//
// Tool_triad::processFile --
//

void Tool_triad::processFile(HumdrumFile& infile) {
	string quality;
	string root;
	string inversion;
	bool hasColor = false;

	for (int i=0; i<infile.getLineCount(); i++) {

		// Preserve global/local comments exactly.
		if (!infile[i].hasSpines()) {
			m_humdrum_text << infile[i] << endl;
			continue;
		} else if (infile[i].isBarline()) {
			HTp tok = infile[i].token(0);
			m_humdrum_text << tok << "\t" << infile[i];
			if (m_rootColorQ) {
				m_humdrum_text << "\t" << tok;
			}
			m_humdrum_text << endl;
			continue;
		} else if (infile[i].isCommentLocal()) {
			m_humdrum_text << "!" << "\t" << infile[i];
			if (m_rootColorQ) {
				m_humdrum_text << "\t" << "!";
			}
			m_humdrum_text << endl;
			continue;
		} else if (infile[i].isExclusive()) {
			m_humdrum_text << "**cdata" << "\t" << infile[i];
			if (m_rootColorQ) {
				m_humdrum_text << "\t" << "**color";
			}
			m_humdrum_text << endl;
			continue;
		} else if (infile[i].isInterpretation()) {
			HTp tok = infile[i].token(0);
			if (tok->compare(0, 2, "*>") == 0) {
				m_humdrum_text << tok << "\t" << infile[i];
				if (m_rootColorQ) {
					m_humdrum_text << "\t" << tok;
				}
				m_humdrum_text << endl;
				continue;
			} else if (*tok == "*-") {
				m_humdrum_text << tok << "\t" << infile[i];
				if (m_rootColorQ) {
					m_humdrum_text << "\t" << tok;
				}
				m_humdrum_text << endl;
				continue;
			} else {
				m_humdrum_text << tok << "\t" << infile[i];
				if (m_rootColorQ) {
					m_humdrum_text << "\t" << "*";
				}
				m_humdrum_text << endl;
				continue;
			}
		}

		if (!infile[i].isData()) {
			m_humdrum_text << "ERROR!" << endl;
			continue;
		}

		quality.clear();
		root.clear();
		inversion.clear();

		map<string, bool> options;
		options["pitches"] = m_pitchesQ;
		options["class"]   = m_classQ;
		options["rest"]    = m_restQ;
		options["low"]     = m_lowQ;
		options["ascii"]   = m_asciiQ;
		options["partial"] = m_partialQ;
		options["unison"]  = m_unisonQ;

		string token = infile[i].getTriadicQuality(
			infile, i, quality, root, inversion, options);

		if (!m_unisonQ && (quality == "U")) {
			quality.clear();
			root.clear();
			inversion.clear();
		}

		if (m_qualityQ) {
			root.clear();
			inversion.clear();
		}

		if (!hasColor && token == "*") {
			token += "color:" + m_color;
			hasColor = true;
		}

		HumRegex hre;
		string letter;
		int index;
		string color;
		if (hre.search(root, "([A-Ga-g])")) {
			letter = hre.getMatch(1);
			if (!letter.empty()) {
				index = std::toupper(letter[0]) - 'C';
				if (index < 0) {
					index += 7;
				}
				color = m_pcColor.at(index);
			}
		}

		if (color.empty()) {
			if (token == "**cdata") {
				color = token;
			}
			color = "black";
		}

		// Construct analysis token for data lines.
		if (token.empty()) {

			if (m_qualityQ) {
				token = quality;
			} else if (m_rootQ) {
				token = root;
				if (!m_noInversionQ) {
					token += inversion;
				}
			} else {
				token = quality;
				if (!root.empty()) {
					token += "(";
					token += root;
					if (!m_noInversionQ) {
						token += inversion;
					}
					token += ")";
				}
			}
		}

		if (token.empty()) {
			token = ".";
		}

		// Ignore hidden comment marker.
		if (token == "!!") {
			m_humdrum_text << infile[i];
			m_humdrum_text << "\t" << color;
			m_humdrum_text << endl;
			continue;
		}

		// Prepend analysis spine.
		if (m_appendQ) {
			m_humdrum_text << infile[i] << "\t" << token;
			m_humdrum_text << "\t" << color;
			m_humdrum_text << endl;
		} else {
			m_humdrum_text << token << "\t" << infile[i];
			m_humdrum_text << "\t" << color;
			m_humdrum_text << endl;
		}
	}
}



// END_MERGE

} // end namespace hum//
