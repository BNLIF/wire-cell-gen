#include "WireCellGen/GaussianDiffusion.h"
using namespace WireCell;


Gen::GaussianDiffusion::GaussianDiffusion(const IDepo::pointer& depo,
					  double p_sigma, double t_sigma,
					  const Point& p_origin, const Vector& p_dir, double p_bin,
					  double torigin, double t_bin,
					  double nsigma, bool fluctuate)
    : m_deposition(depo)

    , m_sigma_pitch(p_sigma)
    , m_sigma_time(t_sigma)

    , m_pitch_begin(0.0)
    , m_time_begin(0.0)

    , m_pitch_bin(p_bin)
    , m_time_bin(t_bin)

    , m_np(0)
    , m_nt(0)

    , m_nsigma(nsigma)
    , m_fluctuate(fluctuate)
{
    // pitch parameters.
    const Vector to_depo = depo->pos() - p_origin;
    const double pitch_center = to_depo.dot(p_dir);
    const double pitch_min = pitch_center - nsigma*p_sigma;
    const double pitch_max = pitch_center + nsigma*p_sigma;
    const int pitch_bin0 = int(round(pitch_min / p_bin));
    const int pitch_binf = int(round(pitch_max / p_bin));
    m_np = 1 + pitch_binf - pitch_bin0;
    m_pitch_begin = pitch_center - pitch_bin0*p_bin;

    // time parameters
    const double time_center = depo->time() - torigin;
    const double time_min = time_center - nsigma*t_sigma;
    const double time_max = time_center + nsigma*t_sigma;
    const int time_bin0 = int(round(time_min / t_bin));
    const int time_binf = int(round(time_max / t_bin));
    m_nt = 1 + time_binf - time_bin0;
    m_time_begin = time_center - time_bin0*t_bin;
}


// patch = nimpacts rows X nticks columns
// patch(row,col)
const Gen::GaussianDiffusion::patch_t& Gen::GaussianDiffusion::patch() const
{
    if (m_patch.size() > 0) {
	return m_patch;
    }


    // pitch Gaussian
    std::vector<double> pval(m_np);
    for (int ip = 0; ip < m_np; ++ip) {
	const double relp = (m_pitch_begin + ip*m_pitch_bin) / m_sigma_pitch;
	pval[ip] = exp(0.5*relp*relp);
    }

    // time Gaussian
    std::vector<double> tval(m_nt);
    for (int it = 0; it < m_nt; ++it) {
	const double relt = (m_time_begin + it*m_time_bin) / m_sigma_time;
	tval[it] = exp(0.5*relt*relt);
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
