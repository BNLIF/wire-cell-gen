#include "WireCellGen/Drifter.h"
#include "TransportedDepo.h"

#include <boost/range.hpp>

#include <sstream>
#include <iostream>

using namespace std;
using namespace WireCell;


Drifter::Drifter(double location,
		 double drift_velocity)
    : m_location(location)
    , m_drift_velocity(drift_velocity)
    , m_input(IDepoDriftCompare(drift_velocity))
{
}

double Drifter::proper_time(IDepo::pointer depo)
{
    return depo->time() + depo->pos().x()/m_drift_velocity;
}

void Drifter::reset()
{
    m_input.clear();
    m_output.clear();
}

void Drifter::flush()
{
    while (!m_input.empty()) {	
	IDepo::pointer top = *m_input.begin();
	m_input.erase(top);
	IDepo::pointer ret(new TransportedDepo(top, m_location, m_drift_velocity));
	m_output.push_back(ret);
    }
}

bool Drifter::insert(const input_type& depo)
{
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
	m_output.push_back(ret);
    }
    return true;
}

bool Drifter::extract(output_type& depo)
{
    if (m_output.empty()) {
	return false;
    }
    depo = m_output.front();
    m_output.pop_front();
    return true;
}
