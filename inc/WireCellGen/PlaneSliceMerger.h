#ifndef WIRECELL_PLANESLICEMERGER
#define WIRECELL_PLANESLICEMERGER

#include "WireCellIface/IPlaneSliceMerger.h"

#include <deque>

namespace WireCell {

    class PlaneSliceMerger : public IPlaneSliceMerger {
    public:
	PlaneSliceMerger();
	virtual ~PlaneSliceMerger() {}

	virtual bool insert(const input_pointer& in, int port);
	virtual bool extract(output_pointer& out);

	virtual int nin() { return m_nin[0]+m_nin[1]+m_nin[2]; }
	virtual int nout() { return m_nout; }

    private:
	typedef std::deque<input_pointer> input_queue;
	typedef std::deque<output_pointer> output_queue;
	std::vector< input_queue > m_input;
	output_queue m_output;

	int m_nin[3], m_nout;
	bool m_eos;
    };
}

#endif
