#include "WireCellGen/Diffuser.h"

#include <cmath>

#include <iostream>
using namespace std;		// debugging

using namespace WireCell;

Diffuser::Diffuser(double binsize_l, double binsize_t,
		   double origin_l, double origin_t,
		   double nsigma)
    : m_origin_l(origin_l), m_origin_t(origin_t),
      m_binsize_l(binsize_l), m_binsize_t(binsize_t), 
      m_nsigma(nsigma)
{
}

Diffuser::~Diffuser()
{
}

std::vector<double> Diffuser::oned(double mean, double sigma, double binsize,
				   const Diffuser::bounds_type& bounds)
{
    int nbins = round((bounds.second - bounds.first)/binsize);

    /// fragment between bin_edge_{low,high}
    std::vector<double> integral(nbins+1, 0.0);
    for (int ibin=0; ibin <= nbins; ++ibin) {
	double absx = bounds.first + ibin*binsize;
	double t = 0.5*(absx-mean)/sigma;
	double e = 0.5*std::erf(t);
	integral[ibin] = e;
    }

    std::vector<double> bins;
    for (int ibin=0; ibin<nbins; ++ibin) {
	bins.push_back(integral[ibin+1] - integral[ibin]);
    }
    return bins;
}


Diffuser::bounds_type Diffuser::bounds(double mean, double sigma, double binsize, double origin)
{
    double low = floor( (mean - m_nsigma*sigma - origin) / binsize ) * binsize + origin;    
    double high = ceil( (mean + m_nsigma*sigma - origin) / binsize ) * binsize + origin;    

    return std::make_pair(low, high);
}


Diffusion::pointer Diffuser::operator()(double mean_l, double mean_t,
					double sigma_l, double sigma_t,
					double weight)
{
    bounds_type bounds_l = bounds(mean_l, sigma_l, m_binsize_l, m_origin_l);
    bounds_type bounds_t = bounds(mean_t, sigma_t, m_binsize_t, m_origin_t);

    std::vector<double> l_bins = oned(mean_l, sigma_l, m_binsize_l, bounds_l);
    std::vector<double> t_bins = oned(mean_t, sigma_t, m_binsize_t, bounds_t);

    if (l_bins.empty() || t_bins.empty()) {
	return nullptr;
    }

    // get normalization
    double power = 0;
    for (auto l : l_bins) {
	for (auto t : t_bins) {
	    power += l*t;
	}
    }
    if (power == 0.0) {
	return nullptr;
    }

    Diffusion* smear = new Diffusion(l_bins.size(), t_bins.size(),
				     bounds_l.first, bounds_t.first,
				     bounds_l.second, bounds_t.second);

    for (int ind_l = 0; ind_l < l_bins.size(); ++ind_l) {
	for (int ind_t = 0; ind_t < t_bins.size(); ++ind_t) {
	    smear->array[ind_l][ind_t] = l_bins[ind_l]*t_bins[ind_t]/power*weight;
	}
    }
    
    Diffusion::pointer ret(smear);
    return ret;
}
