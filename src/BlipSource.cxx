#include "WireCellGen/BlipSource.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(BlipSource, WireCell::Gen::BlipSource, WireCell::IDepoSource, WireCell::IConfigurable);

using namespace WireCell;

Gen::BlipSource::BlipSource()
    : m_rng_tn("Random")
{
}

Gen::BlipSource::~BlipSource()
{
}



WireCell::Configuration Gen::BlipSource::default_configuration() const
{
    Configuration cfg;
    cfg["rng"] = "Random";

    Configuration ene;
    ene["type"] = "mono";	// future: spectrum, uniform, Ar39 
    ene["value"] = 20000.0;
    cfg["charge"] = ene;

    Configuration tim;
    tim["type"] = "decay";
    tim["start"] = 0.0;
    tim["activity"] = 100000*units::Bq; // Ar39 in 2 DUNE drift volumes
    cfg["time"] = tim;

    Configuration pos;
    pos["type"] = "box";
    Configuration tail, head, extent;
    tail[0] = -1*units::m; tail[1] = -1*units::m; tail[2] = -1*units::m;
    head[0] = +1*units::m; head[1] = +1*units::m; head[2] = +1*units::m;
    extent["tail"] = tail; extent["head"] = head;
    pos["extent"] = extent;

    cfg["position"] = pos;

    return cfg;
	
}

struct ReturnValue : public Gen::BlipSource::ScalarMaker {
    double value;
    ReturnValue(double val) : value(val) {}
    double operator()() { return value; }
};
struct DecayTime : public Gen::BlipSource::ScalarMaker {
    IRandom::pointer rng;
    double rate;
    DecayTime(IRandom::pointer rng, double rate) : rng(rng), rate(rate) {}
    double operator()() { return rng->exponential(rate); }
};
struct UniformBox : public Gen::BlipSource::PointMaker {
    IRandom::pointer rng;
    Ray extent;
    UniformBox(IRandom::pointer rng, const Ray& extent) : rng(rng), extent(extent) {}
    Point operator()() {
	return Point(
	    rng->uniform(extent.first.x(), extent.second.x()),
	    rng->uniform(extent.first.y(), extent.second.y()),
	    rng->uniform(extent.first.z(), extent.second.z()));
    }
};
    

void Gen::BlipSource::configure(const WireCell::Configuration& cfg)
{
    m_rng_tn = get(cfg, "rng", m_rng_tn);
    m_rng = Factory::find_tn<IRandom>(m_rng_tn);

    auto ene = cfg["charge"];
    if (ene["type"].asString() == "mono") {
	m_ene = new ReturnValue(ene["value"].asDouble());
    }

    auto tim = cfg["time"];
    m_time = tim["start"].asDouble();
    if (tim["type"].asString() == "decay") {
	m_tim = new DecayTime(m_rng, tim["activity"].asDouble());
    }

    auto pos = cfg["position"];
    if (pos["type"].asString() == "box") {
	Ray box = get<Ray>(pos, "extent");
	m_pos = new UniformBox(m_rng, box);
    }
}

bool Gen::BlipSource::operator()(IDepo::pointer& depo)
{
    m_time += (*m_tim)();
    depo = std::make_shared<SimpleDepo>(m_time, (*m_pos)(), (*m_ene)());
    return true;
}
