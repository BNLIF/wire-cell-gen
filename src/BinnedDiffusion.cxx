#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellGen/GaussianDiffusion.h"


using namespace WireCell;

Gen::BinnedDiffusion::BinnedDiffusion(const Ray& impact,
				      int ntime_samples, double min_time, double max_time,
				      double nsigma, bool fluc)
    : m_window(0,0)
    , m_pitch_origin(impact.first)
    , m_pitch_axis(ray_unit(impact))
    , m_pitch_binsize(ray_length(impact))
    , m_nticks(ntime_samples)
    , m_time_origin(min_time)
    , m_time_binsize((max_time-min_time)/(ntime_samples-1))
    , m_nsigma(nsigma)
    , m_fluctuate(fluc)
{
}

void Gen::BinnedDiffusion::add(IDepo::pointer depo, double sigma_time, double sigma_pitch)
{
    // time parameters
    const double time_center = depo->time() - m_time_origin;
    const double time_min = time_center - m_nsigma*sigma_time;
    const double time_max = time_center + m_nsigma*sigma_time;
    const int time_bin0 = int(round(time_min / m_time_binsize));
    const int time_binf = int(round(time_max / m_time_binsize));
    const int nt = 1 + time_binf - time_bin0;

    Gen::GausDesc time_desc(time_center, sigma_time, m_time_binsize, nt, time_bin0);

    // pitch parameters.
    const double pitch_center = pitch_distance(depo->pos());
    const double pitch_min = pitch_center - m_nsigma*sigma_pitch;
    const double pitch_max = pitch_center + m_nsigma*sigma_pitch;
    const int pitch_bin0 = impact_number(pitch_min);
    const int pitch_binf = impact_number(pitch_max);
    const int np = 1 + pitch_binf - pitch_bin0;

    Gen::GausDesc pitch_desc(pitch_center, sigma_pitch, m_pitch_binsize, np, pitch_bin0);

    auto gd = std::make_shared<GaussianDiffusion>(depo, time_desc, pitch_desc, m_fluctuate);
    this->add(gd);

}

void Gen::BinnedDiffusion::add(std::shared_ptr<GaussianDiffusion> gd)
{
    auto pd = gd->pitch_desc();
    auto arange = pd.absolute_range();
    for (int ind=arange.first; ind<arange.second; ++ind) {
	this->add(gd, ind);
    }
}

void Gen::BinnedDiffusion::add(std::shared_ptr<GaussianDiffusion> gd, int impact)
{
    auto it = m_impacts.find(impact);
    ImpactData::mutable_pointer id = nullptr;
    if (it == m_impacts.end()) {
	id = std::make_shared<ImpactData>(impact);
	m_impacts[impact] = id;
    }
    else {
	id = it->second;
    }
    id->add(gd);
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
    auto ret = m_impacts.find(number);
    if (ret == m_impacts.end()) {
	return nullptr;
    }
    auto id = ret->second;
    if (id->waveform().empty()) {
	id->calculate(m_nticks);
    }
    return id;
}


double Gen::BinnedDiffusion::pitch_distance(const Point& pt) const
{
    return m_pitch_axis.dot(pt - m_pitch_origin);
}


int Gen::BinnedDiffusion::impact_number(double pitch) const
{
    return int(round(pitch/m_pitch_binsize));

}
