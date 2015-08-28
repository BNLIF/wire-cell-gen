#include "WireCellGen/Diffuser.h"

#include <cmath>

#include <iostream>
using namespace std;		// debugging

using namespace WireCell;

Diffuser::Diffuser(BufferedHistogram2D& hist, int nsigma)
    : m_hist(hist)
    , m_nsigma(nsigma)
{
}
Diffuser::~Diffuser()
{
}

std::vector<double> Diffuser::oned(double mean, double sigma, double origin, double binsize) const
{
    // find the high/low edges of bins containing nsigma 
    int bin_edge_low = floor((mean - m_nsigma*sigma - origin)/binsize);
    int bin_edge_high = ceil((mean + m_nsigma*sigma - origin)/binsize);

    if (bin_edge_high < 0) {
	return std::vector<double>();
    }
    if (bin_edge_low < 0) {
	bin_edge_low = 0;
    }
    bin_edge_high = max(bin_edge_low+1, bin_edge_high);

    int nbins = bin_edge_high - bin_edge_low; // >= 1

    // cerr << "Diffuse: oned(mean="<<mean<<", sigma="<<sigma<<", origin="<<origin<<", binsize="<<binsize<<") "
    // 	 << "nbins=" << nbins << ":["<<bin_edge_low<<","<<bin_edge_high<<"]" << endl;

    /// fragment between bin_edge_{low,high}
    std::vector<double> integral(nbins+1, 0.0);
    for (int ibin=0; ibin <= nbins; ++ibin) {
	double absx = origin + (bin_edge_low + ibin)*binsize;
	double t = 0.5*(absx-mean)/sigma;
	double e = 0.5*std::erf(t);
	integral[ibin] = e;
	// cerr << "\tintegral[" << ibin << "] = " << integral[ibin] << " at absx=" << absx << " relx=" << t << endl;
    }

    std::vector<double> bins(bin_edge_high,0.0);
    for (int ibin=0; ibin<nbins; ++ibin) {
	bins[ibin+bin_edge_low] = integral[ibin+1] - integral[ibin];
	// cerr << "\tbins["<<ibin<<"+"<<bin_edge_low<<"] = " << bins[ibin+bin_edge_low]
	//      << " = " << integral[ibin+1] << " - " << integral[ibin] << endl;
    }
    return bins;
}

double Diffuser::diffuse(double x, double y, double sigma_x, double sigma_y, double q)
{
    std::vector<double> xbins = oned(x, sigma_x, m_hist.xmin(), m_hist.xbinsize());
    if (!xbins.size()) {
	return m_hist.xmin();
    }
    std::vector<double> ybins = oned(y, sigma_y, m_hist.ymin(), m_hist.ybinsize());
    if (!ybins.size()) {
	return m_hist.xmin();
    }

    // get normalization
    double power = 0;
    for (int xind = 0; xind < xbins.size(); ++xind) {
	for (int yind = 0; yind < ybins.size(); ++yind) {
	    double p = xbins[xind]*ybins[yind];
	    //cerr << xind << "," << yind << ": " << xbins[xind] << "," << ybins[yind] << " = " << p << endl;
	    power += p;
	}
    }
    if (power == 0.0) {
	return m_hist.xmin();
    }
	

    // cerr << "nbins="<<xbins.size() << " X " << ybins.size() << " power = " << power << endl;
    
    for (int xind = 0; xind < xbins.size(); ++xind) {
	for (int yind = 0; yind < ybins.size(); ++yind) {
	    double xval = m_hist.xmin() + (xind+0.5)*m_hist.xbinsize();
	    double yval = m_hist.ymin() + (yind+0.5)*m_hist.ybinsize();
	    double val = xbins[xind]*ybins[yind]*q/power;
	    if (val <= 0.0) {
		continue;
	    }
	    bool ok = m_hist.fill(xval, yval, val);
	    if (!ok) {
		cerr << "underflow: " << "(" << xval << "," << yval << ") = " << val << endl;
	    }
	}
    }
    

    // return lowest new data
    for (int xind = 0; xind < xbins.size(); ++xind) {
	if (xbins[xind] > 0.0) {
	    return m_hist.xmin() + xind * m_hist.xbinsize();
	}
    }
    return m_hist.xmin();
}
