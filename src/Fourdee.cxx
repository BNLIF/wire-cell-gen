#include "WireCellGen/Fourdee.h"
#include "WireCellGen/FrameUtil.h"
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
    Configuration cfg;

    // the 4 d's and proof the developer can not count:
    put(cfg, "DepoSource", "TrackDepos");
    put(cfg, "Drifter", "Drifter");      
    put(cfg, "Ductor", "Ductor");        
    put(cfg, "Dissonance", "SilentNoise");
    put(cfg, "Digitizer", "Digitizer");  
    put(cfg, "Filter", "");  
    put(cfg, "FrameSink", "DumpFrames"); 

    return cfg;
}


void Gen::Fourdee::configure(const Configuration& thecfg)
{
    std::string tn="";
    Configuration cfg = thecfg;

    cerr << "Gen::Fourdee:configure:\n";

    tn = get<string>(cfg, "DepoSource");
    cerr << "\tDepoSource: " << tn << endl;
    m_depos = Factory::find_tn<IDepoSource>(tn);

    tn = get<string>(cfg, "Drifter");
    cerr << "\tDrifter: " << tn << endl;
    m_drifter = Factory::find_tn<IDrifter>(tn);

    tn = get<string>(cfg, "Ductor");
    cerr << "\tDuctor: " << tn << endl;
    m_ductor = Factory::find_tn<IDuctor>(tn);

        
    tn = get<string>(cfg, "Dissonance","");
    if (tn.empty()) {           // noise is optional
        m_dissonance = nullptr;
        cerr << "\tDissonance: none\n";
    }
    else {
        m_dissonance = Factory::find_tn<IFrameSource>(tn);
        cerr << "\tDissonance: " << tn << endl;
    }

    tn = get<string>(cfg, "Digitizer","");
    if (tn.empty()) {           // digitizer is optional, voltage saved w.o. it.
        m_digitizer = nullptr;
        cerr << "\tDigitizer: none\n";
    }
    else {
        m_digitizer = Factory::find_tn<IFrameFilter>(tn);
        cerr << "\tDigitizer: " << tn << endl;
    }

    tn = get<string>(cfg, "Filter","");
    if (tn.empty()) {           // filter is optional
        m_filter = nullptr;
        cerr << "\tFilter: none\n";
    }
    else {
        m_filter = Factory::find_tn<IFrameFilter>(tn);
        cerr << "\tFilter: " << tn << endl;
    }

    tn = get<string>(cfg, "FrameSink","");
    if (tn.empty()) {           // sink is optional
        m_output = nullptr;
        cerr << "\tSink: none\n";
    }
    else {
        m_output = Factory::find_tn<IFrameSink>(tn);
        cerr << "\tSink: " << tn << endl;
    }
}


void dump(const IFrame::pointer frame)
{
    if (!frame) {
        cerr << "frame: empty!\n";
        return;
    }

    auto traces = frame->traces();
    const int ntraces = traces->size();

    std::vector<int> tbins, tlens;
    for (auto trace : *traces) {
        const int tbin = trace->tbin();
        tbins.push_back(tbin);
        tlens.push_back(tbin+trace->charge().size());
    }

    int tmin = *(std::minmax_element(tbins.begin(), tbins.end()).first);
    int tmax = *(std::minmax_element(tlens.begin(), tlens.end()).second);

    cerr << "frame: #" << frame->ident()
         << " @" << frame->time()/units::ms
         << " with " << ntraces << " traces, tbins in: "
         << "[" << tmin << "," << tmax << "]"
         << endl;
}

