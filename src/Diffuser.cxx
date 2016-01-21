#include "WireCellGen/Diffuser.h"
#include "WireCellGen/Diffusion.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellUtil/Point.h"

#include <cmath>

#include <iostream>
using namespace std;		// debugging

using namespace WireCell;

WIRECELL_NAMEDFACTORY_BEGIN(Diffuser)
WIRECELL_NAMEDFACTORY_INTERFACE(Diffuser, IDiffuser);
WIRECELL_NAMEDFACTORY_INTERFACE(Diffuser, IConfigurable);
WIRECELL_NAMEDFACTORY_END(Diffuser)

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
    , m_eos(false)
{
}

Diffuser::~Diffuser()
{
}

Configuration Diffuser::default_configuration() const
{
    std::string json = R"(
{
"pitch_origin_mm":[0.0,0.0,0.0],
"pitch_direction":[0.0,0.0,1.0],
"pitch_distance_mm":5.0,
"timesslice_ms":2.0,
"timesoffset_ms":0.0,
"starttime_ms":0.0,
"origin_mm":0.0,
"DL_ccps":5.3,
"DT_ccps":12.8,
"drift_velocity_mmpus":1.6,
"max_sigma_l_us":5.0,
"nsigma":3.0
}
)";
    return configuration_loads(json, "json");
}

void Diffuser::configure(const Configuration& cfg)
{
    m_pitch_origin = get<Point>(cfg, "pitch_origin_mm", m_pitch_origin/units::mm)*units::mm;
    m_pitch_direction = get<Point>(cfg, "pitch_direction", m_pitch_direction).norm();
    m_time_offset = get<double>(cfg, "timeoffset_ms", m_time_offset/units::ms)*units::ms;

    m_origin_l = get<double>(cfg, "starttime_ms", m_origin_l/units::ms)*units::ms;
    m_origin_t = get<double>(cfg, "origin_mm", m_origin_t/units::mm)*units::mm;

    m_binsize_l = get<double>(cfg, "timeslice_ms", m_binsize_l/units::mm)*units::mm;
    m_binsize_t = get<double>(cfg, "pitch_distance_mm", m_binsize_t/units::mm)*units::mm;

    const double ccps = units::centimeter2/units::second;
    m_DL = get<double>(cfg, "DL_ccps", m_DL/ccps)*ccps;
    m_DT = get<double>(cfg, "DT_ccps", m_DT/ccps)*ccps;

    const double mmpus = units::millimeter/units::microsecond;
    m_drift_velocity = get<double>(cfg, "drift_velocity_mmpus", m_drift_velocity/mmpus)*mmpus;

    m_max_sigma_l = get<double>(cfg, "max_sigma_l_us", m_max_sigma_l);
    m_nsigma = get<double>(cfg, "nsigma", m_nsigma);
}

void Diffuser::reset()
{
    m_input.clear();
}

bool Diffuser::operator()(const input_pointer& depo, output_queue& outq)
{
    if (m_eos) {
	return false;
    }
    if (!depo) {		// EOS flush
	for (auto diff : m_input) {
	    outq.push_back(diff);
	}
	outq.push_back(nullptr);
	m_eos = true;
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
	outq.push_back(first);
	m_input.erase(first);
    }

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


