//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Jun 30 22:41:24 PDT 1998
// Last Modified: Sun Apr 14 03:29:59 PDT 2024
// Filename:      humlib/src/MuseRecord-note.cpp
// Web Address:   http://github.com/craigsapp/humlib/blob/master/src/MuseRecord-note.cpp
// Syntax:        C++11
// vim:           ts=3
//
// Description:   Note and rest related functions for the MuseRecord class.
//
// To do: check on gracenotes/cuenotes with chord notes.
//

#include "Convert.h"
#include "HumRegex.h"
#include "HumNum.h"
#include "MuseData.h"
#include "MuseRecord.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

using namespace std;

namespace hum {

// START_MERGE


//////////////////////////////
//
// MuseRecord::getNoteField -- returns the string containing the pitch,
//	accidental and octave characters.
//

string MuseRecord::getNoteField(void) {
	switch (getType()) {
		case E_muserec_note_regular:
			return extract(1, 4);
			break;
		case E_muserec_note_chord:
		case E_muserec_note_cue:
		case E_muserec_note_grace:
			return extract(2, 5);
			break;
		default:
			cerr << "Error: cannot use getNoteField function on line: "
			     << getLine() << endl;
	}
	return "";
}



//////////////////////////////
//
// MuseRecord::getOctave -- returns the first numeric character
//	in the note field of a MuseData note record
//

int MuseRecord::getOctave(void) {
	string recordInfo = getNoteField();
	int index = 0;
	while ((index < (int)recordInfo.size()) && !std::isdigit(recordInfo[index])) {
		index++;
	}
	if (index >= (int)recordInfo.size()) {
		cerr << "Error: no octave specification in note field: " << recordInfo
			  << endl;
		return 0;
	}
	return recordInfo[index] - '0';
}


string MuseRecord::getOctaveString(void) {
	string recordInfo = getNoteField();
	int index = 0;
	while ((index < (int)recordInfo.size()) && !std::isdigit(recordInfo[index])) {
		index++;
	}
	if (index >= (int)recordInfo.size()) {
		cerr << "Error: no octave specification in note field: " << recordInfo
			  << endl;
		return "";
	}
	string output;
	output += recordInfo[index];
	return output;
}



//////////////////////////////
//
// MuseRecord::getPitch -- int version returns the base40 representation
//

int MuseRecord::getPitch(void) {
	string recordInfo = getNoteField();
	return Convert::museToBase40(recordInfo);
}


string MuseRecord::getPitchString(void) {
	string output = getNoteField();
	int len = (int)output.size();
	int index = len-1;
	while (index >= 0 && output[index] == ' ') {
		output.resize(index);
		index--;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getPitchClass -- returns the pitch without the octave information
//

int MuseRecord::getPitchClass(void) {
	return getPitch() % 40;
}


string MuseRecord::getPitchClassString(void) {
	string output = getNoteField();
	int index = 0;
	while ((index < (int)output.size()) &&  !std::isdigit(output[index])) {
		index++;
	}
	output.resize(index);
	return output;
}



//////////////////////////////
//
// MuseRecord::getAccidental -- int version return -2 for double flat,
//	-1 for flat, 0 for natural, +1 for sharp, +2 for double sharp
//

int MuseRecord::getAccidental(void) {
	string recordInfo = getNoteField();
	int output = 0;
	int index = 0;
	while ((index < (int)recordInfo.size()) && (index < 16)) {
		if (recordInfo[index] == 'f') {
			output--;
		} else if (recordInfo[index] == '#') {
			output++;
		}
		index++;
	}
	return output;
}


string MuseRecord::getAccidentalString(void) {
	string output;
	int type = getAccidental();
	switch (type) {
		case -2: output = "ff"; break;
		case -1: output =  "f"; break;
		case  0: output =   ""; break;
		case  1: output =  "#"; break;
		case  2: output = "##"; break;
		default:
			output = getNoteField();
			cerr << "Error: unknown type of accidental: " << output << endl;
			return "";
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getBase40 -- return the base40 pitch value of the data
// line.  Middle C set to 40 * 4 + 2;  Returns -100 for non-pitched items.
// (might have to update for note_cur_chord and note_grace_chord which
// do not exist yet.
//

int MuseRecord::getBase40(void) {
	switch (getType()) {
		case E_muserec_note_regular:
		case E_muserec_note_chord:
		case E_muserec_note_cue:
		case E_muserec_note_grace:
			break;
		default:
			return -100;
	}
	return getPitch();
}



//////////////////////////////
//
// MuseRecord::setStemDown --
//

void MuseRecord::setStemDown(void) {
	getColumn(23) = 'd';
}



//////////////////////////////
//
// MuseRecord::setStemUp --
//

void MuseRecord::setStemUp(void) {
	getColumn(23) = 'u';
}



//////////////////////////////
//
// MuseRecord::setPitch -- input is a base40 value which gets converted
// to a diatonic pitch name.
//    Default value: chordnote = 0
//    Default value: gracenote = 0
//

void MuseRecord::setPitch(int base40, int chordnote, int gracenote) {
	string diatonic;
	switch (Convert::base40ToDiatonic(base40) % 7) {
		case 0:  diatonic = 'C'; break;
		case 1:  diatonic = 'D'; break;
		case 2:  diatonic = 'E'; break;
		case 3:  diatonic = 'F'; break;
		case 4:  diatonic = 'G'; break;
		case 5:  diatonic = 'A'; break;
		case 6:  diatonic = 'B'; break;
		default: diatonic = 'X';
	}

	string octave;
	octave  += char('0' + base40 / 40);

	string accidental;
	int acc = Convert::base40ToAccidental(base40);
	switch (acc) {
		case -2:   accidental = "ff"; break;
		case -1:   accidental = "f";  break;
		case +1:   accidental = "#";  break;
		case +2:   accidental = "##"; break;
	}
	string pitchname = diatonic + accidental + octave;

	if (chordnote) {
		if (gracenote) {
			setGraceChordPitch(pitchname);
		} else {
			setChordPitch(pitchname);
		}
	} else {
		setPitch(pitchname);
	}
}


void MuseRecord::setChordPitch(const string& pitchname) {
	getColumn(1) = ' ';
	setPitchAtIndex(1, pitchname);
}

void MuseRecord::setGracePitch(const string& pitchname) {
	getColumn(1) = 'g';
	setPitchAtIndex(1, pitchname);
}

void MuseRecord::setGraceChordPitch(const string& pitchname) {
	getColumn(1) = 'g';
	getColumn(2) = ' ';
	setPitchAtIndex(2, pitchname);
}

void MuseRecord::setCuePitch(const string& pitchname) {
	getColumn(1) = 'c';
	setPitchAtIndex(1, pitchname);
}


void MuseRecord::setPitch(const string& pitchname) {
	int start = 0;
	// If the record is already set to a grace note or a cue note,
	// then place pitch information starting at column 2 (index 1).
	if ((getColumn(1) == 'g') || (getColumn(1) == 'c')) {
		start = 1;
	}
	setPitchAtIndex(start, pitchname);
}


void MuseRecord::setPitchAtIndex(int index, const string& pitchname) {
	int len = (int)pitchname.size();
	if ((len > 4) && (pitchname != "irest")) {
		cerr << "Error in MuseRecord::setPitchAtIndex: " << pitchname << endl;
		return;
	}
	insertString(index+1, pitchname);

	// Clear any text fields not used by current pitch data.
	for (int i=4-len-1; i>=0; i--) {
		(*this)[index + len + i] = ' ';
	}
}



//////////////////////////////
//
// MuseRecord::getTickDurationField -- returns the string containing the
//      duration, and tie information.
//

string MuseRecord::getTickDurationField(void) {
	switch (getType()) {
		case E_muserec_figured_harmony:
		case E_muserec_note_regular:
		case E_muserec_note_chord:
		case E_muserec_rest:
		case E_muserec_backward:
		case E_muserec_forward:
			return extract(6, 9);
			break;
		// these record types do not have duration, per se:
		case E_muserec_note_cue:
		case E_muserec_note_grace:
		default:
			return "    ";
			// cerr << "Error: cannot use getTickDurationField function on line: "
			//      << getLine() << endl;
			// return "";
	}
	return "";
}



//////////////////////////////
//
// MuseRecord::getTickDurationString -- returns the string containing the duration,
//

string MuseRecord::getTickDurationString(void) {
	string output = getTickDurationField();
	int length = (int)output.size();
	int i = length - 1;
	while (i>0 && (output[i] == '-' || output[i] == ' ')) {
		output.resize(i);
		i--;
		length--;
	}

	int start = 0;
	while (output[start] == ' ') {
		start++;
	}

	if (start != 0) {
		for (i=0; i<length-start; i++) {
			output[i] = output[start+i];
		}
	}
	output.resize(length-start);

	return output;
}



//////////////////////////////
//
// MuseRecord::getTickDuration -- return the tick value found
//    in columns 6-8 in some data type, returning 0 if the record
//    type does not have a duration field.
//

int MuseRecord::getTickDuration(void) {
	string recordInfo = getTickDurationString();
	if (recordInfo.empty()) {
		return 0;
	}
	return std::stoi(recordInfo);
}



//////////////////////////////
//
// MuseRecord::getLineTickDuration -- returns the logical duration of the
//      data line.  Supresses the duration field of secondary chord notes.
//

int MuseRecord::getLineTickDuration(void) {
	if (getType() == E_muserec_note_chord) {
		return 0;
	}

	string recordInfo = getTickDurationString();
	if (recordInfo.empty()) {
		return 0;
	}
	int value = std::stoi(recordInfo);
	if (getType() == E_muserec_backspace) {
		return -value;
	}

	return value;
}



//////////////////////////////
//
// MuseRecord::getTicks -- similar to getLineTickDuration, but is non-zero
//    for secondary chord notes.
//

int MuseRecord::getTicks(void) {
	string recordInfo = getTickDurationString();
	if (recordInfo.empty()) {
		return 0;
	}
	int value = std::stoi(recordInfo);
	if (getType() == E_muserec_backspace) {
		return -value;
	}

	return value;
}


//////////////////////////////
//
// MuseRecord::getNoteTickDuration -- Similar to getLineTickDuration,
//    but do not suppress the duration of secondary chord-tones.
//

int MuseRecord::getNoteTickDuration(void) {
	string recordInfo = getTickDurationString();
	int value = 0;
	if (recordInfo.empty()) {
		return value;
	}
	value = std::stoi(recordInfo);
	if (getType() == E_muserec_backspace) {
		return -value;
	}
	return value;
}



//////////////////////////////
//
// MuseRecord::setDots --
//

void MuseRecord::setDots(int value) {
	switch (value) {
		case 0: getColumn(18) = ' ';   break;
		case 1: getColumn(18) = '.';   break;
		case 2: getColumn(18) = ':';   break;
		case 3: getColumn(18) = ';';   break;
		case 4: getColumn(18) = '!';   break;
		default: cerr << "Error in MuseRecord::setDots : " << value << endl;
	}
}



//////////////////////////////
//
// MuseRecord::getDotCount --
//

int MuseRecord::getDotCount(void) {
	char value = getColumn(18);
	switch (value) {
		case ' ': return 0;
		case '.': return 1;
		case ':': return 2;
		case ';': return 3;
		case '!': return 4;
	}
	return 0;
}



//////////////////////////////
//
// MuseRecord::setNoteheadShape -- Duration with augmentation dot component
//      removed.  Duration of 1 is quarter note.
//

void MuseRecord::setNoteheadShape(HumNum duration) {
	HumNum  note8th(1,2);
	HumNum  note16th(1,4);
	HumNum  note32nd(1,8);
	HumNum  note64th(1,16);
	HumNum  note128th(1,32);
	HumNum  note256th(1,64);

	if (duration > 16) {                 // maxima
		setNoteheadMaxima();
	} else if (duration > 8) {           // long
		setNoteheadLong();
	} else if (duration > 4) {           // breve
		if (m_roundBreve) {
			setNoteheadBreveRound();
		} else {
			setNoteheadBreve();
		}
	} else if (duration > 2) {           // whole note
		setNoteheadWhole();
	} else if (duration > 1) {           // half note
		setNoteheadHalf();
	} else if (duration > note8th) {     // quarter note
		setNoteheadQuarter();
	} else if (duration > note16th) {    // eighth note
		setNotehead8th();
	} else if (duration > note32nd) {    // 16th note
		setNotehead16th();
	} else if (duration > note64th) {    // 32nd note
		setNotehead32nd();
	} else if (duration > note128th) {   // 64th note
		setNotehead64th();
	} else if (duration > note256th) {   // 128th note
		setNotehead128th();
	} else if (duration == note256th) {  // 256th note
		// not allowing tuplets on the 256th note level.
		setNotehead256th();
	} else {
		cerr << "Error in duration: " << duration << endl;
		return;
	}
}



//////////////////////////////
//
// MuseRecord::setNoteheadShape -- Duration with augmentation dot component
//      removed.  Duration of 1 is quarter note.
//

void MuseRecord::setNoteheadShapeMensural(HumNum duration) {
	HumNum note8th(1, 2);
	HumNum note16th(1, 4);
	HumNum note32th(1, 8);
	HumNum note64th(1, 16);
	HumNum note128th(1, 32);
	HumNum note256th(1, 64);

	if (duration > 16) {                 // maxima
		setNoteheadMaxima();
	} else if (duration > 8) {           // long
		setNoteheadLong();
	} else if (duration > 4) {           // breve
		setNoteheadBreve();
	} else if (duration > 2) {           // whole note
		setNoteheadWholeMensural();
	} else if (duration > 1) {           // half note
		setNoteheadHalfMensural();
	} else if (duration > note8th) {     // quarter note
		setNoteheadQuarterMensural();
	} else if (duration > note16th) {    // eighth note
		setNotehead8thMensural();
	} else if (duration > note32th) {    // 16th note
		setNotehead16thMensural();
	} else if (duration > note64th) {    // 32nd note
		setNotehead32ndMensural();
	} else if (duration > note128th) {   // 64th note
		setNotehead64thMensural();
	} else if (duration > note256th) {   // 128th note
		setNotehead128thMensural();
	} else if (duration >= note256th) {  // 256th note
		// don't allow tuplets on 256th note level.
		setNotehead256thMensural();
	} else {
		cerr << "Error in duration: " << duration << endl;
		return;
	}
}

void MuseRecord::setNoteheadMaxima(void) {
	if ((*this)[0] == 'c' || ((*this)[0] == 'g')) {
		cerr << "Error: cue/grace notes cannot be maximas in setNoteheadLong"
			  << endl;
		return;
	} else {
		getColumn(17) = 'M';
	}
}

void MuseRecord::setNoteheadLong(void) {
	if ((*this)[0] == 'c' || ((*this)[0] == 'g')) {
		cerr << "Error: cue/grace notes cannot be longs in setNoteheadLong"
			  << endl;
		return;
	} else {
		getColumn(17) = 'L';
	}
}

void MuseRecord::setNoteheadBreve(void) {
	setNoteheadBreveSquare();
}

void MuseRecord::setNoteheadBreveSquare(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = 'A';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = 'A';
	} else {                        // normal note
		getColumn(17) = 'B';
	}
}

void MuseRecord::setNoteheadBreveRound(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = 'A';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = 'A';
	} else {                        // normal note
		getColumn(17) = 'b';
	}
}

void MuseRecord::setNoteheadBreveMensural(void) {
	setNoteheadBreveSquare();
}

void MuseRecord::setNoteheadWhole(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '9';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '9';
	} else {                        // normal note
		getColumn(17) = 'w';
	}
}

void MuseRecord::setNoteheadWholeMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '9';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '9';
	} else {                        // normal note
		getColumn(17) = 'W';
	}
}

