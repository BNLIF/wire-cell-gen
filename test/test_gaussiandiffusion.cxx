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

void test_gd(bool fluctuate)
{
    const double nsigma = 3.0;

    const double t_center = 1*units::ms;
    const double t_sigma = 1*units::us;
    const double t_sample = 0.5*units::us;
    const double t_min = t_center - nsigma*t_sigma;
    const int nt_start = int(round(t_min/t_sample));
    const int nt = int(2*nsigma*t_sigma/t_sample);

    Gen::GausDesc tdesc(t_center, t_sigma, t_sample, nt, nt_start);

    const double p_center = 1*units::m;
    const double p_sigma = 1*units::mm;
    const double p_sample = 0.3*units::mm;
    const double p_min = p_center - nsigma*p_sigma;
    const int np_start = int(round(p_min/p_sample));
    const int np = int(2*nsigma*p_sigma/p_sample);

    Gen::GausDesc pdesc(p_center, p_sigma, p_sample, np, np_start);

    cerr << "nt=" << nt << " np=" << np << endl;

    const double qdepo = 1000.0;
    const Point pdepo(10*units::cm, 0.0, p_center);
    auto depo = std::make_shared<SimpleDepo>(t_center, pdepo, qdepo);

    Gen::GaussianDiffusion gd(depo, tdesc, pdesc, fluctuate);
			 
    auto patch = gd.patch();

    cerr << "rows=" << patch.rows() << " cols=" << patch.cols() << endl;
    Assert (nt == patch.cols());
    Assert (np == patch.rows());

    const double tunit = units::us;	// for display
    const double punit = units::mm;	// for display

    TPolyMarker* marker = new TPolyMarker(1);
    marker->SetPoint(0, t_center/tunit, p_center/punit);
    marker->SetMarkerStyle(5);
    cerr << "center t=" << t_center/tunit << ", p=" << p_center/punit << endl;

    auto tbinning = tdesc.binned_extent();
    auto pbinning = pdesc.binned_extent();
    TH2F* hist = new TH2F("patch1","Diffusion Patch",    
			  nt, tbinning.first/tunit, tbinning.second/tunit,
			  np, pbinning.first/punit, pbinning.second/punit);// we will leak this.

    hist->SetXTitle("time (us)");
    hist->SetYTitle("pitch (mm)");
    for (int it=0; it < nt; ++it) {
	for (int ip=0; ip < np; ++ip) {
	    const double value = patch(ip,it);
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

    test_gd(false);
    canvas.Print(Form("%s.pdf",me),"pdf");
    test_gd(true);
    canvas.Print(Form("%s.pdf",me),"pdf");

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	canvas.Print(Form("%s.pdf]",me),"pdf");
    }

    return 0;
}
