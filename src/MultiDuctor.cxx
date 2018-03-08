#include "WireCellGen/MultiDuctor.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Pimpos.h"
#include "WireCellUtil/Binning.h"
#include "WireCellUtil/Exceptions.h"

// fixme: this needs to move out of iface!
#include "WireCellIface/FrameTools.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"

#include <vector>

WIRECELL_FACTORY(MultiDuctor, WireCell::Gen::MultiDuctor, WireCell::IDuctor, WireCell::IConfigurable);

using namespace WireCell;

Gen::MultiDuctor::MultiDuctor(const std::string anode)
    : m_anode_tn(anode)
    , m_start_time(0.0*units::ns)
    , m_readout_time(5.0*units::ms)
    , m_frame_count(0)
    , m_eos(false)
{
}
Gen::MultiDuctor::~MultiDuctor()
{
}

WireCell::Configuration Gen::MultiDuctor::default_configuration() const
{
    // The "chain" is a list of dictionaries.  Each provides:
    // - ductor: TYPE[:NAME] of a ductor component
    // - rule: name of a rule function to apply
    // - args: args for the rule function
    //
    // Rule functions:
    // wirebounds (list of wire ranges)
    // bool (true/false)

    Configuration cfg;
    cfg["anode"] = m_anode_tn;
    cfg["chain"] = Json::arrayValue;

    /// The initial time for this ductor
    cfg["start_time"] = m_start_time;

    /// The time span for each readout.
    cfg["readout_time"] = m_readout_time;

    /// Allow for a custom starting frame number
    cfg["first_frame_number"] = m_frame_count;
     
    return cfg;
}


struct Wirebounds {
    std::vector<const Pimpos*> pimpos;
    Json::Value jargs;
    Wirebounds(const std::vector<const Pimpos*>& p, Json::Value jargs) : pimpos(p), jargs(jargs) { }

    bool operator()(IDepo::pointer depo) {

        if (!depo) {
            std::cerr << "Wirebounds: no depo\n";
            return false;
        }
            
        // fixme: it's possibly really slow to do all this Json::Value
        // access deep inside this loop.  If so, the Json::Values
        // should be run through once in configure() and stored into
        // some faster data structure.

        // return true if depo is "in" ANY region.
        for (auto jregion : jargs) {
            bool inregion = true;

            // depo must be in ALL ranges of a region
            for (auto jrange: jregion) {
                int iplane = jrange["plane"].asInt();
                int imin = jrange["min"].asInt();
                int imax = jrange["max"].asInt();

                double pitch = pimpos[iplane]->distance(depo->pos());
                int iwire = pimpos[iplane]->region_binning().bin(pitch); // fixme: warning: round off error?
                inregion = inregion && (imin <= iwire && iwire <= imax);
                if (!inregion) {
                    // std::cerr << "Wirebounds: wire: "<<iwire<<" of plane " << iplane << " not in [" << imin << "," << imax << "]\n";
                    break;      // not in this view's region, no reason to keep checking.
                }
            }
            if (inregion) {     // only need one region 
                return true;
            }
        }
        return false;
    }
};

struct ReturnBool {
    bool ok;
    ReturnBool(Json::Value jargs) : ok(jargs.asBool()) {}
    bool operator()(IDepo::pointer depo) {
        if (!depo) {return false;}
        return ok;
    }
};


void Gen::MultiDuctor::configure(const WireCell::Configuration& cfg)
{
    m_readout_time = get<double>(cfg, "readout_time", m_readout_time);
    m_start_time = get<double>(cfg, "start_time", m_start_time);
    m_frame_count = get<int>(cfg, "first_frame_number", m_frame_count);

    m_anode_tn = get(cfg, "anode", m_anode_tn);
    m_anode = Factory::find_tn<IAnodePlane>(m_anode_tn);
    if (!m_anode) {
        std::cerr << "Gen::MultiDuctor::configure: error: unknown anode: " << m_anode_tn << std::endl;
        return;
    }
    auto jchains = cfg["chains"];
    if (jchains.isNull()) {
        std::cerr << "Gen::MultiDuctor::configure: warning: configured with empty collection of chains\n";
        return;
    }

    /// fixme: this is totally going to break when going to two-faced anodes.
    std::vector<const Pimpos*> pimpos;
    for (auto face : m_anode->faces()) {
        for (auto plane : face->planes()) {
            pimpos.push_back(plane->pimpos());
        }
        break;                 // fixme: 
    }

    for (auto jchain : jchains) {
        std::cerr << "Gen::MultiDuctor::configure chain:\n";
        ductorchain_t dchain;

        for (auto jrule : jchain) {
            auto rule = jrule["rule"].asString();
            auto ductor_tn = jrule["ductor"].asString();
            std::cerr << "\tMultiDuctor: " << ductor_tn << " rule: " << rule << std::endl;
            auto ductor = Factory::find_tn<IDuctor>(ductor_tn);
            if (!ductor) {
                THROW(KeyError() << errmsg{"Failed to find (sub) Ductor: " + ductor_tn});
            }
            auto jargs = jrule["args"];
            if (rule == "wirebounds") {
                dchain.push_back(SubDuctor(ductor_tn, Wirebounds(pimpos, jargs), ductor));
                continue;
            }
            if (rule == "bool") {
                dchain.push_back(SubDuctor(ductor_tn, ReturnBool(jargs), ductor));
            }
            m_chains.push_back(dchain);
        }
    } // loop to store chains of ductors
}

