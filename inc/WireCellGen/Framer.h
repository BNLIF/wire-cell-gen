#ifndef WIRECELL_FRAMER
#define WIRECELL_FRAMER

#include "WireCellIface/IFramer.h"

#include <deque>

namespace WireCell {

    /** An object produces IFrame objects from IChannelSlice objects.
     */
    class Framer : public IFramer {

    public:
	Framer();
	Framer(int nticks);
	virtual ~Framer();

	virtual void reset();
	virtual bool operator()(const input_pointer& in, output_queue& outq);

    private:

	std::deque<input_pointer> m_input;

	int m_nticks;
	int m_count;

	void process(output_queue& outq, bool flush = false);

    };
}

#endif
