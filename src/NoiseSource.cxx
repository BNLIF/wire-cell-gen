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
    , m_stop(1.0*units::ms)
    , m_readout(5.0*units::ms)
    , m_tick(0.5*units::us)
    , m_frame_count(0)
    , m_anode_tn(anode)
    , m_model_tn(model)
    , m_rng_tn(rng)
    , m_rep_percent(0.02) // replace 2% at a time
    , m_eos(false)
{
  // initialize the random number ...
  //auto& spec = (*m_model)(0);
  
  // end initialization ..
  
}


Gen::NoiseSource::~NoiseSource()
{
}

WireCell::Configuration Gen::NoiseSource::default_configuration() const
{
    Configuration cfg;
    cfg["start_time"] = m_time;
    cfg["stop_time"] = m_stop;
    cfg["readout_time"] = m_readout;
    cfg["sample_period"] = m_tick;
    cfg["first_frame_number"] = m_frame_count;

    cfg["anode"] = m_anode_tn;
    cfg["model"] = m_model_tn;
    cfg["rng"] = m_rng_tn;
    cfg["replacement_percentage"] = m_rep_percent;
    return cfg;
}

void Gen::NoiseSource::configure(const WireCell::Configuration& cfg)
{
    m_rng_tn = get(cfg, "rng", m_rng_tn);
    m_rng = Factory::find_tn<IRandom>(m_rng_tn);
    if (!m_rng) {
        THROW(KeyError() << errmsg{"failed to get IRandom: " + m_rng_tn});
    }

    m_anode_tn = get(cfg, "anode", m_anode_tn);
    m_anode = Factory::find_tn<IAnodePlane>(m_anode_tn);
    if (!m_anode) {
        THROW(KeyError() << errmsg{"failed to get IAnodePlane: " + m_anode_tn});
    }

    m_model_tn = get(cfg, "model", m_model_tn);
    m_model = Factory::find_tn<IChannelSpectrum>(m_model_tn);
    if (!m_model) {
        THROW(KeyError() << errmsg{"failed to get IChannelSpectrum: " + m_model_tn});
    }

    m_readout = get<double>(cfg, "readout_time", m_readout);
    m_time = get<double>(cfg, "start_time", m_time);
    m_stop = get<double>(cfg, "stop_time", m_stop);
    m_tick = get<double>(cfg, "sample_period", m_tick);
    m_frame_count = get<int>(cfg, "first_frame_number", m_frame_count);
    m_rep_percent = get<double>(cfg,"replacement_percentage",m_rep_percent);
    
    cerr << "Gen::NoiseSource: using IRandom: \"" << m_rng_tn << "\""
         << " IAnodePlane: \"" << m_anode_tn << "\""
         << " IChannelSpectrum: \"" << m_model_tn << "\""
         << " readout time: " << m_readout/units::us << "us\n";


    
}


Waveform::realseq_t Gen::NoiseSource::waveform(int channel_ident)
{
  // In here use the noise model to:
  
  
  // 1) get the amplitude spectrum.  Be careful to hold as reference to avoid copy
  auto& spec = (*m_model)(channel_ident);

  if (random_real_part.size()!=spec.size()){
    random_real_part.resize(spec.size(),0);
    random_imag_part.resize(spec.size(),0);
    for (unsigned int i=0;i<spec.size();i++){
      random_real_part.at(i) = m_rng->normal(0,1);
      random_imag_part.at(i) = m_rng->normal(0,1);
    }
  }

  int shift = m_rng->uniform(0,random_real_part.size());
  int shift1 = m_rng->uniform(0,random_real_part.size());
  // replace certain percentage of the random number
  int step = 1./ m_rep_percent;
  for (int i =shift1; i<shift1 + spec.size(); i+=step){
    if (i<spec.size()){
      random_real_part.at(i) = m_rng->normal(0,1);
      random_imag_part.at(i) = m_rng->normal(0,1);
    }else{
      random_real_part.at(i-spec.size()) = m_rng->normal(0,1);
      random_imag_part.at(i-spec.size()) = m_rng->normal(0,1);
    }
  }
  //std::cout << step << " " << shift << " " << shift1 << std::endl;
  
  //std::cout << spec.size() << " " << m_readout/m_tick << std::endl;
  // for (int i=0;i!=spec.size();i++){
  //   std::cout << i << " " << spec.at(i)/units::mV << std::endl;
  // }
  WireCell::Waveform::compseq_t noise_freq(spec.size(),0); 
  //std::cout << medians_freq.size() << std::endl;
  // 2) properly sample it
  
  
  for (unsigned int i=shift;i<spec.size();i++){
    // int count = i + shift;
    // if (count >= 2 * random_real_part.size()){
    //   count -= 2 * random_real_part.size();
    // }else if (count >= random_real_part.size()){
    //   count -= random_real_part.size();
    // }
    double amplitude = spec.at(i-shift) * sqrt(2./3.1415926);// / units::mV;
    //std::cout << distribution(generator) * amplitude << " " << distribution(generator) * amplitude << std::endl;
    //double real_part = random_real_part.at(count) * amplitude;
    //double imag_part = random_imag_part.at(count) * amplitude;
    // if (i<=spec.size()/2.){
    noise_freq.at(i-shift).real(random_real_part.at(i) * amplitude);
    noise_freq.at(i-shift).imag(random_imag_part.at(i) * amplitude);//= complex_t(real_part,imag_part);
    // }else{
    //   noise_freq.at(i) = std::conj(noise_freq.at(spec.size()-i));
    // }
    // std::cout << " " << noise_freq.at(i) << " " << real_part << " " << imag_part << std::endl;
  }
  for (unsigned int i=0;i<shift;i++){
    double amplitude = spec.at(i+spec.size()-shift) * sqrt(2./3.1415926);
    noise_freq.at(i+spec.size()-shift).real(random_real_part.at(i) * amplitude);
    noise_freq.at(i+spec.size()-shift).imag(random_imag_part.at(i) * amplitude);
  }
  
  
  
  // 3) convert back to time domain (use function "idft()")
  Waveform::realseq_t noise_time = WireCell::Waveform::idft(noise_freq);

  return noise_time;
  // a dummy for now    
  //return Waveform::realseq_t(m_readout/m_tick, 0.0*units::volt); 
}

bool Gen::NoiseSource::operator()(IFrame::pointer& frame)
{
    if (m_eos) {                // This source does not restart.
        return false;
    }

    if (m_time >= m_stop) {
        frame = nullptr;
        m_eos = true;
        return true;
    }
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


