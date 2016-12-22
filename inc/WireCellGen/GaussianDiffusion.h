#ifndef WIRECELLGEN_GaussianDiffusion
#define WIRECELLGEN_GaussianDiffusion

#include "WireCellUtil/Array.h"
#include "WireCellUtil/Binning.h"
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

            std::pair<double,double> sigma_range(double nsigma=3.0) {
                return std::make_pair(center-sigma*nsigma, center+sigma*nsigma);
            }

            /// Return range indices in the given sampling that cover
            /// at most +/- nsigma of the Gaussian.  The range is half
            /// open (.second is +1 beyond what should be accessed).
            std::pair<int,int> subsample_range(int nsamples, double xmin, double xmax, double nsigma=3.0) const;

            /** Sample the Gaussian. */
	    std::vector<double> sample(double start, double step, int nsamples) const;
	    
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

	    GaussianDiffusion(const IDepo::pointer& depo,
			      const GausDesc& time_desc, 
			      const GausDesc& pitch_desc);

            /// This fills the patch once matching the given time and
            /// pitch binning. The patch is limited to the 2D sample
            /// points that cover the subdomain determined by the
            /// number of sigma.  If fluctuate is true then a
            /// total-charge preserving Poisson fluctuation is applied
            /// bin-by-bin to the sampling.  Each cell of the patch
            /// represents the 2D bin-centered sampling of the Gaussian.
            void set_sampling(const Binning& tbin, const Binning& pbin,
                              double nsigma = 3.0, bool fluctuate=false);

	    /// Get the diffusion patch as an array of N_pitch rows X
	    /// N_time columns.  Index as patch(i_pitch, i_time).
	    /// Call set_sampling() first.
	    const patch_t& patch() const;

            /// Return the absolute time bin in the binning corresponding to column 0 of the patch.
            int toffset_bin() const { return m_toffset_bin; }

            /// Return the absolute impact bin in the binning corresponding to column 0 of the patch.
            int poffset_bin() const { return m_poffset_bin; }

	    /// Access deposition.
	    IDepo::pointer depo() const { return m_deposition; }
	    
	    const GausDesc pitch_desc() { return m_pitch_desc; }
	    const GausDesc time_desc() { return m_time_desc; }

        private:

	    IDepo::pointer m_deposition; // just for provenance

	    GausDesc m_time_desc, m_pitch_desc;

	    patch_t m_patch;
            int m_toffset_bin;
            int m_poffset_bin;
	};
    }
}

#endif
