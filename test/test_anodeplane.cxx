#include "WireCellGen/AnodePlane.h"

#include <iostream>

using namespace WireCell;
using namespace std;

int main()
{
    Gen::AnodePlane ap;
    auto cfg = ap.default_configuration();
    cfg["wires"] = "microboone-celltree-wires-v2.json.bz2";
    cfg["fields"] = "ub-10-half.json.bz2";
    ap.configure(cfg);

    int last_ident = -999;
    for (int chid=-1; chid<10000; ++chid) {
        auto wpid = ap.resolve(chid);
        if (wpid.ident() == last_ident) {
            continue;
        }
        cerr << chid << " " << wpid << endl;
        last_ident = wpid.ident();
    }
    return 0;
        
}
