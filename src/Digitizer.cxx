#include "WireCellGen/Digitizer.h"

#include "WireCellIface/IWireSelectors.h"

#include "WireCellUtil/NamedFactory.h"


#include <iostream>		// debugging
using namespace std;
using namespace WireCell;


WIRECELL_NAMEDFACTORY(Digitizer);
WIRECELL_NAMEDFACTORY_ASSOCIATE(Digitizer, IDigitizer);

Digitizer::Digitizer()
{
}
Digitizer::~Digitizer()
{
}

bool Digitizer::sink(const IWireVector& wires)
{
    for (int ind=0; ind<3; ++ind) {
	m_wires[ind].clear();
	copy_if(wires.begin(), wires.end(), back_inserter(m_wires[ind]), select_uvw_wires[ind]);
	std::sort(m_wires[ind].begin(), m_wires[ind].end(), ascending_index);
    }
    return true;
}

bool Digitizer::sink(const IPlaneSliceVector& plane_slices)
{
    m_plane_slices = plane_slices;
}

class SimpleChannelSlice : public IChannelSlice {
    double m_time;
    ChannelCharge m_cc;
public:
    SimpleChannelSlice(double time, const ChannelCharge& cc)
	: m_time(time), m_cc(cc) {}
    
    virtual double time() const { return m_time; }

    virtual ChannelCharge charge() const { return m_cc; }
};

bool Digitizer::source(IChannelSlice::pointer& returned_channel_slice) 
{
    const size_t nplanes = m_plane_slices.size();
    if (! nplanes) {
	return false;
    }
    
    IChannelSlice::ChannelCharge cc;
    double the_times[3] = {0};

    for (int iplane = 0; iplane < nplanes; ++iplane) {
	IPlaneSlice::pointer ps = m_plane_slices[iplane];
	if (!ps) {
	    return false;
	}
	WirePlaneId wpid = ps->planeid();
	IWireVector& wires = m_wires[wpid.index()];
	the_times[wpid.index()] = ps->time();

	for (auto cg : ps->charge_groups()) {
	    int index = cg.first;
	    for (auto q : cg.second) {
		IWire::pointer wire = wires[index];
		cc[wire->channel()] += q;
		++index;
	    }
	}
    }
    // fixme: maybe add check for consistent times between planes....

    IChannelSlice::pointer next(new SimpleChannelSlice(the_times[0], cc));
    returned_channel_slice = next;
    return true;
}

