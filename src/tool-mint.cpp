//
// Programmer:    Wolfgang Drescher <drescher.wolfgang@gmail.com>
// Creation Date: Sat Jul 11 2026
// Filename:      tool-mint.cpp
// URL:           https://github.com/craigsapp/humlib/blob/master/src/tool-mint.cpp
// Syntax:        C++11; humlib
// vim:           syntax=cpp ts=3 noexpandtab nowrap
//
// Description:   Calculate melodic intervals (with diatonic interval quality)
//                between consecutive notes in **kern spines.  This is a
//                reimplementation of the "mint" tool from the original
//                Humdrum Toolkit.
//

#include "tool-mint.h"
#include "Convert.h"

using namespace std;

namespace hum {

// START_MERGE


//////////////////////////////
//
// Tool_mint::Tool_mint -- Set the recognized options for the tool.
//

Tool_mint::Tool_mint(void) {
	define("a|absolute=b",     "do not show the direction of the interval");
	define("c|compound=b",     "reduce compound intervals to simple intervals");
	define("d|diatonic=b",     "only display the diatonic interval number (no interval quality)");
	define("l|lowest=b",       "use the lowest note of a chord instead of the highest note");
	define("x|cdata=b",        "label the spine **cdata-mint instead of **mint");
	define("k|kern-tracks=s",  "process only the specified kern spines");
	define("s|spine|spines=s", "process only the specified spines");
}



//////////////////////////////
//
// Tool_mint::run -- Do the main work of the tool.
//

bool Tool_mint::run(HumdrumFileSet& infiles) {
	bool status = true;
	for (int i = 0; i < infiles.getCount(); i++) {
		status &= run(infiles[i]);
	}
	return status;
}

bool Tool_mint::run(const string& indata, ostream& out) {
	HumdrumFile infile(indata);
	bool status = run(infile);
	if (hasAnyText()) {
		getAllText(out);
	} else {
		out << infile;
	}
	return status;
}

bool Tool_mint::run(HumdrumFile& infile, ostream& out) {
	bool status = run(infile);
	if (hasAnyText()) {
		getAllText(out);
	} else {
		out << infile;
	}
	return status;
}

bool Tool_mint::run(HumdrumFile& infile) {
	initialize();
	processFile(infile);
	return true;
}



//////////////////////////////
//
// Tool_mint::initialize --
//

void Tool_mint::initialize(void) {
	m_absoluteQ = getBoolean("absolute");
	m_compoundQ = getBoolean("compound");
	m_diatonicQ = getBoolean("diatonic");
	m_lowestQ   = getBoolean("lowest");
	m_cdataQ    = getBoolean("cdata");

	if (getBoolean("spines")) {
		m_spines = getString("spines");
	} else if (getBoolean("kern-tracks")) {
		m_kernTracks = getString("kern-tracks");
	}
}



//////////////////////////////
//
// Tool_mint::processFile -- Insert a **mint spine after every processed
//    **kern spine, containing the melodic interval between the current
//    note and the previous note attack in the same voice.
//

void Tool_mint::processFile(HumdrumFile& infile) {
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
	} else if (!m_spines.empty()) {
		fill(m_selectedKernSpines.begin(), m_selectedKernSpines.end(), false);
		infile.makeBooleanTrackList(m_selectedKernSpines, m_spines);
	}

	string exinterp = m_cdataQ ? "**cdata-mint" : "**mint";
	int lineCount = infile.getLineCount();

	for (int i = 0; i < (int)kernspines.size(); i++) {
		if (!m_selectedKernSpines.at(kernspines[i]->getTrack())) {
			continue;
		}
		vector<string> trackData = getTrackData(kernspines[i], lineCount);
		if (i + 1 < (int)kernspines.size()) {
			int nextTrack = kernspines[i + 1]->getTrack();
			infile.insertDataSpineBefore(nextTrack, trackData, ".", exinterp);
		} else {
			infile.appendDataSpine(trackData, ".", exinterp);
		}
	}

	// Enables usage in verovio (`!!!filter: mint`)
	m_humdrum_text << infile;
}



//////////////////////////////
//
// Tool_mint::getTrackData -- Calculate the **mint spine data for a single
//    **kern spine, one entry per line of the file (empty for non-data
//    lines, which are filled in automatically by appendDataSpine/
//    insertDataSpineBefore).
//

