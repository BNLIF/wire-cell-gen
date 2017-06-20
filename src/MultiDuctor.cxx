#include "WireCellGen/MultiDuctor.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Pimpos.h"
#include "WireCellUtil/Binning.h"
#include "WireCellUtil/Exceptions.h"

#include <vector>

WIRECELL_FACTORY(MultiDuctor, WireCell::Gen::MultiDuctor, WireCell::IDuctor, WireCell::IConfigurable);

using namespace WireCell;

Gen::MultiDuctor::MultiDuctor(const std::string anode)
    : m_anode_tn(anode)
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
     return cfg;
 }


struct Wirebounds {
    std::vector<const Pimpos*> pimpos;
    Json::Value jargs;
    Wirebounds(const std::vector<const Pimpos*>& p, Json::Value jargs) : pimpos(p), jargs(jargs) { }

    bool operator()(IDepo::pointer depo) {

        if (!depo) {
            return false;
        }
            
        // fixme: it's possibly really slow to do all this Json::Value
        // access deep inside this loop.  If so, the Json::Values
        // should be run through once in configure() and stored into
        // some faster data structure.

        for (auto jregion : jargs) {
            bool inregion = true;
            for (auto jrange: jregion) {
                int iplane = jrange["plane"].asInt();
                int imin = jrange["min"].asInt();
                int imax = jrange["max"].asInt();

                double pitch = pimpos[iplane]->distance(depo->pos());
                int iwire = pimpos[iplane]->region_binning().bin(pitch); // fixme: warning: round off error?
                inregion = inregion && (imin <= iwire && iwire <= imax);
                if (!inregion) {
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
             std::cerr << "\tDuctor: " << ductor_tn << " rule: " << rule << std::endl;
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

 bool Gen::MultiDuctor::operator()(const input_pointer& depo, output_queue& frames)
 {
     // The DFP protocol calls for a nullptr to indicates "end of
     // stream".  This is usually interpreted as "flush".  Simply pass
     // this on to the sub-ductors.
     if (!depo) {              
         bool all_okay = true;         
         std::cerr << "Gen::MultiDuctor: got null depo, passing on:\n";
         for (auto& chain : m_chains) {
             for (auto& sd : chain) {
                 bool ok = (*sd.ductor)(depo, frames);                 
                 std::cerr << "\t" << sd.name << " ok:" << ok << " frames:" << frames.size() << std::endl;
                 all_okay = all_okay && ok;
             }
         }
         return all_okay;
     }

     // o.w. send the depo down the chain according to the rules.
     bool all_okay = true;
     int count = 0;
     for (auto& chain : m_chains) {
         for (auto& sd : chain) {
             if (!sd.check(depo)) {
                 continue;
             }
             ++count;
             bool ok = (*sd.ductor)(depo, frames);
             all_okay = all_okay && ok;
         }
     }
     if (depo && !count) {
         std::cerr << "Gen::MultiDuctor: no appropriate Ductor for depo at: " << depo->pos() << std::endl;
     }
     if (frames.size()) {
         std::cerr << "Gen::MultiDuctor: returning " << frames.size() << " frames\n";
     }
     return all_okay;
 }


