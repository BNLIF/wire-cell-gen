#include "WireCellGen/Fourdee.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/ConfigManager.h"


WIRECELL_FACTORY(FourDee, WireCell::Gen::Fourdee, WireCell::IApplication, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;


Gen::Fourdee::Fourdee()
{
}

Gen::Fourdee::~Fourdee()
{
}

WireCell::Configuration Gen::Fourdee::default_configuration() const
{
    Configuration cfg;                    // todo:
    put(cfg, "DepoSource", "TrackDepos"); // have
    put(cfg, "Drifter", "Drifter");       // have, needs work
    put(cfg, "Ductor", "Ductor");         // need
    put(cfg, "Dissonance", "SilentNoise"); // needed
    put(cfg, "Digitizer", "Digitizer");    // need to rework frame
    put(cfg, "FrameSink", "DumpFrames"); // needed

    return cfg;
}


void Gen::Fourdee::configure(const Configuration& thecfg)
{
    Configuration cfg = thecfg;

    m_depos = Factory::lookup<IDepoSource>(get<string>(cfg, "DepoSource"));
    m_drifter = Factory::lookup<IDrifter>(get<string>(cfg, "Drifter",""));
    m_ductor = Factory::lookup<IDuctor>(get<string>(cfg, "Ductor",""));
    m_dissonance = Factory::lookup<IFrameSource>(get<string>(cfg, "Dissonance",""));
    m_digitizer = Factory::lookup<IFrameFilter>(get<string>(cfg, "Digitizer",""));
    m_output = Factory::lookup<IFrameSink>(get<string>(cfg, "FrameSink",""));
    

    // fixme: check for failures
}


void Gen::Fourdee::execute()
{
    // implement flow control through the pipeline.

}

