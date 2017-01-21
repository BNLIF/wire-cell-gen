#ifndef WIRECELL_IMPACTZIPPER
#define WIRECELL_IMPACTZIPPER

#include "WireCellUtil/ImpactResponse.h"
#include "WireCellGen/BinnedDiffusion.h"

namespace WireCell {
    namespace Gen {

    
        /** An ImpactZipper "zips" together ImpactData from a
         * BinnedDiffusion and ImpactResponse from a
         * PlaneImpactResponse.  Zipping means to multiply each of the
         * two impact info's frequency-domain spectra and inverse
         * Fourier transform it.  One such waveform is returned for a
         * given wire at a time.
         */
        class ImpactZipper
        {
            const PlaneImpactResponse& m_pir;
            BinnedDiffusion& m_bd;
        public:
            ImpactZipper(const PlaneImpactResponse& pir, BinnedDiffusion& bd);
            virtual ~ImpactZipper();

            /// Return the wire's waveform.
            // fixme: this should be a forward iterator so that it may cal bd.erase() safely to conserve memory
            Waveform::realseq_t waveform(int wire) const;

        };

    }  // Gen
}  // WireCell
#endif /* WIRECELL_IMPACTZIPPER */
