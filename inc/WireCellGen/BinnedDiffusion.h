#ifndef WIRECELLGEN_BINNEDDIFFUSION
#define WIRECELLGEN_BINNEDDIFFUSION

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"
#include "WireCellIface/IDepo.h"

#include "WireCellGen/ImpactData.h"

#include <deque>

namespace WireCell {
    namespace Gen {


	/**  A BinnedDiffusion takes IDepo objects and "paints" them
	 *  onto bins in time and pitch direction (impact position).
	 *
	 *  Partial results are provided inside a window of fixed
	 *  width in the pitch direction.
	 *
	 *  Results are either provided in time or frequency domain.
	 *
	 */
	// fixme: this class is too much of a kitchen sink.
	class BinnedDiffusion {
	public:
	    BinnedDiffusion(const Ray& pitch_bin,
			    int ntime_bins, double min_time, double max_time,
			    double nsigma=3.0);


	    /// Add a diffusion.
	    // Fixme: this is not an IDiffusion! Do not want a large binned object here.
	    void add(IDepo::pointer deposition, double sigma_time, double sigma_pitch);

	    /// Set the window coverage to the half-open range.  This
	    /// should be called after all IDepo objects have been
	    /// added.  Any impact positions seen in the last window
	    /// are not recalculated so sliding the window is optimal,
	    /// random access will lose optimization.
	    void set_window(int begin_impact_number, int end_impact_number);

	    /// Return the data at the give impact position.
	    ImpactData::pointer impact_data(int number) const;

	    // return the distance of the point along the pitch direction
	    double pitch_distance(const Point& pt) const;

	    // return the impact number closest to the given pitch distance.
	    int impact_number(double pitch) const;


	private:
	    
	    // current window set by user.
	    std::pair<int,int> m_window;
	    // actual extent needed to handle diffusion atomically.
	    std::pair<int,int> m_extent;
	    // the content of the current window
	    std::map<int, ImpactData::mutable_pointer> m_impacts;

	    // 3-point that is the origin of the pitch measurement
	    Point m_origin;
	    // unit vector pointing in pitch direction
	    Vector m_pitch_axis;
	    // distance between impact positions
	    double m_impact_bin;
	    
	    std::tuple<int, double, double> m_time_bins;
	    double m_nsigma;

	};


    }

}

#endif
