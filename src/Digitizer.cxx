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

bool Digitizer::set_wires(const IWireVector& wires)
{
    for (int ind=0; ind<3; ++ind) {
	m_wires[ind].clear();
	copy_if(wires.begin(), wires.end(),
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
    return;			// no input buffer
}
bool Digitizer::insert(const input_type& plane_slice_vector)
{
    if (!m_wires[0].size()) {
	cerr << "Digitizer::insert: no wires" << endl;
	return false;
    }

    IChannelSlice::ChannelCharge cc;
    double the_times[3] = {0};

    int nplanes = plane_slice_vector.size();
    for (int iplane = 0; iplane < nplanes; ++iplane) {
	IPlaneSlice::pointer ps = plane_slice_vector[iplane];
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
    m_output.push_back(next);
    return true;
}

bool Digitizer::extract(output_type& channel_slice)
{
    if (m_output.empty()) {
	return false;
    }
    channel_slice = m_output.front();
    m_output.pop_front();
    return true;
}
