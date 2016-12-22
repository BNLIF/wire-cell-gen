#include "WireCellGen/GaussianDiffusion.h"

#include <iostream>		// debugging

using namespace WireCell;
using namespace std;

std::vector<double> Gen::GausDesc::sample(double start, double step, int nsamples) const
{
    std::vector<double> ret(nsamples, 0.0);
    for (int ind=0; ind<nsamples; ++ind) {
        const double rel = (start + ind*step - center)/sigma;
        ret[ind] = exp(-0.5*rel*rel);
    }
    return ret;
}

// std::pair<int,int> Gen::GausDesc::subsample_range(int nsamples, double xmin, double xmax, double nsigma) const
// {
//     const double sample_size = (xmax-xmin)/(nsamples-1);
//     // find closest sample indices
//     int imin = int(round((center - nsigma*sigma - xmin)/sample_size));
//     int imax = int(round((center + nsigma*sigma - xmin)/sample_size));
    
//     return std::make_pair(std::max(imin, 0), std::min(imax+1, nsamples));
// }



/// GaussianDiffusion


Gen::GaussianDiffusion::GaussianDiffusion(const IDepo::pointer& depo,
					  const GausDesc& time_desc, 
					  const GausDesc& pitch_desc)
    : m_deposition(depo)
    , m_time_desc(time_desc)
    , m_pitch_desc(pitch_desc)
    , m_toffset_bin(-1)
    , m_poffset_bin(-1)
{
}

void Gen::GaussianDiffusion::set_sampling(const Binning& tbin, const Binning& pbin,
                                          double nsigma, bool fluctuate)
{
    if (m_patch.size() > 0) {
        return;
    }

    /// Sample time dimension
    auto tval_range = m_time_desc.sigma_range(nsigma);
    auto tbin_range = tbin.sample_bin_range(tval_range.first, tval_range.second);
    const int ntss = tbin_range.second - tbin_range.first;
    m_toffset_bin = tbin_range.first;
    auto tvec =  m_time_desc.sample(tbin.center(m_toffset_bin), tbin.binsize(), ntss);
    //cerr << "tmin,gaus=" << tvalrange.first/units::us << " toff_bin=" << m_toffset_bin << endl;


    /// Sample pitch dimension.
    auto pval_range = m_pitch_desc.sigma_range(nsigma);
    auto pbin_range = pbin.sample_bin_range(pval_range.first, pval_range.second);
    const int npss = pbin_range.second - pbin_range.first+1;
    m_poffset_bin = pbin_range.first;
    auto pvec = m_pitch_desc.sample(pbin.center(m_poffset_bin), pbin.binsize(), npss);
    //cerr << "pmin,gaus=" << prange.first << " poff_edge=" << m_poffset_edge << endl;
    
    patch_t ret = patch_t::Zero(npss, ntss);
    double raw_sum=0.0;

    // Convolve the two independent Gaussians
    for (size_t ip = 0; ip < npss; ++ip) {
	for (size_t it = 0; it < ntss; ++it) {
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

	for (size_t ip = 0; ip < npss; ++ip) {
	    for (size_t it = 0; it < ntss; ++it) {
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


