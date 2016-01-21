#include "WireCellGen/TrackDepos.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"

#include <iostream>		// debug

WIRECELL_FACTORY(TrackDepos, WireCell::TrackDepos, WireCell::IDepoSource);

using namespace std;
using namespace WireCell;

TrackDepos::TrackDepos(double stepsize, double clight)
    : m_stepsize(stepsize)
    , m_clight(clight)
    , m_eos(false)
{
}

TrackDepos::~TrackDepos()
{
}

Configuration TrackDepos::default_configuration() const
{
    std::string json = R"(
{
"step_size_mm":1.0,
"clight":1.0,
"tracks":[{
  "time_s":0.0,
  "dedx":-1.0,
  "ray_mm":[[0.0,0.0,0.0],[1.0,1.0,1.0]]
}]
}
)";
    return configuration_loads(json, "json");
}

void TrackDepos::configure(const Configuration& cfg)
{
    m_stepsize = get<double>(cfg, "step_size_mm", m_stepsize/units::mm)*units::mm;
    m_clight = get<double>(cfg, "clight", m_clight);
    for (auto track : cfg["tracks"]) {
	double time = get<double>(track, "time_s", 0.0);
	double dedx = get<double>(track, "dedx", -1.0);
	Ray rmm = get<Ray>(track, "ray_mm");
	Ray ray(rmm.first*units::millimeter, rmm.second*units::millimeter);
	add_track(time, ray, dedx);
    }
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
	const double now = time + step/(m_clight*units::clight);
	const WireCell::Point here = ray.first + dir * step;
	SimpleDepo* sdepo = new SimpleDepo(now, here, charge_per_depo);
	m_depos.push_back(WireCell::IDepo::pointer(sdepo));
	step += m_stepsize;
	++count;
    }
    // reverse sort by time so we can pop_back in operator()
    std::sort(m_depos.begin(), m_depos.end(), descending_time);
}


bool TrackDepos::operator()(output_pointer& out)
{
    if (m_eos) {
	return false;
    }

    if (m_depos.empty()) {
	m_eos = true;
	out = nullptr;
	return true;
    }

    out = m_depos.back();
    m_depos.pop_back();
    return true;
}

