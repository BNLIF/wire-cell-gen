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
    , m_ntimes(ntime_samples)
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
    const Vector to_depo = depo->pos() - m_pitch_origin;
    const double pitch_center = to_depo.dot(m_pitch_axis);
    const double pitch_min = pitch_center - m_nsigma*sigma_pitch;
    const double pitch_max = pitch_center + m_nsigma*sigma_pitch;
    const int pitch_bin0 = int(round(pitch_min / m_pitch_binsize));
    const int pitch_binf = int(round(pitch_max / m_pitch_binsize));
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
    auto id = m_impacts[impact];
    if (id == nullptr) {
	id = std::make_shared<ImpactData>(impact, impact*m_pitch_binsize); // fixme: maybe include origin?
	m_impacts[impact] = id;
    }
    id->add(gd);
}

void Gen::BinnedDiffusion::set_window(int begin_impact_number, int end_impact_number)
{
}


Gen::ImpactData::pointer Gen::BinnedDiffusion::impact_data(int number) const
{
}


double Gen::BinnedDiffusion::pitch_distance(const Point& pt) const
{
}


int Gen::BinnedDiffusion::impact_number(double pitch) const
{

}
