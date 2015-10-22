#ifndef WIRECELL_WIRESUMMARIZER
#define WIRECELL_WIRESUMMARIZER

#include "WireCellIface/IWireSummarizer.h"
#include "WireCellIface/IWireSummary.h"

#include <deque>

namespace WireCell {

    class WireSummarizer : public IWireSummarizer {
    public:
	WireSummarizer();
	virtual ~WireSummarizer();

	virtual bool insert(const input_pointer& in);
	virtual bool extract(output_pointer& out) {
	    out = m_output;
	    return true;
	}

    private:
	output_pointer m_output;
    };

}

#endif

