#include "WireCellGen/NoiseSource.h"

#include "WireCellUtil/Persist.h"

#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(NoiseSource, WireCell::Gen::NoiseSource, WireCell::IFrameSource, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;

Gen::NoiseSource::NoiseSource()
    : m_filename("")
    , m_scale(1.0)
{
}
Gen::NoiseSource::NoiseSource(const std::string& noise_spectrum_data_filename, double scale)
    : m_filename(noise_spectrum_data_filename)
    , m_scale(scale)
{
}


Gen::NoiseSource::~NoiseSource()
{
}

WireCell::Configuration Gen::NoiseSource::default_configuration() const
{
    Configuration cfg;
    cfg["spectrum_filename"] = m_filename;
    cfg["scale"] = m_scale;
    return cfg;
}

void Gen::NoiseSource::configure(const WireCell::Configuration& cfg)
{
    // unpack configuration using hard coded defaults if the parameter is not set.
    m_filename = get(cfg, "spectrum_filename", m_filename);
    m_scale = get(cfg, "scale", m_scale);

    if (m_filename == "") {
        cerr << "Gen::NoiseSource: no noise spectrum data file given\n";
        return;                 // fixme: throw exception here
    }

    // fixme: should hoist this into a first class citizen ala WireSchema or Response.
    // Use the schema matching Response.

    // The noise spectrum is an array of values covering frequencies
    // from 0 to the Nyquist frequency.  Note, this is one-half of the
    // entire frequency domain.  The "negative" frequencies are omitted.
    // top['noise_spectrum']['array'].keys()
    // ['shape', 'elements']
    // top['noise_spectrum']['nyquist']
    // 1000000.0

    Json::Value jtop = WireCell::Persist::load(m_filename);
    Json::Value jnoise = jtop["noise_spectrum"];
    const double nyquist = jnoise["array"]["nyquist"].asDouble();
    const int nsamps = jnoise["array"]["shape"][0].asInt();
    Json::Value jarray = jnoise["array"]["elements"];
    m_spectra.clear();
    m_spectra.resize(nsamps);
    for (int ind=0; ind<nsamps; ++ind) {
        const double voltage= m_scale*jarray[ind].asDouble();
        m_spectra[ind] = voltage;
    }
    
    //.....
}


bool Gen::NoiseSource::operator()(IFrame::pointer& frame)
{
    frame = nullptr;            // not yet implemented
    return false;
}


