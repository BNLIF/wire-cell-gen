#ifndef WIRECELL_FRAMER
#define WIRECELL_FRAMER

#include "WireCellIface/IFrame.h"
#include "WireCellIface/IChannelSlice.h"

namespace WireCell {

    /** A framer object produces frames.
     *
     * It consumes up to a fixed number of ticks of IChannelSlices and
     * packages them into an IFrame.
     */
    class Framer {
    public:
	Framer(int nticks);
	virtual ~Framer();

	/// Return the next frame at most nticks long.
	IFrame::pointer operator()();

	void connect(const IChannelSlice::source_slot& s) { m_input.connect(s); }

    private:

	IChannelSlice::source_signal m_input;
	int m_nticks;
	int m_count;
    };
}

#endif
