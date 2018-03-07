#include "WireCellGen/DumpDepos.h"
#include "WireCellUtil/NamedFactory.h"

#include <iostream>
#include <sstream>

WIRECELL_FACTORY(DumpDepos, WireCell::DumpDepos, WireCell::IDepoSink);

using namespace WireCell;
using namespace std;

DumpDepos::DumpDepos() : m_nin(0) {}

DumpDepos::~DumpDepos() {}

bool DumpDepos::operator()(const IDepo::pointer& depo)
{
    stringstream msg;		// reduce footprint for having stream split between different threads
    msg << "Depo: (" << (void*)depo.get() << ")";
    if (depo) {
	msg << " t=" << depo->time()
            << "\tq=" << depo->charge()
            << "\tr=" << depo->pos()
            << "\tn=" << m_nin
            << "\n";
        ++m_nin;
    }
    else {
	msg << " EOS after " << m_nin << " depos\n";
    }
    cerr << msg.str();
    return true;
}
