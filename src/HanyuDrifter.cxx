#include "WireCellGen/hanyu_Drifter.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/String.h"

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IAnodeFace.h"
#include "WireCellIface/IWirePlane.h"
#include "WireCellIface/SimpleDepo.h"

#include <boost/range.hpp>

#include <random>
#include <sstream>
#include <iostream>

WIRECELL_FACTORY(hyDrifter, WireCell::Gen::hyDrifter, WireCell::IDrifter, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

Gen::hyDrifter::hyDrifter(const std::string& anode_tn)
    : m_anode_tn(anode_tn)
    , m_DL(7.2 * units::centimeter2/units::second) // from arXiv:1508.07059v2
    , m_DT(12.0 * units::centimeter2/units::second) // ditto
    , m_lifetime(8*units::ms) // read off RHS of figure 6 in MICROBOONE-NOTE-1003-PUB
    , m_fluctuate(true)
    , m_location(-1)            // cache
    , m_speed(-1)               // cache
    , m_input()
    , m_fano(0.1)           // fano factor
    , m_recomb(0.7)         // recombination probability
    , m_eos(false)
{
}

void Gen::hyDrifter::configure(const WireCell::Configuration& cfg)
{
    m_anode_tn = get<string>(cfg, "anode", m_anode_tn);
    this->set_anode();
    m_DL = get<double>(cfg, "DL", m_DL);
    m_DT = get<double>(cfg, "DT", m_DT);
    m_lifetime = get<double>(cfg, "lifetime", m_lifetime);
    m_fluctuate = get<bool>(cfg, "fluctuate", m_fluctuate);
    cerr << "Configured Gen::hyDrifter with anode plane " << m_anode_tn << endl;
}

WireCell::Configuration Gen::hyDrifter::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = m_anode_tn;
    cfg["DL"] = m_DL;
    cfg["DT"] = m_DT;
    cfg["lifetime"] = m_lifetime;
    cfg["fluctuate"] = m_fluctuate;
    return cfg;
}

void Gen::hyDrifter::set_anode()
{
    if (m_anode_tn.empty()) {
        cerr << "Gen::hyDrifter: no anode plane type:name configured yet\n";
        return;
    }
    auto anode = Factory::lookup_tn<IAnodePlane>(m_anode_tn);
    if (!anode) {
        cerr << "Gen::hyDrifter: failed to get anode: \"" << m_anode_tn << "\"\n";
        return;
    }
    if (anode->nfaces() <= 0) {
        cerr << "Gen::hyDrifter: anode " << m_anode_tn << " has no faces\n";
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
    cerr << "Gen::hyDrifter: configured with drift speed=" << m_speed/(units::mm/units::us)
         << "mm/us drifting to x=" << m_location/units::mm << "mm\n";
}

double Gen::hyDrifter::proper_time(IDepo::pointer depo)
{
    return depo->time() + depo->pos().x()/m_speed;
}

void Gen::hyDrifter::reset()
{
    m_input.clear();
}

// fixme: this needs to be moved into a random number generator
// provided through an interface.  For now, just use std.
int random_binomial(int range, double probability)
{
    std::default_random_engine generator; // fixme: this should be a shared, long-lived object....
    std::binomial_distribution<int> distribution(range, probability);
    return distribution(generator);
}


IDepo::pointer Gen::hyDrifter::transport(IDepo::pointer depo)
{
    if (!depo) {
        cerr << "Gen::hyDrifter error, asked to transport null depo\n";
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
    // ionization electrons without recombination
    const double Qi = depo->charge();
    // recombination probability of an electron
    // how to initialize m_recomb? from Deposition?
    double recombprob = m_recomb;
    // absorption probability of an electron
    double absorbprob = 1 - exp(-dt/m_lifetime); 
    // final number of electrons after drift
    double dQ = Qi * recombprob * absorbprob;
    // how many electrons remain, with fluctuation.
    if (m_fluctuate) {
        // fano factor
        // use a binomial distribution with np = Qi, 1-p = m_fano
        // this approximates a Gaussian distribution and has a physical limit >0
        double Qi2 = random_binomial((int)Qi/(1-m_fano), 1-m_fano);
        // binomial distribution
        dQ = random_binomial((int)Qi2, recombprob*absorbprob);
    }
    const double Qf = Qi - dQ;

    const double sigmaL = sqrt(2.0*m_DL*dt);
    const double sigmaT = sqrt(2.0*m_DT*dt);

    auto ret = make_shared<SimpleDepo>(then, there, Qf, depo, sigmaL, sigmaT);
    // cerr << "Gen::hyDrifter: "
    //      << "q:" << ret->charge()/units::eplus << " electrons "
    //      << "t:" << depo->time()/units::us << " -> " << ret->time()/units::us << " ("<<dt/units::us<<") us, "
    //      << "x:" << depo->pos().x()/units::mm << " -> " << ret->pos().x()/units::mm << "mm, "
    //      << "long=" << ret->extent_long()/units::mm << "mm, trans=" << ret->extent_tran()/units::mm << "mm "
    //      << endl;
    return ret;
}

bool Gen::hyDrifter::operator()(const input_pointer& depo, output_queue& outq)
{
    if (m_speed <= 0.0) {
        cerr << "Gen::hyDrifter: illegal drift speed\n";
        return false;
    }
    if (m_eos) {
	return false;
    }

    if (!depo) {		// EOS flush
        //cerr << "Gen::hyDrifter: flushing " << m_input.size() << "\n";
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

	if (bot->time() < tau) {
	    return true;       // bail when we reach unknown territory
	}

	m_input.erase(top);
        outq.push_back(transport(top));
    }
    //cerr << "Gen::hyDrifter: returning " << outq.size() << endl;
    return true;
}

