#ifndef WIRECELLGEN_GaussianDiffusion
#define WIRECELLGEN_GaussianDiffusion

#include "WireCellUtil/Array.h"
#include "WireCellIface/IDepo.h"

#include <memory>

namespace WireCell {
    namespace Gen {

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
			      double p_sigma, double t_sigma,
			      const Point& p_origin, const Vector& p_dir, double p_bin,
			      double torigin, double t_bin,
			      double nsigma=3.0, bool fluctuate = false);

	    /// Get the (np X nt) binned patch with a 2D extent of
	    /// +/-nsigma*(sigmaP X sigmaT).
	    const patch_t& patch() const;

	    /// Access depositing.
	    IDepo::pointer depo() const { return m_deposition; }
	    
	  private:
	    IDepo::pointer m_deposition; // just for provenance

	    double m_sigma_pitch, m_sigma_time;
	    double m_pitch_begin, m_time_begin;
	    double m_pitch_bin,   m_time_bin;
	    int m_np, m_nt;
	    double m_nsigma;
	    bool m_fluctuate;
	    mutable patch_t m_patch;
	};
    }
}

#endif
