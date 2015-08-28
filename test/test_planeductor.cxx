#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Drifter.h"
#include "WireCellGen/TrackDepos.h"

#include "WireCellIface/WirePlaneId.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Testing.h"

#include <iostream>
#include <algorithm> 

using namespace WireCell;
using namespace std;



int main()
{
    // Describe a wire plane is at x=10 with 1 cm pitch
    const double plane_x = 1*units::cm;
    const double plane_pitch = 1*units::cm;
    const double plane_half_width = 10*units::cm;
    const double drift_velocity = 1.0*units::millimeter/units::microsecond;
    const double tick = 1.0*units::microsecond;

    Ray pitch(Point(plane_x,0,-plane_half_width),
	      Point(plane_x,0,-(plane_half_width - plane_pitch)));

    cerr << "Pitch: " << pitch.first << " --> " << pitch.second 
	 << " pitch=" << ray_length(pitch) 
	 << " total wires=" << (2*plane_half_width/plane_pitch)
	 << endl ;
    cerr << "Drift: to x=" << plane_x
	 << " tau=" << plane_x / drift_velocity 
	 << " velocity=" << drift_velocity << endl;

    TrackDepos td;

    const double cm = units::cm;
    td.add_track(1.0*units::microsecond, Ray(Point(plane_x+1*cm, 0, 0),       Point(plane_x+3*cm,0,0)));
    td.add_track(2.0*units::microsecond, Ray(Point(plane_x+2*cm,-1*cm,-1*cm), Point(plane_x+2*cm,cm,cm)));
    td.add_track(5.0*units::microsecond, Ray(Point(plane_x+20*cm,-10*cm,-10*cm), Point(plane_x+10*cm,10*cm,10*cm)));
    td.add_track(10.0*units::microsecond, Ray(Point(plane_x+10*cm,-10*cm,-10*cm), Point(plane_x+10*cm,10*cm,10*cm)));


    double now = 0.0*units::microsecond;
    PlaneDuctor pd(WirePlaneId(kUnknownLayer), pitch, tick, now, drift_velocity);

    
    for (auto depo : *td.depositions()) {
	double tau = pd.proper_tau(depo->time(), depo->pos().x());
	double pdist = pd.pitch_dist(depo->pos());
	//cerr << "Depp: pos=" << depo->pos() << " time=" << depo->time() << " tau=" << tau << " pdist=" << pdist << endl;
	AssertMsg(tau>0, "Negative tau");
    }

    WireCell::Drifter drifter(plane_x, drift_velocity);
    drifter.connect(td);


    pd.connect(boost::ref(drifter));

    while (now < 10*units::microsecond) {
	IPlaneSlice::pointer ps = pd();
	if (!ps) {
	    cerr << "\nNo more data.\n";
	    break;
	}

	double clock = ps->time();
	WirePlaneId wpid = ps->planeid();
	IPlaneSlice::ChargeGrouping cg = ps->charge_groups();

	if (!cg.size()) { continue; }

	cerr << "Clock: " << clock << " plane: " << wpid << " got " << cg.size() << " groups" << endl;

	for (auto group : cg) {
	    cerr << "\t[@" << group.first <<"/"<<group.second.size() <<"]";
	    for (auto q : group.second) {
		cerr << " " << q;
	    }
	    cerr << endl;
	}
    }

}