void MuseRecord::setNoteheadHalf(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '8';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '8';
	} else {                        // normal note
		getColumn(17) = 'h';
	}
}

void MuseRecord::setNoteheadHalfMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '8';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '8';
	} else {                        // normal note
		getColumn(17) = 'H';
	}
}

void MuseRecord::setNoteheadQuarter(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '7';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '7';
	} else {                        // normal note
		getColumn(17) = 'q';
	}
}

void MuseRecord::setNoteheadQuarterMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '7';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '7';
	} else {                        // normal note
		getColumn(17) = 'Q';
	}
}

void MuseRecord::setNotehead8th(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '6';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '6';
	} else {                        // normal note
		getColumn(17) = 'e';
	}
}

void MuseRecord::setNotehead8thMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '6';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '6';
	} else {                        // normal note
		getColumn(17) = 'E';
	}
}

void MuseRecord::setNotehead16th(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '5';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '5';
	} else {                        // normal note
		getColumn(17) = 's';
	}
}

void MuseRecord::setNotehead16thMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '5';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '5';
	} else {                        // normal note
		getColumn(17) = 'S';
	}
}

void MuseRecord::setNotehead32nd(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '4';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '4';
	} else {                        // normal note
		getColumn(17) = 't';
	}
}

void MuseRecord::setNotehead32ndMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '4';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '4';
	} else {                        // normal note
		getColumn(17) = 'T';
	}
}

