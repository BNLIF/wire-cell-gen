#include "WireCellGen/Drifter.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/String.h"

#include "WireCellIface/SimpleDepo.h"

#include <boost/range.hpp>

#include <sstream>
#include <iostream>

WIRECELL_FACTORY(Drifter, WireCell::Gen::Drifter, WireCell::IDrifter, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

Gen::Drifter::Drifter(const std::string& rng_tn)
    : m_rng_tn(rng_tn)
    , m_DL(7.2 * units::centimeter2/units::second) // from arXiv:1508.07059v2
    , m_DT(12.0 * units::centimeter2/units::second) // ditto
    , m_lifetime(8*units::ms) // read off RHS of figure 6 in MICROBOONE-NOTE-1003-PUB
    , m_fluctuate(true)
    , m_location(0)
    , m_speed(1.6*units::mm/units::us)
    , m_input()
{
}

void Gen::Drifter::configure(const WireCell::Configuration& cfg)
{
    m_rng_tn = get(cfg, "rng", m_rng_tn);
    m_rng = Factory::find_tn<IRandom>(m_rng_tn);
    if (!m_rng) {
        cerr << "Gen::Drifter::configure: no such IRandom: " << m_rng_tn << endl;
    }

    m_speed = get<double>(cfg, "speed", m_speed);
    m_location = get<double>(cfg, "location", m_location);
    m_input = DepoTauSortedSet(IDepoDriftCompare(m_speed));

    m_DL = get<double>(cfg, "DL", m_DL);
    m_DT = get<double>(cfg, "DT", m_DT);
    m_lifetime = get<double>(cfg, "lifetime", m_lifetime);
    m_fluctuate = get<bool>(cfg, "fluctuate", m_fluctuate);
    cerr << "Gen::Drifter: configured with location " << m_location << endl;
}

WireCell::Configuration Gen::Drifter::default_configuration() const
{
    Configuration cfg;
    cfg["DL"] = m_DL;
    cfg["DT"] = m_DT;
    cfg["lifetime"] = m_lifetime;
    cfg["fluctuate"] = m_fluctuate;
    cfg["location"] = m_location;
    cfg["speed"] = m_speed;
    return cfg;
}


double Gen::Drifter::proper_time(IDepo::pointer depo)
{
    return depo->time() + depo->pos().x()/m_speed;
}

void Gen::Drifter::reset()
{
    m_input.clear();
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

    // absorption probability of an electron
    const double absorbprob = 1 - exp(-dt/m_lifetime); 

    // final number of electrons after drift if no fluctuation.
    double dQ = Qi * absorbprob;

    // How many electrons remain, with fluctuation.
    // Note: fano/recomb fluctuation should be done before the depo was first made.
    if (m_fluctuate) {
        double sign = 1.0;
        if (dQ < 0) {
            sign = -1.0;
        }
        dQ = sign*m_rng->binomial((int)std::abs(dQ), absorbprob);
    }
    const double Qf = Qi - dQ;

    const double sigmaL = sqrt(2.0*m_DL*dt);
    const double sigmaT = sqrt(2.0*m_DT*dt);

    auto ret = make_shared<SimpleDepo>(then, there, Qf, depo, sigmaL, sigmaT);
    // cerr << "Gen::Drifter: "
    //      << "q:" << ret->charge()/units::eplus << " electrons "
    //      << "t:" << depo->time()/units::us << " -> " << ret->time()/units::us << " ("<<dt/units::us<<") us, "
    //      << "x:" << depo->pos().x()/units::mm << " -> " << ret->pos().x()/units::mm << "mm, "
    //      << "long=" << ret->extent_long()/units::mm << "mm, trans=" << ret->extent_tran()/units::mm << "mm "
    //      << endl;
    return ret;

}

bool Gen::Drifter::operator()(const input_pointer& depo, output_queue& outq)
{
    if (m_speed <= 0.0) {
        cerr << "Gen::Drifter: illegal drift speed\n";
        return false;
    }

    if (!depo) {		// no more inputs expected, EOS flush
        //cerr << "Gen::Drifter: flushing " << m_input.size() << "\n";
	while (!m_input.empty()) {	
	    IDepo::pointer top = *m_input.begin();
	    m_input.erase(top);
	    outq.push_back(transport(top));
	}
	outq.push_back(nullptr);

	return true;
    }

    m_input.insert(depo);
    while (!m_input.empty()) {	
	IDepo::pointer top = *m_input.begin();
	double tau = proper_time(top);

	IDepo::pointer bot = *m_input.rbegin();

	if (bot->time() < tau) {
	    return true;       // bail when we reach unknown territory
	}

	m_input.erase(top);
        outq.push_back(transport(top));
    }
    //cerr << "Gen::Drifter: returning " << outq.size() << endl;
    return true;
}

