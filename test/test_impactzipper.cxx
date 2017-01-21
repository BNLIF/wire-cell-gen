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
#include "TFile.h"
#include "TLine.h"
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
    em("loaded response");

    const char* uvw = "UVW";

    // 1D garfield wires are all parallel
    const double angle = 60*units::degree;
    const Vector upitch(0, -sin(angle),  cos(angle));
    const Vector uwire (0,  cos(angle),  sin(angle));
    const Vector vpitch(0,  cos(angle),  sin(angle));
    const Vector vwire (0, -sin(angle),  cos(angle));
    const Vector wpitch(0, 0, 1);
    const Vector wwire (0, 1, 0);
    Response::Schema::lie(fr.planes[0], upitch, uwire);
    Response::Schema::lie(fr.planes[1], vpitch, vwire);
    Response::Schema::lie(fr.planes[2], wpitch, wwire);


    // Origin where drift and diffusion meets field response.
    Point field_origin(fr.origin, 0, 0);
    cerr << "Field response origin: " << field_origin/units::mm << "mm\n";

    // Describe the W collection plane
    const int nwires = 2001;
    const double wire_pitch = 3*units::mm;
    const int nregion_bins = 10; // fixme: this should come from the Response::Schema.
    const double halfwireextent = wire_pitch * (nwires/2);
    cerr << "Max wire at pitch=" << halfwireextent << endl;

    std::vector<Pimpos> uvw_pimpos{
            Pimpos(nwires, -halfwireextent, halfwireextent,
                   uwire, upitch, field_origin, nregion_bins),
            Pimpos(nwires, -halfwireextent, halfwireextent,
                   vwire, vpitch, field_origin, nregion_bins),
            Pimpos(nwires, -halfwireextent, halfwireextent,
                   wwire, wpitch, field_origin, nregion_bins)};

    // Digitization and time
    const double t0 = 0.0*units::s; // eg, optical trigger from prompt interaction activity
    const double readout_time = 5*units::ms;
    const int nticks = 9600;
    const double tick = 0.5*units::us;
    const double drift_speed = 1.0*units::mm/units::us; // close but fake/arb value!
    Binning tbins(nticks, t0, t0+readout_time);

    // Diffusion
    const int ndiffision_sigma = 3.0;
    const bool fluctuate = true;

    // Generate some trivial tracks
    const double stepsize = 1*units::mm;
    TrackDepos tracks(stepsize);

    // Pick half bogus number ionized electrons per track distance
    const double dedx = 7.0e4/(1*units::cm); // in electrons
    const double charge_per_depo = -(dedx)*stepsize;

    // mostly "prolonged" track in X direction
    tracks.add_track(t0 + 1*units::ms,
                     Ray(Point(1*units::m, 0*units::m, 0*units::m),
                         Point(2*units::m, 0*units::m, 3*units::cm)), // over 10 wires
                     charge_per_depo);
    // mostly "isochronous" track in Z direction
    tracks.add_track(t0 + 2*units::ms,
                     Ray(Point(1*units::m, 0*units::m, -1*units::m),
                         Point(1*units::m + 100*units::us*drift_speed, 0*units::m, +1*units::m)),
                     charge_per_depo);
    // "driftlike" track diagonal in space and drift time
    tracks.add_track(t0 + 3*units::ms,
                     Ray(Point(1*units::m, 0*units::m, -1*units::m),
                         Point(2*units::m, 0*units::m, +1*units::m)),
                     charge_per_depo);
    em("made tracks");

    // Get depos
    auto depos = tracks.depos();
    std::cerr << "got " << depos.size() << " depos from tracks\n";
    em("made depos");

    TFile* rootfile = TFile::Open(Form("%s-uvw.root", argv[0]), "recreate");

    for (int plane_id = 0; plane_id < 3; ++plane_id) {
        Pimpos& pimpos = uvw_pimpos[plane_id];

        // add deposition to binned diffusion
        Gen::BinnedDiffusion bdw(pimpos, tbins, ndiffision_sigma, fluctuate);
        em("made BinnedDiffusion");
        for (auto depo : depos) {
            auto drifted = std::make_shared<Gen::TransportedDepo>(depo, field_origin.x(), drift_speed);

            const double sigma_time = 1.0*units::us; // fixme: should really be function of drift time
            const double sigma_pitch = 1.0*units::mm; // fixme: ditto
	
            bool ok = bdw.add(drifted, sigma_time, sigma_pitch);
            if (!ok) {
                std::cerr << "failed to add: t=" << drifted->time()/units::us << ", pt=" << drifted->pos()/units::mm << std::endl;
            }
            Assert(ok);

            // std::cerr << "depo:"
            //           << " q=" << drifted->charge()
            //           << " time-T0=" << (drifted->time()-t0)/units::us<< "us +/- " << sigma_time/units::us << " us "
            //           << " pt=" << drifted->pos() / units::mm << " mm\n";
            
        }
        em("added track depositions");

        PlaneImpactResponse pirw(fr, plane_id);
        em("made PlaneImpactResponse");

        Gen::ImpactZipper zipper(pirw, bdw);
        em("made ImpactZipper");

        auto pitch_mm = bdw.pitch_range();
        cerr << "Pitch sample range: ["<<pitch_mm.first/units::mm <<","<< pitch_mm.second/units::mm<<"]mm\n";

        const auto rb = pimpos.region_binning();
        const int minwire = rb.bin(pitch_mm.first);
        const int maxwire = rb.bin(pitch_mm.second);
        cerr << "Wires: [" << minwire << "," << maxwire << "]\n";

        std::map<int, Waveform::realseq_t> frame;
        double tottot=0.0;
        for (int iwire=minwire; iwire <= maxwire; ++iwire) {
            auto wave = zipper.waveform(iwire);
            auto tot = Waveform::sum(wave);
            tottot += tot;
            //std::cerr << iwire << " tot=" << tot << " tottot=" << tottot << std::endl;
            frame[iwire] = wave;
        }
        em("zipped through wires");
        cerr << "Tottot = " << tottot << endl;
        Assert(tottot > 0.0);

        auto tmm = bdw.time_range();
        const int tbin0 = tbins.bin(tmm.first);
        const int tbinf = tbins.bin(tmm.second);
        const int ntbins = 1+tbinf-tbin0;

        auto pmm = bdw.pitch_range();
        const int wbin0 = rb.bin(pmm.first);
        const int wbinf = rb.bin(pmm.second);
        const int nwbins = 1+wbinf-wbin0;
    
        //TCanvas canvas("c","canvas",500,500);
        gStyle->SetOptStat(0);
    

        TH2F *hist = new TH2F(Form("h%d", plane_id),
                              Form("Wire vs Tick %c-plane", uvw[plane_id]),
                              ntbins, tbin0, tbin0+ntbins,
                              nwbins, wbin0, wbin0+nwbins);
        hist->SetXTitle("tick");
        hist->SetYTitle("wire");

        std::cerr << nwbins << " wires: [" << wbin0 << "," << wbinf << "], "
                  << ntbins << " ticks: [" << tbin0 << "," << tbinf << "]\n";

        for (auto wire : frame) {
            const int iwire = wire.first;
            Assert(rb.inbounds(iwire));
            const Waveform::realseq_t& wave = wire.second;
            auto tot = Waveform::sum(wave);
            //std::cerr << iwire << " tot=" << tot << std::endl;
            for (int itick=tbin0; itick <= tbinf; ++itick) {
                Assert(tbins.inbounds(itick));
                hist->Fill(itick+0.1, iwire+0.1, wave[itick]);
            }
        }
        hist->Write();

        std::vector<TLine*> lines;
        auto trqs = tracks.tracks();
        for (int iline=0; iline<trqs.size(); ++iline) {
            auto trq = trqs[iline];
            const double time = get<0>(trq);
            const Ray ray = get<1>(trq);

            // this need to subtract off the fr.origin is I think a bug,
            // or at least a bookkeeping detail to ensconce somewhere.  I
            // think FR is taking the start of the path as the time
            // origin.  Something to check...
            const int tick1 = tbins.bin(time + (ray.first.x()-fr.origin)/drift_speed);
            const int tick2 = tbins.bin(time + (ray.second.x()-fr.origin)/drift_speed);
            
            const int wire1 = rb.bin(pimpos.distance(ray.first, plane_id));
            const int wire2 = rb.bin(pimpos.distance(ray.second, plane_id));
            
            cerr << "digitrack: t=" << time << " ticks=["<<tick1<<","<<tick2<<"] wires=["<<wire1<<","<<wire2<<"]\n";
            
            TLine* line = new TLine(tick1, wire1, tick2, wire2);
            line->Write(Form("l%c%d", uvw[plane_id], iline));
        }
    }
    rootfile->Close();
    em("done");

    cerr << em.summary() << endl;
    return 0;
}
