#include "WireCellGen/Digitizer.h"

#include "WireCellIface/IWireSelectors.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"

#include "WireCellUtil/Testing.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(Digitizer, WireCell::Gen::Digitizer, WireCell::IFrameFilter);

using namespace std;
using namespace WireCell;


Gen::Digitizer::Digitizer(const std::string& anode,
                          int resolution, double gain,
                          std::vector<double> fullscale, std::vector<double> baselines)
    : m_anode_tn(anode)
    , m_resolution(resolution)
    , m_gain(gain)
    , m_fullscale(fullscale)
    , m_baselines(baselines)
{
}

Gen::Digitizer::~Digitizer()
{
}

WireCell::Configuration Gen::Digitizer::default_configuration() const
{
    Configuration cfg;
    put(cfg, "anode", m_anode_tn);

    put(cfg, "resolution", m_resolution);
    put(cfg, "gain", m_gain);
    Configuration fs(Json::arrayValue); // fixme: sure would be nice if we had some templated sugar for this
    for (int ind=0; ind<2; ++ind) {
        fs[ind] = m_fullscale[ind];
    }
    cfg["fullscale"] = fs;

    Configuration bl(Json::arrayValue);
    for (int ind=0; ind<3; ++ind) {
        bl[ind] = m_baselines[ind];
    }
    cfg["baselines"] = bl;
    return cfg;
}

void Gen::Digitizer::configure(const Configuration& cfg)
{
    m_anode_tn = get<string>(cfg, "anode", m_anode_tn);
    m_anode = Factory::find_tn<IAnodePlane>(m_anode_tn);
    if (!m_anode) {
        cerr << "Gen::Digitizer: failed to get anode: \"" << m_anode_tn << "\"\n";
        Assert(m_anode);
    }
    //Assert(m_anode->resolve(1104).valid());  // test for microboone setup

    m_resolution = get(cfg, "resolution", m_resolution);
    m_gain = get(cfg, "gain", m_gain);
    m_fullscale = get(cfg, "fullscale", m_fullscale);
    m_baselines = get(cfg, "baselines", m_baselines);

    cerr << "Gen::Digitizer: "
         << "resolution="<<m_resolution<<" bits, "
         << "maxvalue=" << (1<<m_resolution) << " counts, "
         << "gain=" << m_gain << ", "
         << "fullscale=[" << m_fullscale[0]/units::mV << "," << m_fullscale[1]/units::mV << "] mV, "
         << "baselines=[" << m_baselines[0]/units::mV << "," << m_baselines[1]/units::mV << "," << m_baselines[2]/units::mV << "] mV\n";

}


double Gen::Digitizer::digitize(double voltage)
{
    const int adcmaxval = (1<<m_resolution)-1;

    if (voltage <= m_fullscale[0]) {
        return 0;
    }
    if (voltage >= m_fullscale[1]) {
        return adcmaxval;
    }
    const double relvoltage = (voltage - m_fullscale[0])/(m_fullscale[1] - m_fullscale[0]);
    return relvoltage*adcmaxval;
}

bool Gen::Digitizer::operator()(const input_pointer& vframe, output_pointer& adcframe)
{
    auto vtraces = vframe->traces();
    const int ntraces = vtraces->size();
    ITrace::vector adctraces(ntraces);

    for (int itrace=0; itrace<ntraces; ++itrace) {
        auto& vtrace = vtraces->at(itrace);
        const int channel = vtrace->channel();
        WirePlaneId wpid = m_anode->resolve(channel);
        if (!wpid.valid()) {
            cerr << "Got invalid WPID for channel " << channel << ": " << wpid << endl;
        }

        // fixme: WCT needs a better way to handle anode database
        // lookups like this.  Eg, is index() the right level of
        // abstraction?  Could there be different baselines in the
        // same wire plane?
        const double baseline = m_baselines[wpid.index()];
        auto& vwave = vtrace->charge();
        const int nsamps = vwave.size();
        ITrace::ChargeSequence adcwave(nsamps);
        for (int isamp=0; isamp<nsamps; ++isamp) {
            const double voltage = m_gain*vwave[isamp] + baseline;
            const float adcf = digitize(voltage);
            adcwave[isamp] = adcf;
        }
        if (false) {             // debug
            auto vmm = std::minmax_element(vwave.begin(), vwave.end());
            auto adcmm = std::minmax_element(adcwave.begin(), adcwave.end());
            cerr << "Gen::Digitizer: "
                 << "wpid.index=" << wpid.index() << ", "
                 << "wpid.ident=" << wpid.ident() << ", "
                 << "vmm=[" << (*vmm.first)/units::uV << "," << (*vmm.second)/units::uV << "] uV, "
                 << "adcmm=[" << (*adcmm.first) << "," << (*adcmm.second) << "], "
                 << endl;
        }

        adctraces[itrace] = make_shared<SimpleTrace>(channel, vtrace->tbin(), adcwave);
    }
    adcframe = make_shared<SimpleFrame>(vframe->ident(), vframe->time(), adctraces,
                                        vframe->tick(), vframe->masks());
    return true;
}
