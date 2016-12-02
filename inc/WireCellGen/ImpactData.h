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
	    int m_number; 
	    double m_distance; 
	    Waveform::realseq_t m_waveform;
	    Waveform::compseq_t m_spectrum;
	    
	    // Record the diffusions and their pitch bin that contribute to this impact position.
	    std::vector<GaussianDiffusion::pointer> m_diffusions;

	public:
	    typedef std::shared_ptr<ImpactData> mutable_pointer; // for this class
	    typedef std::shared_ptr<const ImpactData> pointer; // for callers

	    ImpactData(int number, double distance);
	    void add(GaussianDiffusion::pointer diffusion);

	    /// The time domain waveform of drifted/diffused charge at this impact position.
	    Waveform::realseq_t waveform();

	    /// The discrete Fourier transform of the above
	    Waveform::compseq_t spectrum();

	    void calculate();
	};
    }
}

#endif
