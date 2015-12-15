#include "WireCellGen/PlaneSliceMerger.h"
#include "WireCellUtil/Units.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace WireCell;

PlaneSliceMerger::PlaneSliceMerger()
{
}
bool PlaneSliceMerger::operator()(const input_vector& invec, output_pointer& out)
{
    // If all ports are in EOS, forward EOS.
    const int ninput = input_types().size();
    int neos=0;			
    for (auto ps : invec) { if (!ps) { ++neos; } }
    if (neos == ninput) {
	out = nullptr;
	return true;
    }

    // Otherwise, just forward what we got.
    if (neos) {
	cerr << "PlaneSliceMerger: found only " << neos << " EOS streams out of " << ninput << endl;
    }

    auto psv = new IPlaneSlice::vector;
    for (auto ps : invec) {
	psv->push_back(ps);
    }
    out = std::shared_ptr<IPlaneSlice::vector>(psv);
    return true;
}

