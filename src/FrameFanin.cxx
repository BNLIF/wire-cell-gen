#include "WireCellGen/FrameFanin.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellIface/SimpleFrame.h"

#include <iostream>

WIRECELL_FACTORY(FrameFanin, WireCell::Gen::FrameFanin,
                 WireCell::IFrameFanin, WireCell::IConfigurable);


using namespace WireCell;

Gen::FrameFanin::FrameFanin(size_t multiplicity)
    : m_multiplicity(multiplicity)
{
}
Gen::FrameFanin::~FrameFanin()
{
}

WireCell::Configuration Gen::FrameFanin::default_configuration() const
{
    Configuration cfg;
    cfg["multiplicity"] = m_multiplicity;
    return cfg;
}
void Gen::FrameFanin::configure(const WireCell::Configuration& cfg)
{
    int m = get<int>(cfg, "multiplicity", (int)m_multiplicity);
    if (m<=0) {
        THROW(ValueError() << errmsg{"FrameFanin multiplicity must be positive"});
    }
    m_multiplicity = m;
}


std::vector<std::string> Gen::FrameFanin::input_types()
{
    const std::string tname = std::string(typeid(input_type).name());
    std::vector<std::string> ret(m_multiplicity, tname);
    return ret;

}

bool Gen::FrameFanin::operator()(const input_vector& invec, output_pointer& out)
{
    out = nullptr;
    size_t neos = 0;
    for (auto fr : invec) {
        if (!fr) {
            ++neos;
        }
    }
    if (neos == invec.size()) {
        return true;
    }
    if (neos) {
        std::cerr << "Gen::FrameFanin: " << neos << " input frames missing\n";
    }


    // fixme: this component is too simple for general use.  It
    // collects all traces from all input frames into the output.  It
    // ignores tags and care if the same channel happens to be
    // represented in multiple input frames.  It also copies meta data
    // from first frame and doesn't care if other frames have
    // different ident, start time or tick.  What it does so far is
    // okay for using it to do multi-apa simulation, assuming a
    // properly sync'ed DFP graph.

    ITrace::vector out_traces;
    IFrame::pointer one = nullptr;
    for (auto fr : invec) {
        if (!fr) { continue; }
        if (!one) { one = fr; }
        auto traces = fr->traces();
        out_traces.insert(out_traces.end(), traces->begin(), traces->end());
    }
    
    auto sf = new SimpleFrame(one->ident(), one->time(), out_traces, one->tick());
    out = IFrame::pointer(sf);
    return true;
}


