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
    , m_buffer(IDepoDriftCompare(drift_velocity))
{
}

double Drifter::proper_time(IDepo::pointer depo) {
    return depo->time() + depo->pos().x()/m_drift_velocity;
}

bool Drifter::buffer()
{
    if (!m_buffer.size()) {	// empty buffer, prime.
	IDepo::pointer depo = *m_input();
	if (!depo) {
	    return false;	// no buffer, no input, done.
	}
	m_buffer.insert(depo);
    }

    while (true) {
	double tau = proper_time(top());
	IDepo::pointer latest = bot();
	if (latest->time() >= tau) {
	    return true;
	}
	latest = *m_input();
	if (!latest) {
	    break;
	}
	m_buffer.insert(latest);
    }

    return m_buffer.size() > 0;
}

// return value and increment
IDepo::pointer Drifter::operator()()
{
    if ( !buffer() ) {
	return 0;
    }
    IDepo::pointer before = pop();
    IDepo::pointer ret(new TransportedDepo(before, m_location, m_drift_velocity));
    //cerr << "Drifter: buffered: " << m_buffer.size() << endl;
    return ret;
}

IDepo::pointer Drifter::pop()
{
    IDepo::pointer ret = top();
    m_buffer.erase(ret);
    return ret;
}

IDepo::pointer Drifter::top()
{
    return *m_buffer.begin();
}
IDepo::pointer Drifter::bot()
{
    return *m_buffer.rbegin();
}


