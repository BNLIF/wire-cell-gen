#ifndef WIRECELL_FRAMER
#define WIRECELL_FRAMER

#include "WireCellIface/IFramer.h"

#include <deque>

namespace WireCell {

    /** A framer object produces frames.
     *
     * It consumes up to a fixed number of ticks of IChannelSlices and
     * packages them into an IFrame.
     */
    class Framer : public IFramer {
    public:
	Framer();
	Framer(int nticks);
	virtual ~Framer();

	virtual bool sink(const IChannelSlice::pointer& channel_slice);
	virtual bool process();
	virtual bool source(IFrame::pointer& frame);

    private:

	std::deque<IChannelSlice::pointer> m_input;
	std::deque<IFrame::pointer> m_output;

	int m_nticks;
	int m_count;
    };
}

#endif
