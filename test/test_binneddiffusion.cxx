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

struct Meta {
    //TApplication* theApp = 0;

    TCanvas* canvas;
    ExecMon em;
    const char* name;

    Meta(const char* name)
    //: theApp(new TApplication (name,0,0))
	: canvas(new TCanvas("canvas","canvas", 500,500))
	, em(name)
	, name(name) {
	print("[");
    }

    void print(const char* extra = "") {
	string fname = Form("%s.pdf%s", name, extra);
	cerr << "Printing: " << fname << endl;
	canvas->Print(fname.c_str(), "pdf");
    }
};

void test_track(Meta& meta, double t0, const Ray& track, double stepsize, bool fluctuate)
{
    const int nwires = 1000;
    const int npmwires = 10;	// effective induction range in # of wire pitches
    const double wire_pitch = 3*units::mm;
    const int nimpacts_per_wire_pitch = 10;
    const double impact_pitch = wire_pitch/nimpacts_per_wire_pitch;

    const Point w_origin(-3*units::mm, 0.0, -1*units::m);
    const Vector impact_step(0.0, 0.0, impact_pitch);
    const Ray impact_ray(w_origin, w_origin+impact_step);

    const int nticks = 9600;
    const double tick = 0.5*units::us;
    const double min_time = 0.0;
    const double max_time = min_time + nticks*tick;
    const int ndiffision_sigma = 3.0;
    
    const double drift_speed = 1.0*units::mm/units::us;

    Gen::BinnedDiffusion bd(impact_ray, nticks, min_time, max_time, ndiffision_sigma, fluctuate);

    auto track_start = track.first;
    auto track_dir = ray_unit(track);
    auto track_length = ray_length(track);

    const double charge = 1000;

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


    for (int iwire = 0; iwire < nwires; ++iwire) {

	const int lo_wire = std::max(iwire-npmwires, 0);
	const int hi_wire = std::min(iwire+npmwires, nwires-1);
	const int lo_impact = int(round((lo_wire - 0.5) * nimpacts_per_wire_pitch));
	const int hi_impact = int(round((hi_wire + 0.5) * nimpacts_per_wire_pitch));

	std::vector<Gen::ImpactData::pointer> collect;

	for (int impact_number = lo_impact; impact_number <= hi_impact; ++impact_number) {
	    auto impact_data = bd.impact_data(impact_number);
	    if (impact_data) {
		collect.push_back(impact_data);
	    }
	}

	if (collect.empty()) {
	    continue;
	}
	
	bd.erase(0, lo_impact);

	auto one = collect.front();

	// find nonzero bounds int the awkward way possible.
	double min_pitch = 0.0;
	double max_pitch = 0.0;
	int min_tick = 0;
	int max_tick = 0;
	for (int ind=0; ind < collect.size(); ++ind ){
	    auto id = collect[ind];
	    auto mm = id->tick_bounds();
	    double pitch = id->pitch_distance();
	    if (!ind) {
		min_pitch = max_pitch = pitch;
		min_tick = mm.first;
		max_tick = mm.second;
		continue;
	    }
	    min_tick = std::min(min_tick, mm.first);
	    max_tick = std::max(max_tick, mm.second);
	    min_pitch = std::min(min_pitch, pitch);
	    max_pitch = std::max(max_pitch, pitch);
	}

	double min_pitch_mm = min_pitch/units::mm;
	double max_pitch_mm = max_pitch/units::mm;
	double min_time_us = (min_tick-0.5)*tick/units::us;
	double max_time_us = (max_tick+0.5)*tick/units::us;
	double num_ticks = max_tick - min_tick + 1;

	cerr << "Histogram: t=[" << min_time_us << "," << max_time_us << "]x" << num_ticks << " "
	     << "p=[" << min_pitch_mm << "," << max_pitch_mm << "]x" << collect.size() << "\n";

	TH2F hist("h","h", num_ticks, min_time_us, max_time_us, collect.size(), min_pitch_mm, max_pitch_mm);
	hist.SetTitle(Form("Diffused charge for wire %d", iwire));
	hist.SetXTitle("time (us)");
	hist.SetYTitle("pitch (mm)");

	for (auto id : collect) {
	    auto wave = id->waveform();
	    double pitch_distance_mm = id->pitch_distance()/units::mm;
	    cerr << "\t" << id->impact_number() << "@" << pitch_distance_mm << " x " << wave.size() <<  endl;
	    Assert (wave.size() == nticks);
	    auto mm = id->tick_bounds();
	    for (int itick=mm.first; itick<mm.second; ++itick) {
		const double time_us = (itick * tick)/units::us;
		hist.Fill(time_us, pitch_distance_mm, wave[itick]);
	    }
	}
	hist.Draw("colz");
	meta.print();
    }
}

int main(int argc, char* argv[])
{
    const char* me = argv[0];

    Meta meta(me);
    gStyle->SetOptStat(0);

    const double t0 = 0*units::us;
    Ray track(Point(1*units::m, -1*units::cm, -1*units::cm),
	      Point(1*units::m, +1*units::cm, +1*units::cm));
    const double stepsize = 1*units::cm;
    test_track(meta, t0, track, stepsize, false);

    meta.print("]");

    return 0;
}
