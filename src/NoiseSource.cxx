#include "WireCellGen/NoiseSource.h"

#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(NoiseSource, WireCell::Gen::NoiseSource, WireCell::IFrameSource, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

Gen::NoiseSource::NoiseSource(const std::string& model, const std::string& anode, const std::string& rng)
    : m_time(0.0*units::ns)
    , m_readout(5.0*units::ms)
    , m_tick(0.5*units::us)
    , m_frame_count(0)
    , m_anode_tn(anode)
    , m_model_tn(model)
    , m_rng_tn(rng)
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
    cfg["rng"] = m_rng_tn;

    return cfg;
}

void Gen::NoiseSource::configure(const WireCell::Configuration& cfg)
{
    m_rng_tn = get(cfg, "rng", m_rng_tn);
    auto rng = Factory::lookup_tn<IRandom>(m_rng_tn);
    if (!rng) {
        cerr << "Gen::NoiseSource: failed to get IRandom: \"" << m_rng_tn << "\"\n";
        return;
    }
    m_gaus = rng->normal(0.0, 1.0);

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
  // In here use the noise model to:

  // 1) get the amplitude spectrum.  Be careful to hold as reference to avoid copy
  auto& spec = (*m_model)(channel_ident);
  
  //std::cout << spec.size() << " " << m_readout/m_tick << std::endl;
  // for (int i=0;i!=spec.size();i++){
  //   std::cout << i << " " << spec.at(i)/units::mV << std::endl;
  // }
  WireCell::Waveform::compseq_t noise_freq(spec.size(),0); 
  //std::cout << medians_freq.size() << std::endl;
  // 2) properly sample it
  for (unsigned int i=0;i<spec.size();i++){
    double amplitude = spec.at(i) * sqrt(2./3.1415926);// / units::mV;
    //std::cout << distribution(generator) * amplitude << " " << distribution(generator) * amplitude << std::endl;
    double real_part = m_gaus() * amplitude;
    double imag_part = m_gaus() * amplitude;
    // if (i<=spec.size()/2.){
    noise_freq.at(i).real(real_part);
    noise_freq.at(i).imag(imag_part);//= complex_t(real_part,imag_part);
    // }else{
    //   noise_freq.at(i) = std::conj(noise_freq.at(spec.size()-i));
    // }

    // std::cout << " " << noise_freq.at(i) << " " << real_part << " " << imag_part << std::endl;
  }
  
  
  
  // 3) convert back to time domain (use function "idft()")
  Waveform::realseq_t noise_time = WireCell::Waveform::idft(noise_freq);

  return noise_time;
  // a dummy for now    
  //return Waveform::realseq_t(m_readout/m_tick, 0.0*units::volt); 
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


