//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue May 19 22:07:53 PDT 2020
// Last Modified: Tue May 19 22:07:56 PDT 2020
// Filename:      tool-chantize.h
// URL:           https://github.com/craigsapp/humlib/blob/master/include/tool-chantize.h
// Syntax:        C++11; humlib
// vim:           syntax=cpp ts=3 noexpandtab nowrap
//
// Description:   Interface for chantize tool.
//

#ifndef _TOOL_CHANTIZE_H_INCLUDED
#define _TOOL_CHANTIZE_H_INCLUDED

#include "HumTool.h"
#include "HumdrumFile.h"

#include <ostream>
#include <string>
#include <vector>

namespace hum {

// START_MERGE

class Tool_chantize : public HumTool {
	public:
		                  Tool_chantize             (void);
		                 ~Tool_chantize             () {};

		bool              run                       (HumdrumFileSet& infiles);
		bool              run                       (HumdrumFile& infile);
		bool              run                       (const std::string& indata, std::ostream& out);
		bool              run                       (HumdrumFile& infile, std::ostream& out);

	protected:
		void              initialize                (HumdrumFile& infile);
		void              processFile               (HumdrumFile& infile);
		void              outputFile                (HumdrumFile& infile);
		void              updateKeySignatures       (HumdrumFile& infile, int lineindex);
		void              checkDataLine             (HumdrumFile& infile, int lineindex);
		void              clearStates               (void);
		void              addBibliographicRecords   (HumdrumFile& infile);
		void              deleteBreaks              (HumdrumFile& infile);
		void              fixEditorialAccidentals   (HumdrumFile& infile);
		void              fixInstrumentAbbreviations(HumdrumFile& infile);
		void              deleteDummyTranspositions (HumdrumFile& infile);
		std::string       getDate                   (void);
		std::vector<bool> getTerminalRestStates      (HumdrumFile& infile);
		bool              hasDiamondNotes           (HumdrumFile& infile);

	private:
		std::vector<std::vector<int>>  m_pstates;
		std::vector<std::vector<int>>  m_kstates;
		std::vector<std::vector<bool>> m_estates;
		bool m_diamondQ = false;
};

// END_MERGE

} // end namespace hum

#endif /* _TOOL_CHANTIZE_H_INCLUDED */



