#include "WireCellGen/WireSummarizer.h"
#include "WireCellGen/WireSummary.h"
#include "WireCellUtil/NamedFactory.h"


using namespace WireCell;

WIRECELL_NAMEDFACTORY(WireSummarizer) {
    WIRECELL_NAMEDFACTORY_INTERFACE(WireSummarizer, IWireSummarizer);
}

WireSummarizer::WireSummarizer()
{
}

WireSummarizer::~WireSummarizer()
{
}

bool WireSummarizer::insert(const input_type& wires)
{
    if (!wires) {
	m_output = nullptr;
	return true;
    }
    m_output = output_type(new WireSummary(*wires));
    return true;
}

