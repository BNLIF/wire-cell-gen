#include "WireCellGen/EmpiricalNoiseModel.h"
#include "WireCellUtil/ExecMon.h"
#include "WireCellUtil/Persist.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellIface/IFrameSource.h"

#include "TCanvas.h"
#include "TFile.h"
#include "TStyle.h"
#include "TH2F.h"


#include <cstdlib>
#include <string>

using namespace std;
using namespace WireCell;

int main(int argc, char* argv[])
{
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellGen");

    string filenames[3] = {
        "microboone-noise-spectra-v2.json.bz2",
        "garfield-1d-3planes-21wires-6impacts-v6.json.bz2",
        "microboone-celltree-wires-v2.json.bz2",
    };

    for (int ind=0; ind<3; ++ind) {
        const int argi = ind+1;
        if (argc <= argi) {
            break;
        }
        filenames[ind] = argv[argi];
    }

    
    ExecMon em;

    // In the real WCT this is done by wire-cell and driven by user
    // configuration.  
    auto anode = Factory::lookup<IAnodePlane>("AnodePlane");
    auto anodecfg = Factory::lookup<IConfigurable>("AnodePlane");
    {
        auto cfg = anodecfg->default_configuration();
        cfg["fields"] = filenames[1];
        cfg["wires"] = filenames[2];
        anodecfg->configure(cfg);
    }

    auto noisemodel = Factory::lookup<IChannelSpectrum>("EmpiricalNoiseModel");
    auto noisemodelcfg = Factory::lookup<IConfigurable>("EmpiricalNoiseModel");
    {
        auto cfg = noisemodelcfg->default_configuration();
        cfg["spectra_file"] = filenames[0];
        noisemodelcfg->configure(cfg);
    }

    auto noisesrc = Factory::lookup<IFrameSource>("NoiseSource");
    auto noisesrccfg = Factory::lookup<IConfigurable>("NoiseSource");
    {
        auto cfg = noisesrccfg->default_configuration();
        cfg["anode"] = "AnodePlane";
        cfg["model"] = "EmpiricalNoiseModel";
        noisesrccfg->configure(cfg);
    }

    em("configuration done");
    
    IFrame::pointer frame;
    bool ok = (*noisesrc)(frame);
    em("got noise frame");
    Assert(ok);

    // WARNING: if you are reading this for ideas of how to use frames
    // and traces beware that, for brevity, this test assumes the
    // frame is a filled in rectangle of channels X ticks and with
    // indexed channel numbers.  In general this is not true!

    auto traces = frame->traces();
    const int ntraces = traces->size();
    const int nticks = traces->at(0)->charge().size();

    string tfilename = Form("%s.root", argv[0]);
    cerr << tfilename << endl;
    TFile* rootfile = TFile::Open(tfilename.c_str(), "recreate");
    //TCanvas* canvas = new TCanvas("c","canvas",1000,1000);
    //gStyle->SetOptStat(0);
    TH2F* hist = new TH2F("noise","Noise Frame",nticks,0,nticks,ntraces,0,ntraces);
    for (auto trace : *traces) {
        int chid = trace->channel();
        const auto& qvec = trace->charge();
        for (int ind=0; ind<nticks; ++ind) {
	  // convert to ADC ... 
	  hist->Fill(ind+0.5, chid+0.5, qvec[ind]/units::mV * 4096/2000.);
        }
    }
    em("filled histogram");
    hist->Write();
    rootfile->Close();
    em("closed ROOT file");

    cerr << em.summary() << endl;
    

    return 0;
}