void MuseRecord::setNotehead64th(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '3';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '3';
	} else {                        // normal note
		getColumn(17) = 'x';
	}
}

void MuseRecord::setNotehead64thMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '3';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '3';
	} else {                        // normal note
		getColumn(17) = 'X';
	}
}

void MuseRecord::setNotehead128th(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '2';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '2';
	} else {                        // normal note
		getColumn(17) = 'y';
	}
}

void MuseRecord::setNotehead128thMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '2';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '2';
	} else {                        // normal note
		getColumn(17) = 'Y';
	}
}

void MuseRecord::setNotehead256th(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '1';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '1';
	} else {                        // normal note
		getColumn(17) = 'z';
	}
}

void MuseRecord::setNotehead256thMensural(void) {
	if ((*this)[0] == 'g') {        // grace note
		getColumn(8) = '1';
	} else if ((*this)[0] == 'c') { // cue-sized note (with duration)
		getColumn(17) = '1';
	} else {                        // normal note
		getColumn(17) = 'Z';
	}
}


/////////////////////////////
//
// MuseRecord::setBack --
//

void MuseRecord::setBack(int value) {
	insertString(1, "back");
	setTicks(value);
}



/////////////////////////////
//
// MuseRecord::setTicks -- return the numeric value in columns 6-9.
//

void MuseRecord::setTicks(int value) {
	if ((value < 0) || (value >= 1000)) {
		cerr << "@ Error: ticks out of range in MuseRecord::setTicks" << endl;
	}
	stringstream ss;
	ss << value;
	int len = (int)ss.str().size();
	insertString(5+3-len+1, ss.str());
}



//////////////////////////////
//
// MuseRecord::getTie --
//

