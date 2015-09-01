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

double Drifter::proper_time(IDepo::pointer depo) {
    return depo->time() + depo->pos().x()/m_drift_velocity;
}

bool Drifter::sink(const IDepo::pointer& depo)
{
    m_input.insert(depo);
    return true;		// we always accept
}

bool Drifter::process()
{
    while (!m_input.empty()) {
	IDepo::pointer top = *m_input.begin();
	double tau = proper_time(top);

	IDepo::pointer bot = *m_input.rbegin();
	if (bot->time() < tau) { // haven't yet buffered enough to
	    break;		 // guarantee proper output ordering.
	}
	IDepo::pointer ret(new TransportedDepo(top, m_location,
					       m_drift_velocity));
	m_input.erase(top);
	m_output.push_back(ret);
    }
    return false;		// we always exhaust all we can. 
}

bool Drifter::source(IDepo::pointer& depo)
{
    if (m_output.empty()) {
	return false;		// no blood from a stone
    }
    depo = m_output.front();
    m_output.pop_front();
    return true;
}





