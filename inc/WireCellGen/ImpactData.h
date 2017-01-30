/**
   ImpactData represents the true charge distribution in time at the
   point in space where a field response function begins.
 */

#include "WireCellUtil/Waveform.h"
#include "WireCellGen/GaussianDiffusion.h"

#include <memory>
#include <vector>

#ifndef WIRECELLGEN_IMPACTDATA
#define WIRECELLGEN_IMPACTDATA

namespace WireCell {
    namespace Gen {
	/// Information that has been collected at one impact position
	class ImpactData {
	    int m_impact;
	    mutable Waveform::realseq_t m_waveform;
	    mutable Waveform::compseq_t m_spectrum;
	    
	    // Record the diffusions and their pitch bin that contribute to this impact position.
	    std::vector<GaussianDiffusion::pointer> m_diffusions;
            
	public:
	    typedef std::shared_ptr<ImpactData> mutable_pointer; // for this class
	    typedef std::shared_ptr<const ImpactData> pointer; // for callers

            /** Create an ImpactData associated with the given
             * absolute impact position. See impact_number() for
             * description of the impact.*/
	    ImpactData(int impact);

            /** Add a (shared) GaussianDiffusion object for
             * consideration.  If any are added which do not overlap
             * with this ImpactData's impact/pitch sample point then
             * they will not contribute to the waveform nor spectrum
             * at this impact. */
	    void add(GaussianDiffusion::pointer diffusion);

            const std::vector<GaussianDiffusion::pointer>& diffusions() const { return m_diffusions; }

            /** Calculate the slice in time across the collected
             * GaussianDiffusion objects.
             *
             * This method is idempotent and must be called before
             * waveform() and spectrum().
            */
	    void calculate(int nticks) const;

	    /**  Return the time domain waveform of drifted/diffused
             *  charge at this impact position. See `calculate()`. */
	    Waveform::realseq_t& waveform() const;

	    /** Return the discrete Fourier transform of the above.
             * See `calculate()`. */
	    Waveform::compseq_t& spectrum() const;

	    /** Return the associated impact number.  This provides a
	    sample count along the pitch direction starting from some
	    externally defined pitch origin. */
            int impact_number() const { return m_impact; }

            /** Return the max time range spanned by the difussions
             * that cover this impact including a width expressed as a
             * factor multiplied by the sigma of the time Gaussian.
             * Set to 0.0 gives collective span of centers.*/
            std::pair<double,double> span(double nsigma = 0.0) const;

            /** Return the smallest, half-open range of tick indices
             * which have only zero values outside. */
            //std::pair<int,int> strip() const;
	};
    }
}

#endif
