#include "WireCellGen/AnodePlane.h"
#include "WireCellGen/AnodeFace.h"
#include "WireCellGen/WirePlane.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Exceptions.h"
#include <iostream>

using namespace WireCell;
using namespace std;

int main(int argc, char* argv[])
{
    Gen::AnodePlane ap;
    auto cfg = ap.default_configuration();
    
    if (argc > 1) {
        cfg["wires"] = argv[1];
    }
    else {
        cfg["wires"] = "microboone-celltree-wires-v2.1.json.bz2";
    }
    cerr << "Wires file: " << cfg["wires"] << endl;
    if (argc > 2) {
        cfg["fields"] = argv[2];
    }
    else {
        cfg["fields"] = "ub-10-half.json.bz2";
    }
    cerr << "Fields file: " << cfg["fields"] << endl;
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
    cerr << "That last one should look bogus.\n";

    for (auto face : ap.faces()) {
        cerr << "face: " << face->ident() << "\n";
        std::vector<float> originx;
        for (auto plane : face->planes()) {
            cerr << "\tplane: " << plane->ident() << "\n";
            
            auto pimpos = plane->pimpos();
            cerr << "\torigin: " << pimpos->origin()/units::mm << "mm\n";
            for (int axis : {0,1,2}) {
                cerr << "\taxis " << axis << ": " << pimpos->axis(axis)/units::mm << "mm\n";
            }
            originx.push_back(pimpos->origin()[0]);
        }

     
        float diff = std::abs(originx.front() - originx.back());
        if (diff > 0.1*units::mm) {
            cerr << "ERROR, field response and wire location data do not match: diff = " << diff/units::mm << "mm\n";
            THROW(ValueError() << errmsg{"field response and wire location data do not match"});
        }
    }


    
    return 0;
        
}
