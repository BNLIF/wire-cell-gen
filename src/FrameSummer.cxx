#include "WireCellGen/FrameSummer.h"
#include "WireCellGen/FrameUtil.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(FrameSummer, WireCell::Gen::FrameSummer,
                 WireCell::IFrameJoiner, WireCell::IConfigurable)

using namespace WireCell;


Configuration Gen::FrameSummer::default_configuration() const
{
    // fixme: maybe add operators, scaleing, offsets.

    Configuration cfg;
    return cfg;
}

void Gen::FrameSummer::configure(const Configuration& cfg)
{

}

bool Gen::FrameSummer::operator()(const input_tuple_type& intup,
                                  output_pointer& out)
{
    auto one = std::get<0>(intup);
    auto two = std::get<1>(intup);
    if (!one or !two) {
        // assume eos
        out = nullptr;
        return true;
    }

    out = Gen::sum(IFrame::vector{one,two}, one->ident());
    return true;
}


Gen::FrameSummer::FrameSummer()
{
}

Gen::FrameSummer::~FrameSummer()
{
}
