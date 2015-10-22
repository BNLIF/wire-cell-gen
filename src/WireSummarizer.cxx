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

bool WireSummarizer::insert(const input_pointer& wires)
{
    if (!wires) {
	m_output = nullptr;
	return true;
    }
    m_output = output_pointer(new WireSummary(*wires));
    return true;
}

