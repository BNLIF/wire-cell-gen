#include "WireCellGen/Misconfigure.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Response.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"

WIRECELL_FACTORY(Misconfigure, WireCell::Gen::Misconfigure,
                 WireCell::IFrameFilter, WireCell::IConfigurable);

using namespace WireCell;

Gen::Misconfigure::Misconfigure()
{
}

Gen::Misconfigure::~Misconfigure()
{
}


WireCell::Configuration Gen::Misconfigure::default_configuration() const
{
    Configuration cfg;

    // nominally correctly configured amplifiers in MB.  They should
    // match what ever was used to create the input waveforms.
    cfg["from"]["gain"] = 14.0*units::mV/units::fC;
    cfg["from"]["shaping"] = 2.2*units::us;

    // Nominally misconfigured amplifiers in MB.  They should match
    // whatever you wished the input waveforms would have been created
    // with.
    cfg["to"]["gain"] = 4.7*units::mV/units::fC;
    cfg["to"]["shaping"] = 1.1*units::us;

    /// The number of samples of the response functions.
    cfg["nsamples"] = 100;
    /// The period of sampling the response functions
    cfg["tick"] = 0.5*units::us;

    return cfg;
}

// a local helper that bundles together a bunch of little steps,
// unpack params, makes cold elec response and returns it's FFT.
static Waveform::compseq_t make_coldelec_spec(std::string dir, Configuration cfg)
{
    const double gain = cfg[dir]["gain"].asDouble();
    const double shaping = cfg[dir]["shaping"].asDouble();
    const double tick = cfg["tick"].asDouble();
    const double nsamples = cfg["nsamples"].asInt();

    const Binning bins(nsamples, 0, nsamples*tick);
    Response::ColdElec coldelec(gain, shaping);
    auto wave = coldelec.generate(bins);
    return Waveform::dft(wave);    
}

void Gen::Misconfigure::configure(const WireCell::Configuration& cfg)
{
    auto fromspec = make_coldelec_spec("from", cfg);
    m_filter = make_coldelec_spec("to", cfg);
    Waveform::shrink(m_filter, fromspec);
}

bool Gen::Misconfigure::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        return true;            // eos, but we don't care here.
    }

    auto traces = in->traces();
    if (!traces) {
        std::cerr << "Gen::Misconfigure: warning no traces in frame for me\n";
        return true;
    }

    size_t ntraces = traces->size();
    ITrace::vector out_traces(ntraces);
    for (size_t ind=0; ind<ntraces; ++ind) {
        auto trace = traces->at(ind);
        auto& wave = trace->charge(); // avoid copy
        auto spec = Waveform::dft(wave);

        // apply filter
        Waveform::scale(spec, m_filter);
        // Should we zero the DC component here?

        auto newwave = Waveform::idft(spec);
        out_traces[ind] = std::make_shared<SimpleTrace>(trace->channel(), trace->tbin(), newwave);
    }

    out = std::make_shared<SimpleFrame>(in->ident(), in->time(), out_traces, in->tick());
    return true;
}

