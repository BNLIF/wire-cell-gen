#include "WireCellUtil/ExecMon.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IDepoSource.h"

#include <iostream>

using namespace WireCell;

int main()
{
    ExecMon em("start");
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellGen");

    // Normal user component code should never do this
    {
	auto rng_cfg = Factory::lookup<IConfigurable>("Random");
	{
	    auto cfg = rng_cfg->default_configuration();
	    rng_cfg->configure(cfg);
	}
	auto bs_cfg = Factory::lookup<IConfigurable>("BlipSource");
	{
	    auto cfg = bs_cfg->default_configuration();
	    bs_cfg->configure(cfg);
	}
    }


    // Normal user component code might do this but shoudl make the
    // name a configurable.
    auto deposrc = Factory::lookup<IDepoSource>("BlipSource");

    em("initialized");

    IDepo::pointer depo;
    for (int ind=0; ind != 1000000; ++ind) { // 1e6 in 300 ms on Xeon E5-2630
	(*deposrc)(depo);
    }

    em("done");
    std::cerr << em.summary() << std::endl;
    return 0;
}
