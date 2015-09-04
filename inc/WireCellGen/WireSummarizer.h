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

	virtual void reset();
	virtual void flush();
	virtual bool insert(const input_type& in);
	virtual bool extract(output_type& out);


    private:
	std::deque<IWireSummary::pointer> m_output;
    };

}

#endif

