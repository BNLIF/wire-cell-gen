#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IDepoSource.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Type.h"

using namespace std;
using namespace WireCell;

int main()
{
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellGen");

    auto ds = Factory::lookup<IDepoSource>("TrackDepos");
    cerr << "TrackDepos is type: " << type(ds) << endl;

    Assert(ds);

    IDepo::pointer depo;
    bool ok = (*ds)(depo);

    Assert(ok);

    return 0;
}
