#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IWire.h"
#include "WireCellIface/IWireGenerator.h"
#include "WireCellIface/IWireSelectors.h"
#include "WireCellIface/IWireSummarizer.h"
#include "WireCellIface/IWireSummary.h"

#include "WireCellUtil/Testing.h"
#include "WireCellUtil/TimeKeeper.h"
#include "WireCellUtil/MemUsage.h"
#include "WireCellUtil/BoundingBox.h"
#include "WireCellUtil/Point.h"

#include "WireCellUtil/NamedFactory.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TLine.h"
#include "TMarker.h"

#include <iostream>
#include <iterator>

using namespace WireCell;
using namespace std;


int main(int argc, char* argv[])
{
    TimeKeeper tk("test wires");
    MemUsage mu("test wires");

    // These are here to force the linker to give us the symbols.  We
    // do not compile against this code!
    WIRECELL_NAMEDFACTORY_USE(WireParams);
    WIRECELL_NAMEDFACTORY_USE(WireGenerator);
    WIRECELL_NAMEDFACTORY_USE(WireSummarizer);

    cout << tk("factories made") << endl;
    cout << mu("factories made") << endl;

    // fixme: this C++ dance to wire up the interfaces may eventually
    // be done inside a workflow engine.

    // fixme: this needs to be done by a configuration service
    auto wp_cfg = WireCell::Factory::lookup<IConfigurable>("WireParams");
    AssertMsg(wp_cfg, "Failed to get IConfigurable from default WireParams");

    auto cfg = wp_cfg->default_configuration();
    double pitch = 10.0;
    cfg.put("pitch_mm.u", pitch);
    cfg.put("pitch_mm.v", pitch);
    cfg.put("pitch_mm.w", pitch);
    wp_cfg->configure(cfg);

    auto wp = WireCell::Factory::lookup<IWireParameters>("WireParams");
    AssertMsg(wp, "Failed to get IWireParameters from default WireParams");
    cout << "Got WireParams IWireParameters interface @ " << wp << endl;
    
    cout << tk("Configured WireParams") << endl;
    cout << mu("Configured WireParams") << endl;

    cout << "Wire Parameters:\n"
	 << "Bounds: " << wp->bounds() << "\n"
	 << "Upitch: " << wp->pitchU() << "\n"
	 << "Vpitch: " << wp->pitchV() << "\n"
	 << "Wpitch: " << wp->pitchW() << "\n"
	 << endl;


    auto wg = WireCell::Factory::lookup<IWireGenerator>("WireGenerator");
    AssertMsg(wg, "Failed to get IWireGenerator from default WireGenerator");
    cout << "Got WireGenerator IWireGenerator interface @ " << wg << endl;

    Assert(wg->insert(wp));

    IWireVector wires;
    Assert(wg->extract(wires));

    cout << tk("Generated ParamWires") << endl;
    cout << mu("Generated ParamWires") << endl;

    int nwires = wires.size();
    cout << "Got " << nwires << " wires" << endl;
    //Assert(1103 == nwires);

    cout << tk("Made local wire collection") << endl;
    cout << mu("Made local wire collection") << endl;

    auto wser = WireCell::Factory::lookup<IWireSummarizer>("WireSummarizer");

    Assert(wser->insert(wires));

    IWireSummary::pointer ws;
    Assert(wser->extract(ws));

    WireCell::BoundingBox bb2 = ws->box();

    cout << tk("Made wire summary") << endl;
    cout << mu("Made wire summary") << endl;

    WireCell::BoundingBox boundingbox;
    for (int ind = 0; ind < wires.size(); ++ind) {
	boundingbox(wires[ind]->ray());
    }
    const Ray& bbox = boundingbox.bounds();

    cout << tk("Made bounding box") << endl;
    cout << mu("Made bounding box") << endl;

    vector<IWire::pointer> u_wires, v_wires, w_wires;
    copy_if(wires.begin(), wires.end(), back_inserter(u_wires), select_u_wires);
    copy_if(wires.begin(), wires.end(), back_inserter(v_wires), select_v_wires);
    copy_if(wires.begin(), wires.end(), back_inserter(w_wires), select_w_wires);

    size_t n_wires[3] = {
	u_wires.size(),
	v_wires.size(),
	w_wires.size()
    };

    TApplication* theApp = 0;
    if (argc > 1) {
	theApp = new TApplication ("test_iwireprovider",0,0);
    }

    TCanvas c;
    TLine l;
    TMarker m;
    m.SetMarkerSize(1);
    m.SetMarkerStyle(20);
    int colors[] = {2,4,1};
    double max_width = 5.0;

    cout << tk("Made TCanvas") << endl;
    cout << mu("Made TCanvas") << endl;

    TH1F* frame = c.DrawFrame(bbox.first.z(), bbox.first.y(),
			      bbox.second.z(), bbox.second.y());
    frame->SetTitle("Wires, red=U, blue=V, thicker=increasing index");
    frame->SetXTitle("Z transverse direction");
    frame->SetYTitle("Y transverse direction");
    for (auto wit = wires.begin(); wit != wires.end(); ++wit) {
	IWire::pointer wire = *wit;

	Ray wray = wire->ray();

	int iplane = wire->planeid().index();
	int index = wire->index();
	double width = ((index+1)*max_width)/n_wires[iplane];

	l.SetLineColor(colors[iplane]);
	l.SetLineWidth(width);
	l.DrawLine(wray.first.z(), wray.first.y(), wray.second.z(), wray.second.y());
	Point cent = wire->center();
	m.SetMarkerColor(colors[iplane]);
	m.DrawMarker(cent.z(), cent.y());
    }
    cout << tk("Canvas drawn") << endl;
    cout << mu("Canvas drawn") << endl;

    if (theApp) {
	theApp->Run();
    }
    else {			// batch
	c.Print("test_iwireprovider.pdf");
    }

    cout << "Timing summary:\n" << tk.summary() << endl;
    cout << "Memory summary:\n" << mu.summary() << endl;

    return 0;
}