void Gen::MultiDuctor::reset()
{
    // forward message
    for (auto& chain : m_chains) {
        for (auto& sd : chain) {
            sd.ductor->reset();
        }
    }
}


bool Gen::MultiDuctor::maybe_extract(const input_pointer& depo, output_queue& outframes)
{
    double target_time = m_start_time + m_readout_time;
    if (depo && depo->time() < target_time) {
        return true;            // not yet
    }

    // Must flush all sub-ductors to assure synchronization.
    for (auto& chain : m_chains) {
        for (auto& sd : chain) {
            output_queue newframes;
            bool ok = (*sd.ductor)(nullptr, newframes); // flush with EOS
            if (!ok) { return false; }
            sd.ductor->reset();
            merge(newframes);   // updates frame buffer
        }
    }

    double tick = 0.5*units::us; // note, could be differ from what
                                 // sub-ductors use when they actually
                                 // have waveforms to give us.
    if (!m_frame_buffer.size()) {
        // we read out, and yet we have nothing
        ITrace::vector traces;
        auto frame = std::make_shared<SimpleFrame>(m_frame_count, m_start_time, traces, tick);
        outframes.push_back(frame);
        m_start_time += m_readout_time;
        ++m_frame_count;
        return true;
    }


    // At this point the m_frame_buffer has at least some frames from
    // multiple sub-ductors, each of different time and channel extent
    // and potentially out of order.

    output_queue to_keep, to_extract;

    for (auto frame: m_frame_buffer) {
        tick = frame->tick();
        int cmp = FrameTools::frmtcmp(frame, target_time);
        if (cmp < 0) {
            to_extract.push_back(frame);
            continue;
        }
        if (cmp > 0) {
            to_keep.push_back(frame);
            continue;
        }                

        auto ff = FrameTools::split(frame, m_start_time);
        to_extract.push_back(ff.first);
        to_keep.push_back(ff.second);
    }
    m_frame_buffer = to_keep;
    
    if (!to_extract.size()) {
        // we read out, and yet we have nothing
        ITrace::vector traces;
        auto frame = std::make_shared<SimpleFrame>(m_frame_count, m_start_time, traces, tick);
        outframes.push_back(frame);
        m_start_time += m_readout_time;
        ++m_frame_count;
        return true;
    }
    

    /// Big fat lazy programmer FIXME: there is a bug waiting to bite
    /// here.  If somehow a sub-ductor manages to make a frame that
    /// remains bigger than m_readout_time after the split() above, it
    /// will cause the final output frame to extend past requested
    /// duration.  There needs to be a loop over to_extract added.

    ITrace::vector traces;
    for (auto frame: to_extract) {
        const double tref = frame->time();
        const int extra_tbins = (tref-m_start_time)/tick;
        for (auto trace : (*frame->traces())) {
            const int tbin = trace->tbin() + extra_tbins;
            const int chid = trace->channel();
            auto mtrace = std::make_shared<SimpleTrace>(chid, tbin, trace->charge());
            traces.push_back(mtrace);
        }
    }
    auto frame = std::make_shared<SimpleFrame>(m_frame_count, m_start_time, traces, tick);
    outframes.push_back(frame);
    m_start_time += m_readout_time;
    ++m_frame_count;
    return true;
}

void Gen::MultiDuctor::merge(const output_queue& newframes)
{
    for (auto frame : newframes) {
        if (!frame) { continue; }
        auto traces = frame->traces();
        if (!traces) { continue; }
        if (traces->empty()) { continue; }
        m_frame_buffer.push_back(frame);
    }
}

bool Gen::MultiDuctor::operator()(const input_pointer& depo, output_queue& outframes)
{
    if (m_eos) {
        return false;
    }

    // end of stream processing
    if (!depo) {              
        std::cerr << "Gen::MultiDuctor: end of stream processing\n";
        bool ok = maybe_extract(depo, outframes);
        outframes.push_back(nullptr); // eos marker
        m_eos = true;
        return ok;            // fixme: this should throw is no okay
    }


    // check each rule in the chain to find match
    bool all_okay = true;
    int count = 0;
    for (auto& chain : m_chains) {

        for (auto& sd : chain) {

            if (!sd.check(depo)) {
                continue;
            }

            // found a matching sub-ductor
            ++count;

            // Normal buffered processing.
            output_queue newframes;
            bool ok = (*sd.ductor)(depo, newframes);
            merge(newframes);
            all_okay = all_okay && ok;
        }
    }
    if (depo && !count) {
        std::cerr << "Gen::MultiDuctor: no appropriate Ductor for depo at: " << depo->pos() << std::endl;
    }

    all_okay = all_okay && maybe_extract(depo, outframes);
    if (outframes.size()) {
        std::cerr << "Gen::MultiDuctor: returning " << outframes.size() << " frames\n";
    }
    return all_okay;            // fixme: this should throw is no okay
}


