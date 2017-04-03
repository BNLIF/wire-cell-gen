#include "WireCellGen/Digitizer.h"

#include "WireCellIface/IWireSelectors.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"

#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(Digitizer, WireCell::Gen::Digitizer, WireCell::IFrameFilter);

using namespace std;
using namespace WireCell;


Gen::Digitizer::Digitizer(int maxsamp, float baseline, float vperadc)
    : m_max_sample(maxsamp)
    , m_voltage_baseline(baseline)
    , m_volts_per_adc(vperadc)

{
}
Gen::Digitizer::~Digitizer()
{
}

WireCell::Configuration Gen::Digitizer::default_configuration() const
{
    Configuration cfg;
    put(cfg, "MaxSample", m_max_sample);
    put(cfg, "Baseline", m_voltage_baseline);
    put(cfg, "VperADC", m_volts_per_adc);
    return cfg;
}

void Gen::Digitizer::configure(const Configuration& cfg)
{
    m_max_sample = get(cfg, "MaxSample", m_max_sample);
    m_voltage_baseline = get(cfg, "Baseline", m_voltage_baseline);
    m_volts_per_adc = get(cfg, "VperADC", m_volts_per_adc);
}


bool Gen::Digitizer::operator()(const input_pointer& vframe, output_pointer& adcframe)
{
    auto vtraces = vframe->traces();
    const int ntraces = vtraces->size();
    ITrace::vector adctraces(ntraces);

    const double voltsrange = (m_max_sample+1)*m_volts_per_adc;

    for (int itrace=0; itrace<ntraces; ++itrace) {
        auto& vtrace = vtraces->at(itrace);
        auto& vwave = vtrace->charge();
        const int nsamps = vwave.size();
        ITrace::ChargeSequence adcwave(nsamps);
        for (int isamp=0; isamp<nsamps; ++isamp) {
            double adc = (vwave[isamp] - m_voltage_baseline)/voltsrange;
            if (adc < 0.0) {
                adc = 0.0; // underflow
            }
            if (adc > m_max_sample) {
                adc = m_max_sample; // overflow
            }
            adcwave[isamp] = adc;
        }
        adctraces[itrace] = make_shared<SimpleTrace>(vtrace->channel(), vtrace->tbin(), adcwave);
    }
    adcframe = make_shared<SimpleFrame>(vframe->ident(), vframe->time(), adctraces,
                                        vframe->tick(), vframe->masks());
    return true;
}
