//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sat Aug  8 12:24:49 PDT 2015
// Last Modified: Fri Aug 14 22:45:49 PDT 2015
// Filename:      HumdrumFileBase.h
// URL:           https://github.com/craigsapp/minHumdrum/blob/master/include/HumdrumFileBase.h
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// Description:   Used to store Humdrum text lines from input stream
//                for further parsing.  This class analyzes the basic
//                spine structure after reading a Humdrum file.  The
//                HumdrumFileStructure class continues structural analysis,
//                primarily of rhythm (generated by **kern, **recip and
//                **koto data).
//

#ifndef _HUMDRUMFILEBASE_H
#define _HUMDRUMFILEBASE_H

#include <iostream>
#include <vector>

#include "HumdrumLine.h"

using namespace std;

namespace minHumdrum {

// START_MERGE

// The following options are used for get[Primary]TrackTokens.
// * OPT_NONULLS    => don't include  null tokens in extracted list
// * OPT_NOMANIP    => don't include  spine manipulators (*^, *v, *x, *+,
//                        but still keep ** and *0).
// * OPT_NOGLOBALS  => don't include global records (global comments, reference
//                        records, and empty lines). In other words, only return
//                        a list of tokens from lines which hasSpines() it true.
#define OPT_NONULLS  0x01
#define OPT_NOMANIP  0x02
#define OPT_NOGLOBAL 0x04

class HumdrumFileBase {
	public:
		              HumdrumFileBase              (void);
		             ~HumdrumFileBase              ();

		bool          read                         (istream& infile);
		bool          read                         (const char*   filename);
		bool          read                         (const string& filename);
		bool          readString                   (const char*   contents);
		bool          readString                   (const string& contents);
		bool parse(istream& contents) { return read(contents); }
		bool parse(const char* contents) { return readString(contents); }
		bool parse(const string& contents) { return readString(contents); }

		HumdrumLine&  operator[]                   (int index);
		int           getLineCount                 (void) const;
		HumdrumToken& token                        (int lineindex, int fieldindex);
		int           getMaxTrack                  (void) const;
		int           getMaxSpine   (void) const { return getMaxTrack(); }
		ostream&      printSpineInfo               (ostream& out = cout);
		ostream&      printDataTypeInfo            (ostream& out = cout);
		ostream&      printTrackInfo               (ostream& out = cout);

		HumdrumToken* getTrackStart                (int track) const;
		int           getTrackEndCount             (int track) const;
		HumdrumToken* getTrackEnd                  (int track,
		                                            int subtrack) const;
		void          createLinesFromTokens        (void);
		void          append                       (const char* line);
		void          append                       (const string& line);

		// spine analysis functionality
		vector<vector<HumdrumToken*> > getTrackSeq       (int track, 
		                                                  int options);
		vector<HumdrumToken*>          getPrimaryTrackSeq(int track,
		                                                  int options);

	protected:
		bool          analyzeTokens                (void);
		bool          analyzeSpines                (void);
		bool          analyzeLinks                 (void);
		bool          analyzeTracks                (void);
		bool          analyzeLines                 (void);
		bool          adjustSpines                 (HumdrumLine& line,
		                                            vector<string>& datatype,
		                                            vector<string>& sinfo);
		string        getMergedSpineInfo           (vector<string>& info,
		                                            int starti, int extra);
		bool          stitchLinesTogether          (HumdrumLine& previous,
		                                            HumdrumLine& next);
		void          addToTrackStarts             (HumdrumToken* token);
		bool          analyzeNonNullDataTokens     (void);
		void          addUniqueTokens          (vector<HumdrumToken*>& target,
		                                       vector<HumdrumToken*>& source);
		bool          processNonNullDataTokensForTrackForward(
		                                        HumdrumToken* starttoken,
		                                        vector<HumdrumToken*> ptokens);
		bool          processNonNullDataTokensForTrackBackward(
		                                        HumdrumToken* starttoken,
		                                        vector<HumdrumToken*> ptokens);

	protected:

		// lines: an array representing lines from the input file.
		vector<HumdrumLine*> lines;

		// trackstarts: list of addresses of the exclusive interpreations
		// in the file.  The first element in the list is reserved, so the
		// number of tracks (primary spines) is equal to one less than the
		// size of this list.
		vector<HumdrumToken*> trackstarts;

		// trackends: list of the addresses of the spine terminators in the file.
		// It is possible that spines can split and their subspines do not merge
		// before termination; therefore, the ends are stored in a 2d array.
		// The first dimension is the track number, and the second dimension
		// is the list of terminators.
		vector<vector<HumdrumToken*> > trackends;

		// barlines: list of barlines in the data.  If the first measures is
		// a pickup measure, then the first entry will not point to the first
		// starting exclusive interpretation line rather than to a barline.
		vector<HumdrumLine*> barlines;
		// Maybe also add "measures" which are complete metrical cycles.

		// ticksperquarternote: this is the number of tick
		int ticksperquarternote;

	public:
		// Dummy functions to allow the HumdrumFile class's inheritance
		// to be shifted between HumdrumFileContent (the top-level default), 
      // HumdrumFileStructure (mid-level interface), or HumdrumFileBase
		// (low-level interface).

		// 
		// HumdrumFileStructure public functions:
		//
		bool readNoRhythm      (istream& infile) { return read(infile); };
		bool readNoRhythm      (const char*   filename) {return read(filename);};
		bool readNoRhythm      (const string& filename) {return read(filename);};
		bool readStringNoRhythm(const char*   contents) {return read(contents);};
		bool readStringNoRhythm(const string& contents) {return read(contents);};
		HumNum       getScoreDuration           (void) const { return 0; };
		ostream&     printDurationInfo          (ostream& out=cout) {return out;};
		int          tpq                        (void) { return 0; }
		int          getBarlineCount            (void) const { return 0; }
		HumdrumLine* getBarline                 (int index) const { return NULL;};
		HumNum       getBarlineDuration         (int index) const { return 0; };
		HumNum       getBarlineDurationFromStart(int index) const { return 0; };
		HumNum       getBarlineDurationToEnd    (int index) const { return 0; };

		// HumdrumFileContent public functions:


	
};

ostream& operator<<(ostream& out, HumdrumFileBase& infile);


// END_MERGE

} // end namespace std;

#endif /* _HUMDRUMFILEBASE_H */



