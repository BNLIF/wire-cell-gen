#include "WireCellGen/GaussianDiffusion.h"

#include <iostream>		// debugging

using namespace WireCell;


Gen::GaussianDiffusion::GaussianDiffusion(const IDepo::pointer& depo,
					  double p_sigma, double t_sigma,
					  const Point& p_origin, const Vector& p_dir, double p_bin,
					  double torigin, double t_bin,
					  double nsigma, bool fluctuate)
    : m_deposition(depo)

    , m_sigma_pitch(p_sigma)
    , m_sigma_time(t_sigma)

    , m_pitch_center(0.0)	// w.r.t. p_origin
    , m_time_center(0.0)	// w.r.t. t_origin

    , m_pitch_binsize(p_bin)
    , m_time_binsize(t_bin)

    , m_np(0)
    , m_nt(0)

    , m_pitch_bin0(0)		// w.r.t. bin at p_origin
    , m_time_bin0(0)		// w.r.t. bin at t_origin

    , m_nsigma(nsigma)
    , m_fluctuate(fluctuate)
{
    
    // time parameters
    m_time_center = depo->time() - torigin;
    const double time_min = m_time_center - nsigma*t_sigma;
    const double time_max = m_time_center + nsigma*t_sigma;
    const int time_bin0 = int(round(time_min / t_bin));
    const int time_binf = int(round(time_max / t_bin));
    m_time_bin0 = time_bin0;
    m_nt = 1 + time_binf - time_bin0;


    // pitch parameters.
    const Vector to_depo = depo->pos() - p_origin;
    m_pitch_center = to_depo.dot(p_dir);
    const double pitch_min = m_pitch_center - nsigma*p_sigma;
    const double pitch_max = m_pitch_center + nsigma*p_sigma;
    const int pitch_bin0 = int(round(pitch_min / p_bin));
    const int pitch_binf = int(round(pitch_max / p_bin));
    m_pitch_bin0 = pitch_bin0;
    m_np = 1 + pitch_binf - pitch_bin0;
}



std::pair<double,double> Gen::GaussianDiffusion::minmax_pitch() const
{
    const double pmin = m_pitch_bin0 * m_pitch_binsize;
    const double pmax = pmin + (m_np-1)*m_pitch_binsize;
    return std::make_pair(pmin, pmax);
}

std::pair<double,double> Gen::GaussianDiffusion::minmax_time() const
{
    const double tmin = m_time_bin0 * m_time_binsize;
    const double tmax = tmin + (m_nt-1)*m_time_binsize;
    return std::make_pair(tmin, tmax);
}

std::pair<double,double> Gen::GaussianDiffusion::center() const
{
    return std::make_pair(m_time_center, m_pitch_center);
}

// patch = nimpacts rows X nticks columns
// patch(row,col)
const Gen::GaussianDiffusion::patch_t& Gen::GaussianDiffusion::patch() const
{
    if (m_patch.size() > 0) {
	return m_patch;
    }


    // pitch Gaussian
    const double pmin = m_pitch_bin0 * m_pitch_binsize;
    std::vector<double> pval(m_np);
    for (int ip = 0; ip < m_np; ++ip) {
	const double relp = (pmin + ip*m_pitch_binsize - m_pitch_center)/m_sigma_pitch;
	pval[ip] = exp(-0.5*relp*relp);
    }

    // time Gaussian
    const double tmin = m_time_bin0 * m_time_binsize;
    std::vector<double> tval(m_nt);
    for (int it = 0; it < m_nt; ++it) {
	const double relt = (tmin + it*m_time_binsize - m_time_center)/m_sigma_time;
	tval[it] = exp(-0.5*relt*relt);
    }

    // convolve the two Gaussians
    patch_t ret = patch_t::Zero(m_np, m_nt);
    double raw_sum=0.0;
    for (int ip = 0; ip < m_np; ++ip) { // impact/pitch position/rows
	for (int it = 0; it < m_nt; ++it) { // ticks/cols
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

	for (int ip = 0; ip < m_np; ++ip) { // impact/pitch position/rows
	    for (int it = 0; it < m_nt; ++it) { // ticks/cols
		const float oldval = ret(ip,it);
		std::poisson_distribution<int> poisson(oldval);
		int nelectrons = poisson(generator);
		fluc_sum += nelectrons;
		ret(ip,it) = (float)nelectrons;
	    }
	}

	ret *= m_deposition->charge() / fluc_sum;
    }

    m_patch = ret;
    return m_patch;
}
