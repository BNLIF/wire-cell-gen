#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Diffuser.h"

#include "WireCellIface/SimplePlaneSlice.h"


#include <iostream>
using namespace std;		// debugging
using namespace WireCell;


PlaneDuctor::PlaneDuctor(WirePlaneId wpid,
			 double lbin, double tbin,
			 double lpos0, double tpos0)
    : m_wpid(wpid)
    , m_lbin(lbin)
    , m_tbin(tbin)
    , m_lpos(lpos0)
    , m_tpos0(tpos0)
{
}
PlaneDuctor::~PlaneDuctor()
{
}

void PlaneDuctor::reset()
{
    m_input.clear();
    m_output.clear();
}

void PlaneDuctor::flush() 
{
    while (latch_one()) { /**/}
    m_output.push_back(eos());
}

bool PlaneDuctor::insert(const input_type& diffusion) 
{
    m_input.insert(diffusion);
    while (latch_one()) { /**/}
    return true;
}

bool PlaneDuctor::extract(output_type& plane_slice) 
{
    if (m_output.empty()) {
	return false;
    }
    plane_slice = m_output.front();
    m_output.pop_front();
    return true;
}

void PlaneDuctor::purge_bygone()
{
    IDiffusionVector to_kill;
    for (auto diff : m_input) {
	// find any in need of purging.
	if (m_lpos >= diff->lend()) {
	    to_kill.push_back(diff);
	    continue;
	}
	break;
    }
    for (auto diff : to_kill) {
	m_input.erase(diff);
    }
}

IPlaneSlice::pointer PlaneDuctor::latch_hits()
{
    IPlaneSlice::WireChargeRunVector wcrv;

    vector<double> wires_charge;
    for (auto diff : m_input) {
	// Skip old patches, call purge_bygone() first to make this a no-op
	if (m_lpos >= diff->lend()) {
	    continue;		
	}

	// the start of this and all subsequent diffusions are at
	// least one bin in the future, so bail.
	if (diff->lbegin() >= m_lpos + m_lbin) {
	    break;
	}

	// Here we have a patch that covers the current time bin.  Dig
	// out the corresponding slice and save it.
	const int lind = (0.5 + m_lpos - diff->lbegin())/m_lbin;

	const int wire_index = (0.5 + diff->tbegin() - m_tpos0)/m_tbin;
	const int nwires_patch = diff->tsize();
	vector<double> wire_charge;
	for (int tind = 0; tind < nwires_patch; ++tind) {
	    wires_charge.push_back(diff->get(lind,tind));
	}
	wcrv.push_back(make_pair(wire_index, wire_charge));
    }

    IPlaneSlice::pointer ret(new SimplePlaneSlice(m_wpid, m_lpos, wcrv));
    return ret;
}

bool PlaneDuctor::latch_one()
{
    purge_bygone();
    if (m_input.empty()) { return false; } // dry

    // fixme: need assurance we have enough buffered!

    IPlaneSlice::pointer ps = latch_hits();
    m_output.push_back(ps);
    m_lpos += m_lbin;		// on to next.
    return true;
}