template<typename DEPOS>
void dump(DEPOS& depos)
{
    std::vector<double> t,x,y,z;
    double qtot = 0.0;
    double qorig = 0.0;

    for (auto depo : depos) {
        if (!depo) {
            cerr << "Gen::Fourdee: null depo" << endl;
            break;
        }
        auto prior = depo->prior();
        if (!prior) {
            cerr << "Gen::Fourdee: null prior depo" << endl;
        }
        else {
            qorig += prior->charge();
        }
        t.push_back(depo->time());
        x.push_back(depo->pos().x());
        y.push_back(depo->pos().y());
        z.push_back(depo->pos().z());
        qtot += depo->charge();
    }

    auto tmm = std::minmax_element(t.begin(), t.end());
    auto xmm = std::minmax_element(x.begin(), x.end());
    auto ymm = std::minmax_element(y.begin(), y.end());
    auto zmm = std::minmax_element(z.begin(), z.end());
    const int ndepos = depos.size();

    std::cerr << "Gen::FourDee: drifted " << ndepos << ", extent:\n"
              << "\tt in [ " << (*tmm.first)/units::us << "," << (*tmm.second)/units::us << "]us,\n"
              << "\tx in [" << (*xmm.first)/units::mm << ","<<(*xmm.second)/units::mm<<"]mm,\n"
              << "\ty in [" << (*ymm.first)/units::mm << ","<<(*ymm.second)/units::mm<<"]mm,\n"
              << "\tz in [" << (*zmm.first)/units::mm << ","<<(*zmm.second)/units::mm<<"]mm,\n"
              << "\tQtot=" << qtot/units::eplus << ", <Qtot>=" << qtot/ndepos/units::eplus << " "
              << " Qorig=" << qorig/units::eplus
              << ", <Qorig>=" << qorig/ndepos/units::eplus << " electrons" << std::endl;

        
}


void Gen::Fourdee::execute()
{
    if (!m_depos) cerr << "no depos" << endl;
    if (!m_drifter) cerr << "no drifter" << endl;
    if (!m_ductor) cerr << "no ductor" << endl;
    if (!m_dissonance) cerr << "no dissonance" << endl;
    if (!m_digitizer) cerr << "no digitizer" << endl;
    if (!m_filter) cerr << "no filter" << endl;
    if (!m_output) cerr << "no output" << endl;

    // here we make a manual pipeline.  In a "real" app this might be
    // a DFP executed by TBB.
    int count=0;
    int ndepos = 0;
    int ndrifted = 0;
    ExecMon em;
    cerr << "Gen::Fourdee: staring\n";
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
        dump(drifted);
        
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
                cerr << "voltframe: ";
                dump(voltframe);

                if (m_dissonance) {
                    cerr << "Gen::FourDee: including noise\n";
                    IFrame::pointer noise;
                    if (!(*m_dissonance)(noise)) {
                        cerr << "Stopping on " << type(*m_dissonance) << endl;
                        goto bail;
                    }
                    if (noise) {
                        cerr << "noiseframe: ";
                        dump(noise);
                        voltframe = Gen::sum(IFrame::vector{voltframe,noise}, voltframe->ident());
                        em("got noise");
                    }
                    else {
                        cerr << "Noise source is empty\n";
                    }
                }

                IFrameFilter::output_pointer adcframe;
                if (m_digitizer) {
                    if (!(*m_digitizer)(voltframe, adcframe)) {
                        cerr << "Stopping on " << type(*m_digitizer) << endl;
                        goto bail;
                    }
                    em("digitized");
                    cerr << "digiframe: ";
                    dump(adcframe);
                }
                else {
                    adcframe = voltframe;
                }

                if (m_filter) {
                    IFrameFilter::output_pointer filtframe;
                    if (!(*m_filter)(adcframe, filtframe)) {
                        cerr << "Stopping on " << type(*m_filter) << endl;
                        goto bail;
                    }
                    adcframe = filtframe;
                    em("filtered");
                }

                if (m_output) {
                    if (!(*m_output)(adcframe)) {
                        cerr << "Stopping on " << type(*m_output) << endl;
                        goto bail;
                    }
                    em("output");
                }
                cerr << "Gen::Fourdee: frame with " << adcframe->traces()->size() << " traces\n";
            }
        }
    }
  bail:             // what's this weird syntax?  What is this, BASIC?
    cerr << em.summary() << endl;
}

