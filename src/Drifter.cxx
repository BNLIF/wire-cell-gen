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
    if (m_eoi) {	   // once we hit EOI, there's no turning back
	return false;
    }

    if (depo) {
	m_input.insert(depo);
    }
    else {// can't just save the EOI nullptr into the input buffer due
	  // to sorting.
	m_eoi = true;
	process();
	m_output.push_back(nullptr);

	cerr << "Drifter: got end of input with output size "
	     << m_output.size()
	     << endl;
    }

    return true;		// otherwise, we always accept
}

void Drifter::process()
{
    // drain input
    while (!m_input.empty()) {	
	IDepo::pointer top = *m_input.begin();
	double tau = proper_time(top);

	IDepo::pointer bot = *m_input.rbegin();

	// Bail out if we have exhausted the available buffer, unless
	// we have it EOI in which case we should drain the remaining
	// buffer.
	if (!m_eoi && bot->time() < tau) {
	    // cerr << "Drifter: tau= " << tau << " time=" << bot->time()
	    // 	 << " #in=" << m_input.size() << " #out=" << m_output.size()
	    // 	 << endl;
	    return;
	}
	IDepo::pointer ret(new TransportedDepo(top, m_location,
					       m_drift_velocity));
	m_input.erase(top);
	// cerr << "Drifter: saving #"<<m_output.size()
	//      << " @ " << ret->time() << " " << ret->pos() << endl;
	m_output.push_back(ret);
    }
    return;
}

bool Drifter::source(IDepo::pointer& depo)
{
    process();

    if (m_output.empty()) {
	//cerr << "Drifter: now empy " << endl;
	return false;		// no blood from a stone
    }
    depo = m_output.front();
    m_output.pop_front();
    //cerr << "Drifter: sourcing #" << m_output.size() << endl;
    //cerr << "front: " << m_output.front() << endl;
    return true;
}





