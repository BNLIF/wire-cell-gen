#include "WireCellGen/DumpDepos.h"
#include "WireCellUtil/NamedFactory.h"

#include <iostream>
#include <sstream>

WIRECELL_FACTORY(DumpDepos, WireCell::DumpDepos, WireCell::IDepoSink);

using namespace WireCell;
using namespace std;

bool DumpDepos::operator()(const IDepo::pointer& depo)
{
    stringstream msg;		// reduce footprint for having stream split between different threads
    msg << "Depo: (" << (void*)depo.get() << ")";
    if (depo) {
	msg << " t=" << depo->time() << "\tq=" << depo->charge() << "\tr=" << depo->pos() << "\n";
    }
    else {
	msg << " empty\n";
    }
    cerr << msg.str();
    return true;
}
