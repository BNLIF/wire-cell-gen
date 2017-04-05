#include "WireCellGen/Drifter.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellGen/TransportedDepo.h"

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IAnodeFace.h"
#include "WireCellIface/IWirePlane.h"

#include <boost/range.hpp>

#include <sstream>
#include <iostream>

WIRECELL_FACTORY(Drifter, WireCell::Drifter, WireCell::IDrifter, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;



Drifter::Drifter(const std::string& anode_tn)
    : m_location(0)
    , m_speed(0)
    , m_input()
    , m_eos(false)
{
    this->set_anode(anode_tn);
}

void Drifter::set_anode(const std::string& anode_tn)
{
    if (anode_tn.empty()) { return; }

    auto anode = Factory::lookup<IAnodePlane>(anode_tn);
    if (!anode) {
        cerr << "Drifter: failed to get anode '" << anode_tn << "'\n";
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
}

void Drifter::configure(const WireCell::Configuration& cfg)
{
    std::string anode_tn = get<string>(cfg, "anode", "");
    this->set_anode(anode_tn);
}

WireCell::Configuration Drifter::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = "";
    return cfg;
}


double Drifter::proper_time(IDepo::pointer depo)
{
    return depo->time() + depo->pos().x()/m_speed;
}

void Drifter::reset()
{
    m_input.clear();
}

bool Drifter::operator()(const input_pointer& depo, output_queue& outq)
{
    if (m_eos) {
	return false;
    }

    if (!depo) {		// EOS flush
	while (!m_input.empty()) {	
	    IDepo::pointer top = *m_input.begin();
	    m_input.erase(top);
	    auto ret = std::make_shared<Gen::TransportedDepo>(top, m_location, m_speed);
	    outq.push_back(ret);
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

	auto ret = std::make_shared<Gen::TransportedDepo>(top, m_location, m_speed);
	m_input.erase(top);
	outq.push_back(ret);
    }
    return true;
}

