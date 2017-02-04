#include "WireCellUtil/Configuration.h"
#include "WireCellUtil/Response.h"
#include "WireCellUtil/Units.h"

#include "TFile.h"

#include <iostream>
#include <string>

using namspace WireCell;

int main(int argc, char *argv[])
{
    if (argc<4) {
        std::cerr << "usage: test_hits2waves <cfg.json> <garfield.json[.bz2]> <output.root>" << std::endl;
        return 1;
    }

    // Warning: this is abusing the configuration by being so
    // monolithic.  It's just for this test!  Do not emulate!
    auto cfg = configuration_load(argv[1]);
    auto fr = Response::Schema::load(argv[2]);
    TFile* rootfile = TFile::Open(argv[3], "recreate");

    const char* uvw = "UVW";

    // 2D garfield wires are all parallel so we "lie" them into 3D
    const double angle = 60*units::degree;
    std::vector<Vector> uvw_pitch{Vector(0, -sin(angle),  cos(angle));
                                  Vector(0,  cos(angle),  sin(angle)),
                                  Vector(0, 0, 1)};
    std::vector<Vector> uvw_wire{Vector(0,  cos(angle),  sin(angle)),
                                 Vector(0, -sin(angle),  cos(angle)),
                                 Vector(0, 1, 0)}
    
    // Origin where drift and diffusion meets field response.
    Point field_origin(fr.origin, 0, 0);
    cerr << "Field response origin: " << field_origin/units::mm << "mm\n";

    // load in configuration parameters
    const double t0 = get(cfg, "t0", 0.0*units::ns);
    const double readout_time = get(cfg, "readout", 5*units::ms);
    const double tick = get(cfg, "tick", 0.5*units::us);
    const int nticks = readout_time/tick;
    const double drift_speed = get(cfg,"speed",1.0*units::mm/units::us);
    Binning tbins(nticks, t0, t0+readout_time);
    const double gain = get(cfg, "gain", 14.7);
    const double shaping  = get(cfg, "shaping", 2.0*units::us);


    // load in configured hits
    WireCell::IDepo::vector orig_depos;
    for (auto hit : cfg["hits"]) {
        auto depo = make_shared<SimpleDepo>(get(cfg,"t"),
                                            Point(get(cfg,"x",0.0), get(cfg,"y",0.0), get(cfg("z"), 0.0)),
                                            get(cfg,"q",1000.0));
        orig_depos.push_back(depo);
    }


    // Do a dumb, simple uniform drift.
    // In a real app this would be a WCT "node".
    // Warning: here we assume goofy implicit units!
    const DL = get(cfg, "DL", 5.3) * units::centimeter2/units::second;
    const DT = get(cfg, "DT", 12.8) * units::centimeter2/units::second;
    WireCell::IDepo::vector drifted_depos;
    for (auto depo : orig_depos) {
        Point here = depo->pos();
        const double dt = drift_speed/(here.x() - field_origin.x());
        const double now = depo->time() + dt;
        here.x(field_origin.x());

        const double tmpcm2 = 2*DL*dt/units::centimeter2;
        //const double sigmaL = sqrt(tmpcm2)*units::centimeter / drift_speed;
        const double sigmaL = sqrt(tmpcm2)*units::centimeter;
        const double sigmaT = sqrt(2*DT*dt/units::centimeter2)*units::centimeter2;

        auto drifted = std::make_shared<Gen::DiffusedDepo>(depo, here, now, sigmaL, sigmaT);
        drifted_depos.push_back(drifted);
    }

    // final drift sim
    for (int iplane=0; iplane<3; ++iplane) {
        auto& pr = fr.planes[iplane];
        Response::Schema::lie(pr, uvw_pitch[iplane], uvw_wire[iplane]);
        const double wire_pitch = fr.pitch;
        const double impact_pitch = pr.paths[1].pitchpos - pr.paths[0].pitchpos;
        const int nregion_bins = round(wire_pitch/impact_pitch);
        const int nwires = cfg["nwires"][iplane];
        const double halfwireextent = wire_pitch * (nwires/2);

        Pimpos pimpos(nwires, -halfwireextent, halfwireextent,
                      uvw_wire[iplane], uvw_pitch[iplane],
                      field_origin, nregion_bins);

        std::cerr << "Plane " << iplane << ": nwires=" << nwires
                  << " half extent:" << halfwireextent
                  << " nimpacts/region: " << nregion_bins << std::endl;
        
        Gen::BinnedDiffusion bindiff(pimpos, tbins, ndiffision_sigma, fluctuate);
        for (auto depo :: drifted_depos) {
            bindiff->add(depo, depo->extent_long() / drift_speed, depo->extent_tran());
        }

        PlaneImpactResponse pir(fr, iplane, tbins, gain, shaping);

        Gen::ImpactZipper zipper(pir, bindiff);

        for (int iwire=0; iwire<nwires; ++iwire) {
            auto wave = zipper.waveform(iwire);
            /// fixme, about here we need to start thinking about
            /// output format.  also noise and digitizing.
        }
    }

    return 0;
}
    
