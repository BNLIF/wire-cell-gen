// A test of the tiling class.  Note this test doesn't use interfaces
// so is not a suitable example for user code.

#include "WireCellIface/IWireSelectors.h"


#include "WireCellGen/WireGenerator.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/WireSummarizer.h"
#include "WireCellGen/WireSummary.h"
#include "WireCellGen/BoundCells.h"
#include "WireCellGen/Tiling.h"

#include "WireCellUtil/Testing.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/TimeKeeper.h"
#include "WireCellUtil/MemUsage.h"

#include "TFile.h"
#include "TPolyMarker.h"
#include "TH2F.h"
#include "TArrow.h"
#include "TCanvas.h"
#include "TDirectory.h"

#include <iostream>

using namespace WireCell;
using namespace std;

TArrow* draw_wire(IWire::pointer w, int color)
{
    const Ray ray = w->ray();
    TArrow* arr = new TArrow(ray.first.z(), ray.first.y(),
			     ray.second.z(), ray.second.y());
    arr->SetLineColor(color);
    arr->Draw();
    return arr;
}

int main()
{
    TimeKeeper tk("test tiling");
    MemUsage mu("test tiling");

    WireParams* params = new WireParams;

    double pitch = 50.0;
    auto cfg = params->default_configuration();
    cfg.put("pitch_mm.u", pitch);
    cfg.put("pitch_mm.v", pitch);
    cfg.put("pitch_mm.w", pitch);
    params->configure(cfg);

    IWireParameters::pointer iwp(params);
    WireGenerator wg;
    Assert(wg.sink(iwp));

    IWireVector wires;
    Assert(wg.source(wires));

    IWireVector u_wires, v_wires, w_wires;

    copy_if(boost::begin(wires), boost::end(wires), 
	    back_inserter(u_wires), select_u_wires);
    copy_if(boost::begin(wires), boost::end(wires), 
	    back_inserter(v_wires), select_v_wires);
    copy_if(boost::begin(wires), boost::end(wires), 
	    back_inserter(w_wires), select_w_wires);
    
    IWireVector* uvw_wires[3] = { &u_wires, &v_wires, &w_wires };

    WireSummarizer wser;
    Assert(wser.sink(wires));

    WireSummary::pointer ws;
    Assert(wser.source(ws));

    BoundCells bc;
    Assert(bc.sink(wires));

    ICellVector cells;
    Assert(bc.source(cells));

    tk("made cells"); mu("made cells");

    Tiling til;
    Assert(til.sink(cells));

    ICellSummary::pointer csum;
    Assert(til.source(csum));


    TFile* tfile = TFile::Open("test_tiling.root","RECREATE");

    TCanvas *canvas = new TCanvas;

    for (auto cell : cells) {
	AssertMsg(cell, "Got null cell.");
	//cerr << "Checking cell #" << cell->ident() << " center=" << cell->center() << endl;
	auto assoc_wires = csum->wires(cell);
	if (3 != assoc_wires.size()) {
	    cerr << "Cell #" << cell->ident()
		 << " with " << assoc_wires.size() << " wires:" ;
	    for (auto w : assoc_wires) {
		cerr << " " << w->ident();
	    }
	    cerr << " cell at " << cell->center();
	    cerr <<  endl;
	}
	AssertMsg(3 == assoc_wires.size(), "Got wrong number of wires");
	
	auto samecell = csum->cell(assoc_wires);
	AssertMsg(samecell, "Failed to get get a round trip cell->wires->cell pointer");
	AssertMsg(samecell->ident() == cell->ident(), "Cell->wires->cell round trip failed.");


	BoundingBox bb(cell->center());
	auto corners = cell->corners();
	for (auto corner : corners) {
	    bb(corner);
	}
	for (auto wire : assoc_wires) {
	    bb(wire->ray());
	}

	// prescale what we bother drawing
	if (cell->ident() % 100 != 1) {
	    continue;
	}

	std::string pad_name = Form("cell%d", cell->ident());
	TDirectory* dir = tfile->mkdir(pad_name.c_str());
	dir->cd();

	canvas->cd();
	canvas->Clear();
	TH1F* frame = canvas->DrawFrame(bb.bounds().first.z(),  bb.bounds().first.y(),
					bb.bounds().second.z(), bb.bounds().second.y());
	frame->SetTitle(Form("Cell %d", cell->ident()));
	frame->SetXTitle("Z transverse direction");
	frame->SetYTitle("Y transverse direction");

	TPolyMarker* cell_pm = new TPolyMarker;
	cell_pm->SetMarkerColor(1);
	cell_pm->SetMarkerStyle(8);
	cell_pm->SetPoint(0, cell->center().z(), cell->center().y());
	int ncorners = 0;
	for (auto corner : corners) {
	    ++ncorners;		// preincrement
	    cell_pm->SetPoint(ncorners, corner.z(), corner.y());
	}
	cell_pm->Draw();

	int colors[3] = {2,4,7};
	int nwires = 0;
	for (auto w: assoc_wires) {
	    draw_wire(w, colors[nwires]);
	    IWireVector& plane_wires = *uvw_wires[w->planeid().index()];

	    int wire_index = w->index();

	    if (wire_index > 0) {
		draw_wire(plane_wires[wire_index-1], colors[nwires]);
	    }
	    if (wire_index < plane_wires.size() - 1) {
		draw_wire(plane_wires[wire_index+1], colors[nwires]);
	    }
	    ++nwires;
	}


	canvas->Write();

	cerr << "Cell #" << cell->ident() << " with " << assoc_wires.size() << " wires:" ;
	for (auto w : assoc_wires) {
	    cerr << " " << w->ident();
	}
	cerr << " cell at " << cell->center();
	cerr <<  endl;

    }

    for (auto wire : wires) {
	AssertMsg(wire, "Got null wire.");

	auto cells = csum->cells(wire);
	//cerr << "Wire #" << wire->ident() << " with " << cells.size() << " cells" <<  endl;
    }

    //tfile->Write();
    tfile->Close();
}
