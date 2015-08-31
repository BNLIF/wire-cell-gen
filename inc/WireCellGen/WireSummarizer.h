#ifndef WIRECELL_WIRESUMMARIZER
#define WIRECELL_WIRESUMMARIZER

#include "WireCellIface/IWireSummarizer.h"

namespace WireCell {

    class WireSummarizer : public IWireSummarizer {
    public:
	WireSummarizer();
	virtual ~WireSummarizer();
	
	bool sink(const IWireVector& wires);
	bool source(IWireSummary::pointer& ws) {
	    ws = m_ws;
	    return true;
	}
	bool process() { return false; }

    private:
	IWireSummary::pointer m_ws;
    };

}

#endif

