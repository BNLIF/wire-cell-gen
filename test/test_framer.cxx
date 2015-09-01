//#include "WireCellGen/Digitizer.h"
#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Drifter.h"
#include "WireCellGen/Digitizer.h"
#include "WireCellGen/TrackDepos.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/WireGenerator.h"
#include "WireCellGen/Framer.h"

#include "WireCellIface/WirePlaneId.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Faninout.h"
#include "WireCellUtil/Testing.h"

#include <iostream>

using namespace WireCell;
using namespace std;

TrackDepos make_tracks() {
    TrackDepos td;

    const double cm = units::cm;
    Ray same_point(Point(cm,-cm,cm), Point(cm,+cm,+cm));

    const double usec = units::microsecond;

    td.add_track(1*usec, same_point);
    td.add_track(10*usec, same_point);
    td.add_track(100*usec, same_point);
    td.add_track(1000*usec, same_point);
    td.add_track(10000*usec, same_point);

    return td;
}


int main()
{
    const double tick = 2.0*units::microsecond; 
    const int nticks_per_frame = 100;
    double now = 0.0*units::microsecond;

    IWireParameters::pointer iwp(new WireParams);

    WireGenerator wg;
    Assert(wg.sink(iwp));
    wg.process();
    IWireVector wires;
    Assert(wg.source(wires));

    TrackDepos td = make_tracks();

    WireCell::Drifter drift[3] = {
	WireCell::Drifter(iwp->pitchU().first.x()),
	WireCell::Drifter(iwp->pitchV().first.x()),
	WireCell::Drifter(iwp->pitchW().first.x())
    };

    PlaneDuctor pds[3] = {
	PlaneDuctor(WirePlaneId(kUlayer), iwp->pitchU(), tick, now),
	PlaneDuctor(WirePlaneId(kVlayer), iwp->pitchV(), tick, now),
	PlaneDuctor(WirePlaneId(kWlayer), iwp->pitchW(), tick, now)
    };
    
    Digitizer digitizer;
    digitizer.sink(wires);

    Framer framer(nticks_per_frame);

    AssertMsg(false, "Need to update this test");

    // while (true) {
    // 	auto depo = td();
    // 	if (!depo) { break; }
    // 	IDepoVector drifted_to_plane;
    // 	for (int iplane=0; iplane<3; ++iplane) {
    // 	    .......

    // 	    drift[iplane].sink(depo);
    // 	    drift[iplane].process();
    // 	    IDepo::pointer drifted;
    // 	    drift[iplane].process();
    // 	}
	

	

    // }

    // // Now connect up the nodes

    // // fan out the depositions
    // depofan.connect(td);

    // // address the fan-out
    // depoU.connect(boost::ref(depofan));
    // depoV.connect(boost::ref(depofan));
    // depoW.connect(boost::ref(depofan));

    // // drift each to the plane
    // driftU.connect(boost::ref(depoU));
    // driftV.connect(boost::ref(depoV));
    // driftW.connect(boost::ref(depoW));

    // // diffuse and digitize
    // pdU.connect(boost::ref(driftU));
    // pdV.connect(boost::ref(driftV));
    // pdW.connect(boost::ref(driftW));

    // // aggregate and digitize
    // digitizer.connect(boost::ref(pdU));
    // digitizer.connect(boost::ref(pdV));
    // digitizer.connect(boost::ref(pdW));

    // framer.connect(boost::ref(digitizer));

    // // process
    // while (true) {
    // 	auto frame = framer();
    // 	if (!frame) { break; }

    // 	int ntraces = boost::distance(frame->range());
    // 	cerr << "Frame: #" << frame->ident()
    // 	     << " at t=" << frame->time()/units::microsecond << " usec"
    // 	     << " with " << ntraces << " traces"
    // 	     << endl;
    // 	for (auto trace : *frame) {
    // 	    cerr << "\ttrace ch:" << trace->channel()
    // 		 << " start tbin=" << trace->tbin()
    // 		 << " #time bins=" << trace->charge().size()
    // 		 << endl;
    // 	}

    // }

    return 0;
}
