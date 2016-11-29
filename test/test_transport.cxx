#include "WireCellGen/Transport.h"
#include "WireCellIface/SimpleDepo.h"

#include <iostream>

using namespace WireCell;

void test_depoplanex()
{
    const double tunit = units::us; // for display

    Gen::DepoPlaneX dpx(1*units::cm);
    std::vector<double> times{10.,25.,50.,100.0,500.0, 1000.0}; // us
    std::vector<double> xes{100.0, 0.0, 50.0, 10.0, 75.0};	// cm

    for (auto t : times) {
	t *= tunit;
	for (auto x : xes) {
	    x *= units::cm;
	    auto depo = std::make_shared<SimpleDepo>(t,Point(x,0.0,0.0));
	    auto newdepo = dpx.add(depo);
	    std::cerr << "\tx=" << x/units::cm << "cm, t=" << newdepo->time()/tunit << "us\n";
	}
	std::cerr << "^ t=" << t/tunit << "us, fot=" << dpx.freezeout_time()/tunit << "us\n";
    }
    dpx.freezeout();
    std::cerr << "frozen with fot=" << dpx.freezeout_time()/tunit << "us\n";
}

int main()
{
    test_depoplanex();
}
