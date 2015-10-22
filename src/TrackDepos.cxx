#include "WireCellGen/TrackDepos.h"
#include "WireCellIface/SimpleDepo.h"

#include "WireCellUtil/Units.h"

#include <iostream>		// debug
using namespace std;
using namespace WireCell;
    
TrackDepos::TrackDepos(double stepsize, double clight)
    : m_stepsize(stepsize)
    , m_clight(clight)
{
}

TrackDepos::~TrackDepos()
{
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
	SimpleDepo* sdepo = new SimpleDepo(now, here, charge_per_depo);
	m_depos.push_back(WireCell::IDepo::pointer(sdepo));
	step += m_stepsize;
	++count;
    }
    // reverse sort by time so we can pop_back in operator()
    std::sort(m_depos.begin(), m_depos.end(), descending_time);
}

WireCell::IDepo::pointer TrackDepos::operator()()
{
    // fixme: should only return eos once, and then start failing.
    if (!m_depos.size()) { return nullptr; }

    WireCell::IDepo::pointer p = m_depos.back();
    m_depos.pop_back();
    return p;
}

bool TrackDepos::extract(output_pointer& out)
{
    out = (*this)();
    return true;
}


// std::shared_ptr<WireCell::IDepo::vector> TrackDepos::depositions()
// {
//     return m_depos;
// }

