#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellGen/GaussianDiffusion.h"


using namespace WireCell;

Gen::BinnedDiffusion::BinnedDiffusion(const Ray& pitch_bin,
				      int ntime_bins, double min_time, double max_time,
				      double nsigma)
    : m_window(0,0)
    , m_extent(0,0)
    , m_origin(pitch_bin.first)
    , m_time_bins(ntime_bins, min_time, max_time)
    , m_nsigma(nsigma)
{

}

void Gen::BinnedDiffusion::add(IDepo::pointer deposition, double sigma_time, double sigma_pitch)
{
    // do:
    //
    // 1) get pitch distance from depo
    //
    // 2) get mean time from depo->time()
    //
    // 3) get np, nt

    // 	    GaussianDiffusion(const IDepo::pointer& depo,
    // 			      double mean_p, double mean_t,
    // 			      double sigma_p, double sigma_t,
    // 			      int np, int nt,
    // 			      double nsigma=3.0, bool fluctuate = false);


    // auto diff = std::make_shared<Gen::GaussianDiffusion>(deposition, sigma_time, sigma_pitch);
    ///
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