string MuseRecord::getTieString(void) {
	string output;
	output += getColumn(9);
	if (output == " ") {
		output = "";
	}
	return output;
}


int MuseRecord::getTie(void) {
	return tieQ();
}


//////////////////////////////
//
// MuseRecord::getTie -- Set a tie marker in column 9.  Currently
// the function does not check the type of data, so will overr-write any
// data found in column 9 (such as if the record is not for a note).
//
// If the input parameter hidden is true, then the visual tie is not
// displayed, but the sounding tie is displayed.
//

int MuseRecord::setTie(int hidden) {
	getColumn(9) = '-';
	if (!hidden) {
		return addAdditionalNotation('-');
	} else {
		return -1;
	}
}



//////////////////////////////
//
// MuseRecord::addAdditionalNotation -- ties, slurs and tuplets.
//    Currently not handling editorial levels.
//

int MuseRecord::addAdditionalNotation(char symbol) {
	// search columns 32 to 43 for the specific symbol.
	// if it is found, then don't add.  If it is not found,
	// then do add.
	int i;
	int blank = -1;
	int nonempty = 0;  // true if a non-space character was found.

	for (i=43; i>=32; i--) {
		if (getColumn(i) == symbol) {
			return i;
		} else if (!nonempty && (getColumn(i) == ' ')) {
			blank = i;
		} else {
			nonempty = i;
		}
	}

	if (symbol == '-') {
		// give preferential treatment to placing only ties in
		// column 32
		if (getColumn(32) == ' ') {
			getColumn(32) = '-';
			return 32;
		}
	}

	if (blank < 0) {
		cerr << "Error in MuseRecord::addAdditionalNotation: "
			  << "no empty space for notation" << endl;
		return 0;
	}

	if ((blank <= 32) && (getColumn(33) == ' ')) {
		// avoid putting non-tie items in column 32.
		blank = 33;
	}

	getColumn(blank) = symbol;
	return blank;
}


// add a multi-character additional notation (such as a dynamic like mf):

int MuseRecord::addAdditionalNotation(const string& symbol) {
	int len = (int)symbol.size();
	// search columns 32 to 43 for the specific symbol.
	// if it is found, then don't add.  If it is not found,
	// then do add.
	int i, j;
	int blank = -1;
	int found = 0;
	int nonempty = 0;  // true if a non-space character was found.

	for (i=43-len; i>=32; i--) {
		found = 1;
		for (j=0; j<len; j++) {
			if (getColumn(i+j) != symbol[j]) {
				found = 0;
				break;
			}
		}
		if (found) {
			return i;
		} else if (!nonempty && (getColumn(i) == ' ')) {
// cout << "@COLUMN " << i << " is blank: " << getColumn(i) << endl;
			blank = i;
			// should check that there are enough blank lines to the right
			// as well...
		} else if (getColumn(i) != ' ') {
			nonempty = i;
		}
	}

	if (blank < 0) {
		cerr << "Error in MuseRecord::addAdditionalNotation2: "
			  << "no empty space for notation" << endl;
		return 0;
	}

// cout << "@ GOT HERE symbol = " << symbol << " and blank = " << blank << endl;
	if ((blank <= 32) && (getColumn(33) == ' ')) {
		// avoid putting non-tie items in column 32.
		blank = 33;
		// not worrying about overwriting something to the right
		// of column 33 since the empty spot was checked starting
		// on the right and moving towards the left.
	}
// cout << "@COLUMN 33 = " << getColumn(33) << endl;
// cout << "@ GOT HERE symbol = " << symbol << " and blank = " << blank << endl;

	for (j=0; j<len; j++) {
		getColumn(blank+j) = symbol[j];
	}
	return blank;
}



//////////////////////////////
//
// MuseRecord::tieQ -- returns true if the current line contains
//   a tie to a note in the future.  Does not check if there is a tie
//   to a note in the past.
//

int MuseRecord::tieQ(void) {
	int output = 0;
	switch (getType()) {
		case E_muserec_note_regular:
		case E_muserec_note_chord:
		case E_muserec_note_cue:
		case E_muserec_note_grace:
			if (getColumn(9) == '-') {
				output = 1;
			} else if (getColumn(9) == ' ') {
				output = 0;
			} else {
				output = -1;
			}
			break;
		default:
			return 0;
	}

	return output;
}


//////////////////////////////////////////////////////////////////////////
//
// graphical and intrepretive information for notes
//

//////////////////////////////
//
// MuseRecord::getFootnoteFlagField -- returns column 13 value
//

string MuseRecord::getFootnoteFlagField(void) {
	allowFigurationAndNotesOnly("getFootnoteField");
	return extract(13, 13);
}



//////////////////////////////
//
// MuseRecord::getFootnoteFlagString --
//

