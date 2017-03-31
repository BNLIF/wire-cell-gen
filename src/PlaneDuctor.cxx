#include "WireCellGen/PlaneDuctor.h"

#include "WireCellIface/SimplePlaneSlice.h"
#include "WireCellIface/WirePlaneId.h"
#include "WireCellUtil/NamedFactory.h"

#include <iostream>
#include <sstream>
#include <vector>

WIRECELL_FACTORY(PlaneDuctor, WireCell::PlaneDuctor, WireCell::IPlaneDuctor, WireCell:: IConfigurable);

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
    , m_eos(false)
{
}
PlaneDuctor::~PlaneDuctor()
{
//    cerr << "~PlaneDuctor("<<m_wpid<<","<<m_nwires<<","<<m_lbin<<","<<m_tbin<<")" << endl;
}


Configuration PlaneDuctor::default_configuration() const
{
    std::string json = R"(
{
"wpid":[1,0,0],
"nwires":0,
"lbin_us":0.5,
"tbin_mm":5.0,
"lpos0_us":0.0,
"tpos0_mm":0.0
}
)";
    return configuration_loads(json, "json");
}

void PlaneDuctor::configure(const Configuration& cfg)
{
    switch (convert<int>(cfg["wpid"][0])) {
    case 1:
	m_wpid = WirePlaneId(kUlayer, convert<int>(cfg["wpid"][1]), convert<int>(cfg["wpid"][2]));
	break;
    case 2:
	m_wpid = WirePlaneId(kVlayer, convert<int>(cfg["wpid"][1]), convert<int>(cfg["wpid"][2]));
	break;
    case 4:
	m_wpid = WirePlaneId(kWlayer, convert<int>(cfg["wpid"][1]), convert<int>(cfg["wpid"][2]));
	break;
    default:
	throw runtime_error("PlaneDuctor configured with unknown wire plane layer");
    }
    m_nwires = get<int>(cfg, "nwires", m_nwires);
    m_lbin  = get<double>(cfg, "lbin_us",   m_lbin/units::microsecond)*units::microsecond;
    m_lpos  = get<double>(cfg, "lpos0_us",  m_lpos/units::microsecond)*units::microsecond;
    m_tbin  = get<double>(cfg, "tbin_mm",   m_tbin/units::millimeter)*units::millimeter;
    m_tpos0 = get<double>(cfg, "tpos0_mm", m_tpos0/units::millimeter)*units::millimeter;

    cerr << "PlaneDuctor::configure(wpid="<<m_wpid<<",nwires="<<m_nwires<<",lbin="<<m_lbin<<",tbin="<<m_tbin<<",lpos="<<m_lpos<<",tpos="<<m_tpos0<<")" << endl;

}

void PlaneDuctor::reset()
{
    m_input.clear();
}

bool PlaneDuctor::operator()(const input_pointer& diff, output_queue& outq)
{
    if (m_wpid.layer() == kUnknownLayer) {
	throw runtime_error("PlaneDuctor configured with undefined wire plane layer");
    }
    if (!m_nwires) {
	throw runtime_error("PlaneDuctor configured with zero wire count");
    }

    if (m_eos) {
	return false;
    }

    if (!diff) {		// flush EOS
	while (true) {
	    purge_bygone();
	    if (m_input.empty()) {	// don't care if starving here.
		break;
	    }	    
	    latch_one(outq);
	}
	outq.push_back(nullptr);
	m_eos = true;
	stringstream msg;
	msg << "PlaneDuctor: EOS for " << m_wpid.layer();
	cerr << msg.str();
	return true;
    }

    stringstream msg;
    msg << "PlaneDuctor: " << m_wpid.layer() << " " << "\tinsert at t="
    	<< m_lpos << "+"<<m_lbin << " diff: [" << diff->lbegin() << " --> "
    	<< diff->lend() << "]\n";
    cerr << msg.str();
    m_input.insert(diff);
    while (true) {
	purge_bygone();
	if (starved()) {
	    break;
	}
	latch_one(outq);
    }

    return true;
}

void PlaneDuctor::latch_one(output_queue& outq)
{
    IPlaneSlice::pointer ps = latch_hits();
    outq.push_back(ps);
    m_lpos += m_lbin;		// on to next.
}


void PlaneDuctor::purge_bygone()
{
    IDiffusion::vector to_kill;
    for (auto diff : m_input) {
	if (diff->lbegin() >= m_lpos) {
	    break;
	}

	if (m_lpos >= diff->lend()) {
	    to_kill.push_back(diff);
	}
    }
    if (to_kill.empty()) {
	return;
    }

    for (auto diff : to_kill) {
	// cerr << m_wpid.layer() << " " << diff.get()
	//      << "\tpurge lpos=" << m_lpos
	//      << " diff=[" << diff->lbegin() << " --> " << diff->lend() << "]@" << diff->lbin() << "*" << diff->lsize()
	//      << endl;
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
	int wbin_begin = (diff->tbegin() - m_tpos0)/diff->tbin();
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
		 << " longitudinal:[" << diff->lbegin() << "," << diff->lend() << "]"
		 << " transverse:[" << diff->tbegin() << "," << diff->tend() << "]"
		 << " I am at lpos=" << m_lpos << "+"<< m_lbin << ", trans:["<<m_tpos0<<","<<m_tpos0+m_nwires*m_tbin << "]"
	    	 << endl;
	    continue;		// patch fully outside
	}

	// cerr << m_wpid.ident() << " " << diff.get()
	//      << " tbins:["<< tbin_begin << "," << tbin_end << "]"
	//      << " wbins:["<< wbin_begin << "," << wbin_end << "] = "
	//      << nwires_patch << " wires"
	//      <<  endl;

        std::vector<double> wire_charge;
	for (int tbin = tbin_begin; tbin < tbin_end; ++tbin) {
	    wire_charge.push_back(diff->get(lbin,tbin));
	}

	IPlaneSlice::WireChargeRun wcr(wbin_begin, wire_charge);
	wcrv.push_back(wcr);
    }

    // if (wcrv.size()) {
    // 	cerr << m_wpid.ident() << "\tslice made at lpos=" << m_lpos << " runs=" << wcrv.size() << endl;
    // }
    IPlaneSlice::pointer ret(new SimplePlaneSlice(m_wpid, m_lpos, wcrv));
    return ret;
}

