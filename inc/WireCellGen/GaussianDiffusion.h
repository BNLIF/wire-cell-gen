#ifndef WIRECELLGEN_GaussianDiffusion
#define WIRECELLGEN_GaussianDiffusion

#include "WireCellUtil/Array.h"
#include "WireCellIface/IDepo.h"

#include <memory>

namespace WireCell {
    namespace Gen {

	/** A GausDesc describes one dimension of a sampled Gaussian
         * distribution.
         *
         * Two are used by GaussianDiffusion.  One describes the
         * transverse dimension along the direction of wire pitch (and
         * for a given wire plane) and one the longitudinal dimension
         * is along the drift direction as measured in time.  */
	struct GausDesc {

            /// The absolute location of the mean of the Gaussian as
            /// measured relative to some externally defined origin.
	    double center;
            /// The Gaussian sigma (half) width.
	    double sigma;
            /// The distance between two consecutive samples of the Gaussian.
//	    double sample_size;
            /// The location of the first sample
//            double sample_zero;
            /// The total number of samples over the domain.
//	    int nsamples;

//	    GausDesc(double center, double sigma, double sample_size, double sample_zero, int nsamples)
            GausDesc(double center, double sigma)
                : center(center)
                , sigma(sigma)
//                , sample_size(sample_size)
//                , sample_zero(sample_zero)
//		, nsamples(nsamples)
		{ }

            /// Return the distance in number of sigma that x is from the center
            double distance(double x) {
                return (x-center)/sigma;
            }

            /// Return range indices in the given sampling that cover
            /// at most +/- nsigma of the Gaussian.  The range is half
            /// open (.second is +1 beyond what should be accessed).
            std::pair<int,int> subsample_range(int nsamples, double xmin, double xmax, double nsigma=3.0) const;

            /** Sample the Gaussian.  Return num samples taken from xmin to xmax. */
	    std::vector<double> sample(int num, double xmin, double xmax) const;
	    
	};

	class GaussianDiffusion {
	  public:

	    typedef std::shared_ptr<GaussianDiffusion> pointer;

	    /// A patch is a 2D array of diffuse charge in (nimpacts X
	    /// nticks) bins.  patch[0] is drifted/diffused charge
	    /// waveform at impact position 0 (relative to min pitch
	    /// for the patch).  patch[0][0] is charge at this impact
	    /// position at time = 0 (relative to min time for the
	    /// patch).  See `bin()`.
	    typedef Array::array_xxf patch_t;

	    /** Create a diffused deposition.
	     */
	    // GaussianDiffusion(const IDepo::pointer& depo,
	    // 		      double p_sigma, double t_sigma,
	    // 		      const Point& p_origin, const Vector& p_dir, double p_bin,
	    // 		      double torigin, double t_bin,
	    // 		      double nsigma=3.0, bool fluctuate = false);

	    GaussianDiffusion(const IDepo::pointer& depo,
			      const GausDesc& time_desc, 
			      const GausDesc& pitch_desc);

            /// This fills the patch once.  The number, min and max
            /// describe the entire sampled domain.  The patch is
            /// limited to the 2D sample points that cover the
            /// subdomain determined by the number of sigma.  If
            /// fluctuate is true then a total-charge preserving
            /// Poisson fluctuation is applied bin-by-bin to the
            /// sampling.
            void set_sampling(int nt, double tmin, double tmax,
                              int np, double pmin, double pmax,
                              double nsigma = 3.0,
                              bool fluctuate=false);

	    /// Get the diffusion patch as an array of N_pitch rows X
	    /// N_time columns.  Index as patch(i_pitch, i_time).
	    /// Call set_sampling() first.
	    const patch_t& patch() const;

            /// Return the number of samples in the time direction to where the patch starts.
            int toffset() const { m_toffset; }

            /// Return the number of samples in the pitch direction to where the patch starts.
            int poffset() const { m_poffset; }

	    /// Access deposition.
	    IDepo::pointer depo() const { return m_deposition; }
	    
	    const GausDesc pitch_desc() { return m_pitch_desc; }
	    const GausDesc time_desc() { return m_time_desc; }

	  private:
	    IDepo::pointer m_deposition; // just for provenance

	    GausDesc m_time_desc, m_pitch_desc;

	    mutable patch_t m_patch;
            mutable int m_toffset, m_poffset;
	};
    }
}

#endif
