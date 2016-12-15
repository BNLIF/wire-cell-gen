#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellGen/GaussianDiffusion.h"


using namespace WireCell;

Gen::BinnedDiffusion::BinnedDiffusion(const Point& origin, const Vector& pitch_dir,
                                      int npitch_samples, double min_pitch, double max_pitch,
				      int ntime_samples, double min_time, double max_time,
				      double nsigma, bool fluc)
    : m_window(0,0)

    , m_origin(origin)
    , m_axis(pitch_dir.magnitude())

    , m_nimpacts(npitch_samples)
    , m_pitch_min(min_pitch)
    , m_pitch_max(max_pitch)
    , m_pitch_sample((max_pitch-min_pitch)/(npitch_samples-1))

    , m_nticks(ntime_samples)
    , m_time_min(min_time)
    , m_time_max(max_time)
    , m_time_sample((max_time-min_time)/(ntime_samples-1))

    , m_nsigma(nsigma)
    , m_fluctuate(fluc)
{
}

bool Gen::BinnedDiffusion::add(IDepo::pointer depo, double sigma_time, double sigma_pitch)
{
    // calculate time parameters and check if contained
    const double center_time = depo->time();
    Gen::GausDesc time_desc(center_time, sigma_time);
    if (time_desc.distance(m_time_min - 0.5*m_time_sample) < -m_nsigma) {
        return false;
    }
    if (time_desc.distance(m_time_max + 0.5*m_time_sample) > +m_nsigma) {    
        return false;
    }

    // calcualte pitch parameters and check if contained.
    const double center_pitch = pitch_distance(depo->pos());
    Gen::GausDesc pitch_desc(center_pitch, sigma_pitch);
    if (pitch_desc.distance(m_pitch_min - 0.5*m_pitch_sample) < -m_nsigma) {
        return false;
    }
    if (pitch_desc.distance(m_pitch_max + 0.5*m_pitch_sample) > +m_nsigma) {
        return false;
    }

    // make GD and add to all covered impacts
    int min_impact = std::max(int(round(center_pitch - sigma_pitch*m_nsigma)), 0);
    int max_impact = std::min(int(round(center_pitch + sigma_pitch*m_nsigma)), m_nimpacts-1);

    auto gd = std::make_shared<GaussianDiffusion>(depo, time_desc, pitch_desc);
    for (int imp = min_impact; imp <= max_impact; ++imp) {
        this->add(gd, imp);
    }

    return true;
}

void Gen::BinnedDiffusion::add(std::shared_ptr<GaussianDiffusion> gd, int impact)
{
    ImpactData::mutable_pointer idptr = nullptr;
    auto it = m_impacts.find(impact);
    if (it == m_impacts.end()) {
	idptr = std::make_shared<ImpactData>(impact);
	m_impacts[impact] = idptr;
    }
    else {
	idptr = it->second;
    }
    idptr->add(gd);
}

void Gen::BinnedDiffusion::erase(int begin_impact_number, int end_impact_number)
{
    // clear all less than begin_impact_number
    //
    //	    std::map<int, ImpactData::mutable_pointer> m_impacts;

    // Q and D
    for (int impact=begin_impact_number; impact<end_impact_number; ++impact) {
	m_impacts.erase(impact);
    }
}


Gen::ImpactData::pointer Gen::BinnedDiffusion::impact_data(int number) const
{
    if (number < 0 || number >= m_nimpacts) {
        return nullptr;
    }

    auto it = m_impacts.find(number);
    if (it == m_impacts.end()) {
	return nullptr;
    }
    auto idptr = it->second;

    // make sure all diffusions have been sampled 
    for (auto diff : idptr->diffusions()) {
        diff->set_sampling(m_nticks, m_time_min, m_time_max,
                           m_nimpacts, m_pitch_min, m_pitch_max,
                           m_nsigma, m_fluctuate);
    }

    idptr->calculate(m_nticks);
    return idptr;
}


double Gen::BinnedDiffusion::pitch_distance(const Point& pt) const
{
    return m_axis.dot(pt - m_origin);
}


int Gen::BinnedDiffusion::impact_index(double pitch) const
{
    return int(round((pitch - m_pitch_min) / m_pitch_sample));
}
