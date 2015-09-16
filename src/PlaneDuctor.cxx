#include "WireCellGen/PlaneDuctor.h"

#include "WireCellIface/SimplePlaneSlice.h"
#include "WireCellIface/WirePlaneId.h"


#include <iostream>
using namespace std;		// debugging
using namespace WireCell;


PlaneDuctor::PlaneDuctor(WirePlaneId wpid,
			 int nwires,
			 double lbin,
			 double tbin,
			 double lpos0,
			 double tpos0)
    : m_wpid(wpid)
    , m_nwires(nwires)
    , m_lbin(lbin)
    , m_tbin(tbin)
    , m_lpos(lpos0)
    , m_tpos0(tpos0)
{
    cerr << "PlaneDuctor("<<m_wpid<<","<<m_nwires<<","<<m_lbin<<","<<m_tbin<<")" << endl;
}
PlaneDuctor::~PlaneDuctor()
{
//    cerr << "~PlaneDuctor("<<m_wpid<<","<<m_nwires<<","<<m_lbin<<","<<m_tbin<<")" << endl;
}

void PlaneDuctor::reset()
{
    m_input.clear();
    m_output.clear();
}

void PlaneDuctor::flush() 
{
    while (true) {
	purge_bygone();
	if (m_input.empty()) {	// don't care if starving here.
	    break;
	}	    
	latch_one();
    }
    m_output.push_back(eos());
}

bool PlaneDuctor::insert(const input_type& diff) 
{
    cerr << m_wpid.ident() << " " << diff.get() << "\tinsert at t="
    	 << m_lpos << " diff: [" << diff->lbegin() << " --> "
    	 << diff->lend() << "]" << endl;
    m_input.insert(diff);
    while (true) {
	purge_bygone();
	if (starved()) {
	    break;
	}
	latch_one();
    }
    return true;
}

void PlaneDuctor::latch_one()
{
    IPlaneSlice::pointer ps = latch_hits();
    m_output.push_back(ps);
    m_lpos += m_lbin;		// on to next.
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
    if (to_kill.empty()) {
	return;
    }
    // cerr << "PlaneDuctor::purge_bygone() wpid" << m_wpid
    // 	 << " killing "	 << to_kill.size() 
    // 	 << " at t=" << m_lpos
    //cerr << endl;

    for (auto diff : to_kill) {
	cerr << m_wpid.ident() << " " << diff.get()
	     << "\tpurge lpos=" << m_lpos
	     << " diff=[" << diff->lbegin() << " --> " << diff->lend() << "]@" << diff->lbin() << "*" << diff->lsize()
	     << endl;
	m_input.erase(diff);
    }
}


bool PlaneDuctor::starved()
{
    if (m_input.empty()) {
	return true;
    }

    // We need to buffer enough so that at least one diffusion begins
    // greater than one bin in the future.
    auto last = *m_input.rbegin();
    return last->lbegin() <= m_lpos;
}

IPlaneSlice::pointer PlaneDuctor::latch_hits()
{
    IPlaneSlice::WireChargeRunVector wcrv;

    // cerr << m_wpid.ident() << "\tlatch_hits()"
    // 	 << " #input=" << m_input.size()
    // 	 << " #output=" << m_output.size()
    // 	 << " lpos=" << m_lpos
    // 	 << " span:[" << (*m_input.begin())->lbegin() << " --> " << (*m_input.rbegin())->lend() << "]"
    // 	 << endl;

    for (auto diff : m_input) {
	// Skip old patches, call purge_bygone() first to make this a no-op
	if (diff->lend() <= m_lpos) {
	    // should never print.
	    cerr << m_wpid.ident() << " " << diff.get()
		 << " PlaneDuctor: error, why have I not been purged? "
		 << " must skip old diffusion: [" << diff->lbegin() << " --> " << diff->lend() << "] < "
		 << " lpos=" << m_lpos << endl;
	    continue;
	}
	if (m_lpos + diff->lbin() <= diff->lbegin()) {
	    // cerr << m_wpid.ident() << " " << diff.get()
	    // 	 << "\tskip future diffusion: [" << diff->lbegin() << " --> " << diff->lend() << "] < "
	    // 	 << " lpos=" << m_lpos << endl;
	    break;
	}    

	// "now" is inside the patch time

	// time from start of patch to "now"
	const double ldelta = m_lpos - diff->lbegin();
	const int lbin = ldelta/diff->lbin();

	// longitudinal done, now transverse:

	// transverse bins and wire indices
	int tbin_begin = 0;
	int tbin_end   = diff->tsize();
	int wbin_begin = diff->tbegin() - m_tpos0 - 0.5*diff->tbin();
	int wbin_end   = wbin_begin + diff->tsize();

	// cerr << m_wpid.ident() << " " << diff.get()
	//      << "\tlpos=" << m_lpos
	//      << " diff=[" << diff->lbegin() << " --> " << diff->lend() << "]@" << diff->lbin() << "*" << diff->lsize()
	//      << " ldelta="<<ldelta << ", lbin=" << lbin
	//      << endl;

	// if underflow
	if (wbin_begin < 0) {
	    tbin_begin += -wbin_begin;
	    wbin_begin = 0;
	}
	
	// if overflow
	if (wbin_end > m_nwires) {
	    tbin_end -= wbin_end - m_nwires;
	    wbin_end = m_nwires;
	}

	int nwires_patch = wbin_end - wbin_begin;
	if (nwires_patch <= 0) {
	    cerr << m_wpid.ident() << " " << diff.get()
	    	 << " patch outside, nwires_patch = " << nwires_patch
	    	 << endl;
	    continue;		// patch fully outside
	}

	cerr << m_wpid.ident() << " " << diff.get()
	     << " tbins:["<< tbin_begin << "," << tbin_end << "]"
	     << " wbins:["<< wbin_begin << "," << wbin_end << "] = "
	     << nwires_patch << " wires"
	     <<  endl;

	vector<double> wire_charge;
	for (int tbin = tbin_begin; tbin < tbin_end; ++tbin) {
	    wire_charge.push_back(diff->get(lbin,tbin));
	}

	IPlaneSlice::WireChargeRun wcr(wbin_begin, wire_charge);
	wcrv.push_back(wcr);
    }

    if (wcrv.size()) {
    	cerr << m_wpid.ident() << "\tslice made at lpos=" << m_lpos << " runs=" << wcrv.size() << endl;
    }
    IPlaneSlice::pointer ret(new SimplePlaneSlice(m_wpid, m_lpos, wcrv));
    return ret;
}

