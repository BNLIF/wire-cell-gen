
#include "WireCellGen/EmpiricalNoiseModel.h"


#include "WireCellUtil/Configuration.h"
#include "WireCellUtil/Persist.h"

#include "WireCellUtil/NamedFactory.h"

#include <iostream>             // debug

WIRECELL_FACTORY(EmpiricalNoiseModel, WireCell::Gen::EmpiricalNoiseModel,
                 WireCell::IChannelSpectrum, WireCell::IConfigurable);

 
using namespace WireCell;

Gen::EmpiricalNoiseModel::EmpiricalNoiseModel(const std::string& spectra_file,
                                              const int nsamples,
                                              const double period,
                                              const double wire_length_scale,
					      // const double time_scale,
					      // const double gain_scale,
					      // const double freq_scale,
                                              const std::string anode_tn)
    : m_spectra_file(spectra_file)
    , m_nsamples(nsamples)
    , m_period(period)
    , m_wlres(wire_length_scale)
    // , m_tres(time_scale)
    // , m_gres(gain_scale)
    // , m_fres(freq_scale)
    , m_anode_tn(anode_tn)
    , m_amp_cache(4)
{
}


Gen::EmpiricalNoiseModel::~EmpiricalNoiseModel()
{
}

void Gen::EmpiricalNoiseModel::gen_elec_resp_default(){
  
  double shaping[5]={1,1.1,2,2.2,3};
  for (int i=0;i!=5;i++){
    Response::ColdElec elec_resp(1, shaping[i]);
    auto sig   =   elec_resp.generate(WireCell::Waveform::Domain(0, m_nsamples*m_period), m_nsamples);
    
  }
  //auto to_sig   =   to_ce.generate(WireCell::Waveform::Domain(0, m_nsamples*m_tick), m_nsamples);
}

WireCell::Configuration Gen::EmpiricalNoiseModel::default_configuration() const
{
    Configuration cfg;

    /// The file holding the spectral data
    cfg["spectra_file"] = m_spectra_file;
    cfg["nsamples"] = m_nsamples; // number of samples up to Nyquist frequency
    cfg["period"] = m_period; 
    cfg["wire_length_scale"] = m_wlres;    // 
    // cfg["time_scale"] = m_tres;
    // cfg["gain_scale"] = m_gres;
    // cfg["freq_scale"] = m_fres;
    cfg["anode"] = m_anode_tn;            // name of IAnodePlane component

    return cfg;
}

void Gen::EmpiricalNoiseModel::resample(NoiseSpectrum& spectrum) const
{
  // std::cout << spectrum.nsamples << " " << m_nsamples << " " << spectrum.period << " " <<
  // m_period << std::endl;

    if (spectrum.nsamples == m_nsamples && spectrum.period == m_period) {
        return;
    }
    
    double scale = sqrt(m_nsamples/(spectrum.nsamples*1.0) * spectrum.period / (m_period*1.0));

    spectrum.constant *= scale;

    for (int ind = 0; ind!=spectrum.amps.size(); ind++){
      spectrum.amps[ind] *= scale;
    }

    
    //std::cout << spectrum.constant << " " << spectrum.amps[0] << std::endl;

    

    return;
}

void Gen::EmpiricalNoiseModel::configure(const WireCell::Configuration& cfg)
{
    m_anode_tn = get(cfg, "anode", m_anode_tn);
    m_anode = Factory::lookup_tn<IAnodePlane>(m_anode_tn);

    m_spectra_file = get(cfg, "spectra_file", m_spectra_file);
    m_nsamples = get(cfg, "nsamples", m_nsamples);
    m_period = get(cfg, "period", m_period);
    m_wlres = get(cfg, "wire_length_scale", m_wlres);
    // m_tres = get(cfg, "time_scale", m_tres);
    // m_gres = get(cfg, "gain_scale", m_gres);
    // m_fres = get(cfg, "freq_scale", m_fres);

    // Load the data file assuming.  The file should be a list of
    // dictionaries matching the
    // wirecell.sigproc.noise.schema.NoiseSpectrum class.  Fixme:
    // should break out this code separate.
    auto jdat = Persist::load(m_spectra_file);
    const int nentries = jdat.size();
    m_spectral_data.clear();
    for (int ientry=0; ientry<nentries; ++ientry) {
        auto jentry = jdat[ientry];
        NoiseSpectrum *nsptr = new NoiseSpectrum();

        nsptr->plane = jentry["plane"].asInt();
        nsptr->nsamples = jentry["nsamples"].asInt();
        nsptr->period = jentry["period"].asFloat();// * m_tres;  
        nsptr->gain = jentry["gain"].asFloat();// *m_gres;    
        nsptr->shaping = jentry["shaping"].asFloat();// * m_tres; 
        nsptr->wirelen = jentry["wirelen"].asFloat();// * m_wlres; 
        nsptr->constant = jentry["const"].asFloat();

        std::cerr << "loading: " << nsptr->plane << " " << nsptr->wirelen/units::cm << " cm" << std::endl;

        auto jfreqs = jentry["freqs"];
        const int nfreqs = jfreqs.size();
        nsptr->freqs.resize(nfreqs, 0.0);
        for (int ind=0; ind<nfreqs; ++ind) {
	  nsptr->freqs[ind] = jfreqs[ind].asFloat();// * m_fres;
	    //std::cout << nsptr->freqs[ind] << " " << std::endl;
        }
        auto jamps = jentry["amps"];
        const int namps = jamps.size();
        nsptr->amps.resize(namps+1, 0.0);
        for (int ind=0; ind<namps; ++ind) {
            nsptr->amps[ind] = jamps[ind].asFloat();
        }
	// put the constant term at the end of the amplitude ... 
	nsptr->amps[namps] = nsptr->constant;

        resample(*nsptr);
        m_spectral_data[nsptr->plane].push_back(nsptr); // assumes ordered by wire length!
    }        
}


