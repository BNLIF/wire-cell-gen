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

#include "WireCellRootVis/CanvasApp.h"
#include "WireCellRootVis/Drawers.h"

#include "TPolyMarker.h"
#include "TH2F.h"
#include "TArrow.h"
#include "TCanvas.h"

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


TPolyMarker* draw_cell(ICell::pointer cell, int color=1, int style=8)
{
    TPolyMarker* pm = new TPolyMarker;
    pm->SetMarkerColor(color);
    pm->SetMarkerStyle(style);
    pm->SetPoint(0, cell->center().z(), cell->center().y());
    int ncorners = 0;
    for (auto corner : cell->corners()) {
	++ncorners;		// preincrement
	pm->SetPoint(ncorners, corner.z(), corner.y());
    }
    pm->Draw();
    return pm;
}

int main(int argc, char *argv[])
{
    WireCellRootVis::CanvasApp app(argv[0], argc>1, 1000,1000);

    TimeKeeper tk("test tiling");
    MemUsage mu("test tiling");

    WireParams* params = new WireParams;

    double pitch = 50.0;
    auto cfg = params->default_configuration();
    put(cfg, "pitch_mm.u", pitch);
    put(cfg, "pitch_mm.v", pitch);
    put(cfg, "pitch_mm.w", pitch);
    params->configure(cfg);

    IWireParameters::pointer iwp(params);
    WireGenerator wg;
    Assert(wg.insert(iwp));

    IWire::shared_vector wires;
    Assert(wg.extract(wires));
    Assert(wires);

    IWire::vector u_wires, v_wires, w_wires;

    copy_if(wires->begin(), wires->end(),
	    back_inserter(u_wires), select_u_wires);
    copy_if(wires->begin(), wires->end(),
	    back_inserter(v_wires), select_v_wires);
    copy_if(wires->begin(), wires->end(),
	    back_inserter(w_wires), select_w_wires);
    
    IWire::vector* uvw_wires[3] = { &u_wires, &v_wires, &w_wires };

    WireSummarizer wser;
    Assert(wser.insert(wires));

    WireSummary::pointer ws;
    Assert(wser.extract(ws));
    Assert(ws);

    BoundCells bc;
    Assert(bc.insert(wires));

    ICell::shared_vector cells;
    Assert(bc.extract(cells));
    Assert(cells);

    tk("made cells"); mu("made cells");

    Tiling til;
    Assert(til.insert(cells));

    ICellSummary::pointer csum;
    Assert(til.extract(csum));
    Assert(csum);


    const int ncolors = 3;
    int colors[ncolors] = {2,4,7};

    for (auto cell : *cells) {
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

	auto neighbors = csum->neighbors(cell);
	Assert(!neighbors.empty());

	BoundingBox bb(cell->center());
	auto corners = cell->corners();
	for (auto corner : corners) {
	    bb(corner);
	}
	for (auto wire : assoc_wires) {
	    bb(wire->ray());
	}
	for (auto nc : neighbors) {
	    for (auto corn : nc->corners()) {
		bb(corn);
	    }
	}
	cerr << "Cell " << cell->ident()
	     << " with " << neighbors.size() << " neighbors" << endl;
	Assert(neighbors.size() >= 3 || neighbors.size() <= 12);

	// prescale what we bother drawing
	if (cell->ident() % 100 != 1) {
	    continue;
	}

	std::string pad_name = Form("cell%d", cell->ident());

	TH1F* frame = app.canvas().DrawFrame(bb.bounds().first.z(),  bb.bounds().first.y(),
					     bb.bounds().second.z(), bb.bounds().second.y());
	frame->SetTitle(Form("Cell %d", cell->ident()));
	frame->SetXTitle("Z transverse direction");
	frame->SetYTitle("Y transverse direction");

	draw_cell(cell);
	int nneighbors = 0;
	for (auto nc : neighbors) {
	    draw_cell(nc, colors[nneighbors%ncolors], 4);
	    ++nneighbors;
	}

	int colors[3] = {2,4,7};
	int nwires = 0;
	for (auto w: assoc_wires) {
	    draw_wire(w, colors[nwires]);
	    IWire::vector& plane_wires = *uvw_wires[w->planeid().index()];

	    int wire_index = w->index();

	    if (wire_index > 0) {
		draw_wire(plane_wires[wire_index-1], colors[nwires]);
	    }
	    if (wire_index < plane_wires.size() - 1) {
		draw_wire(plane_wires[wire_index+1], colors[nwires]);
	    }
	    ++nwires;
	}

	app.pdf();

	cerr << "Cell #" << cell->ident() << " with " << assoc_wires.size() << " wires:" ;
	for (auto w : assoc_wires) {
	    cerr << " " << w->ident();
	}
	cerr << " cell at " << cell->center();
	cerr <<  endl;

    }

    for (auto wire : *wires) {
	AssertMsg(wire, "Got null wire.");

	auto these_cells = csum->cells(wire);
	//cerr << "Wire #" << wire->ident() << " with " << cells.size() << " cells" <<  endl;
    }

    app.run();
}
