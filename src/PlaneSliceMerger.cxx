#include "WireCellGen/PlaneSliceMerger.h"
#include "WireCellUtil/Units.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace WireCell;

PlaneSliceMerger::PlaneSliceMerger()
    : m_input(3)
    , m_nout(0)
    , m_eos(false)
{
    m_nin[0] = m_nin[1] = m_nin[2] = 0;
}
bool PlaneSliceMerger::insert(const input_pointer& in, int port)
{
    if (m_eos) { return false; }
    if (port < 0 || port >= 3) { return false; }
    ++m_nin[port];

    m_input[port].push_back(in);
    {
	stringstream msg;
	msg << "PlaneSliceMerger: " << m_nin[port] << "[" << port << "]\tinsert ";
	if (in) {
	    msg << "at " << in->time() << "\n";
	}
	else {
	    msg << "@ EOS\n";
	}
	cerr << msg.str();
    }

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
	    m_eos = true;
	    return true;
	}

	if (n_eos > 0) {
	    stringstream msg;
	    msg << "PlaneSliceMerger: EOS out of sync! #eos=" << n_eos << "\n";
	    string uvw="UVW";
	    for (int ind=0; ind<3; ++ind) {
		if (psv->at(ind)) {
		    msg << "\t:have "<<uvw[ind]<<" at t=" << psv->at(ind)->time() << "\n";
		}
		else {
		    msg << "\t:miss "<<uvw[ind]<<"\n";
		}
	    }
	    cerr << msg.str();
	    m_eos = true;
	    m_output.push_back(nullptr);
	    return true;	// fixme: this is ditching data!
	}

	if (std::abs(psv->at(0)->time() - psv->at(1)->time()) > units::nanosecond ||
	    std::abs(psv->at(0)->time() - psv->at(2)->time()) > units::nanosecond)
	{
	    stringstream msg;
	    msg << "PlaneSliceMerger: out of sync! "
		 << "\tU plane @ " << psv->at(0)->time() << "\n"
		 << "\tV plane @ " << psv->at(1)->time() << "\n"
		 << "\tW plane @ " << psv->at(2)->time() << "\n";
	    cerr << msg.str();
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
    {
	stringstream msg;
	msg << "PlaneSliceMerger: " << m_nout << "\textract";
	if (out) {
	    msg << " planes @ "
		<< out->at(0)->time() << ", "
		<< out->at(1)->time() << ", "
		<< out->at(2)->time() << "\n";
	}
	else {
	    msg << " @ EOS\n";
	}
	cerr << msg.str();
    }

    return true;
}
