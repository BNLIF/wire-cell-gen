#include "WireCellGen/Diffuser.h"

#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Units.h"


#include "TApplication.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TStyle.h"
#include "TPolyMarker.h"

#include <iostream>

using namespace WireCell;
using namespace std;


void dump_smear(Diffusion::pointer smear)
{
    cerr << "L: [ " << smear->lmin << " , " << smear->lmax << " ] x " << smear->lbin << endl;
    cerr << "T: [ " << smear->tmin << " , " << smear->tmax << " ] x " << smear->tbin << endl;
    for (int tind = 0; tind < smear->tsize(); ++tind) {
	for (int lind = 0; lind < smear->lsize(); ++lind) {
	    cerr << "\t" << smear->array[lind][tind];
	}
	cerr << endl;
    }
}

void test_one()
{
    // binsize_l, binsize_t
    Diffuser diff(1000, 1);
    // mean_l, mean_t, sigma_l, sigma_t
    dump_smear(diff(20000, 20, 2000, 2));

    for (double mean = -100; mean <= 100; mean += 0.11) {
	Diffuser::bounds_type bb = diff.bounds(mean, 1.5, 1.0);
	//cerr << "mean=" << mean << " [" << bb.first << " --> " << bb.second << "]" << endl;
	Assert(bb.second - bb.first == 10.0);
    }
}


void test_plot_hist(TCanvas& canvas)
{
    canvas.Clear();

    const int nsigma = 3;
    const double drift_velocity = 1.6 * units::mm/units::microsecond;
    const double binsize_l = 0.5*units::microsecond*drift_velocity;
    const double binsize_t = 5*units::mm;

    const double sigma_l = 3.0*units::microsecond*drift_velocity;
    const double sigma_t = 3*units::mm;

    Diffuser diff(binsize_l, binsize_t);

    vector< Diffusion::pointer> diffs;
    vector< pair<double,double> > pts;

    // make a diagnonal
    for (double step=0; step<200; step += 5) {
    	double mean_l = (0.5+step)*binsize_l;
    	double mean_t = (0.5+step)*binsize_t;
    	diffs.push_back(diff(mean_l, mean_t, sigma_l, sigma_t));
    	pts.push_back(make_pair(mean_l, mean_t));
    }

    // and two isolated dots
    diffs.push_back(diff(10*binsize_l, 100*binsize_t, 3*sigma_l, 3*sigma_t, 10.0));
    diffs.push_back(diff(100*binsize_l, 10*binsize_t, 3*sigma_l, 3*sigma_t, 10.0));

    double min_l=0,min_t=0,max_l=0,max_t=0;
    for (auto d : diffs) {
	min_l = min(min_l, d->lmin);
	min_t = min(min_t, d->tmin);
	max_l = max(max_l, d->lmax);
	max_t = max(max_t, d->tmax);
    }


    TH2F* h = new TH2F("smear","Smear",
		       (max_l-min_l)/binsize_l, min_l, max_l,
		       (max_t-min_t)/binsize_t, min_t, max_t);
    for (auto smear : diffs) {
	for (int tind = 0; tind < smear->tsize(); ++tind) {
	    for (int lind = 0; lind < smear->lsize(); ++lind) {
		h->Fill(smear->lpos(lind), smear->tpos(tind), smear->array[lind][tind]);
	    }
	}
    }

    h->SetXTitle("Longitudinal direction");
    h->SetYTitle("Transverse direction");
    h->Draw("colz");

    TPolyMarker* pm = new TPolyMarker;
    pm->SetMarkerColor(5);
    pm->SetMarkerStyle(8);
    int count = 0;
    for (auto xy : pts) {
    	pm->SetPoint(count++, xy.first, xy.second);
    }
    pm->Draw();

    gStyle->SetOptStat(11111111);
    canvas.Print("test_diffuser.pdf","pdf");
}


int main(int argc, char* argv[])
{
    test_one();

    TCanvas canvas("c","c",500,500);
    canvas.Print("test_diffuser.pdf[","pdf");

    test_plot_hist(canvas);

    canvas.Print("test_diffuser.pdf]","pdf");
}
