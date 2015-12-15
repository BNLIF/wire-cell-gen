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
    , m_eos(false)
{
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

