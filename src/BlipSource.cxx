#include "WireCellGen/BlipSource.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(BlipSource, WireCell::Gen::BlipSource, WireCell::IDepoSource, WireCell::IConfigurable);

using namespace WireCell;

Gen::BlipSource::BlipSource()
    : m_rng_tn("Random")
    , m_time(0.0)
    , m_ene(nullptr)
    , m_tim(nullptr)
    , m_pos(nullptr)
{
}

Gen::BlipSource::~BlipSource()
{
    delete m_ene; m_ene = nullptr;
    delete m_tim; m_tim = nullptr;
    delete m_pos; m_pos = nullptr;
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
    tail["x"] = -1*units::m; tail["y"] = -1*units::m; tail["z"] = -1*units::m;
    head["x"] = +1*units::m; head["y"] = +1*units::m; head["z"] = +1*units::m;
    extent["tail"] = tail; extent["head"] = head;
    pos["extent"] = extent;

    cfg["position"] = pos;

    return cfg;
	
}

struct ReturnValue : public Gen::BlipSource::ScalarMaker {
    double value;
    ReturnValue(double val) : value(val) {}
    ~ReturnValue() {}
    double operator()() { return value; }
};
struct DecayTime : public Gen::BlipSource::ScalarMaker {
    IRandom::pointer rng;
    double rate;
    DecayTime(IRandom::pointer rng, double rate) : rng(rng), rate(rate) {}
    ~DecayTime() {}
    double operator()() { return rng->exponential(rate); }
};
struct UniformBox : public Gen::BlipSource::PointMaker {
    IRandom::pointer rng;
    Ray extent;
    UniformBox(IRandom::pointer rng, const Ray& extent) : rng(rng), extent(extent) {}
    ~UniformBox() {}
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
    else {
	std::cerr <<"BlipSource: no charge configuration\n";
    }

    auto tim = cfg["time"];
    m_time = tim["start"].asDouble();
    if (tim["type"].asString() == "decay") {
	m_tim = new DecayTime(m_rng, tim["activity"].asDouble());
    }
    else {
	std::cerr <<"BlipSource: no time configuration\n";
    }

    auto pos = cfg["position"];
    if (pos["type"].asString() == "box") {
	Ray box = WireCell::convert<Ray>(pos["extent"]);
	m_pos = new UniformBox(m_rng, box);
    }
    else {
	std::cerr <<"BlipSource: no position configuration\n";
    }
}

bool Gen::BlipSource::operator()(IDepo::pointer& depo)
{
    m_time += (*m_tim)();
    depo = std::make_shared<SimpleDepo>(m_time, (*m_pos)(), (*m_ene)());
    return true;
}
