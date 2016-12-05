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

	    ImpactData(int impact);

	    void add(GaussianDiffusion::pointer diffusion);

	    /// The time domain waveform of drifted/diffused charge at this impact position.
	    Waveform::realseq_t& waveform() const;

	    /// The discrete Fourier transform of the above
	    Waveform::compseq_t& spectrum() const;

	    /// The impact number 
	    int impact_number() const { return m_impact; }

	    /// The corresponding pitch distance
	    double pitch_distance() const;

	    /// The minimum ticks range that bounds all diffusion patches.
	    std::pair<int,int> tick_bounds() const;


	    // This must be called before waveform() or spectrum()
	    void calculate(int nticks) const;
	};
    }
}

#endif