vector<string> Tool_mint::getTrackData(HTp kernstart, int lineCount) {
	vector<string> trackData(lineCount);
	HTp current = kernstart;
	while (current) {
		if (current->isData()) {
			trackData[current->getLineIndex()] = getIntervalToken(current);
		}
		current = current->getNextToken();
	}
	return trackData;
}



//////////////////////////////
//
// Tool_mint::getIntervalToken -- Calculate the melodic interval token for
//    the given **kern token with respect to the previous note attack in
//    the same voice (spine/subspine).  Rests are transparent: the interval
//    is always calculated across intervening rests, using the last actual
//    pitch that was sounded.
//

string Tool_mint::getIntervalToken(HTp token) {
	if (token->isNull()) {
		return ".";
	}
	if (token->isRest()) {
		return "r";
	}
	if (!token->isNoteAttack()) {
		// sustained note (tied continuation): no new interval to report
		return ".";
	}

	int currPitch = getRepresentativeBase40Pitch(token);
	if (currPitch == 0) {
		return ".";
	}

	HTp prevToken = getPreviousAttackToken(token);
	if (!prevToken) {
		// first note of the voice (or only preceded by rests): no interval yet
		return "[" + Convert::base40ToKern(currPitch) + "]";
	}

	int prevPitch = getRepresentativeBase40Pitch(prevToken);
	int interval = currPitch - prevPitch;

	string sign;
	if (!m_absoluteQ) {
		if (interval > 0) {
			sign = "+";
		} else if (interval < 0) {
			sign = "-";
		}
	}

	int absInterval = abs(interval);
	int diatonic = Convert::base40IntervalToDiatonic(absInterval) + 1;
	if (m_compoundQ) {
		diatonic = ((diatonic - 1) % 7) + 1;
	}

	if (m_diatonicQ) {
		return sign + to_string(diatonic);
	}

	return sign + getIntervalQuality(absInterval) + to_string(diatonic);
}



//////////////////////////////
//
// Tool_mint::getPreviousAttackToken -- Search backwards in the same voice
//    (spine/subspine) for the previous note attack, ignoring rests, nulls,
//    and tied continuations.  Returns NULL if no such note exists.
//

HTp Tool_mint::getPreviousAttackToken(HTp token) {
	HTp current = token->getPreviousToken();
	while (current) {
		if (!current->isData() || current->isNull() || current->isRest() || !current->isNoteAttack()) {
			current = current->getPreviousToken();
			continue;
		}
		return current;
	}
	return NULL;
}



//////////////////////////////
//
// Tool_mint::getRepresentativeBase40Pitch -- Return the base-40 pitch that
//    represents a (possibly chordal) **kern token: the highest pitch by
//    default, or the lowest pitch if the -l option was given.
//

int Tool_mint::getRepresentativeBase40Pitch(HTp token) {
	vector<int> pitches = m_lowestQ ? token->getBase40PitchesSortLH() : token->getBase40PitchesSortHL();
	for (int i = 0; i < (int)pitches.size(); i++) {
		if (pitches[i] != 0) {
			return abs(pitches[i]);
		}
	}
	return 0;
}



//////////////////////////////
//
// Tool_mint::getIntervalQuality -- Return the interval quality abbreviation
//    (P = perfect, M = major, m = minor, A = augmented, AA = doubly
//    augmented, d = diminished, dd = doubly diminished) for a base-40
//    interval magnitude.
//

string Tool_mint::getIntervalQuality(int base40interval) {
	switch (base40interval % 40) {
		case  0: return "P";  // unison
		case  1: return "A";
		case  2: return "AA";
		case  3: return "X";
		case  4: return "d";
		case  5: return "m";
		case  6: return "M";
		case  7: return "A";
		case  8: return "AA";
		case  9: return "X";
		case 10: return "d";
		case 11: return "m";
		case 12: return "M";
		case 13: return "A";
		case 14: return "AA";
		case 15: return "dd";
		case 16: return "d";
		case 17: return "P";
		case 18: return "A";
		case 19: return "AA";
		case 20: return "X";
		case 21: return "dd";
		case 22: return "d";
		case 23: return "P";
		case 24: return "A";
		case 25: return "AA";
		case 26: return "X";
		case 27: return "d";
		case 28: return "m";
		case 29: return "M";
		case 30: return "A";
		case 31: return "AA";
		case 32: return "X";
		case 33: return "d";
		case 34: return "m";
		case 35: return "M";
		case 36: return "A";
		case 37: return "AA";
		case 38: return "dd";
		case 39: return "d";
		default: return "X";
	}
}


// END_MERGE

} // end namespace hum
