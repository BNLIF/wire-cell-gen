#include "WireCellGen/ImpactZipper.h"
#include "WireCellGen/TrackDepos.h"
#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellGen/TransportedDepo.h"
#include "WireCellUtil/ImpactResponse.h"
#include "WireCellUtil/ExecMon.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Binning.h"
#include "WireCellUtil/Testing.h"

#include "TCanvas.h"
#include "TStyle.h"
#include "TH2F.h"

#include <iostream>

using namespace WireCell;
using namespace std;


int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "This test requires an Wire Cell Field Response input file." << std::endl;
	return 0;
    }

    WireCell::ExecMon em(argv[0]);
    auto fr = Response::Schema::load(argv[1]);
    std::cerr << em("loaded response") << std::endl;

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
    std::cerr << em("made PlaneImpactResponse") << std::endl;

    // Origin where drift and diffusion meets field response.
    Point field_origin(fr.origin, 0, 0);

    // Describe the W collection plane
    const int nwires = 2000;
    const double wire_pitch = 3*units::mm;
    const int nregion_bins = 10; // fixme: this should come from the Response::Schema.
    const double halfwireextent = wire_pitch * 0.5*nwires;

    Pimpos pimpos(nwires, -halfwireextent, halfwireextent,
                  wwire, wpitch, field_origin, nregion_bins);


    // Digitization and time
    const double trigger_time = 1.0*units::s; // eg, optical trigger from prompt interaction activity
    const double readout_time = 5*units::ms;
    const int nticks = 9600;
    const double tick = 0.5*units::us;
    const double drift_speed = 1.0*units::mm/units::us; // close but fake/arb value!
    Binning tbins(nticks, trigger_time, trigger_time+readout_time);

    // Diffusion
    const int ndiffision_sigma = 3.0;
    const bool fluctuate = false;

    // Generate some trivial tracks
    const double stepsize = 1*units::mm;
    TrackDepos tracks(stepsize);

    // random "box" in space and time
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_real_distribution<> rpos(-1*units::cm, 1*units::cm);
    std::uniform_real_distribution<> tpos(0*units::us, 10*units::us);
    const Point interaction_box_center(1*units::m, 0*units::m, 0*units::m);

    // ionized electrons per track distance
    const double dedx = 500.0/(1.0*units::cm);
    const int ntracks = 2;
    for (int itrack = 0; itrack < ntracks; ++itrack) {
        Point p1(rpos(re), rpos(re), rpos(re));
        Point p2(rpos(re), rpos(re), rpos(re));
        double time = tpos(re);
        std::cerr << "track #"<<itrack << "\n"
                  << "\tt=" << trigger_time/units::s << "s + " << time/units::us << "us\n"
                  << "\tp1=" << p1/units::mm << "mm\n"
                  << "\tp2=" << p2/units::mm << "mm\n"
                  << "\t c=" << interaction_box_center/units::mm << "mm\n"
                  << std::endl;
        const Ray ray(p1+interaction_box_center,p2+interaction_box_center);
        tracks.add_track(time+trigger_time, ray, dedx);
    }
    std::cerr << em("made tracks") << std::endl;

    // Get depos
    auto depos = tracks.depos();
    std::cerr << "got " << depos.size() << " depos from "<<ntracks<<" tracks\n";
    std::cerr << em("made tracks") << std::endl;

    // add deposition to binned diffusion
    Gen::BinnedDiffusion bdw(pimpos, tbins, ndiffision_sigma, fluctuate);

    std::cerr << em("made BinnedDiffusion") << std::endl;
    for (auto depo : depos) {
        auto drifted = std::make_shared<Gen::TransportedDepo>(depo, 0.0, drift_speed);

	const double sigma_time = 1.0*units::us; // fixme: should really be function of drift time
        const double sigma_pitch = 1.0*units::mm; // fixme: ditto
	
	bool ok = bdw.add(drifted, sigma_time, sigma_pitch);
        if (!ok) {
            std::cerr << "failed to add: t=" << drifted->time()/units::us << ", pt=" << drifted->pos()/units::mm << std::endl;
        }
        Assert(ok);

        std::cerr << "depo:  time=1s+" << (depo->time()-1*units::s)/units::us<< "us +/- " << sigma_time/units::us << " us "
                  << " pt=" << depo->pos() / units::mm << " mm\n";
        
        std::cerr << "drift: time=1s+" << (drifted->time()-1*units::s)/units::us<< "us +/- " << sigma_time/units::us << " us "
                  << " pt=" << drifted->pos() / units::mm << " mm\n";

    }
    std::cerr << em("added track depositions") << std::endl;

    // Zip!
    Gen::ImpactZipper zipper(pirw, bdw);
    std::cerr << em("made ImpactZipper") << std::endl;

    auto pitch_mm = bdw.pitch_range();
    cerr << "Pitch sample range: ["<<pitch_mm.first/units::mm <<","<< pitch_mm.second/units::mm<<"]mm\n";

    const auto rb = pimpos.region_binning();
    const int minwire = rb.bin(pitch_mm.first);
    const int maxwire = rb.bin(pitch_mm.second);
    cerr << "Wires: [" << minwire << "," << maxwire << "]\n";

    std::vector<Waveform::realseq_t> wframe;
    double tottot=0.0;
    for (int iwire=minwire; iwire<maxwire; ++iwire) {
        auto wave = zipper.waveform(iwire);
        auto tot = Waveform::sum(wave);
        tottot += tot;
        std::cerr << iwire << " tot=" << tot << " tottot=" << tottot << std::endl;
        wframe.push_back(wave);
    }
    std::cerr << em("zipped through wires") << std::endl;
    Assert(tottot > 0.0);


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
