#include "WireCellGen/Digitizer.h"

#include "WireCellIface/IWireSelectors.h"

#include "WireCellUtil/NamedFactory.h"


#include <iostream>		// debugging
using namespace std;
using namespace WireCell;


WIRECELL_NAMEDFACTORY(Digitizer);
WIRECELL_NAMEDFACTORY_ASSOCIATE(Digitizer, IWireSink);

Digitizer::Digitizer()
{
}
Digitizer::~Digitizer()
{
}

void Digitizer::sink(const IWire::iterator_range& wr)
{
    for (int ind=0; ind<3; ++ind) {
	m_wires[ind].clear();
	copy_if(boost::begin(wr), boost::end(wr), back_inserter(m_wires[ind]), select_uvw_wires[ind]);
	std::sort(m_wires[ind].begin(), m_wires[ind].end(), ascending_index);
    }
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

IChannelSlice::pointer Digitizer::operator()()
{
    IPlaneSliceVector psvec = m_input();
    if (!psvec.size()) { return nullptr; }

    IChannelSlice::ChannelCharge cc;
    double the_times[3] = {0};

    for (int iplane = 0; iplane<psvec.size(); ++iplane) {
	IPlaneSlice::pointer ps = psvec[iplane];
	if (!ps) {
	    //cerr << "No plane slice at " << iplane << " out of " << psvec.size() << endl;
	    return nullptr;
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

    return IChannelSlice::pointer(new SimpleChannelSlice(the_times[0], cc));

}
