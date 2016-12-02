#ifndef WIRECELLGEN_GaussianDiffusion
#define WIRECELLGEN_GaussianDiffusion

#include "WireCellUtil/Array.h"
#include "WireCellIface/IDepo.h"

#include <memory>

namespace WireCell {
    namespace Gen {

	/// Describe a binned and truncated Gaussian
	struct GausDesc {
	    double center;
	    double sigma;
	    double sample_size;
	    int nsamples;
	    int sample_begin;

	    GausDesc(double center,double sigma,double sample_size,int nsamples,int sample_begin)
		: center(center)
		, sigma(sigma)
		, sample_size(sample_size)
		, nsamples(nsamples)
		, sample_begin(sample_begin)
		{ }
		
	    // The [begin,end) half open relative sample index range.
	    std::pair<int,int> relative_range() const {
		return std::make_pair(0, nsamples);
	    }
	    // The [begin,end) half open absolute sample index range.
	    std::pair<int,int> absolute_range() const {
		return std::make_pair(sample_begin, sample_begin + nsamples);
	    }
	    // The center-to-center extent of sampling
	    std::pair<double,double> sampled_extent() const {
		auto bb = absolute_range();
		return std::make_pair(bb.first*sample_size, bb.second*sample_size);
	    }
	    // The edge-to-edge extent of the sampling.
	    std::pair<double,double> binned_extent() const {
		auto bb = absolute_range();
		return std::make_pair((bb.first-0.5)*sample_size, (bb.second-0.5)*sample_size);
	    }
	    std::vector<double> sample() const {
		std::vector<double> ret(nsamples);
		const double vmin = sample_begin * sample_size;
		for (int ind=0; ind<nsamples; ++ind) {
		    const double rel = (vmin + ind*sample_size - center)/sigma;
		    ret[ind] = exp(-0.5*rel*rel);
		}
		return ret;
	    }
	    
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
			      const GausDesc& pitch_desc,
			      bool fluctuate = false);

	    /// Get the diffusion patch as an array of N_pitch rows X
	    /// N_time columns.  Index as patch(i_pitch, i_time).
	    const patch_t& patch() const;

	    /// Access depositing.
	    IDepo::pointer depo() const { return m_deposition; }
	    
	    const GausDesc pitch_desc() { return m_pitch_desc; }
	    const GausDesc time_desc() { return m_time_desc; }

	  private:
	    IDepo::pointer m_deposition; // just for provenance

	    GausDesc m_time_desc, m_pitch_desc;

	    bool m_fluctuate;
	    mutable patch_t m_patch;
	};
    }
}

#endif
