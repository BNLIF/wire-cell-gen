
#include "WireCellGen/EmpiricalNoiseModel.h"


#include "WireCellUtil/Configuration.h"
#include "WireCellUtil/Persist.h"

#include "WireCellUtil/NamedFactory.h"
WIRECELL_FACTORY(EmpiricalNoiseModel, WireCell::Gen::EmpiricalNoiseModel,
                 WireCell::IChannelSpectrum, WireCell::IConfigurable);


using namespace WireCell;

Gen::EmpiricalNoiseModel::EmpiricalNoiseModel(const std::string& spectra_file,
                                              const int nsamples,
                                              const double nyquist_frequency,
                                              const double wire_length_scale,
                                              const std::string anode_tn)
    : m_spectra_file(spectra_file)
    , m_nsamples(nsamples)
    , m_nyqfreq(nyquist_frequency)
    , m_wlres(wire_length_scale)
    , m_anode_tn(anode_tn)

{
}

Gen::EmpiricalNoiseModel::~EmpiricalNoiseModel()
{
}

WireCell::Configuration Gen::EmpiricalNoiseModel::default_configuration() const
{
    Configuration cfg;

    /// The file holding the spectral data
    cfg["spectra_file"] = m_spectra_file;
    cfg["nsamples"] = m_nsamples; // number of samples up to Nyquist frequency
    cfg["nyquist_frequency"] = m_nyqfreq; // remember this is in WCT SoU for frequency!
    cfg["wire_length_scale"] = m_wlres;    // 
    cfg["anode"] = m_anode_tn;            // name of IAnodePlane component

    return cfg;
}

IChannelSpectrum::amplitude_t Gen::EmpiricalNoiseModel::resample(double nyqfreq, const amplitude_t& amp) const
{
    if ((int)amp.size() == m_nsamples && nyqfreq == m_nyqfreq) {
        return amp;
    }

    // fixme/todo: resample to match required sampling; for now,
    // return bogus/empty amplitude spectrum
    return IChannelSpectrum::amplitude_t();
}

void Gen::EmpiricalNoiseModel::configure(const WireCell::Configuration& cfg)
{
    m_anode_tn = get(cfg, "anode", m_anode_tn);
    m_anode = Factory::lookup_tn<IAnodePlane>(m_anode_tn);

    m_spectra_file = get(cfg, "spectra_file", m_spectra_file);
    m_nsamples = get(cfg, "nsamples", m_nsamples);
    m_nyqfreq = get(cfg, "nyquist_frequency", m_nyqfreq);
    m_wlres = get(cfg, "wire_length_scale", m_wlres);

    // Load the data file assuming.  This chunk of code implicitly
    // defines the required data schema.  Fixme: should break this out
    // and make it more formal.
    auto jdat = Persist::load(m_spectra_file);
    const int nentries = jdat.size();
    m_wire_amplitudes.clear();
    m_wire_amplitudes.resize(nentries);
    for (auto jentry : jdat) { // list of dicts ordered by increasing wire length
        auto jlen = jentry["wire_length"];
        auto jnyq = jentry["nyquist"];   // 
        auto jamp = jentry["amplitude"]; // array of sampled amplitude
                                         // up to Nyquist frequency
        const int nsamp = jamp.size();
        amplitude_t amp(nsamp);
        for (int ind=0; ind<nsamp; ++ind) {
            amp[ind] = jamp[ind].asFloat();
        }
        auto rsamp = resample(jnyq.asDouble(), amp);
        m_wire_amplitudes.push_back(make_pair(jlen.asDouble(), rsamp));
    }        
}


IChannelSpectrum::amplitude_t Gen::EmpiricalNoiseModel::interpolate(double wire_length) const
{
    // Linearly interpolate spectral data to wire length.

    // Any wire lengths
    // which are out of bounds causes the nearest result to be
    // returned (flat extrapolation)
    const auto& front = m_wire_amplitudes.front();
    if (wire_length <= front.first) {
        return front.second;
    }
    const auto& back = m_wire_amplitudes.back();
    if (wire_length >= back.first) {
        return back.second;
    }
    const int n = m_wire_amplitudes.size();
    for (int ind=1; ind<n; ++ind) {
        auto& hi = m_wire_amplitudes[ind];
        if (hi.first < wire_length) {
            continue;
        }
        auto& lo = m_wire_amplitudes[ind-1];

        const double delta = hi.first - lo.first;
        const double dist = wire_length - lo.first;
        const double mu = dist/delta;

        const int nsamp = lo.second.size();
        amplitude_t amp(nsamp);
        for (int isamp=0; isamp<nsamp; ++isamp) {
            amp[isamp] = lo.second[isamp]*(1-mu) + hi.second[isamp]*mu;
        }
        return amp;
    }
    // should not get here
    return amplitude_t();

}

const IChannelSpectrum::amplitude_t& Gen::EmpiricalNoiseModel::operator()(int chid) const
{
    auto chlen = m_chid_to_intlen.find(chid);
    int ilen=0;
    if (chlen == m_chid_to_intlen.end()) {
        auto wires =  m_anode->wires(chid);
        double len = 0.0;
        for (auto wire : wires) {
            len += ray_length(wire->ray());
        }

        ilen = int(len/m_wlres);
        m_chid_to_intlen[chid] = ilen;
    }
    else {
        ilen = chlen->second;
    }

    auto lenamp = m_amp_cache.find(ilen);
    if (lenamp == m_amp_cache.end()) {
        auto amp = interpolate(ilen*m_wlres);
        m_amp_cache[ilen] = amp;
        return m_amp_cache[ilen];
    }
    return lenamp->second;
}



