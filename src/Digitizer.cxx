#include "WireCellGen/Digitizer.h"

#include "WireCellIface/IWireSelectors.h"

#include "WireCellUtil/NamedFactory.h"


#include <iostream>		// debugging
using namespace std;
using namespace WireCell;


WIRECELL_NAMEDFACTORY_BEGIN(Digitizer)
WIRECELL_NAMEDFACTORY_INTERFACE(Digitizer, IDigitizer);
WIRECELL_NAMEDFACTORY_END(Digitizer)

Digitizer::Digitizer()
{
}
Digitizer::~Digitizer()
{
}

bool Digitizer::set_wires(const IWire::shared_vector& wires)
{
    for (int ind=0; ind<3; ++ind) {
	m_wires[ind].clear();
	copy_if(wires->begin(), wires->end(),
		back_inserter(m_wires[ind]), select_uvw_wires[ind]);
	std::sort(m_wires[ind].begin(), m_wires[ind].end(), ascending_index);
    }
    return true;
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

void Digitizer::reset()
{
    m_output.clear();
}
void Digitizer::flush()
{
    m_output.push_back(nullptr);
    return;			// no input buffer
}
bool Digitizer::insert(const input_pointer& plane_slice_vector)
{
    if (!plane_slice_vector) {
	flush();
	return true;
    }

    if (!m_wires[0].size()) {
	cerr << "Digitizer::insert: no wires" << endl;
	return false;
    }

    WireCell::ChannelCharge cc;
    double the_time = -1;

    int nplanes = plane_slice_vector->size();
    for (int iplane = 0; iplane < nplanes; ++iplane) {
	IPlaneSlice::pointer ps = plane_slice_vector->at(iplane);
	if (!ps) {
	    //cerr << "Digitizer::insert: ignoring null PlaneSlice" << endl;
	    continue;
	}
	WirePlaneId wpid = ps->planeid();
	IWire::vector& wires = m_wires[wpid.index()];
	the_time = ps->time();

	for (auto wcr : ps->charge_runs()) {
	    int index = wcr.first;
	    for (auto q : wcr.second) {
		IWire::pointer wire = wires[index];
		cc[wire->channel()] += Quantity(q, sqrt(q));
		++index;
	    }
	}
    }
    // fixme: maybe add check for consistent times between planes....

    IChannelSlice::pointer next(new SimpleChannelSlice(the_time, cc));
    m_output.push_back(next);
    return true;
}

bool Digitizer::extract(output_pointer& channel_slice)
{
    if (m_output.empty()) {
	return false;
    }
    channel_slice = m_output.front();
    m_output.pop_front();
    return true;
}
