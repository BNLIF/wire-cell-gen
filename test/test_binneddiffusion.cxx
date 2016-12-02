#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellUtil/ExecMon.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TFile.h"
#include "TH2F.h"
#include "TPolyMarker.h"

#include <iostream>

using namespace WireCell;
using namespace std;

void test_track(double t0, const Ray& track, double stepsize, bool fluctuate)
{
    const double nsigma = 3.0;

    const Point w_origin(-3*units::mm, 0.0, -1*units::m);
    const Vector impact_step(0.0, 0.0, 0.3*units::mm);
    const Ray impact_ray(w_origin, w_origin+impact_step);

    const int nticks = 9600;
    const double tick = 0.5*units::us;
    const double min_tick = 0.0;
    const double max_tick = min_tick + nticks*tick;
    const int ndiffision_sigma = 3.0;
    
    const double drift_speed = 1.6*units::mm/units::us;

    Gen::BinnedDiffusion bd(impact_ray, nticks, min_tick, max_tick, ndiffision_sigma, fluctuate);

    auto track_start = track.first;
    auto track_dir = ray_unit(track);
    auto track_length = ray_length(track);

    const double charge = 1000;	// fixme: how about fluctuate this....

    const double DL=5.3*units::centimeter2/units::second;
    const double DT=12.8*units::centimeter2/units::second;

    for (double dist=0.0; dist < track_length; dist += stepsize) {
	auto pt = track_start + dist*track_dir;
	double drift_time = pt.x()/drift_speed;
	pt.x(0);		// insta-drift

	const double tmpcm2 = 2*DL*drift_time/units::centimeter2;
	const double sigmaL = sqrt(tmpcm2)*units::centimeter / drift_speed;
	const double sigmaT = sqrt(2*DT*drift_time/units::centimeter2)*units::centimeter2;
	
	auto depo = std::make_shared<SimpleDepo>(t0+drift_time, pt, charge);
	bd.add(depo, sigmaL, sigmaT);
    }

    //.....

}

int main(int argc, char* argv[])
{
    const char* me = argv[0];

    TApplication* theApp = 0;
    if (argc > 1) {
	theApp = new TApplication (me,0,0);
    }
    TFile output(Form("%s.root", me), "RECREATE");
    TCanvas canvas("canvas","canvas",500,500);
    canvas.Print(Form("%s.pdf[", me),"pdf");
    gStyle->SetOptStat(0);

    //test_track(...);
    canvas.Print(Form("%s.pdf",me),"pdf");

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	canvas.Print(Form("%s.pdf]",me),"pdf");
    }

    return 0;
}
