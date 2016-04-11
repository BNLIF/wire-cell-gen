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
    if (depo) {
	msg << "Depo: t=" << depo->time() << "\tq=" << depo->charge() << "\tr=" << depo->pos() << "\n";
    }
    else {
	msg << "Depo: empty\n";
    }
    cerr << msg.str();
    return true;
}