string MuseRecord::getFootnoteFlagString(void) {
	string output = getFootnoteFlagField();
	if (output[0] == ' ') {
		output = "";
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getFootnoteFlag --
//

int MuseRecord::getFootnoteFlag(void) {
	int output = 0;
	string recordInfo = getFootnoteFlagString();
	if (recordInfo[0] == ' ') {
		output = -1;
	} else {
		output = (int)strtol(recordInfo.c_str(), NULL, 36);
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::footnoteFlagQ --
//

int MuseRecord::footnoteFlagQ(void) {
	int output = 0;
	string recordInfo = getFootnoteFlagField();
	if (recordInfo[0] == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getLevelField -- return column 14
//

string MuseRecord::getLevelField(void) {
	allowFigurationAndNotesOnly("getLevelField");
	return extract(14, 14);
}



//////////////////////////////
//
// MuseRecord::getLevel --
//

string MuseRecord::getLevelString(void) {
	string output = getLevelField();
	if (output[0] == ' ') {
		output = "";
	}
	return output;
}


int MuseRecord::getLevel(void) {
	int output = 1;
	string recordInfo = getLevelField();
	if (recordInfo[0] == ' ') {
		output = 1;
	} else {
		output = (int)strtol(recordInfo.c_str(), NULL, 36);
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::levelQ --
//

int MuseRecord::levelQ(void) {
	int output = 0;
	string recordInfo = getLevelField();
	if (recordInfo[0] == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getTrackField -- return column 15
//

string MuseRecord::getTrackField(void) {
	if (!isAnyNoteOrRest()) {
		return extract(15, 15);
	} else {
		return " ";
	}
}



//////////////////////////////
//
// MuseRecord::getTrackString --
//

string MuseRecord::getTrackString(void) {
	string output = getTrackField();
	if (output[0] == ' ') {
		output = "";
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getTrack -- Return 0 if no track information (implicitly track 1,
//     or unlabelled higher track).
//

int MuseRecord::getTrack(void) {
	int output = 1;
	string recordInfo = getTrackField();
	if (recordInfo[0] == ' ') {
		output = 0;
	} else {
		output = (int)strtol(recordInfo.c_str(), NULL, 36);
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::trackQ --
//

int MuseRecord::trackQ(void) {
	int output = 0;
	string recordInfo = getTrackField();
	if (recordInfo[0] == ' ') {
		output = 0;
	} else {
		output = 1;
	}

	return output;
}



//////////////////////////////
//
// MuseRecord::getGraphicNoteTypeField -- return column 17
//

string MuseRecord::getGraphicNoteTypeField(void) {
// allowNotesOnly("getGraphicNoteTypefield");
	if (getLength() < 17) {
		return " ";
	} else {
		return extract(17, 17);
	}
}



//////////////////////////////
//
// MuseRecord::getGraphicNoteType --
//

string MuseRecord::getGraphicNoteTypeString(void) {
	string output = getGraphicNoteTypeField();
	if (output[0] == ' ') {
		output = "";
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getGraphicRecip --
//

string MuseRecord::getGraphicRecip(void) {
	int notetype = getGraphicNoteType();
	string output;
	switch (notetype) {
		case -3: output = "0000"; break;  // double-maxima
		case -2: output = "000"; break;   // maxima
		case -1: output = "00"; break;    // long
		default:
			output = to_string(notetype);  // regular **recip number
	}
	int dotcount = getDotCount();
	for (int i=0; i<dotcount; i++) {
		output += '.';
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getGraphicNoteType --
//

int MuseRecord::getGraphicNoteType(void) {
	int output = 0;
	string recordInfo = getGraphicNoteTypeField();
	if (recordInfo[0] == ' ') {
		if (isInvisibleRest()) {
			// invisible rests do not have graphic note types
			// so make one up from the logical note type
			HumNum value = getTickDuration();
			value /= getTpq();
			if (value >= 32) {
				return -2;
			} else if (value >= 16) {
				return -1;
			} else if (value >= 8) {
				return 0;
			} else if (value >= 4) {
				return 1;
			} else if (value >= 2) {
				return 2;
			} else if (value >= 1) {
				return 4;
			} else if (value.getFloat() >= 0.5) {
				return 8;
			} else if (value.getFloat() >= 0.25) {
				return 16;
			} else if (value.getFloat() >= 0.125) {
				return 32;
			} else if (value.getFloat() >= 0.0625) {
				return 64;
			} else if (value.getFloat() >= 1.0/128) {
				return 128;
			} else if (value.getFloat() >= 1.0/256) {
				return 256;
			} else if (value.getFloat() >= 1.0/512) {
				return 512;
			} else {
				return 0;
			}
		} else {
			cerr << "Error: no graphic note type specified: " << getLine() << endl;
			return 0;
		}
	}

	switch (recordInfo[0]) {
		case 'M':                          // Maxima
			output = -2;           break;
		case 'L':   case 'B':              // Longa
			output = -1;           break;
		case 'b':   case 'A':              // Breve
			output = 0;            break;
		case 'w':   case '9':              // Whole
			output = 1;            break;
		case 'h':   case '8':              // Half
			output = 2;            break;
		case 'q':   case '7':              // Quarter
			output = 4;            break;
		case 'e':   case '6':              // Eighth
			output = 8;            break;
		case 's':   case '5':              // Sixteenth
			output = 16;           break;
		case 't':   case '4':              // 32nd note
			output = 32;           break;
		case 'x':   case '3':              // 64th note
			output = 64;           break;
		case 'y':   case '2':              // 128th note
			output = 128;          break;
		case 'z':   case '1':              // 256th note
			output = 256;          break;
		default:
			cerr << "Error: unknown graphical note type in column 17: "
				  << getLine() << endl;
	}

	return output;
}


//////////////////////////////
//
// MuseRecord::graphicNoteTypeQ --
//

int MuseRecord::graphicNoteTypeQ(void) {
	int output = 0;
	string recordInfo = getGraphicNoteTypeField();
	if (recordInfo[0] == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::graphicNoteTypeSize -- return 0 if cue note size,
//	otherwise, it will return 1 if regular size
//

int MuseRecord::getGraphicNoteTypeSize(void) {
	int output = 1;
	string recordInfo = getGraphicNoteTypeField();
	if (recordInfo[0] == ' ') {
		cerr << "Error: not graphic note specified in column 17: "
			  << getLine() << endl;
		return 0;
	}

	switch (recordInfo[0]) {
		case 'L': case 'b': case 'w': case 'h': case 'q': case 'e':
		case 's': case 't': case 'x': case 'y': case 'z':
			output = 1;
			break;
		case 'B': case 'A': case '9': case '8': case '7': case '6':
		case '5': case '4': case '3': case '2': case '1':
			output = 0;
			break;
		default:
			cerr << "Error: unknown graphical note type in column 17: "
				  << getLine() << endl;
			return 0;
	}

	return output;
}



//////////////////////////////
//
// MuseRecord::getProlongationField -- returns column 18
//

string MuseRecord::getProlongationField(void) {
//   allowNotesOnly("getProlongationField");   ---> rests also
	if (getLength() < 18) {
		return " ";
	} else {
		return extract(18, 18);
	}
}



//////////////////////////////
//
// MuseRecord::getProlongationString --
//

string MuseRecord::getProlongationString(void) {
	string output = getProlongationField();
	if (output[0] == ' ') {
		output = "";
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getProlongation --
//

int MuseRecord::getProlongation(void) {
	int output = 0;
	string recordInfo = getProlongationField();
	switch (recordInfo[0]) {
		case ' ':   output = 0;   break;
		case '.':   output = 1;   break;
		case ':':   output = 2;   break;
		default:
			cerr << "Error: unknon prologation character (column 18): "
				  << getLine() << endl;
			return 0;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getStringProlongation --
//

string MuseRecord::getStringProlongation(void) {
	switch (getProlongation()) {
		case 0:   return "";     break;
		case 1:   return ".";    break;
		case 2:   return "..";   break;
		case 3:   return "...";  break;
		case 4:   return "...."; break;
		default:
			cerr << "Error: unknown number of prolongation dots (column 18): "
				  << getLine() << endl;
			return "";
	}
	return "";
}



//////////////////////////////
//
// MuseRecord::prolongationQ --
//

int MuseRecord::prolongationQ(void) {
	return getProlongation();
}


//////////////////////////////
//
// MuseRecord::getNotatedAccidentalField -- actual notated accidental is
//     stored in column 19.
//

string MuseRecord::getNotatedAccidentalField(void) {
	allowNotesOnly("getNotatedAccidentalField");
	if (getLength() < 19) {
		return " ";
	} else {
		string temp;
		temp += getColumn(19);
		return temp;
	}
}



//////////////////////////////
//
// MuseRecord::getNotatedAccidentalString --
//

string MuseRecord::getNotatedAccidentalString(void) {
	string output = getNotatedAccidentalField();
	if (output[0] == ' ') {
		output = "";
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getNotatedAccidental --
//

int MuseRecord::getNotatedAccidental(void) {
	int output = 0;
	string recordInfo = getNotatedAccidentalField();
	switch (recordInfo[0]) {
		case ' ':   output =  0;   break;
		case '#':   output =  1;   break;
		case 'n':   output =  0;   break;
		case 'f':   output = -1;   break;
		case 'x':   output =  2;   break;
		case 'X':   output =  2;   break;
		case '&':   output = -2;   break;
		case 'S':   output =  1;   break;
		case 'F':   output = -1;   break;
		default:
			cerr << "Error: unknown accidental: " << recordInfo[0] << endl;
			return 0;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::notatedAccidentalQ --
//

int MuseRecord::notatedAccidentalQ(void) {
	int output;
	string recordInfo = getNotatedAccidentalField();
	if (recordInfo[0] == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



///////////////////////////////
//
// MuseRecord::getTimeModificationField -- return columns 20 -- 22.
//

string MuseRecord::getTimeModificationField(void) {
//   allowNotesOnly("getTimeModificationField");   ---> rests also
	if (getLength() < 20) {
		return  "   ";
	} else {
		return extract(20, 22);
	}
}



//////////////////////////////
//
// MuseRecord::getTimeModificationString --
//

string MuseRecord::getTimeModificationString(void) {
	string output = getTimeModificationField();
	HumRegex hre;
	if (hre.search(output, "[1-9A-Z]:[1-9A-Z]")) {
		return output;
	}
	return "";
}



//////////////////////////////
//
// MuseRecord::getTimeModification --
//

HumNum MuseRecord::getTimeModification(void) {
	string output = getTimeModificationField();
	HumRegex hre;
	if (hre.search(output, "([1-9A-Z]):([1-9A-Z])")) {
		string top = hre.getMatch(1);
		string bot = hre.getMatch(2);
		int topint = (int)strtol(top.c_str(), NULL, 36);
		int botint = (int)strtol(top.c_str(), NULL, 36);
		HumNum number(topint, botint);
		return number;
	} else {
		if (hre.search(output, "^([1-9A-Z])")) {
			string value = hre.getMatch(1);
			int top = (int)strtol(value.c_str(), NULL, 36);
			// Time modification can be "3  " for triplets.
			HumNum out(top, 2);
			return out;
		} else {
			return 1;
		}
	}
}



//////////////////////////////
//
// MuseRecord::getTimeModificationLeftField -- return column 20
//

string MuseRecord::getTimeModificationLeftField(void) {
	string output = getTimeModificationField();
	HumRegex hre;
	if (!hre.search(output, "^[1-9A-Z]:[1-9A-Z]$")) {
		return " ";
	}
	return output.substr(0, 1);
}



//////////////////////////////
//
// MuseRecord::getTimeModificationLeftString --
//

string MuseRecord::getTimeModificationLeftString(void) {
	string output = getTimeModificationField();
	HumRegex hre;
	if (!hre.search(output, "^[1-9A-Z]:[1-9A-Z]$")) {
		return "";
	}
	return output.substr(0, 1);
}



//////////////////////////////
//
// MuseRecord::getTimeModificationLeft --
//

int MuseRecord::getTimeModificationLeft(void) {
	int output = 0;
	string recordInfo = getTimeModificationLeftString();
	if (recordInfo.empty()) {
		return 1;
	} else {
		output = (int)strtol(recordInfo.c_str(), NULL, 36);
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getTimeModificationRightField -- return column 20
//

string MuseRecord::getTimeModificationRightField(void) {
	string output = getTimeModificationField();
	output = output[2];
	return output;
}



//////////////////////////////
//
// MuseRecord::getTimeModificationRight --
//

string MuseRecord::getTimeModificationRightString(void) {
	HumRegex hre;
	string output = getTimeModificationField();
	if (!hre.search(output, "^[1-9A-Z]:[1-9A-Z]$")) {
		return " ";
	}
	return output.substr(2, 1);
}



//////////////////////////////
//
// MuseRecord::getTimeModificationRight --
//

int MuseRecord::getTimeModificationRight(void) {
	string recordInfo = getTimeModificationRightString();
	HumRegex hre;
	if (recordInfo.empty()) {
		return 1;
	} else if (!hre.search(recordInfo, "^[1-9A-Z]$")) {
		return 1;
	} else {
		return (int)strtol(recordInfo.c_str(), NULL, 36);
	}
}



//////////////////////////////
//
// MuseRecord::timeModificationQ --
//

bool MuseRecord::timeModificationQ(void) {
	string recordInfo = getTimeModificationField();
	HumRegex hre;
	if (hre.search(recordInfo, "^[1-9A-Z]:[1-9A-Z]$")) {
		return true;
	} else {
		return false;
	}
}



//////////////////////////////
//
// MuseRecord::timeModificationLeftQ --
//

bool MuseRecord::timeModificationLeftQ(void) {
	string recordInfo = getTimeModificationField();
	HumRegex hre;
	string value;
	value.push_back(recordInfo.at(0));
	if (hre.search(value, "^[1-9A-Z]$")) {
		return true;
	} else {
		return false;
	}
}



//////////////////////////////
//
// MuseRecord::timeModificationRightQ --
//

bool MuseRecord::timeModificationRightQ(void) {
	string recordInfo = getTimeModificationField();
	HumRegex hre;
	string value;
	value.push_back(recordInfo.at(0));
	if (hre.search(value, "^[1-9A-Z]$")) {
		return true;
	} else {
		return false;
	}
}



//////////////////////////////
//
// MuseRecord::getStemDirectionField --
//

string MuseRecord::getStemDirectionField(void) {
	allowNotesOnly("getStemDirectionField");
	if (getLength() < 23) {
		return " ";
	} else {
		string temp;
		temp += getColumn(23);
		return temp;
	}
}



//////////////////////////////
//
// MuseRecord::getStemDirectionString --
//

string MuseRecord::getStemDirectionString(void) {
	string output = getStemDirectionField();
	if (output[0] == ' ') {
		output = "";
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getStemDirection --
//

int MuseRecord::getStemDirection(void) {
	int output = 0;
	string recordInfo = getStemDirectionField();
	switch (recordInfo[0]) {
		case 'u':   output = 1;   break;
		case 'd':   output = -1;  break;
		case ' ':   output = 0;   break;
		default:
			cerr << "Error: unknown stem direction: " << recordInfo[0] << endl;
			return 0;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::stemDirectionQ --
//

int MuseRecord::stemDirectionQ(void) {
	int output = 0;
	string recordInfo = getStemDirectionField();
	if (recordInfo[0] == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getStaffField  -- returns column 24.
//

string MuseRecord::getStaffField(void) {
	allowNotesOnly("getStaffField");
	if (getLength() < 24) {
		return " ";
	} else {
		string temp;
		temp += getColumn(24);
		return temp;
	}
}



//////////////////////////////
//
// MuseRecord::getStaffString --
//

string MuseRecord::getStaffString(void) {
	string output = getStaffField();
	if (output[0] == ' ') {
		output = "";
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getStaff --
//

int MuseRecord::getStaff(void) {
	int output = 1;
	string recordInfo = getStaffField();
	if (recordInfo[0] == ' ') {
		output = 1;
	} else {
		output = (int)strtol(recordInfo.c_str(), NULL, 36);
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::staffQ --
//

int MuseRecord::staffQ(void) {
	int output = 0;
	string recordInfo = getStaffField();
	if (recordInfo[0] == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getBeamField --
//

string MuseRecord::getBeamField(void) {
	allowNotesOnly("getBeamField");
	if (getLength() < 26) {
		return "      ";
	} else {
		return extract(26, 31);
	}
}



//////////////////////////////
//
// MuseRecord::setBeamInfo --
//

void MuseRecord::setBeamInfo(string& strang) {
	setColumns(strang, 26, 31);
}



//////////////////////////////
//
// MuseRecord::beamQ --
//

int MuseRecord::beamQ(void) {
	int output = 0;
	allowNotesOnly("beamQ");
	if (getLength() < 26) {
		output = 0;
	} else {
		for (int i=26; i<=31; i++) {
			if (getColumn(i) != ' ') {
				output = 1;
				break;
			}
		}
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getBeam8 -- column 26
//

char MuseRecord::getBeam8(void) {
	allowNotesOnly("getBeam8");
	return getColumn(26);
}



//////////////////////////////
//
// MuseRecord::getBeam16 -- column 27
//

char MuseRecord::getBeam16(void) {
	allowNotesOnly("getBeam16");
	return getColumn(27);
}



//////////////////////////////
//
// MuseRecord::getBeam32 -- column 28
//

char MuseRecord::getBeam32(void) {
	allowNotesOnly("getBeam32");
	return getColumn(28);
}



//////////////////////////////
//
// MuseRecord::getBeam64 -- column 29
//

char MuseRecord::getBeam64(void) {
	allowNotesOnly("getBeam64");
	return getColumn(29);
}



//////////////////////////////
//
// MuseRecord::getBeam128 -- column 30
//

char MuseRecord::getBeam128(void) {
	allowNotesOnly("getBeam128");
	return getColumn(30);
}



//////////////////////////////
//
// MuseRecord::getBeam256 -- column 31
//

char MuseRecord::getBeam256(void) {
	allowNotesOnly("getBeam256");
	return getColumn(31);
}



//////////////////////////////
//
// MuseRecord::beam8Q --
//

int MuseRecord::beam8Q(void) {
	int output = 0;
	if (getBeam8() == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::beam16Q --
//

int MuseRecord::beam16Q(void) {
	int output = 0;
	if (getBeam16() == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::beam32Q --
//

int MuseRecord::beam32Q(void) {
	int output = 0;
	if (getBeam32() == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::beam64Q --
//

int MuseRecord::beam64Q(void) {
	int output = 0;
	if (getBeam64() == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::beam128Q --
//

int MuseRecord::beam128Q(void) {
	int output = 0;
	if (getBeam128() == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::beam256Q --
//

int MuseRecord::beam256Q(void) {
	int output = 0;
	if (getBeam256() == ' ') {
		output = 0;
	} else {
		output = 1;
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getAdditionalNotationsField -- returns the contents
// 	of columns 32-43.
//

string MuseRecord::getAdditionalNotationsField(void) {
	allowNotesOnly("getAdditionalNotationsField");
	return extract(32, 43);
}



//////////////////////////////
//
// MuseRecord::additionalNotationsQ --
//

int MuseRecord::additionalNotationsQ(void) {
	int output = 0;
	if (getLength() < 32) {
		output = 0;
	} else {
		for (int i=32; i<=43; i++) {
			if (getColumn(i) != ' ') {
				output = 1;
				break;
			}
		}
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getAddCount -- returns the number of items
//	in the additional notations field
//

int MuseRecord::getAddCount(void) {
	string addString = getAdditionalNotationsField();
	string addElement;    // element from the notation field

	int count = 0;
	int index = 0;
	while (getAddElementIndex(index, addElement, addString)) {
		count++;
	}

	return count;
}



//////////////////////////////
//
// MuseRecord::getAddItem -- returns the specified item
//	in the additional notations field
//

string MuseRecord::getAddItem(int elementIndex) {
	string output;
	int count = 0;
	int index = 0;
	string addString = getAdditionalNotationsField();

	while (count <= elementIndex) {
		getAddElementIndex(index, output, addString);
		count++;
	}

	return output;
}



//////////////////////////////
//
// MuseRecord::getAddItemLevel -- returns the specified item's
//	editorial level in the additional notations field
//

int MuseRecord::getAddItemLevel(int elementIndex) {
	int count = 0;
	int index = 0;
	string number;
	string addString = getAdditionalNotationsField();
	string elementString; // element field

	while (count < elementIndex) {
		getAddElementIndex(index, elementString, addString);
		count++;
	}

	int output = -1;
repeating:
	while (addString[index] != '&' && index >= 0) {
		index--;
	}
	if (addString[index] == '&' && !isalnum(addString[index+1])) {
		index--;
		goto repeating;
	} else if (addString[index] == '&') {
		number = addString[index+1];
		output = (int)strtol(number.c_str(), NULL, 36);
	}

	return output;
}



//////////////////////////////
//
// MuseRecord::getEditorialLevels -- returns a string containing the
//	edit levels given in the additional notation fields
//

string MuseRecord::getEditorialLevels(void) {
	string output;
	string addString = getAdditionalNotationsField();
	for (int index = 0; index < 12-1; index++) {
		if (addString[index] == '&' && isalnum(addString[index+1])) {
			output += addString[index+1];
		}
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::addEditorialLevelQ -- returns true if there are any editorial
//	levels present in the additional notations fields
//

int MuseRecord::addEditorialLevelQ(void) {
	string addString = getAdditionalNotationsField();
	int output = 0;
	for (int i=0; i<12-1; i++) {   // minus one for width 2 (&0)
		if (addString[i] == '&' && isalnum(addString[i+1])) {
			output = 1;
		}
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::findField -- returns true when it finds the first
//	instance of the key in the additional fields record.
//

int MuseRecord::findField(const string& key) {
	int len = (int)key.size();
	string notations = getAdditionalNotationsField();
	int output = 0;
	for (int i=0; i<12-len; i++) {
		if (notations[i] == key[0]) {
			output = 1;
			for (int j=0; j<len; j++) {
				if (notations[i] != key[j]) {
					output = 0;
					goto endofloop;
				}
			}
		}

		if (output == 1) {
			break;
		}
endofloop:
	;
	}

	return output;
}



//////////////////////////////
//
// MuseRecord::findField --
//

int MuseRecord::findField(char key, int mincol, int maxcol) {
	int start = mincol;
	int stop = getLength() - 1;

	if (start > stop) {
		return -1;
	}

	if (maxcol < stop) {
		stop = maxcol;
	}

	int i;
	for (i=start; i<=stop; i++) {
		if (m_recordString[i-1] == key) {
			return i;   // return the column which is offset from 1
		}
	}

	return -1;
}



//////////////////////////////
//
// MuseRecord::getSlurParameterRegion --
//

string MuseRecord::getSlurParameterRegion(void) {
	return getColumns(31, 43);
}



//////////////////////////////
//
// MuseRecord::getSlurStartColumn -- search column 32 to 43 for a slur
//    marker.  Returns the first one found from left to right.
//    returns -1 if a slur character was not found.
//

int MuseRecord::getSlurStartColumn(void) {
	int start = 31;
	int stop = getLength() - 1;
	if (stop >= 43) {
		stop = 42;
	}
	int i;
	for (i=start; i<=stop; i++) {
		switch (m_recordString[i]) {
			case '(':   // slur level 1
			case '[':   // slur level 2
			case '{':   // slur level 3
			case 'z':   // slur level 4
				return i+1;  // column is offset from 1
		}
	}

	return -1;
}



//////////////////////////////
//
// MuseRecord::getTextUnderlayField -- returns the contents
// 	of columns 44-80.
//

string MuseRecord::getTextUnderlayField(void) {
	allowNotesOnly("getTextUnderlayField");
	return extract(44, 80);
}



//////////////////////////////
//
// MuseRecord::textUnderlayQ --
//

int MuseRecord::textUnderlayQ(void) {
	int output = 0;
	if (getLength() < 44) {
		output = 0;
	} else {
		for (int i=44; i<=80; i++) {
			if (getColumn(i) != ' ') {
				output = 1;
				break;
			}
		}
	}
	return output;
}



//////////////////////////////
//
// MuseRecord::getVerseCount --
//

int MuseRecord::getVerseCount(void) {
	if (!textUnderlayQ()) {
		return 0;
	}

	int count = 1;
	for (int i=44; i<=getLength() && i <= 80; i++) {
		if (getColumn(i) == '|') {
			count++;
		}
	}

	return count;
}



//////////////////////////////
//
// MuseRecord::getVerse --
//

string MuseRecord::getVerse(int index) {
	string output;
	if (!textUnderlayQ()) {
		return output;
	}
	int verseCount = getVerseCount();
	if (index >= verseCount) {
		return output;
	}

	int tindex = 44;
	int c = 0;
	while (c < index && tindex < 80) {
		if (getColumn(tindex) == '|') {
			c++;
		}
		tindex++;
	}

	while (tindex <= 80 && getColumn(tindex) != '|') {
		output += getColumn(tindex++);
	}

	// remove trailing spaces
	int zindex = (int)output.size() - 1;
	while (output[zindex] == ' ') {
		zindex--;
	}
	zindex++;
	output.resize(zindex);

	// remove leading spaces
	int spacecount = 0;
	while (output[spacecount] == ' ') {
		spacecount++;
	}

	// problem here?
	for (int rr = 0; rr <= zindex-spacecount; rr++) {
		output[rr] = output[rr+spacecount];
	}

	return output;
}



//////////////////////////////
//
// MuseRecord::getVerseUtf8 --
//

string MuseRecord::getVerseUtf8(int index) {
	string tverse = getVerse(index);
	return MuseRecord::musedataToUtf8(tverse);
}



//////////////////////////////
//
// MuseRecord::getSlurInfo --
//
//   ( ) = regular slur
//   [ ] = second levels slur, convert to &( and &)
//   { } = third level slur, convert to &&( and &&)
//   Z   = fourth level slur (how to close?)
//

void MuseRecord::getSlurInfo(string& slurstarts, string& slurends) {
	slurstarts.clear();
	slurends.clear();

	string data = getSlurParameterRegion();
	for (int i=0; i<(int)data.size(); i++) {
		if (data[i] == '(') {
			slurstarts += '(';
		} else if (data[i] == ')') {
			slurends += ')';
		} else if (data[i] == '[') {
			slurstarts += "&{";
		} else if (data[i] == ']') {
			slurends += "&)";
		} else if (data[i] == '{') {
			slurstarts += "&&(";
		} else if (data[i] == '}') {
			slurends += "&&)";
		}
	}
}


// END_MERGE

} // end namespace hum



