#include "WireCellRootVis/CanvasApp.h"

#include "WireCellGen/Digitizer.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/WireGenerator.h"
#include "WireCellGen/WireSummary.h"

#include "WireCellIface/WirePlaneId.h"
#include "WireCellIface/SimplePlaneSlice.h"

#include "WireCellUtil/Testing.h"


#include <iostream>
#include <vector>

using namespace WireCell;
using namespace std;


IPlaneSlice::pointer make_planeslice(WirePlaneId wpid, double the_time)

{
    IPlaneSlice::WireChargeRun wcr1(3, {1.0,2.0,3.0,2.0,1.0});
    IPlaneSlice::WireChargeRunVector wcrv = { wcr1 };

    
    return IPlaneSlice::pointer(new SimplePlaneSlice(wpid, the_time, wcrv));
}

int main(int argc, char* argv[])
{
    IWireParameters::pointer iwp(new WireParams);
    WireGenerator wg;
    Assert(wg.insert(iwp));

    IWire::shared_vector wires;
    Assert(wg.extract(wires));
    Assert(wires);
    Assert(wires->size());

    WireSummary ws(*wires);

    Digitizer digitizer;
    digitizer.set_wires(wires);

    const double the_time = 11.0*units::microsecond;

    IPlaneSlice::shared_vector psv(new IPlaneSliceVector({
		make_planeslice(WirePlaneId(kUlayer), the_time),
		make_planeslice(WirePlaneId(kVlayer), the_time),
		make_planeslice(WirePlaneId(kWlayer), the_time)}));


    Assert(digitizer.insert(psv));

    IChannelSlice::pointer cs;
    Assert(digitizer.extract(cs));
    Assert(cs);

    ChannelCharge cc = cs->charge();
    Assert(!cc.empty());

    cerr << "ChannelSlice time = " << cs->time() << endl;
    Assert(cs->time() == the_time);

    for (auto cq : cc) {	// for each hit channel
	int channel = cq.first;
	Quantity q = cq.second;
	Assert(std::abs(sqrt(q.mean()) - q.sigma()) < 0.00000001);
	cerr << "channel " << channel << " q= " << cq.second << " wids:";
	// for each wire (segment) on that channel
	for (auto wire : ws.by_channel(cq.first)) {
	    cerr << " " << wire->ident();
	}
	cerr << endl;
    }

    Assert(digitizer.insert(nullptr));
    IChannelSlice::pointer eos;
    Assert(digitizer.extract(eos));
    Assert(!eos);


    return 0;
}
