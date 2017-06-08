#include "WireCellGen/EmpiricalNoiseModel.h"
#include "WireCellUtil/Persist.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IChannelStatus.h"

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

    
    // In the real WCT this is done by wire-cell and driven by user
    // configuration.  
    auto anode = Factory::lookup<IAnodePlane>("AnodePlane");
    auto anodecfg = Factory::lookup<IConfigurable>("AnodePlane");
    auto acfg = anodecfg->default_configuration();
    acfg["fields"] = filenames[1];
    acfg["wires"] = filenames[2];
    anodecfg->configure(acfg);

    auto chanstat = Factory::lookup<IChannelStatus>("StaticChannelStatus");
    auto chanstatcfg = Factory::lookup<IConfigurable>("StaticChannelStatus");
    {
        // In the real app this would be in a JSON or Jsonnet config
        // file in wire-cell-cfg.  Here, just to avoid an external
        // file we define a couple hard-coded deviant values somewhat
        // laboriously
        auto cfg = chanstatcfg->default_configuration();
        // cfg["deviants"] = Json::arrayValue;
        // cfg["deviants"][0]["chid"] = 100;
        // cfg["deviants"][0]["gain"] = 4.7*units::mV/units::fC;
        // cfg["deviants"][0]["shaping"] = 1.0*units::us;
        // cfg["deviants"][1]["chid"] = 200;
        // cfg["deviants"][1]["gain"] = 4.7*units::mV/units::fC;
        // cfg["deviants"][1]["shaping"] = 1.0*units::us;
        chanstatcfg->configure(cfg);
    }

    cerr << "Creating EmpiricalNoiseModel...\n";
    Gen::EmpiricalNoiseModel empnomo(filenames[0]);
    cerr << "Get default con fig\n";
    auto cfg = empnomo.default_configuration();
    cerr << "Set configuration\n";
    empnomo.configure(cfg);

    auto chids = anode->channels();
    cerr << "Got " << chids.size() << " channels\n";
    for (auto chid : chids) {
        const auto& amp = empnomo(chid);
        double tot = 0;
        for (auto v : amp) {
            tot += v;
        }
        cerr << "ch:" << chid << " " << amp.size()
             << " tot=" << tot << " " << empnomo.gain(chid)/(units::mV/units::fC)
	     << " " << empnomo.shaping_time(chid)/units::us
	     << " " << empnomo.freq().size() 
	     << " " << amp.size() 
             << endl;
	//break;
    }


    return 0;
}
