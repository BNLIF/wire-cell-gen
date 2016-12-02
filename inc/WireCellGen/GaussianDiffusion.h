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
	    
	    int npitches() const { return m_np; }
	    int ntimes() const { return m_nt; }

	    /// The first and last pitches in the patch, relative to pitch origin.
	    std::pair<double,double> minmax_pitch() const;

	    /// The first and last times in the patch, relative to time origin.
	    std::pair<double,double> minmax_time() const;

	    /// Return actual center in (time,pitch) relative to given origin.
	    std::pair<double,double> center() const;

	  private:
	    IDepo::pointer m_deposition; // just for provenance

	    double m_sigma_pitch, m_sigma_time;
	    double m_pitch_center, m_time_center;
	    double m_pitch_binsize,   m_time_binsize;
	    int m_np, m_nt;
	    int m_pitch_bin0, m_time_bin0;
	    double m_nsigma;
	    bool m_fluctuate;
	    mutable patch_t m_patch;
	};
    }
}

#endif
