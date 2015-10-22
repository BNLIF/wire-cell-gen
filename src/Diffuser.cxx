#include "WireCellGen/Diffuser.h"
#include "WireCellGen/Diffusion.h"

#include "WireCellUtil/Point.h"

#include <cmath>

#include <iostream>
using namespace std;		// debugging

using namespace WireCell;

Diffuser::Diffuser(const Ray& pitch,
		   double binsize_l,
		   double time_offset,
		   double origin_l,
		   double DL,
		   double DT,
		   double drift_velocity,
		   double max_sigma_l,
		   double nsigma)
    : m_pitch_origin(pitch.first)
    , m_pitch_direction(ray_unit(pitch))
    , m_time_offset(time_offset)
    , m_origin_l(origin_l)
    , m_origin_t(0.0)		// measure pitch direction from pitch_origin
    , m_binsize_l(binsize_l)
    , m_binsize_t(ray_length(pitch))
    , m_DL(DL)
    , m_DT(DT)
    , m_drift_velocity(drift_velocity)
    , m_max_sigma_l(max_sigma_l)
    , m_nsigma(nsigma)
{
}

Diffuser::~Diffuser()
{
}

void Diffuser::reset()
{
    m_input.clear();
    m_output.clear();
}
void Diffuser::flush()
{
    for (auto diff : m_input) {
	m_output.push_back(diff);
    }
    m_output.push_back(nullptr);
}
bool Diffuser::insert(const input_pointer& depo)
{
    if (!depo) {
	flush();
	return true;
    }

    auto first = *depo_chain(depo).rbegin();
    const double drift_distance = first->pos().x() - depo->pos().x();
    const double drift_time = drift_distance / m_drift_velocity;
    
    const double tmpcm2 = 2*m_DL*drift_time/units::centimeter2;
    const double sigmaL = sqrt(tmpcm2)*units::centimeter / m_drift_velocity;
    const double sigmaT = sqrt(2*m_DT*drift_time/units::centimeter2)*units::centimeter2;
	
    const Vector to_depo = depo->pos() - m_pitch_origin;
    const double pitch_distance = m_pitch_direction.dot(to_depo);

    IDiffusion::pointer diff = this->diffuse(m_time_offset + depo->time(), pitch_distance,
					     sigmaL, sigmaT, depo->charge(), depo);
    m_input.insert(diff);
    return true;
}

bool Diffuser::extract(output_pointer& diffusion)
{
    while (m_input.size() > 2) {
	auto first = *m_input.begin();
	auto last = *m_input.rbegin();
	const double last_center = 0.5*(last->lend() + last->lbegin());
	if (last_center - first->lbegin() < m_max_sigma_l*m_nsigma) {
	    break; // new input with long diffusion may still take lead
	}

	// now we are guaranteed no newly added diffusion can have a
	// leading edge long enough to surpass that of the current
	// leader.
	m_output.push_back(first);
	m_input.erase(first);
    }
	
    if (m_output.empty()) {
	return false;
    }
    diffusion = m_output.front();
    m_output.pop_front();
    return true;
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


IDiffusion::pointer Diffuser::diffuse(double mean_l, double mean_t,
				      double sigma_l, double sigma_t,
				      double weight, IDepo::pointer depo)
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

    Diffusion* smear = new Diffusion(depo,
				     l_bins.size(), t_bins.size(),
				     bounds_l.first, bounds_t.first,
				     bounds_l.second, bounds_t.second);

    for (int ind_l = 0; ind_l < l_bins.size(); ++ind_l) {
	for (int ind_t = 0; ind_t < t_bins.size(); ++ind_t) {
	    double value = l_bins[ind_l]*t_bins[ind_t]/power*weight;
	    smear->set(ind_l, ind_t, value);
	}
    }
    
    IDiffusion::pointer ret(smear);
    return ret;
}


