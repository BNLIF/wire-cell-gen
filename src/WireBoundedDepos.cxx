#include "WireCellGen/WireBoundedDepos.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/String.h"

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IAnodeFace.h"
#include "WireCellIface/IWirePlane.h"
#include "WireCellIface/SimpleDepo.h"

#include <boost/range.hpp>

#include <sstream>
#include <iostream>

WIRECELL_FACTORY(WireBoundedDepos, WireCell::Gen::WireBoundedDepos,
                 WireCell::IDrifter, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

WireCell::Configuration Gen::WireBoundedDepos::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = "";

    // A the list of wire regions
    // [{
    //   plane: <plane-number>,
    //   min: <min-wire-number>,
    //   max: <max-wire-number,
    // },...]
    cfg["wires"] = Json::arrayValue;

    cfg["mode"] = "accept";

    return cfg;
}

void Gen::WireBoundedDepos::configure(const WireCell::Configuration& cfg)
{
    // we just want the pimpos but best to keep a handle on the anode
    // since it's shared_ptr memory.
    m_anode = Factory::find_tn<IAnodePlane>(cfg["anode"].asString());

    for (auto face : m_anode->faces()) {
        if (face->planes().empty()) {
            std::cerr << "Gen::WireBoundedDepos: not given multi-plane AnodeFace for face "
                      << face->ident() << "\n";
            continue;
        }
        for (auto plane : face->planes()) {
            const size_t iplane = plane->ident();
            if (m_pimpos.size() <= iplane) {
                m_pimpos.resize(iplane+1);
            }
            m_pimpos[iplane] = plane->pimpos();
        }
        break;                 // fixme: 
    }
    m_accept = cfg["mode"].asString() == "accept";

    auto jwires = cfg["wires"];
    for (auto jone : jwires) {
        m_wires.push_back(wire_bounds_t(jone["plane"].asInt(),
                                        jone["min"].asInt(),
                                        jone["max"].asInt()));
    }

    std::cerr << "WireBoundedDepos: " << cfg ["mode"]
              << " with " << m_wires.size() << " wires in "
              << m_pimpos.size() << " planes\n";
}

bool Gen::WireBoundedDepos::operator()(const input_pointer& depo, output_queue& outq)
{
    if (!depo) {                // EOS protocol
        outq.push_back(nullptr);
        return true;
    }

    // lazily calcualte nearest wire
    const size_t nplanes = m_pimpos.size();
    std::vector<bool> already(nplanes, false);
    std::vector<int> closest_wire(nplanes, -1);

    for (const auto& pmm : m_wires) {
        
        const int iplane = get<0>(pmm);
        const int imin = get<1>(pmm);
        const int imax = get<2>(pmm);

        if (!already[iplane]) { // lazy
            const double pitch = m_pimpos[iplane]->distance(depo->pos(), 2);
            closest_wire[iplane] = m_pimpos[iplane]->region_binning().bin(pitch);
            already[iplane] = true;
        }
        const int iwire = closest_wire[iplane];
        const bool inregion = imin <= iwire && iwire <= imax;

        if (inregion) {
            if (m_accept) {
                outq.push_back(depo);
            }
            // accept or reject, we landed this depo in a configured
            // region.
            return true;
        }
    }
    if (!m_accept) {            // depo missed all rejections.
        outq.push_back(depo);
    }
    return true;
}

Gen::WireBoundedDepos::WireBoundedDepos()
    : m_accept(true)
{
}
Gen::WireBoundedDepos::~WireBoundedDepos()
{
}
