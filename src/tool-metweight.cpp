//
// Programmer:    Wolfgang Drescher <drescher.wolfgang@gmail.com>
// Creation Date: Thu Jul 16 2026
// Filename:      tool-metweight.cpp
// URL:           https://github.com/craigsapp/humlib/blob/master/src/tool-metweight.cpp
// Syntax:        C++11; humlib
// vim:           syntax=cpp ts=3 noexpandtab nowrap
//
// Description:   Label the metric weight (strong/half-strong/weak) of note
//                and rest attacks in **kern spines, derived algorithmically
//                from the time signature (see getWeightClass).
//

#include "tool-metweight.h"
#include "tool-meter.h"
#include "Convert.h"

using namespace std;

namespace hum {

// START_MERGE


//////////////////////////////
//
// Tool_metweight::Tool_metweight -- Set the recognized options for the tool.
//

Tool_metweight::Tool_metweight(void) {
	define("f|full=b",         "print full text labels (strong/half-strong/weak) instead of abbreviations");
	define("i|integer=b",      "print integer rank labels (1/2/3) instead of abbreviations");
	define("x|cdata=b",        "label the spine **cdata-metweight instead of **metweight");
	define("k|kern-tracks=s",  "process only the specified kern spines");
	define("s|spine|spines=s", "process only the specified spines");
}



//////////////////////////////
//
// Tool_metweight::run -- Do the main work of the tool.
//

bool Tool_metweight::run(HumdrumFileSet& infiles) {
	bool status = true;
	for (int i = 0; i < infiles.getCount(); i++) {
		status &= run(infiles[i]);
	}
	return status;
}

bool Tool_metweight::run(const string& indata, ostream& out) {
	HumdrumFile infile(indata);
	bool status = run(infile);
	if (hasAnyText()) {
		getAllText(out);
	} else {
		out << infile;
	}
	return status;
}

bool Tool_metweight::run(HumdrumFile& infile, ostream& out) {
	bool status = run(infile);
	if (hasAnyText()) {
		getAllText(out);
	} else {
		out << infile;
	}
	return status;
}

bool Tool_metweight::run(HumdrumFile& infile) {
	initialize();
	processFile(infile);
	return true;
}



//////////////////////////////
//
// Tool_metweight::initialize --
//

void Tool_metweight::initialize(void) {
	m_fullQ    = getBoolean("full");
	m_integerQ = getBoolean("integer");
	m_cdataQ   = getBoolean("cdata");

	if (getBoolean("spine")) {
		m_spineTracks = getString("spine");
	} else if (getBoolean("kern-tracks")) {
		m_kernTracks = getString("kern-tracks");
	}
}



//////////////////////////////
//
// Tool_metweight::processFile -- Insert a **metweight spine after every
//    processed **kern spine, containing the metric weight of the current
//    note/rest attack.
//

void Tool_metweight::processFile(HumdrumFile& infile) {
	int maxTrack = infile.getMaxTrack();

	m_selectedKernSpines.resize(maxTrack + 1); // +1 since track=0 is not used
	fill(m_selectedKernSpines.begin(), m_selectedKernSpines.end(), true);

	vector<HTp> kernspines = infile.getKernSpineStartList();

	// Calculate which input spines to process based on -k or -s option:
	if (!m_kernTracks.empty()) {
		vector<int> ktracks = Convert::extractIntegerList(m_kernTracks, maxTrack);
		fill(m_selectedKernSpines.begin(), m_selectedKernSpines.end(), false);
		for (int i = 0; i < (int)ktracks.size(); i++) {
			int index = ktracks[i] - 1;
			if ((index < 0) || (index >= (int)kernspines.size())) {
				continue;
			}
			int track = kernspines.at(index)->getTrack();
			m_selectedKernSpines.at(track) = true;
		}
	} else if (!m_spineTracks.empty()) {
		fill(m_selectedKernSpines.begin(), m_selectedKernSpines.end(), false);
		infile.makeBooleanTrackList(m_selectedKernSpines, m_spineTracks);
	}

	vector<HTp> voices;
	for (int i = 0; i < (int)kernspines.size(); i++) {
		if (m_selectedKernSpines.at(kernspines[i]->getTrack())) {
			voices.push_back(kernspines[i]);
		}
	}
	if (voices.empty()) {
		return;
	}

	// Delegate time-signature/beat-position analysis (including pickup
	// measures) to the meter tool, which annotates each **kern attack
	// with "auto" parameters read back out below; "-r" also covers rests.
	Tool_meter meterTool;
	meterTool.process("-r");
	meterTool.run(infile);

	vector<vector<string>> results;
	fillVoiceResults(results, infile, voices);

	string exinterp = m_cdataQ ? "**cdata-metweight" : "**metweight";
	infile.appendDataSpine(results.back(), ".", exinterp);
	for (int i = (int)voices.size() - 1; i > 0; i--) {
		int track = voices[i]->getTrack();
		infile.insertDataSpineBefore(track, results[i - 1], ".", exinterp);
	}
	infile.createLinesFromTokens();

	// Enable usage in verovio (`!!!filter: metweight`)
	m_humdrum_text << infile;
}



//////////////////////////////
//
// Tool_metweight::fillVoiceResults -- Calculate the **metweight tokens for
//    each selected **kern spine, one value per line, for
//    appendDataSpine()/insertDataSpineBefore() to consume.
//

void Tool_metweight::fillVoiceResults(vector<vector<string>>& results, HumdrumFile& infile,
		const vector<HTp>& voices) {

	int lineCount = infile.getLineCount();
	results.clear();
	results.resize(voices.size());
	for (int v = 0; v < (int)voices.size(); v++) {
		results[v].resize(lineCount, ".");
	}

	for (int i = 0; i < lineCount; i++) {
		if (!infile[i].isData()) {
			continue;
		}
		for (int v = 0; v < (int)voices.size(); v++) {
			results[v][i] = getWeightToken(infile, i, voices[v]->getTrack());
		}
	}
}



//////////////////////////////
//
// Tool_metweight::getWeightToken -- Metric weight token for the track's
//    **kern token on this line, using the position Tool_meter annotated.
//    Null tokens return "."; non-attacks/unrecognized meters return WEIGHT_NONE.
//

string Tool_metweight::getWeightToken(HumdrumFile& infile, int line, int track) {
	HTp token = NULL;
	for (int j = 0; j < infile[line].getFieldCount(); j++) {
		HTp tok = infile.token(line, j);
		if (tok->getTrack() == track) {
			token = tok;
			break;
		}
	}
	if (!token) {
		return ".";
	}

	if (token->isNull()) {
		return ".";
	}

	if (!token->getValueBool("auto", "hasData")) {
		return formatWeightClass(WEIGHT_NONE);
	}

	int top = token->getValueInt("auto", "numerator");
	int bot = token->getValueInt("auto", "denominator");
	HumNum beat(token->getValue("auto", "zeroBeat"));

	return formatWeightClass(getWeightClass(top, bot, beat));
}



//////////////////////////////
//
// Tool_metweight::getWeightClass -- Derive the metric weight of a
//    0-indexed beat position (Tool_meter's "zeroBeat" units) from the time
//    signature by bisecting the measure's main beats: the downbeat is
//    strong; if there is an even number of main beats, the one starting
//    the second half is half-strong; every other main beat is weak.  A
//    compound meter with exactly 2 main beats (6/8) also classifies its
//    eighth-note subdivisions as weak, since those are themselves counted;
//    everything else (a simple beat's "and", 9/8's/12/8's subdivisions,
//    deeper subdivisions, syncopated offbeats) is left unclassified.
//

int Tool_metweight::getWeightClass(int top, int bot, HumNum beat) {
	if ((top <= 0) || (bot <= 0) || (beat < 0)) {
		return WEIGHT_NONE;
	}

	int multiple = top / 3;
	int remainder = top % 3;
	bool compound = (bot >= 8) && (multiple > 1) && (remainder == 0);

	int mainBeatCount = compound ? multiple : top;
	if (mainBeatCount <= 0) {
		return WEIGHT_NONE;
	}

	int mainBeatIndex = beat.getInteger();
	HumNum fraction = beat - HumNum(mainBeatIndex);

	if (fraction == 0) {
		if (mainBeatIndex == 0) {
			return WEIGHT_STRONG;
		}
		if (((mainBeatCount % 2) == 0) && (mainBeatIndex == mainBeatCount / 2)) {
			return WEIGHT_HALF_STRONG;
		}
		if (mainBeatIndex < mainBeatCount) {
			return WEIGHT_WEAK;
		}
		return WEIGHT_NONE;
	}

	// Only a compound meter with exactly 2 main beats (6/8) classifies its
	// eighth-note subdivisions as weak; 9/8, 12/8, etc. are felt/conducted
	// at the main-beat level only, so their subdivisions stay unclassified.
	if (compound && (mainBeatCount == 2) && ((fraction == HumNum(1, 3)) || (fraction == HumNum(2, 3)))) {
		return WEIGHT_WEAK;
	}

	return WEIGHT_NONE;
}



//////////////////////////////
//
// Tool_metweight::formatWeightClass -- Convert a weight class into the
//    token representation selected by the -f/-i options (abbreviated
//    strong/half-strong/weak codes by default).
//

string Tool_metweight::formatWeightClass(int weightClass) {
	if (m_integerQ) {
		switch (weightClass) {
			case WEIGHT_STRONG:      return "1";
			case WEIGHT_HALF_STRONG: return "2";
			case WEIGHT_WEAK:        return "3";
			default:                 return ".";
		}
	} else if (m_fullQ) {
		switch (weightClass) {
			case WEIGHT_STRONG:      return "strong";
			case WEIGHT_HALF_STRONG: return "half-strong";
			case WEIGHT_WEAK:        return "weak";
			default:                 return ".";
		}
	} else {
		switch (weightClass) {
			case WEIGHT_STRONG:      return "s";
			case WEIGHT_HALF_STRONG: return "hs";
			case WEIGHT_WEAK:        return "w";
			default:                 return ".";
		}
	}
}


// END_MERGE

} // end namespace hum
