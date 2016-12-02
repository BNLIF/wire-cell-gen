#include "WireCellGen/GaussianDiffusion.h"

#include <iostream>		// debugging

using namespace WireCell;

Gen::GaussianDiffusion::GaussianDiffusion(const IDepo::pointer& depo,
					  const GausDesc& time_desc, 
					  const GausDesc& pitch_desc,
					  bool fluctuate)
    : m_deposition(depo)
    , m_time_desc(time_desc)
    , m_pitch_desc(pitch_desc)
    , m_fluctuate(fluctuate)
{
}

// patch = nimpacts rows X nticks columns
// patch(row,col)
const Gen::GaussianDiffusion::patch_t& Gen::GaussianDiffusion::patch() const
{
    if (m_patch.size() > 0) {
	return m_patch;
    }
    
    auto pval = m_pitch_desc.sample();
    auto psiz = pval.size();
    auto tval = m_time_desc.sample();
    auto tsiz = tval.size();

    patch_t ret = patch_t::Zero(psiz, tsiz);
    double raw_sum=0.0;

    for (size_t ip = 0; ip < psiz; ++ip) {
	for (size_t it = 0; it < tsiz; ++it) {
	    const double val = pval[ip]*tval[it];
	    raw_sum += val;
	    ret(ip,it) = (float)val;
	}
    }

    // normalize to total charge
    ret *= m_deposition->charge() / raw_sum;

    if (m_fluctuate) {
	double fluc_sum = 0;
	std::default_random_engine generator;

	for (size_t ip = 0; ip < psiz; ++ip) {
	    for (size_t it = 0; it < tsiz; ++it) {
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
    return m_patch;
}


