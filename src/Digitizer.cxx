#include "WireCellGen/Digitizer.h"

#include "WireCellIface/IWireSelectors.h"

#include "WireCellUtil/NamedFactory.h"


#include <iostream>		// debugging
#include <sstream>		// debugging
using namespace std;
using namespace WireCell;


WIRECELL_NAMEDFACTORY_BEGIN(Digitizer)
WIRECELL_NAMEDFACTORY_INTERFACE(Digitizer, IDigitizer);
WIRECELL_NAMEDFACTORY_END(Digitizer)

Digitizer::Digitizer()
    : m_count(0)
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

bool Digitizer::operator()(const input_pointer& plane_slice_vector, output_pointer& channel_slice)
{
    ++m_count;

    channel_slice = nullptr;
    if (!m_wires[0].size()) {
	cerr << "Digitizer: no wires after " << m_count << endl;
	return false;
    }

    if (!plane_slice_vector) {
	cerr << "Digitizer: nullptr at " << m_count << endl;
	return true;
    }

    WireCell::ChannelCharge cc;
    double the_time = -1;

    stringstream msg;
    msg << "Digitizer: " << m_count << " t=" << plane_slice_vector->at(0)->time()<< "\n";

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

	double qtot = 0.0;
	for (auto wcr : ps->charge_runs()) {
	    int index = wcr.first;
	    for (auto q : wcr.second) {
		IWire::pointer wire = wires[index];
		cc[wire->channel()] += Quantity(q, sqrt(q));
		++index;
		qtot += q;
	    }
	}
	msg << "\t" << iplane << " q=" << qtot << " @ t=" << the_time << "\n";
    }
    // fixme: maybe add check for consistent times between planes....

    cerr << msg.str();


    channel_slice = IChannelSlice::pointer(new SimpleChannelSlice(the_time, cc));
    return true;
}
