#include "WireCellIface/IConfigurable.h"

#include "WireCellIface/IWireParameters.h"
#include "WireCellIface/IWireGenerator.h"

#include "WireCellIface/ICell.h"
#include "WireCellIface/ICellMaker.h"

#include "WireCellUtil/BoundingBox.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/TimeKeeper.h"

#include "WireCellUtil/NamedFactory.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TPolyLine.h"
#include "TMarker.h"



#include <iostream>
#include <vector>

using namespace boost::posix_time;
using namespace WireCell;
using namespace std;

int main(int argc, char* argv[])
{
    TimeKeeper tk("test cells");

    cout << tk("factories made") << endl;

    // fixme: this C++ dance to wire up the interfaces may eventually
    // be done inside a workflow engine.

    // fixme: this needs to be done by a configuration service
    auto wp_cfg = WireCell::Factory::lookup<IConfigurable>("WireParams");
    AssertMsg(wp_cfg, "Failed to get IConfigurable from default WireParams");
    auto cfg = wp_cfg->default_configuration();
    double pitch_mm = 100.0;
    cfg.put("pitch_mm.u", pitch_mm);
    cfg.put("pitch_mm.v", pitch_mm);
    cfg.put("pitch_mm.w", pitch_mm);
    wp_cfg->configure(cfg);
    cout << tk("Configured WireParams") << endl;

    //cout << configuration_dumps(cfg) << endl;

    auto wp_wps = WireCell::Factory::lookup<IWireParameters>("WireParams");
    AssertMsg(wp_wps, "Failed to get IWireParameters from default WireParams");
    cout << "Got WireParams IWireParameters interface @ " << wp_wps << endl;
    
    auto pw_gen = WireCell::Factory::lookup<IWireGenerator>("WireGenerator");
    AssertMsg(pw_gen, "Failed to get IWireGenerator from default WireGenerator");
    cout << "Got WireGenerator IWireGenerator interface @ " << pw_gen << endl;
    Assert(pw_gen->insert(wp_wps));

    IWireGenerator::output_type wires;
    Assert(pw_gen->extract(wires));

    int nwires = wires->size();
    cout << "Got " << nwires << " wires" << endl;
    //Assert(747 == nwires);
    cout << tk("Got ParamWires to local collection") << endl;


    auto bc = WireCell::Factory::lookup<ICellMaker>("BoundCells");
    AssertMsg(bc, "Failed to get ICellMaker from default BoundCells");
    cout << "Got BoundCells ICellMaker interface @ " << bc << endl;

    Assert(bc->insert(wires));

    ICellMaker::output_type cells;
    Assert(bc->extract(cells));
    cout << tk("BoundCells generated") << endl;

    int ncells = cells->size();
    cout << "Got " << ncells << " cells" << endl;
    cout << tk("Got BoundCells to local collection") << endl;
    AssertMsg(ncells, "Got no cells");

    WireCell::BoundingBox boundingbox;
    for (auto cell : *cells) {
	boundingbox(cell->center());
    }
    const Ray& bbox = boundingbox.bounds();
    cout << tk("Made bounding box") << endl;

    TApplication* theApp = 0;
    if (argc > 1) {
	theApp = new TApplication ("test_iwirecell",0,0);
    }
    TCanvas c;
    TMarker m;
    m.SetMarkerSize(1);
    m.SetMarkerStyle(20);
    int colors[] = {2,4,1};
    c.DrawFrame(bbox.first.z(), bbox.first.y(), bbox.second.z(), bbox.second.y());
    cout << tk("Started TCanvas") << endl;

    for (int cind = 0; cind < cells->size(); ++cind) {
	ICell::pointer cell = cells->at(cind);

	TPolyLine *pl = new TPolyLine; // Hi and welcome to Leak City.
	PointVector corners = cell->corners();
	for (int corner_ind=0; corner_ind < corners.size(); ++corner_ind) {
	    const Point& corner = corners[corner_ind];
	    pl->SetPoint(corner_ind, corner.z(), corner.y());
	}
	pl->SetFillColor((cind%10)+1); // try to stay out of the vomit colors
	pl->Draw();
    }
    cout << tk("Canvas drawn") << endl;

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	c.Print("test_iwirecell.pdf");
    }


    return 0;
}

