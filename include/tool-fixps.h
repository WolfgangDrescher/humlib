//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Feb  3 18:46:23 PST 2021
// Last Modified: Wed Feb  3 18:46:26 PST 2021
// Filename:      tool-fixps.h
// URL:           https://github.com/craigsapp/humlib/blob/master/include/tool-fixps.h
// Syntax:        C++11; humlib
// vim:           syntax=cpp ts=3 noexpandtab nowrap
//
// Description:   Interface for fixps tool.
//

#ifndef _TOOL_FIXPS_H_INCLUDED
#define _TOOL_FIXPS_H_INCLUDED

#include "HumTool.h"
#include "HumdrumFile.h"

#include <ostream>
#include <string>
#include <vector>

namespace hum {

// START_MERGE

class Tool_fixps : public HumTool {
	public:
		         Tool_fixps         (void);
		        ~Tool_fixps         () {};

		bool     run                (HumdrumFileSet& infiles);
		bool     run                (HumdrumFile& infile);
		bool     run                (const std::string& indata, std::ostream& out);
		bool     run                (HumdrumFile& infile, std::ostream& out);

	protected:
		void     initialize         (HumdrumFile& infile);
		void     processFile        (HumdrumFile& infile);
		void     markEmptyVoices    (HumdrumFile& infile);
		void     removeEmpties      (std::vector<std::vector<HTp>>& newlist, HumdrumFile& infile);
		void     removeDuplicateDynamics(HumdrumFile& infile);
		void     outputNewSpining   (std::vector<std::vector<HTp>>& newlist, HumdrumFile& infile);
		void     printNewManipulator(HumdrumFile& infile, std::vector<std::vector<HTp>>& newlist, int line);

	private:

};

// END_MERGE

} // end namespace hum

#endif /* _TOOL_FIXPS_H_INCLUDED */