IChannelSpectrum::amplitude_t Gen::EmpiricalNoiseModel::interpolate(int iplane, double wire_length) const
{
    auto it = m_spectral_data.find(iplane);
    if (it == m_spectral_data.end()) {
        return amplitude_t();
    }

    const auto& spectra = it->second;

    // Linearly interpolate spectral data to wire length.

    // Any wire lengths
    // which are out of bounds causes the nearest result to be
    // returned (flat extrapolation)
    const NoiseSpectrum* front = spectra.front();
    if (wire_length <= front->wirelen) {
        return front->amps;
    }
    const NoiseSpectrum* back = spectra.back();
    if (wire_length >= back->wirelen) {
        return back->amps;
    }

    const int nspectra = spectra.size();
    for (int ind=1; ind<nspectra; ++ind) {
        const NoiseSpectrum* hi = spectra[ind];
        if (hi->wirelen < wire_length) {
            continue;
        }
        const NoiseSpectrum* lo = spectra[ind-1];

        const double delta = hi->wirelen - lo->wirelen;
        const double dist = wire_length - lo->wirelen;
        const double mu = dist/delta;

        const int nsamp = lo->amps.size();
        amplitude_t amp(nsamp);
        for (int isamp=0; isamp<nsamp; ++isamp) { // regular amplitude ... 
            amp[isamp] = lo->amps[isamp]*(1-mu) + hi->amps[isamp]*mu;
        }
	// amp[nsamp] = lo->constant*(1-mu) + hi->constant*mu; // do the constant term ...
        return amp;
    }
    // should not get here
    return amplitude_t();

}

const std::vector<float>& Gen::EmpiricalNoiseModel::freq() const
{
  // assume frequency is universal ... 
  const int iplane = 0;
  auto it = m_spectral_data.find(iplane);
  const auto& spectra = it->second;
  return spectra.front()->freqs;
}

const double Gen::EmpiricalNoiseModel::shaping_time(int chid) const
{
  auto wpid = m_anode->resolve(chid);
  const int iplane = wpid.index();
  auto it = m_spectral_data.find(iplane);
  const auto& spectra = it->second;
  return spectra.front()->shaping;
}

const double Gen::EmpiricalNoiseModel::gain(int chid) const
{
  auto wpid = m_anode->resolve(chid);
  const int iplane = wpid.index();
  auto it = m_spectral_data.find(iplane);
  const auto& spectra = it->second;
  return spectra.front()->gain;
}



const IChannelSpectrum::amplitude_t& Gen::EmpiricalNoiseModel::operator()(int chid) const
{
    // get truncated wire length for cache
    int ilen=0;
    auto chlen = m_chid_to_intlen.find(chid);
    if (chlen == m_chid_to_intlen.end()) {  // new channel
        auto wires =  m_anode->wires(chid); // sum up wire length
        double len = 0.0;
        for (auto wire : wires) {
            len += ray_length(wire->ray());
        }


	// len is already in cm ... 
	// cache every cm ...
        ilen = int(len/m_wlres);
        m_chid_to_intlen[chid] = ilen;
    }
    else {
        ilen = chlen->second;
    }

    auto wpid = m_anode->resolve(chid);
    const int iplane = wpid.index();

    // find 
    auto& amp_cache = m_amp_cache[iplane];
    auto lenamp = amp_cache.find(ilen);
    if (lenamp == amp_cache.end()) {
      auto amp = interpolate(iplane, ilen*m_wlres); // interpolate ... 
        amp_cache[ilen] = amp;
        return amp_cache[ilen];
    }
    return lenamp->second;
}



