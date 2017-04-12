#include "WireCellGen/Drifter.h"
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

WIRECELL_FACTORY(Drifter, WireCell::Gen::Drifter, WireCell::IDrifter, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

Gen::Drifter::Drifter(const std::string& anode_tn)
    : m_anode_tn(anode_tn)
    , m_DL(7.2 * units::centimeter2/units::second) // from arXiv:1508.07059v2
    , m_DT(12.0 * units::centimeter2/units::second) // ditto
    , m_lifetime(8*units::ms) // read off RHS of figure 6 in MICROBOONE-NOTE-1003-PUB
    , m_fluctuate(true)
    , m_location(-1)            // cache
    , m_speed(-1)               // cache
    , m_input()
    , m_eos(false)
{
}

void Gen::Drifter::configure(const WireCell::Configuration& cfg)
{
    m_anode_tn = get<string>(cfg, "anode", m_anode_tn);
    this->set_anode();
    m_DL = get<double>(cfg, "DL", m_DL);
    m_DT = get<double>(cfg, "DT", m_DT);
    m_lifetime = get<double>(cfg, "lifetime", m_lifetime);
    m_fluctuate = get<bool>(cfg, "fluctuate", m_fluctuate);
    cerr << "Configured Gen::Drifter with anode plane " << m_anode_tn << endl;
}

WireCell::Configuration Gen::Drifter::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = m_anode_tn;
    cfg["DL"] = m_DL;
    cfg["DT"] = m_DT;
    cfg["lifetime"] = m_lifetime;
    cfg["fluctuate"] = m_fluctuate;
    return cfg;
}

void Gen::Drifter::set_anode()
{
    if (m_anode_tn.empty()) {
        cerr << "Gen::Drifter: no anode plane type:name configured yet\n";
        return;
    }
    auto anode = Factory::lookup_tn<IAnodePlane>(m_anode_tn);
    if (!anode) {
        cerr << "Gen::Drifter: failed to get anode: \"" << m_anode_tn << "\"\n";
        return;
    }
    if (anode->nfaces() <= 0) {
        cerr << "Gen::Drifter: anode " << m_anode_tn << " has no faces\n";
        return;
    }

    /// Warning: this assumes "front" face.
    /// Fixme: it's also kind of a long road to walk.....
    auto faces = anode->faces();
    auto face = faces[0];
    auto planes = face->planes();
    auto plane = planes[face->nplanes()-1];
    auto pir = plane->pir();
    const auto& fr = pir->field_response();
    m_speed = fr.speed;
    m_location = fr.origin;
    m_input = DepoTauSortedSet(IDepoDriftCompare(m_speed));
    cerr << "Gen::Drifter: configured with drift speed=" << m_speed/(units::mm/units::us)
         << " drifting to x=" << m_location/units::mm << "mm\n";
}

double Gen::Drifter::proper_time(IDepo::pointer depo)
{
    return depo->time() + depo->pos().x()/m_speed;
}

void Gen::Drifter::reset()
{
    m_input.clear();
}

// fixme: this needs to be moved into a random number generator
// provided through an interface.  For now, just use std.
int random_poisson(double mean)
{
    std::default_random_engine generator; // fixme: this should be a shared, long-lived object....
    std::poisson_distribution<int> distribution(mean);
    return distribution(generator);
}

IDepo::pointer Gen::Drifter::transport(IDepo::pointer depo)
{
    if (!depo) {
        cerr << "Gen::Drifter error, asked to transport null depo\n";
        return nullptr;
    }
    // position
    const Point here = depo->pos();
    const Point there(m_location, here.y(), here.z());

    // time
    const double now = depo->time();
    const double dt = (here.x() - m_location)/m_speed;
    const double then = now + dt;

    // number of electrons to start with
    const double Qi = depo->charge();
    // final number of electrons after drift
    double dQ = Qi * (1 - exp(-dt/m_lifetime));
    // how many electrons remain, with fluctuation.
    if (m_fluctuate) {
        dQ = random_poisson(dQ);
    }
    const double Qf = Qi - dQ;

    const double tmpcm2 = 2*m_DL*dt/units::centimeter2;
    //const double sigmaL = sqrt(tmpcm2)*units::centimeter / drift_speed;
    const double sigmaL = sqrt(tmpcm2)*units::centimeter;
    const double sigmaT = sqrt(2*m_DT*dt/units::centimeter2)*units::centimeter2;

    return make_shared<SimpleDepo>(then, there, Qf, depo, sigmaL, sigmaT);
}

bool Gen::Drifter::operator()(const input_pointer& depo, output_queue& outq)
{
    if (m_speed <= 0.0) {
        cerr << "Gen::Drifter: illegal drift speed\n";
        return false;
    }
    if (m_eos) {
	return false;
    }

    if (!depo) {		// EOS flush
	while (!m_input.empty()) {	
	    IDepo::pointer top = *m_input.begin();
	    m_input.erase(top);
	    outq.push_back(transport(top));
	}
	outq.push_back(nullptr);

	m_eos = true;
	return true;
    }

    m_input.insert(depo);
    while (!m_input.empty()) {	
	IDepo::pointer top = *m_input.begin();
	double tau = proper_time(top);

	IDepo::pointer bot = *m_input.rbegin();

        // cerr << "Drifter: processing bot:" << bot->time()/units::ms << "ms, "
        //      << "tau:" << tau/units::ms << "ms, top: t=" << top->time()/units::ms << "ms "
        //      << "x=" << top->pos().x()/units::mm << "mm, speed=" << m_speed/(units::mm/units::us) << "mm/ms\n";

	if (bot->time() < tau) {
	    return true;       // bail when we reach unknown territory
	}

	m_input.erase(top);
	outq.push_back(transport(top));
    }
    return true;
}

