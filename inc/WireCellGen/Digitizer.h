#ifndef WIRECELL_DIGITIZER
#define WIRECELL_DIGITIZER

#include "WireCellUtil/Faninout.h"

#include "WireCellIface/IChannelSlice.h"
#include "WireCellIface/IPlaneSlice.h"
#include "WireCellIface/IWire.h"

namespace WireCell {

    class Digitizer : public IWireSink {
    public:
	Digitizer();       
	virtual ~Digitizer();       

	/// Set the wires to use. (IWireSink)
	virtual void sink(const IWire::iterator_range& wires);
	// fixme: I really should get rid of the IWireSink interface

	/// Return the next channel slice
	IChannelSlice::pointer operator()();

	/// Call once on each U, V and W plane ductor.
	void connect(const IPlaneSlice::source_slot& s) { m_input.connect(s); }

    private:

	typedef boost::signals2::signal<IPlaneSlice::pointer (), Fanin< IPlaneSliceVector > > signal_type;
	signal_type m_input;

	IWireVector m_wires[3];
    };

}
#endif
