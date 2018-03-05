#include "WireCellGen/PipeGraphWrappers.h"
#include "WireCellGen/TrackDepos.h"

#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IDepoSink.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Type.h"

#include <iostream>

using namespace std;
using namespace WireCell;

IDepoSource::pointer make_tracks() {
    Gen::TrackDepos* td = new Gen::TrackDepos;
    td->add_track(10, Ray(Point(10,0,0), Point(100,10,10)));
    td->add_track(120, Ray(Point( 1,0,0), Point( 2,-100,0)));
    td->add_track(99, Ray(Point(130,50,50), Point(11, -50,-30)));
    return IDepoSource::pointer(td);
}


int main()
{
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellGen");

    // Normally this is handed by the configuration layer, not user code
    auto ds = make_tracks();
    auto dd = Factory::lookup<IDepoSink>("DumpDepos");


    PipeGraph::Wrapper::Factory pgwfac;

    auto dsnode = pgwfac(ds);
    cerr << "TrackDepos node is type: " << type(dsnode) << endl;

    auto ddnode = pgwfac(dd);
    cerr << "DumpDepos node is type: " << type(ddnode) << endl;

    PipeGraph::Graph graph;
    graph.connect(dsnode, ddnode);
    graph.execute();

    return 0;
}
