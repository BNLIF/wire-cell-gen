#include "WireCellGen/Framer.h"

#include "WireCellIface/SimpleChannelSlice.h"

#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Units.h"

#include <iostream>

using namespace WireCell;
using namespace std;

int main()
{

    Framer framer;

    ChannelCharge cc;
    cc[1] = 1.0;
    cc[2] = 2.0;
    cc[3] = 3.0;
    const double the_time = 11.0*units::microsecond;
    IChannelSlice::pointer cs(new SimpleChannelSlice(the_time, cc));

    Assert(framer.insert(cs));
    Assert(framer.insert(nullptr));

    IFrame::pointer frame;
    Assert(framer.extract(frame));
    Assert(frame);

    Assert(frame->time() == the_time);
    ITrace::shared_vector traces = frame->traces();
    Assert(traces);
    Assert(!traces->empty());
    int ntraces = traces->size();

    cerr << "test_framer: Frame: #" << frame->ident()
	 << " at t=" << frame->time()/units::microsecond << " usec"
	 << " with " << ntraces << " traces"
	 << endl;

    for (auto trace : *traces) {
	cerr << "\ttrace ch:" << trace->channel()
	     << " start tbin=" << trace->tbin()
	     << " #time bins=" << trace->charge().size()
	     << endl;
    }

    IFrame::pointer eos;
    Assert(framer.extract(eos));
    Assert(!eos);

    return 0;
}
