#include "WireCellGen/Ductor.h"
#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellGen/ImpactZipper.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include <string>

WIRECELL_FACTORY(Ductor, WireCell::Gen::Ductor, WireCell::IDuctor, WireCell::IConfigurable);


using namespace std;
using namespace WireCell;

Gen::Ductor::Ductor()
    : m_start_time(0.0*units::ns)
    , m_readout_time(5.0*units::ms)
    , m_drift_speed(1.0*units::mm/units::us)
    , m_nsigma(3.0)
    , m_fluctuate(true)
    , m_nticks(10000)
    , m_frame_count(0)

{
}

WireCell::Configuration Gen::Ductor::default_configuration() const
{
    Configuration cfg;

    /// How many Gaussian sigma due to diffusion to keep before truncating.
    put(cfg, "nsigma", m_nsigma);

    /// Whether to fluctuate the final Gaussian deposition.
    put(cfg, "fluctuate", m_fluctuate);

    /// The initial time for this ductor
    put(cfg, "start_time", m_start_time);

    /// The time span for each readout.
    put(cfg, "readout_time", m_readout_time);

    /// The nominal speed of drifting electrons
    put(cfg, "drift_speed", m_drift_speed);

    /// Allow for a custom starting frame number
    put(cfg, "first_frame_number", m_frame_count);

    return cfg;
}

void Gen::Ductor::configure(const WireCell::Configuration& cfg)
{
    string anode = get<string>(cfg, "anodeplane", "");
    m_anode = Factory::lookup<IAnodePlane>(anode);

    m_nsigma = get<double>(cfg, "nsigma", m_nsigma);
    m_fluctuate = get<bool>(cfg, "fluctuate", m_fluctuate);
    m_readout_time = get<double>(cfg, "readout_time", m_start_time);
    m_start_time = get<double>(cfg, "start_time", m_start_time);
    m_drift_speed = get<double>(cfg, "drift_speed", m_drift_speed);
    m_frame_count = get<int>(cfg, "first_frame_number", m_frame_count);

}

void Gen::Ductor::process(output_queue& frames)
{
    ITrace::vector traces;

    // BIG FAT FIXME: this is not checked!!! it is certainly broken.

    for (auto face : m_anode->faces()) {
        for (auto plane : face->planes()) {

            const Pimpos* pimpos = plane->pimpos();
            const PlaneImpactResponse* pir = plane->pir();

            Binning tbins(pir->tbins().nbins(), m_start_time, m_start_time+m_readout_time);


            Gen::BinnedDiffusion bindiff(*pimpos, tbins, m_nsigma, m_fluctuate);
            for (auto depo : m_depos) {
                bindiff.add(depo, depo->extent_long() / m_drift_speed, depo->extent_tran());
            }

            auto& wires = plane->wires();


            Gen::ImpactZipper zipper(*pir, bindiff);

            const int nwires = pimpos->region_binning().nbins();
            for (int iwire=0; iwire<nwires; ++iwire) {
                auto wave = zipper.waveform(iwire);
                
                auto mm = Waveform::edge(wave);
                if (mm.first == wave.size()) { // all zero
                    continue;
                }
                
                
                int chid = wires[iwire]->channel();
                int tbin = mm.first;
                ITrace::ChargeSequence charge(wave.begin()+mm.first, wave.begin()+mm.second);
                auto trace = make_shared<SimpleTrace>(chid, tbin, charge);
                traces.push_back(trace);
            }
        }
    }

    auto frame = make_shared<SimpleFrame>(m_frame_count, m_start_time, traces, m_readout_time/m_nticks);
    frames.push_back(frame);

    m_depos.clear();
    m_start_time += m_readout_time;
    ++m_frame_count;
}


bool Gen::Ductor::operator()(const input_pointer& depo, output_queue& frames)
{
    double target_time = m_start_time + m_readout_time;
    if (!depo || depo->time() > target_time) {
        process(frames);
    }

    if (depo) {
        m_depos.push_back(depo);
    }

    return true;
}

