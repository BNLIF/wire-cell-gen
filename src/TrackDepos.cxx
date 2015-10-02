#include "WireCellGen/TrackDepos.h"
#include "WireCellIface/SimpleDepo.h"

#include "WireCellUtil/Units.h"

#include <iostream>		// debug
using namespace std;
using namespace WireCell;
    
TrackDepos::TrackDepos(double stepsize, double clight)
    : m_stepsize(stepsize)
    , m_clight(clight)
    , m_depos(new WireCell::IDepoVector)
{
    // hold on to the IDepoVector by shared pointer so that we can be
    // easily copied as we are wont to do as a signal slot.
}

void TrackDepos::add_track(double time, const WireCell::Ray& ray, double dedx)
{
    cerr << "TrackDepos.add_track(" << time / units::microsecond << "us "
	 << ray.first << " -->  " << ray.second << ")" << endl;

    const WireCell::Vector dir = WireCell::ray_unit(ray);
    const double length = WireCell::ray_length(ray);
    double step = 0;
    int count = 0;

    double charge_per_depo = 1.0;
    if (dedx > 0) {
	charge_per_depo = dedx / (length / m_stepsize);
    }
    else if (dedx < 0) {
	charge_per_depo = -dedx;
    }

    while (step < length) {
	const double now = time + step/m_clight;
	const WireCell::Point here = ray.first + dir * step;
	m_depos->push_back(WireCell::IDepo::pointer(new SimpleDepo(now, here, charge_per_depo)));
	step += m_stepsize;
	++count;
    }
    // reverse sort by time so we can pop_back in operator()
    std::sort(m_depos->begin(), m_depos->end(), descending_time);
}

WireCell::IDepo::pointer TrackDepos::operator()()
{
    if (!m_depos->size()) { return nullptr; }
    WireCell::IDepo::pointer p = m_depos->back();
    m_depos->pop_back();
    return p;
}

std::shared_ptr<WireCell::IDepoVector> TrackDepos::depositions()
{
    return m_depos;
}

