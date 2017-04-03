#include "WireCellGen/Digislicer.h"

#include "WireCellIface/IWireSelectors.h"

#include "WireCellUtil/NamedFactory.h"


#include <iostream>		// debugging
#include <sstream>		// debugging

WIRECELL_FACTORY(Digislicer, WireCell::Digislicer, WireCell::IDigislicer);

using namespace std;
using namespace WireCell;


Digislicer::Digislicer()
{
}
Digislicer::~Digislicer()
{
}

// bool Digislicer::set_wires(const IWire::shared_vector& wires)
// {
//     for (int ind=0; ind<3; ++ind) {
// 	m_wires[ind].clear();
// 	copy_if(wires->begin(), wires->end(),
// 		back_inserter(m_wires[ind]), select_uvw_wires[ind]);
// 	std::sort(m_wires[ind].begin(), m_wires[ind].end(), ascending_index);
//     }
//     return true;
// }


class SimpleChannelSlice : public IChannelSlice {
    double m_time;
    ChannelCharge m_cc;
public:
    SimpleChannelSlice(double time, const ChannelCharge& cc)
	: m_time(time), m_cc(cc) {}
    
    virtual double time() const { return m_time; }

    virtual ChannelCharge charge() const { return m_cc; }
};

//bool Digislicer::operator()(const input_pointer& plane_slice_vector, output_pointer& channel_slice)
bool Digislicer::operator()(const input_tuple_type& intup, output_pointer& channel_slice)
{
    const IPlaneSlice::shared_vector& plane_slice_vector = std::get<0>(intup);
    if (!plane_slice_vector) {
	cerr << "Digislicer: got EOS\n";
	channel_slice = nullptr;
	return true;
    }

    const IWire::shared_vector& all_wires = std::get<1>(intup);
    // fixme: this probably needs optimization to not run each call.
    IWire::vector plane_wires[3];
    for (int ind=0; ind<3; ++ind) {
	copy_if(all_wires->begin(), all_wires->end(),
		back_inserter(plane_wires[ind]), select_uvw_wires[ind]);
	std::sort(plane_wires[ind].begin(), plane_wires[ind].end(), ascending_index);
    }


    WireCell::ChannelCharge cc;
    double the_time = -1;

    stringstream msg;
    msg << "Digislicer: t=" << plane_slice_vector->at(0)->time()<< "\n";

    int nplanes = plane_slice_vector->size();
    for (int iplane = 0; iplane < nplanes; ++iplane) {
	IPlaneSlice::pointer ps = plane_slice_vector->at(iplane);
	if (!ps) {
	    //cerr << "Digislicer: ignoring null PlaneSlice" << endl;
	    continue;
	}
	WirePlaneId wpid = ps->planeid();
	IWire::vector& wires = plane_wires[wpid.index()];
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
