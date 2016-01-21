#include "WireCellGen/Framer.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellGen/ZSEndedTrace.h"

#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(Framer, WireCell::Framer, WireCell::IFramer);

using namespace std;
using namespace WireCell;


Framer::Framer()		// fixme: needed for factory
    : m_nticks(100)
    , m_count(0)
{
}

Framer::Framer(int nticks)
    : m_nticks(nticks)
    , m_count(0)
{
}
Framer::~Framer()
{
}


void Framer::reset()
{
    m_input.clear();
}

bool Framer::operator()(const input_pointer& channel_slice, output_queue& outq)
{
    if (!channel_slice) { // EOS flush
	process(outq, true);
	outq.push_back(nullptr);
	return true;
    }

    m_input.push_back(channel_slice);
    process(outq, false);
    return true;
}

void Framer::process(output_queue& outq, bool flush)
{
    // make as many frames as we can, including final drain
    while (m_input.size() && (flush || m_input.size() >= m_nticks)) {

	std::map<int, ZSEndedTrace*> ch2trace;
	double time = -1;

	// pick up at most nticks.
	for (int itick=0; itick < m_nticks && m_input.size(); ++itick) {
	    IChannelSlice::pointer chslice = m_input.front();
	    m_input.pop_front();

	    if (!itick) {		// first time through
		time = chslice->time();
	    }

	    auto cc = chslice->charge();
	    for (auto chq : cc) {
		int chn = chq.first;
		double charge = chq.second;
		ZSEndedTrace* zst = ch2trace[chn];
		if (!zst) {
		    zst = new ZSEndedTrace(chn);
		    ch2trace[chn] = zst;
		}
		(*zst)(itick, charge);
	    }	
	}
	if (time < 0) {		// failed to get any channel slices
	    cerr << "Framer: got bogus data" << endl;
	    continue;
	}

	ITrace::vector traces;
	for (auto ct : ch2trace) {
	    ZSEndedTrace* trace = ct.second;
	    traces.push_back(ITrace::pointer(trace));
	}
	IFrame::pointer newframe(new SimpleFrame(m_count++, time, traces));
	outq.push_back(newframe);
    }
}

