#include "WireCellGen/WireSummarizer.h"
#include "WireCellGen/WireSummary.h"
#include "WireCellUtil/NamedFactory.h"


using namespace WireCell;

WIRECELL_NAMEDFACTORY(WireSummarizer);
WIRECELL_NAMEDFACTORY_ASSOCIATE(WireSummarizer, IWireSummarizer);

WireSummarizer::WireSummarizer()
{
}

WireSummarizer::~WireSummarizer()
{
}

void WireSummarizer::reset()
{
    m_output.clear();
}

void WireSummarizer::flush()
{
    m_output.push_back(eos());
    return;			// no input buffer.
}

bool WireSummarizer::insert(const input_type& wires)
{
    m_output.push_back(IWireSummary::pointer(new WireSummary(wires)));
    return true;
}

bool WireSummarizer::extract(output_type& out)
{
    if (m_output.empty()) {
	return false;
    }
    out = m_output.front();
    m_output.pop_front();
    return true;
}

