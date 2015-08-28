#include "WireCellGen/Framer.h"
#include "WireCellGen/SimpleFrame.h"
#include "WireCellGen/ZSEndedTrace.h"

#include <iostream>
using namespace std;
using namespace WireCell;

Framer::Framer(int nticks)
    : m_nticks(nticks)
    , m_count(0)
{
}
Framer::~Framer()
{
}



IFrame::pointer Framer::operator()()
{
    std::map<int, ZSEndedTrace*> ch2trace;

    double time = -1;
    for (int itick=0; itick < m_nticks; ++itick) {
	IChannelSlice::pointer chslice = *m_input();
	if (!chslice) {
	    cerr << "Out of channel slices at tick " << itick << " in frame " << m_count << endl;
	    break;
	}

	if (!itick) {
	    time = chslice->time();
	}

	IChannelSlice::ChannelCharge cc = chslice->charge();
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
	return nullptr;
    }

    ITraceVector traces;
    for (auto ct : ch2trace) {
	ZSEndedTrace* trace = ct.second;
	traces.push_back(ITrace::pointer(trace));
    }
    return IFrame::pointer(new SimpleFrame(m_count++, time, traces));
}
