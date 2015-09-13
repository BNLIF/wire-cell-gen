//#include "WireCellGen/Digitizer.h"
#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Drifter.h"
#include "WireCellGen/Digitizer.h"
#include "WireCellGen/TrackDepos.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/WireGenerator.h"
#include "WireCellGen/WireSummary.h"

#include "WireCellIface/WirePlaneId.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Faninout.h"
#include "WireCellUtil/Testing.h"


#include "TApplication.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TLine.h"
#include "TPolyLine.h"
#include "TPolyMarker.h"

#include <iostream>
#include <boost/signals2.hpp>
#include <typeinfo>
#include <vector>

using namespace WireCell;
using namespace std;

TrackDepos make_tracks() {
    TrackDepos td;

    const double cm = units::cm;
    Ray same_point(Point(10*cm, -10*cm,  1*cm),
		   Point(11*cm, + 1*cm, 30*cm));

    const double usec = units::microsecond;

    td.add_track(1*usec, same_point);
    td.add_track(10*usec, same_point);
    td.add_track(100*usec, same_point);
    td.add_track(1000*usec, same_point);
    td.add_track(10000*usec, same_point);

    return td;
}


int main(int argc, char* argv[])
{
    // we make some plots
    TApplication* theApp = 0;
    if (argc > 1) {
	theApp = new TApplication ("test_digitizer",0,0);
    }

    TCanvas canvas("c","c",500,500);
    const char* pdf = "test_digitizer.pdf";
    canvas.Print(Form("%s[",pdf), "pdf");
    int colors[3] = {2, 4, 1};

    const double enlarge = 1.05;
    const double max_width = 5;
    const double max_charge = 25;


    const double tick = 2.0*units::microsecond; 
    const int nticks_per_frame = 100;
    double now = 0.0*units::microsecond;

    IWireParameters::pointer iwp(new WireParams);

    WireGenerator wg;
    Assert(wg.insert(iwp));

    IWireVector wires;
    Assert(wg.extract(wires));
    Assert(wires.size());

    Ray bbox = iwp->bounds();
    WireSummary ws(wires);

    TrackDepos td = make_tracks();

    // immediately slurp all depos in to local collection to make some plots
    IDepoVector depos;
    while (true) {
	auto depo = td();
	if (!depo) { break; }
	depos.push_back(depo);
    }

    canvas.Clear();
    canvas.Divide(2,2);
    {
	canvas.cd(1);
	TH1F* frame = canvas.DrawFrame(enlarge*bbox.first.z(), enlarge*bbox.first.y(),
				       enlarge*bbox.second.z(), enlarge*bbox.second.y());
	frame->SetXTitle("Transverse Z direction");
	frame->SetYTitle("Transverse Y (W) direction");
	frame->SetTitle(Form("%d depositions", (int)depos.size()));
	TPolyMarker* pm = new TPolyMarker;
	pm->SetMarkerColor(5);
	pm->SetMarkerStyle(8);
	int ndepo = 0;
	for (auto depo : depos) {
	    pm->SetMarkerColor(1);
	    pm->SetMarkerStyle(8);
	    pm->SetPoint(ndepo, depo->pos().z(), depo->pos().y());
	    ++ndepo;
	}
	pm->Draw();
    }
    {
	canvas.cd(2);
	TH1F* frame = canvas.DrawFrame(enlarge*bbox.first.x(), enlarge*bbox.first.y(),
				       enlarge*bbox.second.x(), enlarge*bbox.second.y());
	frame->SetXTitle("Transverse X direction");
	frame->SetYTitle("Transverse Y (W) direction");
	frame->SetTitle(Form("%d depositions", (int)depos.size()));
	TPolyMarker* pm = new TPolyMarker;
	pm->SetMarkerColor(5);
	pm->SetMarkerStyle(8);
	int ndepo = 0;
	for (auto depo : depos) {
	    pm->SetMarkerColor(1);
	    pm->SetMarkerStyle(8);
	    pm->SetPoint(ndepo, depo->pos().x(), depo->pos().y());
	    ++ndepo;
	}
	pm->Draw();
    }
    {
	canvas.cd(3);
	TH1F* frame = canvas.DrawFrame(enlarge*bbox.first.z(), enlarge*bbox.first.x(),
				       enlarge*bbox.second.z(), enlarge*bbox.second.x());
	frame->SetXTitle("Transverse Z direction");
	frame->SetYTitle("Transverse X direction");
	frame->SetTitle(Form("%d depositions", (int)depos.size()));
	TPolyMarker* pm = new TPolyMarker;
	pm->SetMarkerColor(5);
	pm->SetMarkerStyle(8);
	int ndepo = 0;
	for (auto depo : depos) {
	    pm->SetMarkerColor(1);
	    pm->SetMarkerStyle(8);
	    pm->SetPoint(ndepo, depo->pos().z(), depo->pos().x());
	    ++ndepo;
	}
	pm->Draw();
    }
    canvas.Print(pdf, "pdf");    


    // drift

    std::vector<WireCell::Drifter*> drifters = {
	new Drifter(iwp->pitchU().first.x()),
	new Drifter(iwp->pitchV().first.x()),
	new Drifter(iwp->pitchW().first.x())
    };

    // load up drifters all the way
    for (auto depo : depos) {
	for (int ind=0; ind<3; ++ind) {
	    Assert(drifters[ind]->insert(depo));
	}
    }
    // and flush them out 
    for (int ind=0; ind<3; ++ind) {
	drifters[ind]->flush();

	cerr << "drifter #"  << ind
	     << ": #in=" << drifters[ind]->ninput()
	     << " #out=" << drifters[ind]->noutput()
	     << endl;
    }

    AssertMsg(false, "Finish this test");
    // diffuse + collect/induce

    // std::vector<PlaneDuctor*> ductors = {
    // 	new PlaneDuctor(WirePlaneId(kUlayer), iwp->pitchU(), tick, now),
    // 	new PlaneDuctor(WirePlaneId(kVlayer), iwp->pitchV(), tick, now),
    // 	new PlaneDuctor(WirePlaneId(kWlayer), iwp->pitchW(), tick, now)
    // };

    // int ndrifted[3] = {0};
    // while (true) {
    // 	int n_ok = 0;
    // 	for (int ind=0; ind < 3; ++ind) {
    // 	    IDepo::pointer depo;
    // 	    if (!drifters[ind]->extract(depo)) {
    // 		cerr << "Failed to extract from drifter " << ind << endl;
    // 		continue;
    // 	    }
    // 	    ++ndrifted[ind];
    // 	    Assert(depo);
    // 	    depos.push_back(depo);
    // 	    Assert(ductors[ind]->insert(depo));
    // 	    ++n_ok;
    // 	}
    // 	if (n_ok == 0) {
    // 	    break;
    // 	}
    // 	Assert(n_ok == 3);

    // 	for (int ind=0; ind < 3; ++ind) {
    // 	    ductors[ind]->flush();
    // 	    cerr << "^ " << ind
    // 		 << " #in=" << ductors[ind]->ninput()
    // 		 << " #out=" << ductors[ind]->noutput()
    // 		 << " #buf=" << ductors[ind]->nbuffer()
    // 		 << endl;
    // 	}


    // }
    // Assert(ductors[0]->noutput() == ductors[1]->noutput() && ductors[1]->noutput() == ductors[2]->noutput());


    // Digitizer digitizer;
    // digitizer.set_wires(wires);

    // int nducted[3] = {0};
    // while (true) {
    // 	IPlaneSliceVector psv(3);
    // 	int n_ok = 0;
    // 	int n_eos = 0;

    // 	for (int ind=0; ind<3; ++ind) {

    // 	    if (!ductors[ind]->extract(psv[ind])) {
    // 		cerr << "ductor #"<<ind<<"failed"<<endl;
    // 		continue;
    // 	    }
    // 	    ++nducted[ind];
    // 	    ++n_ok;
    // 	    if (psv[ind] == ductors[ind]->eos()) {
    // 		++n_eos;
    // 	    }
    // 	}
    // 	if (n_ok == 0) {
    // 	    cerr << "Got no channel slices from plane ductors" << endl;
    // 	    break;
    // 	}
    // 	Assert(n_ok == 3);

    // 	if (!(n_eos ==0 || n_eos == 3)) {
    // 	    cerr << "Lost sync! #eos:" << n_eos << endl;
    // 	    cerr << "ndrifted/nducted ninput/noutput:\n";
    // 	    for (int ind=0; ind<3; ++ind) {
    // 		cerr << "\t" << ndrifted[ind] << "/" << nducted[ind]
    // 		     << " " << ductors[ind]->ninput() << "/" << ductors[ind]->noutput()
    // 		     << endl;
    // 	    }
    // 	}
    // 	Assert(n_eos == 0 || n_eos == 3);
    // 	if (n_eos == 3) {
    // 	    cerr << "Got three EOS from plane ductors" << endl;
    // 	    break;
    // 	}

    // 	Assert(digitizer.insert(psv));
    // }
    
    // digitizer.flush();

    // while (true) {
    // 	IChannelSlice::pointer csp;
    // 	if (!digitizer.extract(csp)) {
    // 	    cerr << "Digitizer fails to produce output" << endl;
    // 	    break;
    // 	}
    // 	if (csp == digitizer.eos()) {
    // 	    cerr << "Digitizer reaches EOS" << endl;
    // 	    break;
    // 	}

    // 	ChannelCharge cc = csp->charge();
    // 	int nhit_wires = cc.size();
    // 	if (!nhit_wires) {
    // 	    //cerr << "Digitizer returns no charge at " << csp->time() << endl;
    // 	    continue;
    // 	}

    // 	cerr << "Digitized " << cc.size() << " at " << csp->time() << endl;
    // 	canvas.Clear();
    // 	TH1F* frame = canvas.DrawFrame(enlarge*bbox.first.z(), enlarge*bbox.first.y(),
    // 				       enlarge*bbox.second.z(), enlarge*bbox.second.y());

    // 	frame->SetXTitle("Transverse Z direction");
    // 	frame->SetYTitle("Transverse Y (W) direction");
    // 	frame->SetTitle(Form("t=%f %d hit wires red=U, blue=V, +X (-drift) direction into page",
    // 			     csp->time(), nhit_wires));

    // 	for (auto cq : cc) {	// for each hit channel

    // 	    // for each wire on that channel
    // 	    for (auto wire : ws.by_channel(cq.first)) {
    // 		Ray r = wire->ray();

    // 		TLine* a_wire = new TLine(r.first.z(), r.first.y(), r.second.z(), r.second.y());
    // 		a_wire->SetLineColor(colors[wire->planeid().index()]);
    // 		double width = cq.second.mean()/max_charge*max_width;
    // 		if (width<1) { width = 1.0; }
    // 		a_wire->SetLineWidth(width);
    // 		a_wire->Draw();	    
    // 	    }
    // 	}

    // 	TPolyMarker* pm = new TPolyMarker;
    // 	pm->SetMarkerColor(1);
    // 	pm->SetMarkerStyle(8);
    // 	int ndepo = 0;
    // 	for (auto depo : depos) {
    // 	    if (std::abs(depo->time() - csp->time()) < 0.5*units::microsecond) {
    // 		//cerr << ndepo << " t=" << depo->time() << " at: " << depo->pos() << endl;
    // 		pm->SetPoint(ndepo, depo->pos().z(), depo->pos().y());
    // 		++ndepo;
    // 	    }
    // 	}
    // 	pm->Draw();

    // 	canvas.Print(pdf, "pdf");
    // }
    // canvas.Print(Form("%s]",pdf), "pdf");



    return 0;
}
