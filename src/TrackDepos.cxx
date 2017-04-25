#include "WireCellGen/TrackDepos.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Persist.h"

#include <iostream>		// debug
#include <sstream>

WIRECELL_FACTORY(TrackDepos, WireCell::Gen::TrackDepos, WireCell::IDepoSource, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

Gen::TrackDepos::TrackDepos(double stepsize, double clight)
    : m_stepsize(stepsize)
    , m_clight(clight)
    , m_eos(false)
{
}

Gen::TrackDepos::~TrackDepos()
{
}

Configuration Gen::TrackDepos::default_configuration() const
{
    std::string json = R"(
{
"step_size":0.1,
"clight":1.0,
"tracks":[]
}
)";
    return Persist::loads(json);
}

void Gen::TrackDepos::configure(const Configuration& cfg)
{
    m_stepsize = get<double>(cfg, "step_size", m_stepsize);
    Assert(m_stepsize > 0);
    m_clight = get<double>(cfg, "clight", m_clight);
    for (auto track : cfg["tracks"]) {
	double time = get<double>(track, "time", 0.0);
	double charge = get<double>(track, "charge", -1.0);
	Ray ray = get<Ray>(track, "ray");
	add_track(time, ray, charge);
    }
}

static std::string dump(IDepo::pointer d)
{
    std::stringstream ss;
    ss << "q=" << d->charge()/units::eplus << "eles, t=" << d->time()/units::us << "us, r=" << d->pos()/units::mm << "mm";
    return ss.str();
}

void Gen::TrackDepos::add_track(double time, const WireCell::Ray& ray, double charge)
{
    // cerr << "Gen::TrackDepos::add_track(" << time << ", (" << ray.first << " -> " << ray.second << "), " << charge << ")\n";
    m_tracks.push_back(track_t(time, ray, charge));

    const WireCell::Vector dir = WireCell::ray_unit(ray);
    const double length = WireCell::ray_length(ray);
    double step = 0;
    int count = 0;

    double charge_per_depo = units::eplus; // charge of one positron
    if (charge > 0) {
	charge_per_depo = charge / (length / m_stepsize);
    }
    else if (charge < 0) {
	charge_per_depo = -charge;
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
    // cerr << "Gen::TrackDepos: " << m_depos.size() << " depos over " << length/units::mm << "mm\n";
    // cerr << "\treverse first:" << dump(m_depos[0]) << endl;
    // cerr << "\t reverse last:" << dump(m_depos[m_depos.size()-1]) << endl;
}


bool Gen::TrackDepos::operator()(output_pointer& out)
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
    //cerr << "Gen::TrackDepos: " << m_depos.size() << " left, sending: " << dump(out) << endl;
    return true;
}

