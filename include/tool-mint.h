//
// Programmer:    Wolfgang Drescher <drescher.wolfgang@gmail.com>
// Creation Date: Sat Jul 11 2026
// Filename:      tool-mint.h
// URL:           https://github.com/craigsapp/humlib/blob/master/include/tool-mint.h
// Syntax:        C++11; humlib
// vim:           syntax=cpp ts=3 noexpandtab nowrap
//
// Description:   Interface for mint tool, which calculates melodic
//                intervals between consecutive notes in **kern spines.
//

#ifndef _TOOL_MINT_H_INCLUDED
#define _TOOL_MINT_H_INCLUDED

#include "HumTool.h"
#include "HumdrumFile.h"

#include <ostream>
#include <string>
#include <vector>

namespace hum {

// START_MERGE

class Tool_mint : public HumTool {

	public:
		     Tool_mint  (void);
		     ~Tool_mint () {};

		bool run     (HumdrumFileSet& infiles);
		bool run     (HumdrumFile& infile);
		bool run     (const std::string& indata, std::ostream& out);
		bool run     (HumdrumFile& infile, std::ostream& out);

	protected:
		void        initialize             (void);
		void        processFile            (HumdrumFile& infile);
		void        analyzeLine            (HumdrumFile& infile, int line);
		int         processKernSpines      (HumdrumFile& infile, int line, int start);
		std::string getIntervalToken       (HTp token);
		std::string getIntervalQuality     (int base40interval);
		HTp         getPreviousAttackToken (HTp token);
		int         getRepresentativeBase40Pitch(HTp token);

	private:
		bool m_absoluteQ = false; // -a option: hide direction of the interval
		bool m_compoundQ = false; // -c option: reduce compound intervals to simple intervals
		bool m_diatonicQ = false; // -d option: only display the diatonic interval number
		bool m_lowestQ   = false; // -l option: use lowest note of a chord instead of the highest
		bool m_cdataQ    = false; // -x option: label the spine **cdata-mint instead of **mint

		std::string       m_kernTracks  = ""; // used with -k option
		std::string       m_spineTracks = ""; // used with -s option
		std::vector<bool> m_selectedKernSpines; // used with -k and -s option

};

// END_MERGE

} // end namespace hum

#endif /* _TOOL_MINT_H_INCLUDED */
