#include "WireCellGen/FrameFanin.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellIface/SimpleFrame.h"

#include <iostream>

WIRECELL_FACTORY(FrameFanin, WireCell::Gen::FrameFanin,
                 WireCell::IFrameFanin, WireCell::IConfigurable)


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
    cfg["multiplicity"] = (int)m_multiplicity;
    // A non-null entry in this array is taken as a string and used to
    // tag traces which arrive on the corresponding input port when
    // they are placed  to the output frame.
    cfg["tags"] = Json::arrayValue; 
    return cfg;
}
void Gen::FrameFanin::configure(const WireCell::Configuration& cfg)
{
    int m = get<int>(cfg, "multiplicity", (int)m_multiplicity);
    if (m<=0) {
        THROW(ValueError() << errmsg{"FrameFanin multiplicity must be positive"});
    }
    m_multiplicity = m;
    m_tags.resize(m);
    auto jtags = cfg["tags"];
    for (int ind=0; ind<m; ++ind) {
        m_tags[ind] = convert<std::string>(jtags[ind], "");
    }
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
    for (const auto& fr : invec) {
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

    if (invec.size() != m_multiplicity) {
        std::cerr << "Gen::FrameFanin: got unexpected multiplicity, got:" << invec.size() << " want:" << m_multiplicity << std::endl;
        THROW(ValueError() << errmsg{"unexpected multiplicity"});
    }


    std::vector<IFrame::trace_list_t> by_port(m_multiplicity);

    ITrace::vector out_traces;
    IFrame::pointer one = nullptr;
    for (size_t iport=0; iport < m_multiplicity; ++iport) {
        const auto& fr = invec[iport];
        if (!one) { one = fr; }
        auto traces = fr->traces();

        if (! m_tags[iport].empty() ) {
            IFrame::trace_list_t tl(traces->size());
            std::iota(tl.begin(), tl.end(), out_traces.size());
            by_port[iport] = tl;
        }
        out_traces.insert(out_traces.end(), traces->begin(), traces->end());
    }
    
    auto sf = new SimpleFrame(one->ident(), one->time(), out_traces, one->tick());
    for (size_t iport=0; iport < m_multiplicity; ++iport) {
        if (m_tags[iport].empty()) {
            continue;
        }
        std::cerr << "Tagging " << by_port[iport].size() << " traces from port " << iport << " with " << m_tags[iport] << std::endl;
        sf->tag_traces(m_tags[iport], by_port[iport]);
    }

    out = IFrame::pointer(sf);
    return true;
}


