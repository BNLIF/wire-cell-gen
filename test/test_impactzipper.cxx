#include "WireCellGen/ImpactZipper.h"
#include "WireCellGen/TrackDepos.h"
#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellUtil/ImpactResponse.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellUtil/ExecMon.h"
#include "WireCellUtil/Point.h"

#include "TCanvas.h"
#include "TStyle.h"
#include "TH2F.h"

#include <iostream>

using namespace WireCell;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "This test requires an Wire Cell Field Response input file." << std::endl;
	return 0;
    }

    WireCell::ExecMon em(argv[0]);
    auto fr = Response::Schema::load(argv[1]);
    em("loaded");

    // 1D garfield wires are all parallel
    const double angle = 60*units::degree;
    Vector upitch(0, -sin(angle),  cos(angle));
    Vector uwire (0,  cos(angle),  sin(angle));
    Vector vpitch(0,  cos(angle),  sin(angle));
    Vector vwire (0, -sin(angle),  cos(angle));
    Vector wpitch(0, 0, 1);
    Vector wwire (0, 1, 0);
    Response::Schema::lie(fr.planes[0], upitch, uwire);
    Response::Schema::lie(fr.planes[1], vpitch, vwire);
    Response::Schema::lie(fr.planes[2], wpitch, wwire);
    const int wplaneid = 2;
    PlaneImpactResponse pirw(fr,wplaneid);


    // diffusion
    const double t0 = 1.0*units::s;
    const int nticks = 9600;
    const double tick = 0.5*units::us;
    const double drift_speed = 1.0*units::mm/units::us;
    const int nwires = 1000;
    const int npmwires = 10;	// effective induction range in # of wire pitches
    const double wire_pitch = 3*units::mm;
    const int nimpacts_per_wire_pitch = 10;
    const double impact_pitch = wire_pitch/nimpacts_per_wire_pitch;
    const double z_half_width = 0.5*nwires*wire_pitch;
    const Point w_origin(-3*units::mm, 0.0, -z_half_width);
    const Vector w_pitchdir(0.0, 0.0, 1.0);
    const double min_time = t0;
    const double max_time = min_time + nticks*tick;
    const int ndiffision_sigma = 3.0;
    const bool fluctuate = false;

    Gen::BinnedDiffusion bdw(w_origin, w_pitchdir,
                             nwires*npmwires, -z_half_width, z_half_width,
                             nticks, t0, t0 + nticks*tick,
                             ndiffision_sigma, fluctuate);

    // tracks
    const double stepsize = 1*units::mm;
    TrackDepos tracks(stepsize);

    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_real_distribution<> rpos(-1*units::cm, 1*units::cm);
    std::uniform_real_distribution<> tpos(0*units::us, 10*units::us);

    const double dedx = 500.0/(1.0*units::cm);
    const int ntracks = 2;
    const double xoffset = 30*units::cm;
    for (int itrack = 0; itrack < ntracks; ++itrack) {
        Point p1(xoffset+rpos(re), rpos(re), rpos(re));
        Point p2(xoffset+rpos(re), rpos(re), rpos(re));
        tracks.add_track(tpos(re), Ray(p1,p2), dedx);
    }

    // Get depos
    auto depos = tracks.depos();
    std::cerr << "got " << depos.size() << " depos from "<<ntracks<<" tracks\n";
    const double DL=5.3*units::centimeter2/units::second;
    const double DT=12.8*units::centimeter2/units::second;


    // add deposition to binned diffusion
    for (auto depo : depos) {
        Point pt = depo->pos();
	double drift_time = pt.x()/drift_speed;
	pt.x(0);		// insta-drift

	const double tmpcm2 = 2*DL*drift_time/units::centimeter2;
	const double sigmaL = sqrt(tmpcm2)*units::centimeter / drift_speed;
	const double sigmaT = sqrt(2*DT*drift_time/units::centimeter2)*units::centimeter2;
	
	auto drifted = std::make_shared<SimpleDepo>(depo->time()+drift_time, pt, depo->charge());
	bdw.add(drifted, sigmaL, sigmaT);
    }

    // Zip!
    Gen::ImpactZipper zipper(pirw, bdw);

    std::vector<Waveform::realseq_t> wframe;
    int minwire = -1, maxwire=-1;
    for (int iwire=0; iwire<nwires; ++iwire) {
        auto wave = zipper.waveform(iwire);
        auto tot = Waveform::sum(wave);
        //std::cerr << iwire << " tot=" << tot << std::endl;
        if (minwire < 0 && tot > 0) {
            minwire = iwire;
        }
        if (tot > 0) {
            maxwire = iwire;
        }
        wframe.push_back(wave);
    }

    int mintick = -1, maxtick=-1;
    for (int iwire=minwire; iwire<=maxwire; ++iwire) {    
        auto wave = wframe[iwire];
        int lmintick=-1, lmaxtick=-1;
        for (int itick=0; itick<wave.size(); ++itick) {
            if (lmintick < 0 || wave[itick] > 0) {
                lmintick = itick;
            }
            if (wave[itick] > 0) {
                lmaxtick = itick;
            }
        }
        if (mintick<0 || lmintick < mintick) { mintick = lmintick; }
        if (maxtick<0 || lmaxtick > maxtick) { maxtick = lmaxtick; }
    }


    TCanvas canvas("c","canvas",500,500);

    gStyle->SetOptStat(0);

    TH2F hist("h","Pitch vs Time W-plane",
              maxtick-mintick, mintick, maxtick,
              maxwire-minwire, minwire, maxwire);
    std::cerr << "wires: [" << minwire << "," << maxwire << "], ticks: [" << mintick << "," << maxtick << "]\n";

    for (int iwire=minwire; iwire<=maxwire; ++iwire) {    
        auto wave = wframe[iwire];
        auto tot = Waveform::sum(wave);
        //std::cerr << iwire << " tot=" << tot << std::endl;
        for (int itick=mintick; itick <= maxtick; ++itick) {
            hist.Fill(itick+0.1, iwire+0.1, wave[itick]);
        }
    }

    hist.Draw("colz");
    canvas.Print(Form("%s.pdf", argv[0]));


    return 0;
}
