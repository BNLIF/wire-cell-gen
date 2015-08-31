#include "WireCellGen/WireSummarizer.h"
#include "WireCellGen/WireSummary.h"
#include "WireCellUtil/NamedFactory.h"


using namespace WireCell;

WIRECELL_NAMEDFACTORY(WireSummarizer);
WIRECELL_NAMEDFACTORY_ASSOCIATE(WireSummarizer, IWireSummarizer);

WireSummarizer::WireSummarizer()
    : m_ws(nullptr)
{
}

WireSummarizer::~WireSummarizer()
{
}

bool WireSummarizer::sink(const IWireVector& wires)
{
    m_ws = IWireSummary::pointer(new WireSummary(wires));
    return true;
}

