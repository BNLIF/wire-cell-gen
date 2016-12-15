#include "WireCellGen/ImpactZipper.h"

using namespace WireCell;
Gen::ImpactZipper::ImpactZipper(const PlaneImpactResponse& pir, BinnedDiffusion& bd)
    :m_pir(pir), m_bd(bd)
{
    
}



Gen::ImpactZipper::~ImpactZipper()
{
}


Waveform::realseq_t Gen::ImpactZipper::waveform(int iwire) const
{
    const int nimpperwire = m_pir.nimp_per_wire();
    const int abs_impact_begin = nimpperwire * iwire;
    const int abs_impact_end = abs_impact_begin + nimpperwire * m_pir.nwires() ;
        

    Waveform::compseq_t spec(m_bd.nsamples(), Waveform::complex_t(0.0,0.0));

    int nfound = 0;
    for (int abs_imp = abs_impact_begin; abs_imp < abs_impact_end; ++abs_imp) {
        auto id = m_bd.impact_data(abs_imp);
        if (!id) {
            continue;
        }

        const Waveform::compseq_t& charge_spectrum = id->spectrum();
        if (charge_spectrum.empty()) {
            continue;
        }

        ++nfound;
        Waveform::increase(spec, charge_spectrum);
    }

    m_bd.erase(0, abs_impact_begin); // clear memory

    if (!nfound) {
        return Waveform::realseq_t(m_bd.nsamples(), 0.0);
    }
    
    auto waveform = Waveform::idft(spec);

    return waveform;
}
