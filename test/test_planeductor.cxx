#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Diffusion.h"

#include "WireCellIface/WirePlaneId.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Units.h"

#include <iostream>
#include <algorithm> 

using namespace WireCell;
using namespace std;



int main()
{
    const int nlong = 10;
    const int ntrans = 10;
    const double lbin = 0.5*units::microsecond;
    const double tbin = 5.0*units::millimeter;
    const double lpos0 = 0.0*units::microsecond;
    const double tpos0 = 0.0*units::millimeter;

    const double lmean = 0.5*nlong*lbin;
    const double lsigma = 3*lbin;
    const double tmean = 0.5*ntrans*tbin;
    const double tsigma = 3*tbin;

    Diffusion* diffusion =
	new Diffusion(nullptr, nlong, ntrans, lpos0, tpos0, nlong*lbin, ntrans*tbin);
    for (int tind=0; tind<ntrans; ++tind) {
	const double t = tpos0 + (0.5*ntrans - tind) * tbin;
	double tx = (t-tmean)/tsigma;
	tx *= tx;
	for (int lind=0; lind<nlong; ++lind) {
	    const double l = lpos0 + (0.5*nlong - lind) * lbin;	    
	    double lx = (l-lmean)/lsigma;
	    lx *= lx;
	    const double v = exp(-(tx + lx));
	    cerr << "set("<<lind<<" , " <<tind<< " , " << v << ")" << endl;
	    diffusion->set(lind,tind,v);
	}
    }
    IDiffusion::pointer idiff(diffusion);


    const WirePlaneId wpid(kWlayer);
    PlaneDuctor pd(wpid, lbin, tbin, lpos0, tpos0);

    Assert(pd.insert(idiff));
    pd.flush();
    IPlaneSlice::pointer ps;
    Assert(pd.extract(ps));
    Assert(ps->planeid() == wpid);
    cerr << "lpos0 = " << lpos0 << " ps->time() = " << ps->time() << endl;
    Assert(lpos0 == ps->time());
    cerr << "# charge runs: " << ps->charge_runs().size() << endl;
    Assert(ps->charge_runs().size() > 0);
    Assert(ps->flatten().size() > 0);
    
    return 0;
}
