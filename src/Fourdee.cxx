#include "WireCellGen/Fourdee.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/ConfigManager.h"
#include "WireCellUtil/ExecMon.h"

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
    std::string tn="";
    Configuration cfg = thecfg;

    m_depos = Factory::lookup<IDepoSource>(get<string>(cfg, "DepoSource"));
    // fixme: need to convert MeV to number of electrons with: Fano,
    // Number_electrons_per_mev and Recombination.
    m_drifter = Factory::lookup<IDrifter>(get<string>(cfg, "Drifter",""));
    m_ductor = Factory::lookup<IDuctor>(get<string>(cfg, "Ductor",""));
    tn = get<string>(cfg, "Dissonance","");
    if (tn.empty()) {           // noise is optional
        m_dissonance = nullptr;
    }
    else {
        m_dissonance = Factory::lookup<IFrameSource>(tn);
    }
    tn = get<string>(cfg, "Digitizer","");
    if (tn.empty()) {           // digitizer is optional, voltage saved w.o. it.
        m_digitizer = nullptr;
    }
    else {
        m_digitizer = Factory::lookup<IFrameFilter>(tn);
    }
    m_output = Factory::lookup<IFrameSink>(get<string>(cfg, "FrameSink",""));
    

    // fixme: check for failures
}


template<typename DEPOS>
void dump(DEPOS& depos)
{
    double tmin = -999, tmax = -999, xmin = -999, xmax = -999;
    double qtot = 0.0;
    double qorig = 0.0;
    int ndepos = 0;
    for (auto depo : depos) {
        if (!depo) {
            cerr << "Gen::Fourdee: null depo" << endl;
            continue;
        }
        ++ndepos;
        double t = depo->time();
        double x = depo->pos().x();
        qtot += depo->charge();
        qorig += depo->prior()->charge();
        if (tmin < 0) {
            tmin = tmax = t;
            xmin = xmax = x;
            continue;
        }
        if (t < tmin) { tmin = t; }
        if (t > tmax) { tmax = t; }
        if (x < xmin) { xmin = x; }
        if (x > xmax) { xmax = x; }
    }

    cerr << "Gen::FourDee: drifted " << ndepos
         << ", x in [" << xmin/units::mm << ","<<xmax/units::mm<<"]mm, "
         << "t in [" << tmin/units::us << "," << tmax/units::us << "]us, "
         << " Qtot=" << qtot/units::eplus << ", <Qtot>=" << qtot/ndepos/units::eplus << " "
         << " Qorig=" << qorig/units::eplus
         << ", <Qorig>=" << qorig/ndepos/units::eplus << " electrons" << endl;

        
}


void Gen::Fourdee::execute()
{
    if (!m_depos) cerr << "no depos" << endl;
    if (!m_drifter) cerr << "no drifter" << endl;
    if (!m_ductor) cerr << "no ductor" << endl;
    if (!m_dissonance) cerr << "no dissonance" << endl;
    if (!m_digitizer) cerr << "no digitizer" << endl;
    if (!m_output) cerr << "no output" << endl;

    // here we make a manual pipeline.  In a "real" app this might be
    // a DFP executed by TBB.
    int count=0;
    int ndepos = 0;
    int ndrifted = 0;
    ExecMon em;
    while (true) {
        ++count;

        IDepo::pointer depo;
        if (!(*m_depos)(depo)) {
            cerr << "DepoSource is empty\n";
        }
        if (!depo) {
            cerr << "Got null depo from source at "<<count<< endl;
        }
        else {
            ++ndepos;
        }
        //cerr << "Gen::FourDee: seen " << ndepos << " depos\n";

        IDrifter::output_queue drifted;
        if (!(*m_drifter)(depo, drifted)) {
            cerr << "Stopping on " << type(*m_drifter) << endl;
            goto bail;
        }
        if (drifted.empty()) {
            continue;
        }
        ndrifted += drifted.size();
        cerr << "Gen::FourDee: seen " << ndrifted << " drifted\n";
        //dump(drifted);
        
        for (auto drifted_depo : drifted) {
            IDuctor::output_queue frames;
            if (!(*m_ductor)(drifted_depo, frames)) {
                cerr << "Stopping on " << type(*m_ductor) << endl;
                goto bail;
            }
            if (frames.empty()) {
                continue;
            }
            cerr << "Gen::FourDee: got " << frames.size() << " frames\n";

            for (IFrameFilter::input_pointer voltframe : frames) {
                em("got frame");

                /// fixme: needs implementing of "add()".
                // if (m_dissonance) {
                //     IFrame::pointer noise;
                //     if (!(*m_dissonance)(noise)) {
                //         cerr << "Stopping on " << type(*m_dissonance) << endl;
                //         goto bail;
                //     }
                //     voltframe = add(voltframe, noise);
                // }

                IFrameFilter::output_pointer adcframe;
                if (m_digitizer) {
                    if (!(*m_digitizer)(voltframe, adcframe)) {
                        cerr << "Stopping on " << type(*m_digitizer) << endl;
                        goto bail;
                    }
                }
                else {
                    adcframe = voltframe;
                }
                em("digitized");
                cerr << "frame with " << adcframe->traces()->size() << " traces\n";
                if (!(*m_output)(adcframe)) {
                    cerr << "Stopping on " << type(*m_output) << endl;
                    goto bail;
                }
                em("output");
            }
        }
    }
  bail:             // what's this weird syntax?  What is this, BASIC?
    cerr << em.summary() << endl;
}

