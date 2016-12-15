/**
   

 */

#ifndef WIRECELLGEN_BINNEDDIFFUSION
#define WIRECELLGEN_BINNEDDIFFUSION

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"
#include "WireCellIface/IDepo.h"

#include "WireCellGen/ImpactData.h"

#include <deque>

namespace WireCell {
    namespace Gen {


	/**  A BinnedDiffusion maintains an association between impact
	 * positions along the pitch direction of a wire plane and
	 * the diffused depositions that drift to them.
         *
         * It covers a fixed and discretely sampled time and pitch
         * domain.
	 */
	class BinnedDiffusion {
	public:

	    /** Create a BinnedDiffusion. 

               Arguments are:
	      	
               - pitch_origin :: the 3-space position pointing to the
                 origin of the pitch coordinate.  This may typically
                 be the center of the zero'th wire in the plane.

               - pitch_direction :: a 3-vector of unit length pointing
                 in the wire pitch direction.

               - npitch_samples :: the number of samples of impact
                 positions along the pitch direction.

               - min_pitch :: the minimum pitch location to sample.

               - max_pitch :: the maximum pitch location to sample.
               
	       - ntime_samples :: number of samples in time
              
	       - min_time :: the minimum time to sample
              
	       - max_time :: the maximum time to sample
              
	       - nsigma :: number of sigma the 2D (transverse X
                 longitudinal) Gaussian extends.
              
	       - fluctuate :: set to true if charge-preserving Poisson
                 fluctuations are applied.
	     */
	    BinnedDiffusion(const Point& pitch_origin, const Vector& pitch_direction,
                            int npitch_samples, double min_pitch, double max_pitch,
			    int ntime_samples, double min_time, double max_time,
			    double nsigma=3.0, bool fluctuate=false);


	    /// Add a deposition and its associated diffusion sigmas.
	    /// Return false if no activity falls within the domain.
	    bool add(IDepo::pointer deposition, double sigma_time, double sigma_pitch);

	    /// Unconditionally associate an already built
	    /// GaussianDiffusion to one impact.  
	    void add(std::shared_ptr<GaussianDiffusion> gd, int impact_index);

	    /// Drop any stored ImpactData withing the half open
	    /// impact index range.
	    void erase(int begin_impact_index, int end_impact_index);

	    /// Return the data at the give impact position.
	    ImpactData::pointer impact_data(int impact_index) const;

	    // Return the absolute distance of the point along the
	    // pitch direction.  This may be outside the pitch domain.
	    double pitch_distance(const Point& pt) const;

	    // Return the impact index in the domain closest to the
	    // given pitch distance.  Negative or greater or equal to
	    // nsamples() indicates outside of the pitch domain.
	    int impact_index(double pitch) const;

            int nsamples() const { return m_nticks;}


	private:
	    
	    // current window set by user.
	    std::pair<int,int> m_window;
	    // the content of the current window
	    std::map<int, ImpactData::mutable_pointer> m_impacts;

	    // 3-point that is the origin of the pitch measurement
	    const Point m_origin;
	    // unit vector pointing in pitch direction
	    const Vector m_axis;

            const int m_nimpacts;
            const double m_pitch_min;
            const double m_pitch_max;
            const double m_pitch_sample;
	    
	    const int m_nticks;
	    const double m_time_min;
	    const double m_time_max;
            const double m_time_sample;

	    double m_nsigma;
	    bool m_fluctuate;
	};


    }

}

#endif
