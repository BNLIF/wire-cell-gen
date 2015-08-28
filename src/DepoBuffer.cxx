#include "WireCellGen/DepoBuffer.h"

#include <iostream>
using namespace std;

using namespace WireCell;

DepoBuffer::DepoBuffer(double window, double now)
    : m_window(window), m_now(now) {}
    
DepoBuffer::~DepoBuffer()
{
}

IDepoVector DepoBuffer::advance(double deltat)
{
    IDepoVector ret;
    if (deltat <= 0) { return ret; }
    m_now += deltat;
    double tmin = m_now - 0.5*m_window;
    double tmax = m_now + 0.5*m_window;

    // push new
    while (m_depos.empty() || m_depos.back()->time() < tmax) {
	IDepo::pointer depo = *m_input();
	if (!depo) { break; }
	cerr << "Adding " << depo->time() << " in [" << tmin << "," << tmax << "]"<< endl;
	add(depo);
    }

    // pop old
    while (m_depos.size() && m_depos.front()->time() < tmin) {
	ret.push_back(m_depos.front());
	m_depos.pop_front();
    }

    return ret;
}

void DepoBuffer::add(IDepo::pointer depo)
{
    m_depos.push_back(depo);
}

DepoBuffer::iterator_range DepoBuffer::operator()()
{
    return DepoBuffer::iterator_range(m_depos.begin(), m_depos.end());
}
