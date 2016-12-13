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
        

    Waveform::compseq_t spec(m_bd.nsamples());

    for (int abs_imp = abs_impact_begin; abs_imp < abs_impact_end; ++abs_imp) {
        auto id = m_bd.impact_data(abs_imp);
        if (!id) {
            continue;
        }

        const Waveform::compseq_t& charge_spectrum = id->spectrum();
        if (charge_spectrum.empty()) {
            continue;
        }

        Waveform::increase(spec, charge_spectrum);
    }
    
    auto waveform = Waveform::idft(spec);

    m_bd.erase(0, abs_impact_begin); // clear memory

    return waveform;
}
