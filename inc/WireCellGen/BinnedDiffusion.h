#ifndef WIRECELLGEN_BINNEDDIFFUSION
#define WIRECELLGEN_BINNEDDIFFUSION

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"
#include "WireCellIface/IDepo.h"

#include "WireCellGen/ImpactData.h"

#include <deque>

namespace WireCell {
    namespace Gen {


	/**  A BinnedDiffusion takes IDepo objects and "paints" their
	 *  diffusion onto bins in time and pitch direction (impact
	 *  position).
	 *
	 *  Partial results are provided inside a window of fixed
	 *  width in the pitch direction.
	 *
	 *  Results are either provided in time or frequency domain.
	 *
	 */
	class BinnedDiffusion {
	public:

	    /** Created a BinnedDiffusion. 
	     *	
	     * - pitch_coordinate :: ray perpendicular to and iwth
	     * tail on center of first wire and with length of one
	     * impact position.
	     *
	     * - ntime_samples :: number of samples in time
	     * - time_0 :: time of first sample
	     * - time_f :: time of final sample
	     * - nsigma :: number of sigma the 2D (transverse X longitudinal) Gaussian extends.
	     * - fluctuate :: set to true if charge-preserving Poisson fluctuations are applied.
	     */
	    BinnedDiffusion(const Ray& pitch_coordinate,
			    int ntime_samples, double time_0, double time_f,
			    double nsigma=3.0, bool fluctuate=false);


	    /// Add a diffusion.
	    void add(IDepo::pointer deposition, double sigma_time, double sigma_pitch);

	    /// Add a already build GaussianDiffusion to all relevant impacts.
	    // Fixme: this is not an IDiffusion! Do not want a large binned object here.
	    void add(std::shared_ptr<GaussianDiffusion> gd);

	    /// Add a already build GaussianDiffusion to one impact.
	    // Fixme: this is not an IDiffusion! Do not want a large binned object here.
	    void add(std::shared_ptr<GaussianDiffusion> gd, int impact);

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
	    // the content of the current window
	    std::map<int, ImpactData::mutable_pointer> m_impacts;

	    // 3-point that is the origin of the pitch measurement
	    Point m_pitch_origin;
	    // unit vector pointing in pitch direction
	    Vector m_pitch_axis;
	    // distance between impact positions
	    double m_pitch_binsize;
	    
	    int m_ntimes;
	    double m_time_origin;
	    double m_time_binsize;

	    double m_nsigma;
	    bool m_fluctuate;
	};


    }

}

#endif
