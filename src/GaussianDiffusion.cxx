#include "WireCellGen/GaussianDiffusion.h"

#include <iostream>		// debugging

using namespace WireCell;

std::vector<double> Gen::GausDesc::sample(int nsamples, double xmin, double xmax) const
{
    const double step = (xmax-xmin)/(nsamples-1);
    std::vector<double> ret(nsamples, 0.0);
    for (int ind=0; ind<nsamples; ++ind) {
        const double rel = (xmin + ind*step - center)/sigma;
        ret[ind] = exp(-0.5*rel*rel);
    }
    return ret;
}

std::pair<int,int> Gen::GausDesc::subsample_range(int nsamples, double xmin, double xmax, double nsigma) const
{
    const double sample_size = (xmax-xmin)/(nsamples-1);
    // find closest sample indices
    int imin = int(round((center - nsigma*sigma - xmin)/sample_size));
    int imax = int(round((center + nsigma*sigma - xmin)/sample_size));
    
    return std::make_pair(std::max(imin, 0), std::min(imax+1, nsamples));
}




Gen::GaussianDiffusion::GaussianDiffusion(const IDepo::pointer& depo,
					  const GausDesc& time_desc, 
					  const GausDesc& pitch_desc)
    : m_deposition(depo)
    , m_time_desc(time_desc)
    , m_pitch_desc(pitch_desc)
    , m_toffset(-1)
    , m_poffset(-1)
{
}

void Gen::GaussianDiffusion::set_sampling(int nt, double tmin, double tmax, 
                                          int np, double pmin, double pmax,
                                          double nsigma, bool fluctuate)
{
    if (m_patch.size() > 0) {
        return;
    }

    const double dt = (tmax - tmin)/(nt-1);
    const double dp = (pmax - pmin)/(np-1);

    auto trange = m_time_desc.subsample_range(nt, tmin, tmax, nsigma);
    auto prange = m_pitch_desc.subsample_range(np, pmin, pmax, nsigma);

    m_toffset = trange.first;
    m_poffset = prange.first;

    const double tminss = tmin + m_toffset*dt;
    const double pminss = pmin + m_poffset*dp;

    const int ntss = trange.second - trange.first;
    const int npss = prange.second - prange.first;

    auto tvec = m_time_desc.sample (ntss, tminss, tminss + dt*ntss);
    auto pvec = m_pitch_desc.sample(npss, pminss, pminss + dp*npss);
    
    patch_t ret = patch_t::Zero(np, nt);
    double raw_sum=0.0;

    // Convolve the two independent Gaussians
    for (size_t ip = 0; ip < np; ++ip) {
	for (size_t it = 0; it < nt; ++it) {
	    const double val = pvec[ip]*tvec[it];
	    raw_sum += val;
	    ret(ip,it) = (float)val;
	}
    }

    // normalize to total charge
    ret *= m_deposition->charge() / raw_sum;

    if (fluctuate) {
	double fluc_sum = 0;
	std::default_random_engine generator;

	for (size_t ip = 0; ip < np; ++ip) {
	    for (size_t it = 0; it < nt; ++it) {
		const float oldval = ret(ip,it);
		std::poisson_distribution<int> poisson(oldval);
		float nelectrons = poisson(generator);
		fluc_sum += nelectrons;
		ret(ip,it) = nelectrons;
	    }
	}

	ret *= m_deposition->charge() / fluc_sum;
    }

    m_patch = ret;
}

// patch = nimpacts rows X nticks columns
// patch(row,col)
const Gen::GaussianDiffusion::patch_t& Gen::GaussianDiffusion::patch() const
{
    return m_patch;
}


