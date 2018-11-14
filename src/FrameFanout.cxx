#include "WireCellGen/FrameFanout.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellIface/SimpleFrame.h"

#include <iostream>

WIRECELL_FACTORY(FrameFanout, WireCell::Gen::FrameFanout,
                 WireCell::IFrameFanout, WireCell::IConfigurable)


using namespace WireCell;

Gen::FrameFanout::FrameFanout(size_t multiplicity)
    : m_multiplicity(multiplicity)
{
}
Gen::FrameFanout::~FrameFanout()
{
}

WireCell::Configuration Gen::FrameFanout::default_configuration() const
{
    Configuration cfg;
    cfg["multiplicity"] = (int)m_multiplicity;
    return cfg;
}
void Gen::FrameFanout::configure(const WireCell::Configuration& cfg)
{
    int m = get<int>(cfg, "multiplicity", (int)m_multiplicity);
    if (m<=0) {
        THROW(ValueError() << errmsg{"FrameFanout multiplicity must be positive"});
    }
    m_multiplicity = m;
}


std::vector<std::string> Gen::FrameFanout::output_types()
{
    const std::string tname = std::string(typeid(output_type).name());
    std::vector<std::string> ret(m_multiplicity, tname);
    return ret;
}


bool Gen::FrameFanout::operator()(const input_pointer& in, output_vector& outv)
{
    // Note: if "in" indicates EOS, just pass it on
    if (in) {
        std::cerr << "DepoSetFanout (" << m_multiplicity << ") fanout data\n";
    }
    else {
        std::cerr << "DepoSetFanout (" << m_multiplicity << ") fanout EOS\n";
    }

    outv.resize(m_multiplicity);
    for (size_t ind=0; ind<m_multiplicity; ++ind) {
        outv[ind] = in;
    }
    return true;
}


