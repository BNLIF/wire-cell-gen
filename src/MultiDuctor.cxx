#include "WireCellGen/MultiDuctor.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Pimpos.h"
#include "WireCellUtil/Binning.h"

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
    bool operator()(IDepo::pointer depo) { return ok; }
};


 void Gen::MultiDuctor::configure(const WireCell::Configuration& cfg)
 {
     m_anode_tn = get(cfg, "anode", m_anode_tn);
     m_anode = Factory::lookup_tn<IAnodePlane>(m_anode_tn);
     if (!m_anode) {
         std::cerr << "Gen::MultiDuctor::configure: error: unknown anode: " << m_anode_tn << std::endl;
         return;
     }
     auto chain = cfg["chain"];
     if (chain.isNull()) {
         std::cerr << "Gen::MultiDuctor::configure: warning: configured with empty chain\n";
         return;
     }

     /// fixme: this is totally going to break when going to multi-faced anodes.
     std::vector<const Pimpos*> pimpos;
     for (auto face : m_anode->faces()) {
         for (auto plane : face->planes()) {
             pimpos.push_back(plane->pimpos());
         }
         break;                 // fixme: 
     }

     for (auto jrule : chain) {
         auto rule = jrule["rule"].asString();
         auto ductor_tn = jrule["ductor"].asString();
         auto ductor = Factory::lookup_tn<IDuctor>(ductor_tn);
         if (!ductor) {
             std::cerr << "Gen::MultiDuctor: failed to lookup (sub) Ductor: " << ductor_tn << std::endl;
             continue;          // fixme: need to throw exception
         }
         auto jargs = jrule["args"];
         if (rule == "wirebounds") {
             m_subductors.push_back(SubDuctor(Wirebounds(pimpos, jargs), ductor));
             continue;
         }
         if (rule == "bool") {
             m_subductors.push_back(SubDuctor(ReturnBool(jargs), ductor));
         }
     }

 }

 void Gen::MultiDuctor::reset()
 {
 }

 bool Gen::MultiDuctor::operator()(const input_pointer& depo, output_queue& frames)
 {
     for (auto sd : m_subductors) {
         if (!sd.check(depo)) {
             continue;
         }
         return (*sd.ductor)(depo, frames);
     }
     std::cerr << "Gen::MultiDuctor: no appropriate Ductor for depo at: " << depo->pos() << std::endl;
     return false;
 }


