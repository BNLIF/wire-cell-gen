#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Diffusion.h"

#include "WireCellIface/WirePlaneId.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Units.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TArrow.h"
#include "TLine.h"
#include "TStyle.h"


#include <iostream>
#include <algorithm> 

using namespace WireCell;
using namespace std;



void draw_diffusion(TVirtualPad* pad, IDiffusion::pointer diff)
{
    TH2F* hist = new TH2F("diff","Diffusion Patch",
			  diff->lsize(), diff->lbegin(), diff->lend(),
			  diff->tsize(), diff->tbegin(), diff->tend());
    hist->SetXTitle("Longitudinal (time) dimension");
    hist->SetYTitle("Transverse (wire) dimension");
    
    for (int tind=0; tind<diff->tsize(); ++tind) {
	const double t = diff->tpos(tind, 0.5);
	for (int lind=0; lind<diff->lsize(); ++lind) {
	    const double l = diff->lpos(lind, 0.5);
	    double v = diff->get(lind, tind);
	    hist->Fill(l,t,v);
	}
    }
    hist->Draw("colz");
}

int main(int argc, char* argv[])
{
    const int nlong = 10;
    const int ntrans = 10;
    const double lbin = 0.5*units::microsecond;
    const double tbin = 5.0*units::millimeter;
    const double lpos0 = 13*lbin;
    const double tpos0 = 10*tbin;

    const double lmean = lpos0 + 0.5*nlong*lbin;
    const double lsigma = 3*lbin;
    const double tmean = tpos0 + 0.5*ntrans*tbin;
    const double tsigma = 3*tbin;

    TApplication* theApp = 0;
    if (argc > 1) {
	theApp = new TApplication ("test_planeductor",0,0);
    }
    TCanvas canvas("c","c",500,500);
    canvas.Divide(2,2);

    Diffusion* diffusion =
	new Diffusion(nullptr, nlong, ntrans, lpos0, tpos0, lpos0+nlong*lbin, tpos0+ntrans*tbin);
    for (int tind=0; tind<ntrans; ++tind) {
	const double t = tpos0 + (tind+0.5)*tbin; // bin center
	double tx = (t-tmean)/tsigma;
	tx *= tx;
	for (int lind=0; lind<nlong; ++lind) {
	    const double l = lpos0 + (lind+0.5)*lbin; // bin center
	    double lx = (l-lmean)/lsigma;
	    lx *= lx;

	    const double v = exp(-(tx + lx));
	    cerr << "diffusion->set("<<lind<<" , " <<tind<< " , " << v << ")"
		 << " tx=" << tx << " lx=" << lx
		 << endl;
	    diffusion->set(lind,tind,v);
	}
    }
    IDiffusion::pointer idiff(diffusion);
    cerr << "test_planeductor: made IDiffusion patch: "
	 << " l: "<<idiff->lsize()<<": ["<<idiff->lbegin()<<" --> " << idiff->lend() << "]"
	 << " t: "<<idiff->tsize()<<": ["<<idiff->tbegin()<<" --> " << idiff->tend() << "]"
	 << endl;


    draw_diffusion(canvas.cd(1), idiff);


    const WirePlaneId wpid(kWlayer);

    const int nwires = tpos0/tbin + ntrans;
    PlaneDuctor pd(wpid, nwires, lbin, tbin); // start at lpos=tpos=0.0.

    IPlaneDuctor::output_queue psq;
    Assert(pd(idiff, psq));
    Assert(pd(nullptr, psq));	// EOS flush

    TH2F* pshist = new TH2F("ps","PlaneSlices vs Time",
			    100, 0, 100,
			    100, 0, 100);
    pshist->SetXTitle("Time slice number");
    pshist->SetYTitle("Wire index");    

    double now = 0.0;
    int nslices = 0;
    int time_index = 0;
    for (auto ps : psq) {
	if (!ps) {
	    cerr << "Reached EOS from plane ductor" << endl;
	    break;
	}

	Assert(ps->planeid() == wpid);
	cerr << "now=" << now << " lpos0=" << lpos0 << " ps->time()=" << ps->time() << endl;
	Assert(now == ps->time());

	IPlaneSlice::WireChargeRunVector wcrv = ps->charge_runs();
	std::vector<double> flat = ps->flatten();

	cerr << "# charge runs: " << wcrv.size()
	     << " #channels: " << flat.size()
	     << endl;
	for(auto wcr: wcrv) {
	    int wire_index = wcr.first;
	    cerr << "wire_index=" << wire_index << " #wires: " << wcr.second.size() << "q=:";
	    for (auto q : wcr.second) {
		cerr << " " << q;
		pshist->Fill(time_index, wire_index, q);
		++wire_index;
	    }
	    cerr << endl;
	    Assert(wcr.first == 10);
	}

	if (!wcrv.empty()) {
	    ++nslices;
	}

	// set up next expected time.
	now += lbin;
	++time_index;
    };
    
    cerr << "Got nslices=" << nslices << endl;
    Assert(nslices == 10);
    canvas.cd(2);
    pshist->Draw("colz");

    gStyle->SetOptStat(111111);

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	canvas.Print("test_planeductor.pdf");
    }

    return 0;
}
