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
    , m_eoi(false)
    , m_input(IDepoDriftCompare(drift_velocity))
{
}

double Drifter::proper_time(IDepo::pointer depo) {
    return depo->time() + depo->pos().x()/m_drift_velocity;
}

bool Drifter::sink(const IDepo::pointer& depo)
{
    if (depo) {
	m_input.insert(depo);
    }
    else {
	cerr << "Drifter: got end of input" << endl;
	m_eoi = true;
    }

    return true;		// we always accept
}

bool Drifter::process()
{
    while (!m_input.empty()) {
	IDepo::pointer top = *m_input.begin();
	double tau = proper_time(top);

	IDepo::pointer bot = *m_input.rbegin();
	if (!m_eoi && bot->time() < tau) {
	    cerr << "Drifter: tau=: " << tau << " time=" << bot->time()
		 << " #in=" << m_input.size() << " #out=" << m_output.size()
		 << endl;
	    break;		 // guarantee proper output ordering.
	}
	IDepo::pointer ret(new TransportedDepo(top, m_location,
					       m_drift_velocity));
	m_input.erase(top);
	m_output.push_back(ret);
	cerr << "Drifter: saving: " << ret->time() << " " << ret->pos() << endl;
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





