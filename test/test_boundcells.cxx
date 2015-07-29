// Note, this test directly links to Nav classes.  You should instead
// use Interfaces in real applications.

#include "WireCellIface/IWireSelectors.h"
#include "WireCellNav/ParamWires.h"
#include "WireCellNav/WireParams.h"
#include "WireCellNav/BoundCells.h"

#include "WireCellUtil/Testing.h"
#include "WireCellUtil/TimeKeeper.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TPolyLine.h"
#include "TLine.h"
#include "TBox.h"

#include <boost/range.hpp>

#include <iostream>
#include <cmath>

using namespace WireCell;
using namespace std;

void draw_wires(ParamWires& pw)
{
    int colors[3] = {2, 4, 1};

    vector<const IWire*> wires(pw.wires_begin(), pw.wires_end());
    int nwires = wires.size();
    cerr << "nwires = " << nwires << endl;
    for (int wind=0; wind < nwires; ++wind) {
	const IWire& wire = *wires[wind];
	int iplane = wire.plane();
	int index = wire.index();

	const Ray ray = wire.ray();
	TLine* a_wire = new TLine(ray.first.z(), ray.first.y(),
				  ray.second.z(), ray.second.y());
	a_wire->SetLineColor(colors[iplane]);
	a_wire->SetLineWidth(1);
	a_wire->Draw();
    }
}

void draw_cells(BoundCells& bc)
{
    const int ncolors= 4;
    int colors[ncolors] = {2, 4, 6, 8};

    vector<const ICell*> cells(bc.cells_begin(), bc.cells_end());

    const double reduce = 0.9;
    int ncells = cells.size();
    cerr << "ncells = " << ncells << endl;
    for (int cind = 0; cind < ncells; ++cind) {
	const ICell& cell = *cells[cind];

        WireCell::PointVector corners = cell.corners();
	const Point center = cell.center();
	//cerr << cind << ": " << center<< endl;

	size_t ncorners = corners.size();
	
	TPolyLine* pl = new TPolyLine;
	int ind=0;
	Point first_point;
	for (; ind<ncorners; ++ind) {
	    const Point& p = corners[ind];
	    const Point shrunk = reduce*(p - center) + center;
	    pl->SetPoint(ind, shrunk.z(), shrunk.y());
	    if (!ind) {
		first_point = shrunk;
	    }
	}
	pl->SetPoint(ind, first_point.z(), first_point.y());
	//pl->SetLineColor(colors[cind%ncolors]);
	pl->SetLineColor(cind%8+1);
	pl->SetLineWidth(2);
	pl->Draw();
    }

}

int main(int argc, char** argv)
{
    bool interactive = argc > 1;

    TimeKeeper tk("test bound cells");

    WireParams params;

    /* Some timing numbers for default 10m x 10m:
     * p(mm), #w, #c, t(ms)
     * 3.0, 1243, 288211, 5358
     * 5.0, 747, 104273, 1938
     * 10.0, 371, 26029, 668
     * 20.0, 185, 6561, 132
     * 50.0, 75, 1067, 22
     * 100.0, 37, 263, 6
     */
    double pitch = 50.0;	// leave it large so the results can actually be seen
    auto cfg = params.default_configuration();
    cfg.put("pitch_mm.u", pitch);
    cfg.put("pitch_mm.v", pitch);
    cfg.put("pitch_mm.w", pitch);
    params.configure(cfg);

    ParamWires pw;
    pw.generate(params);
    cout << tk("generated wires") << endl;

    BoundCells bc;
    bc.generate(pw.wires_begin(), pw.wires_end());

    cout << tk("generated cells") << endl;

    const Ray& bbox = params.bounds();

    TApplication* theApp = 0;
    if (interactive) {
	theApp = new TApplication ("test_boundcells",0,0);
    }

    TCanvas c;
    const double enlarge = 2.0;
    c.DrawFrame(enlarge*bbox.first.z(), enlarge*bbox.first.y(),
		enlarge*bbox.second.z(), enlarge*bbox.second.y());

    TBox* box = new TBox(bbox.first.z(), bbox.first.y(),
			 bbox.second.z(), bbox.second.y());
    box->Draw();
    cout << tk("set up canvas") << endl;
    draw_cells(bc);
    cout << tk("drew cells") << endl;
    draw_wires(pw);
    cout << tk("drew wires") << endl;

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	c.Print("test_boundcells.pdf");
    }


    return 0;
}
