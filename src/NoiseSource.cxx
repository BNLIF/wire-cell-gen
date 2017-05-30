#include "WireCellGen/NoiseSource.h"

#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(NoiseSource, WireCell::Gen::NoiseSource, WireCell::IFrameSource, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

Gen::NoiseSource::NoiseSource(const std::string& model, const std::string& anode)
    : m_time(0.0*units::ns)
    , m_readout(5.0*units::ms)
    , m_tick(0.5*units::us)
    , m_frame_count(0)
    , m_anode_tn(anode)
    , m_model_tn(model)
{
}


Gen::NoiseSource::~NoiseSource()
{
}

WireCell::Configuration Gen::NoiseSource::default_configuration() const
{
    Configuration cfg;
    cfg["start_time"] = m_time;
    cfg["readout_time"] = m_readout;
    cfg["sample_period"] = m_tick;
    cfg["first_frame_number"] = m_frame_count;

    cfg["anode"] = m_anode_tn;
    cfg["model"] = m_model_tn;

    return cfg;
}

void Gen::NoiseSource::configure(const WireCell::Configuration& cfg)
{
    m_anode_tn = get(cfg, "anode", m_anode_tn);
    m_anode = Factory::lookup_tn<IAnodePlane>(m_anode_tn);
    if (!m_anode) {
        cerr << "Gen::NoiseSource: failed to get anode: \"" << m_anode_tn << "\"\n";
        return;
    }
    m_model_tn = get(cfg, "model", m_model_tn);
    m_model = Factory::lookup_tn<IChannelSpectrum>(m_model_tn);

    m_readout = get<double>(cfg, "readout_time", m_readout);
    m_time = get<double>(cfg, "start_time", m_time);
    m_tick = get<double>(cfg, "sample_period", m_tick);
    m_frame_count = get<int>(cfg, "first_frame_number", m_frame_count);
}


Waveform::realseq_t Gen::NoiseSource::waveform(int channel_ident)
{
    // fixme/todo:
    // In here use the noise model to:

    // 1) get the amplitude spectrum.  Be careful to hold as reference to avoid copy

    // auto& spec = (*m_model)(channel_ident);

    // 2) properly sample it
    // 3) convert back to time domain (use function "idft()")

    // a dummy for now    
    return Waveform::realseq_t(m_readout/m_tick, 0.0*units::volt); 
}

bool Gen::NoiseSource::operator()(IFrame::pointer& frame)
{
    ITrace::vector traces;
    const int tbin = 0;
    int nsamples = 0;
    for (auto chid : m_anode->channels()) {
        Waveform::realseq_t noise = waveform(chid);
        auto trace = make_shared<SimpleTrace>(chid, tbin, noise);
        traces.push_back(trace);
        nsamples += noise.size();
    }
    cerr << "Gen::NoiseSource: made " << traces.size() << " traces, "
         << nsamples << " samples\n";
    frame = make_shared<SimpleFrame>(m_frame_count, m_time, traces, m_tick);
    m_time += m_readout;
    ++m_frame_count;
    return true;
}


