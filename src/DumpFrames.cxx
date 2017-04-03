#include "WireCellGen/DumpFrames.h"

#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(DumpFrames, WireCell::Gen::DumpFrames, WireCell::IFrameSink);

using namespace std;
using namespace WireCell;

Gen::DumpFrames::DumpFrames()
{
}


Gen::DumpFrames::~DumpFrames()
{
}


bool Gen::DumpFrames::operator()(const IFrame::pointer& frame)
{
    auto traces = frame->traces();
    const int ntraces = traces->size();
    
    cerr << "Frame: #" << frame->ident()
         << " @" << frame->time()
         << " with " << ntraces << " traces" << endl;
    
}

