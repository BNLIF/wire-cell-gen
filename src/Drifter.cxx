#include "WireCellGen/Drifter.h"
#include "WireCellUtil/NamedFactory.h"
#include "TransportedDepo.h"

#include <boost/range.hpp>

#include <sstream>
#include <iostream>

WIRECELL_FACTORY(Drifter, WireCell::Drifter, WireCell::IDrifter, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;



Drifter::Drifter(double location,
		 double drift_velocity)
    : m_location(location)
    , m_drift_velocity(drift_velocity)
    , m_input(IDepoDriftCompare(drift_velocity))
    , m_eos(false)
{
}

void Drifter::configure(const WireCell::Configuration& cfg)
{
    m_location = get<double>(cfg, "location_mm", m_location/units::mm)*units::mm;
    const double mmus = units::mm/units::microsecond;
    m_drift_velocity = get<double>(cfg, "drift_velocity_mm/us", m_drift_velocity/mmus)*mmus;
}

WireCell::Configuration Drifter::default_configuration() const
{
    std::string json = R"(
{
"location_mm":0.0,
"drift_velocity_mm/us":1.6
}
)";
    return configuration_loads(json, "json");
}


double Drifter::proper_time(IDepo::pointer depo)
{
    return depo->time() + depo->pos().x()/m_drift_velocity;
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
	    IDepo::pointer ret(new TransportedDepo(top, m_location, m_drift_velocity));
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

	IDepo::pointer ret(new TransportedDepo(top, m_location, m_drift_velocity));
	m_input.erase(top);
	outq.push_back(ret);
    }
    return true;
}

