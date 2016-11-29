#include "WireCellGen/Transport.h"
#include "WireCellGen/TransportedDepo.h"

using namespace WireCell;

Gen::DepoPlaneX::DepoPlaneX(double planex, double speed)
    : m_planex(planex)
    , m_speed(speed)
    , m_queue(IDepoDriftCompare(speed))
{
}


IDepo::pointer Gen::DepoPlaneX::add(const IDepo::pointer& depo)
{
    drain(depo->time());
    IDepo::pointer newdepo(new TransportedDepo(depo, m_planex, m_speed));
    m_queue.insert(newdepo);
    return newdepo;
}

double Gen::DepoPlaneX::freezeout_time() const
{
    if (m_frozen.empty()) {
	return -1.0*units::second;
    }
    return m_frozen.back()->time();
}

void Gen::DepoPlaneX::drain(double time)
{
    for (auto depo : m_queue) {
	if (depo->time() < time) {
	    m_frozen.push_back(depo);
	    m_queue.erase(depo);
	}
    }
}

void Gen::DepoPlaneX::freezeout()
{
    for (auto depo : m_queue) {
	m_frozen.push_back(depo);
	m_queue.erase(depo);
    }
}
