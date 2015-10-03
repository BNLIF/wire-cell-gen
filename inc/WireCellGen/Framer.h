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
	virtual bool insert(const input_type& in);
	virtual bool extract(output_type& out);

    private:
	void flush();

	std::deque<input_type> m_input;
	std::deque<output_type> m_output;

	int m_nticks;
	int m_count;

	void process(bool flush = false);

    };
}

#endif
