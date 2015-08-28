#include "WireCellGen/Drifter.h"
#include "WireCellGen/TrackDepos.h"

#include "WireCellUtil/Testing.h"
#include "WireCellUtil/TimeKeeper.h"
#include "WireCellUtil/BoundingBox.h"
#include "WireCellUtil/RangeFeed.h"

#include "TApplication.h"
#include "TCanvas.h"

// 3d
#include "TView.h"
#include "TPolyLine3D.h"
#include "TPolyMarker3D.h"
#include "TColor.h"


#include <iostream>
#include <sstream>

using namespace WireCell;
using namespace std;

TrackDepos make_tracks() {
    TrackDepos td;
    td.add_track(10, Ray(Point(10,0,0), Point(100,10,10)));
    td.add_track(120, Ray(Point( 1,0,0), Point( 2,-100,0)));
    td.add_track(99, Ray(Point(130,50,50), Point(11, -50,-30)));
    return td;
}

IDepoVector get_depos()
{
    TrackDepos td = make_tracks();
    shared_ptr<IDepoVector> sdepov = td.depositions();
    IDepoVector copy = *sdepov.get();
    return std::move(copy);
}

Ray make_bbox()
{
    BoundingBox bbox(Ray(Point(-1,-1,-1), Point(1,1,1)));
    for (auto depo : get_depos()) {
	bbox(depo->pos());
    }
    Ray bb = bbox.bounds();
    cout << "Bounds: " << bb.first << " --> " << bb.second << endl;
    return bb;
}


void test_sort()
{
    IDepoVector activity = get_depos();
    int norig = activity.size();

    sort(activity.begin(), activity.end(), ascending_time);
    AssertMsg (norig == activity.size(), "Sort lost depos");

    int nsorted = 0;
    int last_time = -1;
    for (auto depo : activity) {
	Assert(depo->time() >= last_time);
	last_time = depo->time();
	++nsorted;
    }
    Assert(nsorted);
}

void test_feed()
{
    IDepoVector activity = get_depos();
    int count=0, norig = activity.size();
    WireCell::RangeFeed<IDepoVector::iterator> feed(activity.begin(), activity.end());
    WireCell::IDepo::pointer p;
    while ((p=feed())) {
	++count;
    }
    AssertMsg(count == norig , "Lost some points from feed"); 
}

IDepoVector test_drifted()
{
    IDepoVector activity = get_depos(), result;
    int count=0, norig = activity.size();
    WireCell::RangeFeed<IDepoVector::iterator> feed(activity.begin(), activity.end());
    WireCell::Drifter drifter;
    drifter.connect(feed);

    WireCell::IDepo::pointer p;
    while ((p=drifter())) {
	WireCell::IDepoVector vec = depo_chain(p);
	AssertMsg(vec.size() > 1, "The history of the drifted deposition is truncated.");

	result.push_back(p);
	++count;
    }
    AssertMsg(count == norig , "Lost some points drifting"); 
    return result;
}


int main(int argc, char* argv[])
{
    TimeKeeper tk("test_drifter");

    test_sort();
    cout << tk("sorted") << endl;
    test_feed();
    cout << tk("range feed") << endl;
    IDepoVector drifted = test_drifted();
    cout << tk("transport") << endl;
    cout << tk.summary() << endl;

    
    Ray bb = make_bbox();

    TApplication* theApp = 0;
    if (argc > 1) {
	theApp = new TApplication ("test_drifter",0,0);
    }

    TCanvas c("c","c",800,800);

    TView* view = TView::CreateView(1);
    view->SetRange(bb.first.x(),bb.first.y(),bb.first.z(),
		   bb.second.x(),bb.second.y(),bb.second.z());
    view->ShowAxis();


    // draw raw activity
    IDepoVector activity = get_depos();
    TPolyMarker3D orig(activity.size(), 6);
    orig.SetMarkerColor(2);
    int indx=0;
    for (auto depo : activity) {
	const Point& p = depo->pos();
	orig.SetPoint(indx++, p.x(), p.y(), p.z());
    }
    orig.Draw();

    // draw drifted
    double tmin=-1, tmax=-1;
    for (auto depo : drifted) {
	auto history = depo_chain(depo);
	Assert(history.size() > 1);

	if (tmin<0 && tmax<0) {
	    tmin = tmax = depo->time();
	    continue;
	}
	tmin = min(tmin, depo->time());
	tmax = max(tmax, depo->time());
    }
    cerr << "Time bounds: " << tmin << " < " << tmax << endl;

    for (auto depo : drifted) {

	TPolyMarker3D* pm = new TPolyMarker3D(1,8);
	const Point& p = depo->pos();
	pm->SetPoint(0, p.x(), p.y(), p.z());

	double rel = depo->time()/(tmax-tmin);
	int col = TColor::GetColorPalette( int(rel*TColor::GetNumberOfColors()) );
	pm->SetMarkerColor(col);

	pm->Draw();
    }

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	c.Print("test_drifter.pdf");
    }

    return 0;
}
