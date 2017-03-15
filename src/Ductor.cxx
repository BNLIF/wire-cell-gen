#include "WireCellGen/Ductor.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Point.h"

#include <string>

using namespace std;
using namespace WireCell;

Gen::Ductor::Ductor()
    : m_start_time(0.0*units::ns)
    , m_readout_period(5.0*units::ms)
    , m_sample_period(0.5*units::us)
    , m_drift_speed(1.0*units::mm/units::us)
    , m_nsigma(3.0)
    , m_fluctuate(true)
    , m_gain(1.0)
    , m_shaping(1.0*units::us)
{
}

WireCell::Configuration Gen::Ductor::default_configuration() const
{
    Configuration cfg;

    /// Path to compressed JSON file holding field responses
    put(cfg, "field_response_file","");

    /// Electronics gain assumed in [mV/fC]
    put(cfg, "gain", 14.0);

    /// Electronics shaping time 
    put(cfg, "shaping", 2.0*units::us);

    /// Number of wires in each plane
    Configuration nwires;       // fixme, would be nice if there was some syntactic sugar.
    nwires[0] = 2001;
    nwires[1] = 2001;
    nwires[2] = 2001;
    cfg["nwires"] = nwires;

    /// Wire angles in each plane
    const double angle = 60*units::degree;
    Configuration angles;
    angles[0] = 0;
    angles[1] = -angle;
    angles[2] = angle;
    cfg["angles"] = angles;

    /// How many Gaussian sigma due to diffusion to keep before truncating.
    put(cfg, "nsigma", 3.0);

    /// Whether to fluctuate the final Gaussian deposition.
    put(cfg, "fluctuate", true);

    /// The initial time for this ductor
    put(cfg, "start_time", 0.0*units::ns);

    /// The period over which to latch responses
    put(cfg, "readout_time", 5*units::ms);

    /// The sample period
    put(cfg, "tick", 0.5*units::us);

    /// The nominal speed of drifting electrons
    put(cfg, "drift_speed", 1.0*units::mm/units::us);

    return cfg;

}

void Gen::Ductor::configure(const WireCell::Configuration& cfg)
{
    const string fname = get<string>(cfg, "field_response_file");
    m_fr = Response::Schema::load(fname.c_str());

    const vector<int> nwiresv = get< vector<int> >(cfg, "nwires");
    const vector<double> angles = get< vector<double> >(cfg, "angles");

    m_nsigma = get<double>(cfg, "nsigma", m_nsigma);
    m_fluctuate = get<bool>(cfg, "fluctuate", m_fluctuate);
    m_start_time = get<double>(cfg, "start_time", m_start_time);
    m_readout_period = get<double>(cfg, "readout_time", m_readout_period);
    m_sample_period = get<double>(cfg, "tick", m_sample_period);
    m_drift_speed = get<double>(cfg, "drift_speed", m_drift_speed);
    m_gain = get<double>(cfg, "gain", m_gain);
    m_shaping = get<double>(cfg, "shaping", m_shaping);
    
    // Origin where drift and diffusion meets field response.
    const Point field_origin(m_fr.origin, 0, 0);

    const int nplanes = m_fr.planes.size();
    m_pimpos.clear();
    m_pimpos.resize(nplanes);
    for (int iplane=0; iplane < nplanes; ++iplane) {
        const int nwires = nwiresv[iplane]; // fixme, check for overflow
        const double angle = angles[iplane]; // fixme, check for overflow
        const Vector vpitch(0, sin(angle), cos(angle));
        const Vector vwire(0, cos(angle), sin(angle));

        auto& pr = m_fr.planes[iplane];
        Response::Schema::lie(pr, vpitch, vwire);
        const double wire_pitch = pr.pitch;
        const double impact_pitch = pr.paths[1].pitchpos - pr.paths[0].pitchpos;
        const int nregion_bins = round(wire_pitch/impact_pitch);
        const double halfwireextent = wire_pitch * (nwires/2);

        Pimpos* pimpos = new Pimpos(nwires, -halfwireextent, halfwireextent,
                                    vwire, vpitch,
                                    field_origin, nregion_bins);
        m_pimpos[iplane] = pimpos;
    }
}

bool Gen::Ductor::operator()(const input_pointer& depo, output_queue& frames)
{
    // 1) buffer just enough depos, returning until full
    // 2) loop over pimpos/planes, creating frame
    // 3) deal with readout edges
    // 4) need channel map

}

