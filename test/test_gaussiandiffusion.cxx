#include "WireCellGen/GaussianDiffusion.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"

#include <iostream>

using namespace WireCell;


void test_gd() 
{
    auto depo = std::make_shared<SimpleDepo>(0, Point(10*units::cm, 0.0, 0.0), 1000);

    const Point w_origin(-3*units::mm, 0.0, -5*units::m);
    const Vector w_pdir(0.0, 0.0, 1.0);
    const double impact = 0.3*units::mm;

    
    Gen::GaussianDiffusion gd(depo,
			      1.0*units::mm, 1.0*units::us,
			      w_origin, w_pdir, impact,
			      0.0, 0.5*units::us);
			 
    auto patch = gd.patch();
    std::cerr << patch << std::endl;
    
}

int main()
{
    test_gd();
    return 0;
}
