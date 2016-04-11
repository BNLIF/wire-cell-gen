#ifndef WIRECELL_DUMPDEPOS
#define WIRECELL_DUMPDEPOS

#include "WireCellIface/IDepoSink.h"

namespace WireCell {

    class DumpDepos : public IDepoSink {
    public:
	virtual bool operator()(const IDepo::pointer& depo);
    };

}

#endif
