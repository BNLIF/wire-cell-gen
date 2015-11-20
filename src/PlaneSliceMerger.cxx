#include "WireCellGen/PlaneSliceMerger.h"
#include "WireCellUtil/Units.h"

#include <iostream>

using namespace std;
using namespace WireCell;

PlaneSliceMerger::PlaneSliceMerger()
    : m_input(3)
    , m_nin(0)
    , m_nout(0)
{
}
bool PlaneSliceMerger::insert(const input_pointer& in, int port)
{
    if (port < 0 || port >= 3) { return false; }
    ++m_nin;

    m_input[port].push_back(in);

    while (true) {
	for (int ind = 0; ind < 3; ++ind) {
	    if (m_input[ind].empty()) {
		return true;
	    }
	}

	auto psv = new IPlaneSlice::vector;
	int n_eos = 3;
	for (int ind = 0; ind < 3; ++ind) {
	    auto ps = m_input[ind].front();
	    psv->push_back(ps);
	    if (ps) { -- n_eos; }
	    m_input[ind].pop_front();
	}

	if (n_eos == 3) {
	    delete psv;
	    m_output.push_back(nullptr);
	    return true;
	}

	if (n_eos > 0) {
	    cerr << "PlaneSliceMerger: EOS out of sync!\n";
	    return true;
	}

	if (std::abs(psv->at(0)->time() - psv->at(1)->time()) > units::nanosecond ||
	    std::abs(psv->at(0)->time() - psv->at(2)->time()) > units::nanosecond)
	{
	    cerr << "PlaneSliceMerger: out of sync! "
		 << "\tU plane @ " << psv->at(0)->time() << "\n"
		 << "\tV plane @ " << psv->at(1)->time() << "\n"
		 << "\tW plane @ " << psv->at(2)->time() << "\n";
	}

	m_output.push_back(IPlaneSlice::shared_vector(psv));
    }

    return true;
}
bool PlaneSliceMerger::extract(output_pointer& out)
{
    if (m_output.empty()) {
	return false;
    }
    out = m_output.front();
    m_output.pop_front();
    ++m_nout;
    return true;
}
