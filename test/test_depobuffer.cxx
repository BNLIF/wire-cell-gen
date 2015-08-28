#include "WireCellIface/IDepo.h"
#include "WireCellGen/DepoBuffer.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Testing.h"

#include <iostream>

using namespace WireCell;
using namespace std;

class DumbDepo : public IDepo {
    double m_time;
public:
    DumbDepo(double t=0) : m_time(t)  {}

    virtual const Point& pos() const { return std::move(Point()); }
    virtual double time() const { return m_time; }
    virtual double charge() const { return 0; }
    virtual WireCell::IDepo::pointer prior() const { return 0; } // primordial
};

struct DepoGen {
    double now;
    DepoGen() : now(0) {}
    IDepo::pointer operator()() {
	now += 0.1;
	//cerr << "DepoGen at " << now << endl;
	return IDepo::pointer(new DumbDepo(now));
    }
};

void test_depogen()
{
    DepoGen dg;
    for (int ind=0; ind<10; ++ind) {
	auto depo = dg();
	double targ = 0.1*(ind+1);
	cout << "ind=" << ind << " targ=" << targ << " t=" << depo->time() << endl;
	Assert (fabs(targ - depo->time()) < 1e-6);
    }
}

void test_depobuf()
{
    const double start = 0;
    const double window = 2.5;
    DepoBuffer buf(window);
    buf.connect(DepoGen());

    const double step = 0.5;
    for (double now = 0; now < 10; now+=step) {
	double tmin = now-0.5*window;
	double tmax = now+0.5*window;

	cout << "at: ["<<tmin << " -- " << now << " -- " << tmax << "]"<< endl;

	auto r = buf();
	int ndepos = boost::distance(r);
	cout << "\tbuffer is " << ndepos << " deep" << endl;
	cout << "\t";
	for (auto depo : r) {
	    cout << depo->time() << " ";
	    Assert(depo->time() > tmin);
	    Assert(depo->time() < tmax + step);// this isn't really a proper test
	}
	cout << endl;

	auto lost = buf.advance(step);
	cout << "\tadvance drops: " << lost.size() << endl;
    }
}

int main()
{
    test_depogen();
    test_depobuf();

    return 0;
}

