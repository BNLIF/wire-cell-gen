#include "WireCellGen/VagabondX.h"
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

WIRECELL_FACTORY(VagabondX, WireCell::Gen::VagabondX, WireCell::IDrifter, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

// Xregion helper
Gen::VagabondX::Xregion::Xregion(double ax, double cx)
    : anode(ax)
    , cathode(cx)
{
}
bool Gen::VagabondX::Xregion::inside(double x) const
{
    return (anode < x and x < cathode) or (cathode < x and x < anode);
}


Gen::VagabondX::VagabondX()
    : m_rng(nullptr)
    , m_rng_tn("Random")
    , m_DL(7.2 * units::centimeter2/units::second) // from arXiv:1508.07059v2
    , m_DT(12.0 * units::centimeter2/units::second) // ditto
    , m_lifetime(8*units::ms) // read off RHS of figure 6 in MICROBOONE-NOTE-1003-PUB
    , m_fluctuate(true)
    , m_speed(1.6*units::mm/units::us)
    , n_dropped(0)
    , n_drifted(0)
{
}

Gen::VagabondX::~VagabondX()
{
}

WireCell::Configuration Gen::VagabondX::default_configuration() const
{
    Configuration cfg;
    cfg["DL"] = m_DL;
    cfg["DT"] = m_DT;
    cfg["lifetime"] = m_lifetime;
    cfg["fluctuate"] = m_fluctuate;
    cfg["drift_speed"] = m_speed;

    // The xregions are a list of objects.  Each object has an "anode"
    // and a "cathode" attribute which specify the global X coordinate
    // where their planes are located.
    cfg["xregions"] = Json::arrayValue;
    return cfg;
}

void Gen::VagabondX::configure(const WireCell::Configuration& cfg)
{
    reset();

    m_rng_tn = get(cfg, "rng", m_rng_tn);
    m_rng = Factory::find_tn<IRandom>(m_rng_tn);

    m_DL = get<double>(cfg, "DL", m_DL);
    m_DT = get<double>(cfg, "DT", m_DT);
    m_lifetime = get<double>(cfg, "lifetime", m_lifetime);
    m_fluctuate = get<bool>(cfg, "fluctuate", m_fluctuate);
    m_speed = get<double>(cfg, "drift_speed", m_speed);

    for (auto jone : cfg["xregions"]) {
        m_xregions.push_back(Xregion(jone["anode"].asDouble(), jone["cathode"].asDouble()));
        cerr << "Gen::VagabondX: {anode:"<<m_xregions.back().anode/units::cm<<"cm, cathode:"<<m_xregions.back().cathode/units::cm<<"cm}\n";
    }
}

void Gen::VagabondX::reset()
{
    m_xregions.clear();
}


bool Gen::VagabondX::insert(const input_pointer& depo)
{
    // Find which X region to add, or reject.  Maybe there is a faster
    // way to do this than a loop.  For example, the regions could be
    // examined in order to find some regular bining of the X axis and
    // then at worse only explicitly check extent partial bins near to
    // their edges.
    auto xrit = std::find_if(m_xregions.begin(), m_xregions.end(), 
                             Gen::VagabondX::IsInside(depo));
    if (xrit == m_xregions.end()) {
        //cerr << "VX: outside " << depo->pos()/units::cm << "cm \n";
        return false;           // miss
    }

    // Now actually do the drift.
    Point pos = depo->pos();

    //cerr << "VX: inside " << pos/units::cm << "cm in ["<<xrit->anode/units::cm<<","<<xrit->cathode/units::cm<<"]cm\n";

    const double dt = std::abs((xrit->anode - pos.x())/m_speed);
    pos.x(xrit->anode);         // replace

    // number of electrons to start with
    const double Qi = depo->charge();
    // final number of electrons after drift
    double dQ = Qi * (1 - exp(-dt/m_lifetime));
    // how many electrons remain, with fluctuation.
    if (m_fluctuate) {
        const double sign = dQ < 0 ? -1.0 : 1.0;
        dQ = m_rng->poisson(std::abs(dQ));
        dQ = sign*dQ;
    }
    const double Qf = Qi - dQ;

    const double sigmaL = sqrt(2.0*m_DL*dt);
    const double sigmaT = sqrt(2.0*m_DT*dt);

    auto newdepo = make_shared<SimpleDepo>(depo->time() + dt, pos, Qf, depo, sigmaL, sigmaT);
    xrit->depos.push_back(newdepo);
    return true;
}    


bool by_time(const IDepo::pointer& lhs, const IDepo::pointer& rhs) {
    return lhs->time() < rhs->time();
}

// save all cached depos to the output queue sorted in time order
void Gen::VagabondX::flush(output_queue& outq)
{
    for (auto& xr : m_xregions) {
        outq.insert(outq.end(), xr.depos.begin(), xr.depos.end());
        xr.depos.clear();
    }
    std::sort(outq.begin(), outq.end(), by_time);
    outq.push_back(nullptr);
}

void Gen::VagabondX::flush_ripe(output_queue& outq, double now)
{
    // It might be faster to use a sorted set which would avoid an
    // exhaustive iteration of each depos stash.  Or not.
    for (auto& xr : m_xregions) {
        std::vector<IDepo::pointer> to_keep;
        for (auto& depo : xr.depos) {
            if (depo->time() < now) {
                outq.push_back(depo);
            }
            else {
                to_keep.push_back(depo);
            }
        }
        xr.depos = to_keep;
        //cerr << "VX: outputing " << outq.size() << ", keeping " << to_keep.size()
        //     << " for [" << xr.anode/units::cm <<","<<xr.cathode/units::cm<<"]\n";
    }
    std::sort(outq.begin(), outq.end(), by_time);
}


// always returns true because by hook or crook we consume the input.
bool Gen::VagabondX::operator()(const input_pointer& depo, output_queue& outq)
{
    if (m_speed <= 0.0) {
        cerr << "Gen::VagabondX: illegal drift speed\n";
        return false;
    }

    if (!depo) {		// no more inputs expected, EOS flush

        flush(outq);

        if (n_dropped) {
            cerr << "Gen::VagabondX: at EOS, dropped " << n_dropped << "/" << n_dropped+n_drifted << " depos from stream\n";
        }
        n_drifted = n_dropped = 0;
        return true;
    }

    bool ok = insert(depo);
    if (!ok) {
        ++n_dropped;            // depo is outside our regions but
        return true;            // nothing changed, so just bail
    }
    ++n_drifted;

    // At this point, time has advanced and maybe some drifted repos
    // are ripe for removal.

    flush_ripe(outq, depo->time());

    return true;
}

