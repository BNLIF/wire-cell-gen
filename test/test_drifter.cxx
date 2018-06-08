#include "WireCellIface/IDrifter.h"
#include "WireCellGen/TrackDepos.h"

#include "WireCellUtil/Testing.h"
#include "WireCellUtil/TimeKeeper.h"
#include "WireCellUtil/BoundingBox.h"
#include "WireCellUtil/RangeFeed.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"
#include "WireCellIface/IRandom.h"
#include "WireCellIface/IConfigurable.h"

#include "TApplication.h"
#include "TCanvas.h"

// 3d
#include "TView.h"
#include "TPolyLine3D.h"
#include "TPolyMarker3D.h"
#include "TColor.h"


#include <iostream>
#include <sstream>

#include "anode_loader.h"

using namespace WireCell;
using namespace std;

Gen::TrackDepos make_tracks() {
    // warning, this bipases some logic in TrackDepos::configure().
    Gen::TrackDepos td;
    td.add_track(10, Ray(Point(10,0,0), Point(100,10,10)));
    td.add_track(120, Ray(Point( 1,0,0), Point( 2,-100,0)));
    td.add_track(99, Ray(Point(130,50,50), Point(11, -50,-30)));
    return td;
}

// Return a vector of depositions.  Vector is in a shared_ptr and is
// not EOS/nullptr terminated.
IDepo::shared_vector get_depos()
{
    Gen::TrackDepos td = make_tracks();
    IDepo::vector* ret = new IDepo::vector;
    while (true) {
	IDepo::pointer depo;
        bool ok = td(depo);

        // Note, TrackDepos returns !ok when its empty.  If it's
        // properly configured (unlike this test) it will return ok
        // and set the depo to nullptr to indicate EOS before
        // returning !ok.  Here, we just bail in either case.
	if (!ok or !depo) {
	    break;
	}
	ret->push_back(depo);
    }
    return IDepo::shared_vector(ret);
}

Ray make_bbox()
{
    BoundingBox bbox(Ray(Point(-1,-1,-1), Point(1,1,1)));
    IDepo::vector activity(*get_depos());
    for (auto depo : activity) {
	bbox(depo->pos());
    }
    Ray bb = bbox.bounds();
    cout << "Bounds: " << bb.first << " --> " << bb.second << endl;
    return bb;
}


void test_sort()
{
    IDepo::vector activity(*get_depos());
    size_t norig = activity.size();

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
    IDepo::vector activity(*get_depos());
    int count=0, norig = activity.size();
    WireCell::RangeFeed<IDepo::vector::iterator> feed(activity.begin(), activity.end());
    WireCell::IDepo::pointer p;
    while ((p=feed())) {
	++count;
    }
    AssertMsg(count == norig , "Lost some points from feed"); 
}

IDepo::vector test_drifted()
{
    IDepo::vector result, activity(*get_depos());
    activity.push_back(nullptr); // EOS

    auto drifter = Factory::lookup<IDrifter>("Drifter");
    Assert(drifter);

    WireCell::IDrifter::output_queue outq;
    for (auto in : activity) {
	bool ok = (*drifter)(in, outq);
	Assert(ok);
	for (auto d : outq) {
	    result.push_back(d);
	}
    }
    Assert(!result.back());

    for (auto out : result) {
	if (!out) { break; }
	WireCell::IDepo::vector vec = depo_chain(out);
	AssertMsg(vec.size() > 1, "The history of the drifted deposition is truncated.");
    }

    cerr << "test_drifter: start with: " << activity.size()
	 << ", after drifting have: " << result.size() << endl;
    AssertMsg(activity.size() == result.size(), "Lost some points drifting"); 
    return result;
}


int main(int argc, char* argv[])
{
    std::string detector = "uboone";
    if (argc > 1) {
        detector = argv[1];
    }
    auto anode_tns = anode_loader(detector);


    {
        auto icfg = Factory::lookup<IConfigurable>("Random");
        auto cfg = icfg->default_configuration();
        icfg->configure(cfg);
    }
    {
        auto icfg = Factory::lookup<IConfigurable>("Drifter");
        auto cfg = icfg->default_configuration();
        cfg["anode"] = anode_tns[0];
        icfg->configure(cfg);
    }


    TimeKeeper tk("test_drifter");
    

    test_sort();
    cout << tk("sorted") << endl;
    test_feed();
    cout << tk("range feed") << endl;
    IDepo::vector drifted = test_drifted();
    cout << tk("transport") << endl;
    cout << tk.summary() << endl;
    
    Ray bb = make_bbox();


    TCanvas c("c","c",800,800);

    TView* view = TView::CreateView(1);
    view->SetRange(bb.first.x(),bb.first.y(),bb.first.z(),
		   bb.second.x(),bb.second.y(),bb.second.z());
    view->ShowAxis();


    // draw raw activity
    IDepo::vector activity(*get_depos());
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
	if (!depo) {
	    cerr << "Reached EOI"<< endl;
	    break;
	}
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
	if (!depo) {
	    cerr << "Reached EOI"<< endl;
	    break;
	}


	TPolyMarker3D* pm = new TPolyMarker3D(1,8);
	const Point& p = depo->pos();
	pm->SetPoint(0, p.x(), p.y(), p.z());

	double rel = depo->time()/(tmax-tmin);
	int col = TColor::GetColorPalette( int(rel*TColor::GetNumberOfColors()) );
	pm->SetMarkerColor(col);

	pm->Draw();
    }

    c.Print(String::format("%s.pdf", argv[0]).c_str());


    return 0;
}
