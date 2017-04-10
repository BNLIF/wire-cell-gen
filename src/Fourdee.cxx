#include "WireCellGen/Fourdee.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/ConfigManager.h"

WIRECELL_FACTORY(FourDee, WireCell::Gen::Fourdee, WireCell::IApplication, WireCell::IConfigurable);

using namespace std;
using namespace WireCell;


Gen::Fourdee::Fourdee()
    : m_depos(nullptr)
    , m_drifter(nullptr)
    , m_ductor(nullptr)
    , m_dissonance(nullptr)
    , m_digitizer(nullptr)
    , m_output(nullptr)
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
    put(cfg, "Ductor", "Ductor");         // have
    put(cfg, "Dissonance", "SilentNoise"); // needed
    put(cfg, "Digitizer", "Digitizer");    // need to rework frame
    put(cfg, "FrameSink", "DumpFrames"); // needed

    return cfg;
}


void Gen::Fourdee::configure(const Configuration& thecfg)
{
    Configuration cfg = thecfg;
    m_depos = Factory::lookup<IDepoSource>(get<string>(cfg, "DepoSource"));
    // fixme: need to convert MeV to number of electrons with: Fano,
    // Number_electrons_per_mev and Recombination.
    m_drifter = Factory::lookup<IDrifter>(get<string>(cfg, "Drifter",""));
    m_ductor = Factory::lookup<IDuctor>(get<string>(cfg, "Ductor",""));
    m_dissonance = Factory::lookup<IFrameSource>(get<string>(cfg, "Dissonance",""));
    m_digitizer = Factory::lookup<IFrameFilter>(get<string>(cfg, "Digitizer",""));
    m_output = Factory::lookup<IFrameSink>(get<string>(cfg, "FrameSink",""));
    

    // fixme: check for failures
}


template<typename DEPOS>
void dump(DEPOS& depos)
{
    double tmin = -999, tmax = -999, xmin = -999, xmax = -999;
    double qtot = 0.0;
    double qorig = 0.0;
    for (auto depo : depos) {
        if (!depo) {
            cerr << "Fourdee: null depo" << endl;
            continue;
        }
        double t = depo->time();
        double x = depo->pos().x();
        qtot += depo->charge();
        qorig += depo->prior()->charge();
        if (tmin < 0) {
            tmin = tmax = t;
            xmin = xmax = x;
            continue;
        }
        if (tmin < t) { tmin = t; }
        if (tmax > t) { tmax = t; }
        if (xmin < x) { xmin = x; }
        if (xmax > x) { xmax = x; }
    }
    
    const int ndepos = depos.size();
    cerr << "Drifted " << ndepos
         << ", x in [" << xmin/units::mm << ","<<xmax/units::mm<<"], "
         << "t in [" << tmin/units::us << "," << tmax/units::us << "], "
         << " Qtot=" << qtot << ", <Qtot>=" << qtot/ndepos << " "
         << " Qorig=" << qorig << ", <Qorig>=" << qorig/ndepos << endl;

        
}


void Gen::Fourdee::execute()
{
    cerr << "TrackDepos is type: " << type(m_depos) << endl;

    if (!m_depos) cerr << "no depos" << endl;
    if (!m_drifter) cerr << "no drifter" << endl;
    if (!m_ductor) cerr << "no ductor" << endl;
    if (!m_dissonance) cerr << "no dissonance" << endl;
    if (!m_digitizer) cerr << "no digitizer" << endl;
    if (!m_output) cerr << "no output" << endl;

    // here we make a manual pipeline.  In a "real" app this might be
    // a DFP executed by TBB.
    int count=0;
    while (true) {
        ++count;

        IDepo::pointer depo;
        bool ok = (*m_depos)(depo);
        if (!(*m_depos)(depo)) {
            cerr << "Stopping on " << type(*m_depos) << endl;
            return;
        }
        if (!depo) {
            //cerr << "Got null depo from source at "<<count<< endl;
        }


        IDrifter::output_queue drifted;
        if (!(*m_drifter)(depo, drifted)) {
            cerr << "Stopping on " << type(*m_drifter) << endl;
            return;
        }
        if (drifted.empty()) {
            continue;
        }


        dump(drifted);
        
        for (auto drifted_depo : drifted) {
            IDuctor::output_queue frames;
            if (!(*m_ductor)(drifted_depo, frames)) {
                cerr << "Stopping on " << type(*m_ductor) << endl;
                return;
            }
            if (frames.empty()) {
                continue;
            }

            for (IFrameFilter::input_pointer voltframe : frames) {
/// Don't deal with noise quite yet.
//                IFrame::pointer noise;
//                if (!(*m_dissonance(noise))) {
//                    cerr << "Stopping on " << type(*m_dissonance) << endl;
//                    return;
//                }
//                voltframe = add(voltframe, noise);

                IFrameFilter::output_pointer adcframe;
                if (!(*m_digitizer)(voltframe, adcframe)) {
                    cerr << "Stopping on " << type(*m_digitizer) << endl;
                    return;
                }

                if (!(*m_output)(adcframe)) {
                    cerr << "Stopping on " << type(*m_output) << endl;
                    return;
                }
            }
        }
    }
}

