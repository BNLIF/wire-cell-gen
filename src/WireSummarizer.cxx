#include "WireCellGen/WireSummarizer.h"
#include "WireCellGen/WireSummary.h"
#include "WireCellUtil/NamedFactory.h"


using namespace WireCell;

WIRECELL_NAMEDFACTORY_BEGIN(WireSummarizer)
WIRECELL_NAMEDFACTORY_INTERFACE(WireSummarizer, IWireSummarizer);
WIRECELL_NAMEDFACTORY_END(WireSummarizer)

WireSummarizer::WireSummarizer()
{
}

WireSummarizer::~WireSummarizer()
{
}

bool WireSummarizer::operator()(const input_pointer& wires, output_pointer& ws)
{
    if (!wires) {
	ws = nullptr;
	return true;
    }
    ws = output_pointer(new WireSummary(*wires));
    return true;
}

