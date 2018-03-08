#include "WireCellGen/DumpDepos.h"
#include "WireCellUtil/NamedFactory.h"

#include <iostream>
#include <sstream>

WIRECELL_FACTORY(DumpDepos, WireCell::DumpDepos, WireCell::IDepoSink);

using namespace WireCell;
using namespace std;

DumpDepos::DumpDepos() : m_nin(0), m_eos(false) {}

DumpDepos::~DumpDepos() {}

bool DumpDepos::operator()(const IDepo::pointer& depo)
{
    if (m_eos) {
        // just handle EOS once
        return false;
    }

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
        m_eos = true;
	msg << " EOS after " << m_nin << " depos\n";
    }
    cerr << msg.str();
    return true;
}
