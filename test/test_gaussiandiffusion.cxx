#include "WireCellGen/GaussianDiffusion.h"
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

void test_gd()
{
    const double tdepo = 1*units::ms;
    const double qdepo = 1000.0;
    const Point pdepo(10*units::cm, 0.0, 0.0);
    auto depo = std::make_shared<SimpleDepo>(tdepo, pdepo, qdepo);

    const Point w_origin(-3*units::mm, 0.0, -5*units::m);
    const Vector w_pdir(0.0, 0.0, 1.0);
    const double pbin = 0.3*units::mm;

    const double torigin = 0.0*units::us;
    const double tbin = 0.5*units::us;
    


    Gen::GaussianDiffusion gd(depo,
			      1.0*units::mm, 1.0*units::us,
			      w_origin, w_pdir, pbin,
			      0.0, tbin);
			 
    auto patch = gd.patch();

    int np = gd.npitches();
    int nt = gd.ntimes();

    Assert (np == patch.rows());
    Assert (nt == patch.cols());

    auto pdom = gd.minmax_pitch();
    pdom.first /= units::mm;	// put into explicit units
    pdom.second /= units::mm;	// put into explicit units
    auto tdom = gd.minmax_time();
    tdom.first /= units::us;	// put into explicit units
    tdom.second /= units::us;	// put into explicit units

    auto tpcenter = gd.center();
    tpcenter.first /= units::us;
    tpcenter.second /= units::mm;

    cerr << "time : " << nt << "@" << tpcenter.first << ": [" << tdom.first << "," << tdom.second << "] us\n";
    cerr << "pitch: " << np << "@" << tpcenter.second << ": [" << pdom.first << "," << pdom.second << "] mm\n";

    TPolyMarker* marker = new TPolyMarker(1);
    marker->SetPoint(0, tpcenter.first, tpcenter.second);
    marker->SetMarkerStyle(5);

    // UNIT CHANGE
    const double tbinus = tbin/units::us;
    const double pbinmm = pbin/units::mm;

    // we will leak this.
    TH2F* hist = new TH2F("patch1","Diffusion Patch",
			  nt, tdom.first-0.5*tbinus, tdom.second+0.5*tbinus,
			  np, pdom.first-0.5*pbinmm, pdom.second+0.5*pbinmm);
    hist->SetXTitle("time (us)");
    hist->SetYTitle("pitch (mm)");
    for (int it=0; it < nt; ++it) {
	const double time = tdom.first + float(it)*tbinus;
	for (int ip=0; ip < np; ++ip) {
	    const double pitch = pdom.first + float(ip)*pbinmm;
	    const double value = patch(ip,it);
	    //hist->Fill(time, pitch, value);
	    hist->SetBinContent(it+1, ip+1, value);
	}
    }
    hist->Write();
    hist->Draw("colz");
    marker->Draw();
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

    test_gd();
    canvas.Print(Form("%s.pdf",me),"pdf");

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	canvas.Print(Form("%s.pdf]",me),"pdf");
    }

    return 0;
}
