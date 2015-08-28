#include "WireCellUtil/BufferedHistogram2D.h"
#include "WireCellUtil/Testing.h"
#include "WireCellGen/Diffuser.h"

#include <iostream>

using namespace WireCell;
using namespace std;

void dump_hist(BufferedHistogram2D& hist)
{
    while (hist.size()) {
	double x = hist.xmin();
	std::vector<double> slice = hist.popx();
	if (!slice.size()) { continue; }
	cerr << "[" << hist.size() << "/" << slice.size()<<"] " << x;
	for (auto d : slice) {
	    cerr << " " << d;
	}
	cerr << endl;
    }
}

int test_dump()
{
    BufferedHistogram2D hist;
    Diffuser diff(hist, 3);

    double xmin2 = diff.diffuse(15.0,10.0,1.0,1.0);
    cerr << "depo2 xmin = " << xmin2 << endl;

    double xmin1 = diff.diffuse(4.0,2.0,1.0,1.0);
    cerr << "depo1 xmin = " << xmin1 << endl;

    dump_hist(hist);

    return 0;

}

int test1()
{
    BufferedHistogram2D hist(1000,10,1000,10);
    Diffuser diff(hist, 3);

    auto qa = diff.oned(11010, 103, 7000, 1000);
    Assert(qa.size());
    double tot_qa = 0;
    for (auto q: qa) {
	tot_qa += q;
    }
    Assert(tot_qa > 0);

    auto qb = diff.oned(100, 1.6, -5, 10);
    Assert(qb.size());
    double tot_qb = 0;
    for (auto q: qb) {
	tot_qb += q;
    }
    Assert(tot_qb > 0);
}

int test2()
{
    const double xbinsize = 1000;
    const double ybinsize = 1;
    const int nsigma = 3;


    BufferedHistogram2D hist(xbinsize, ybinsize);
    Diffuser diff(hist, nsigma);

    const double tau = 110028;
    const double sigma_tau = 325.576;
    const double sigma_trans = 5.0;
    for (double trans=0; trans<20; trans+=0.01) {
	cerr << "test_diffuser: trans=" << trans << endl;
	diff.diffuse(tau, trans, sigma_tau, sigma_trans, 1.0);
    }
    dump_hist(hist);
}


int main()
{
    //test1();
    test2();
}
