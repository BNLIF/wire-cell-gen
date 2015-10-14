// Note, this test directly links to Gen classes.  You should instead
// use Interfaces in real applications.

#include "WireCellGen/WireGenerator.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/BoundCells.h"

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

void draw_wires(const IWire::vector& wires)
{
    int colors[3] = {2, 4, 1};

    int nwires = wires.size();
    cerr << "nwires = " << nwires << endl;
    for (auto wire : wires) {
	int iplane = wire->planeid().index();
	int index = wire->index();

	const Ray ray = wire->ray();
	TLine* a_wire = new TLine(ray.first.z(), ray.first.y(),
				  ray.second.z(), ray.second.y());
	a_wire->SetLineColor(colors[iplane]);
	a_wire->SetLineWidth(1);
	a_wire->Draw();
    }
}

void draw_cells(const ICell::vector& cells)
{
    const int ncolors= 4;
    int colors[ncolors] = {2, 4, 6, 8};

    const double reduce = 0.8;
    int ncells = cells.size();
    cerr << "ncells = " << ncells << endl;
    for (int cind = 0; cind < ncells; ++cind) {
	ICell::pointer cell = cells[cind];

        WireCell::PointVector corners = cell->corners();
	const Point center = cell->center();
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

    WireParams* params = new WireParams;

    /* Some timing numbers for default 1m x 1m:
     * p(mm), #w, #c, t(ms)
     * 3.0, 1243, 288211, 5358
     * 5.0, 747, 104273, 1938
     * 10.0, 371, 26029, 668
     * 20.0, 185, 6561, 132
     * 50.0, 75, 1067, 22
     * 100.0, 37, 263, 6
     *
     * with 2m x 2m,
     * 3.0, 2485, 1154415, 21283
     */
    double pitch = 50.0*units::mm; // leave it large so the results can actually be seen
    double angle_deg = 35.0;	   // approx
    double full_width = 1.0*units::meter;
    auto cfg = params->default_configuration();
    put(cfg, "pitch_mm.u", pitch);
    put(cfg, "pitch_mm.v", pitch);
    put(cfg, "pitch_mm.w", pitch);
    put(cfg, "angle_deg.u", angle_deg);
    put(cfg, "angle_deg.v", -angle_deg);
    put(cfg, "size_mm.y", full_width);
    put(cfg, "size_mm.z", full_width);
    params->configure(cfg);

    WireGenerator wiregen;
    IWireParameters::pointer iparams(params);
    wiregen.insert(iparams);
    IWire::shared_vector wires;
    wiregen.extract(wires);
    cout << tk("generated wires") << endl;

    BoundCells bc;
    bc.insert(wires);
    ICell::shared_vector cells;
    bc.extract(cells);
    cout << tk("generated cells") << endl;

    const Ray& bbox = params->bounds();

    TApplication* theApp = 0;
    if (interactive) {
	theApp = new TApplication ("test_boundcells_dune",0,0);
    }

    TCanvas c("c","c",2000,2000);
    const double enlarge = 1.1;
    c.DrawFrame(enlarge*bbox.first.z(), enlarge*bbox.first.y(),
    		enlarge*bbox.second.z(), enlarge*bbox.second.y());

    TBox* box = new TBox(bbox.first.z(), bbox.first.y(),
    			 bbox.second.z(), bbox.second.y());
    box->Draw();
    cout << tk("set up canvas") << endl;
    draw_cells(*cells);
    cout << tk("drew cells") << endl;
    draw_wires(*wires);
    cout << tk("drew wires") << endl;

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	c.Print("test_boundcells_dune.pdf");
	c.Print("test_boundcells_dune.png");
    }


    return 0;
}
