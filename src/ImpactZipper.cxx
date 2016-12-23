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
    const double pitch_range = m_pir.pitch_range();

    const auto pimpos = m_bd.pimpos();
    const auto rb = pimpos.region_binning();
    const auto ib = pimpos.impact_binning();
    const double wire_pos = rb.center(iwire);

    const int min_impact = ib.edge_index(wire_pos - 0.5*pitch_range);
    const int max_impact = ib.edge_index(wire_pos + 0.5*pitch_range);

    const int nsamples = m_bd.tbins().nbins();
    Waveform::compseq_t spec(nsamples, Waveform::complex_t(0.0,0.0));

    int nfound=0;

    for (int imp = min_impact; imp <= max_impact; ++imp) {
        auto id = m_bd.impact_data(imp);
        if (!id) {
            //std::cerr << "ImpactZipper: no data for absolute impact number: " << imp << std::endl;
            continue;
        }
        
        const Waveform::compseq_t& charge_spectrum = id->spectrum();
        if (charge_spectrum.empty()) {
            //std::cerr << "ImpactZipper: no charge for absolute impact number: " << abs_imp << std::endl;
            continue;
        }

        ++nfound;
        Waveform::increase(spec, charge_spectrum);
    }
    //std::cerr << "ImpactZipper: found " << nfound << " in abs impact: ["  << min_impact << ","<< max_impact << "]\n";

    // Clear memory assuming next call is iwire+1..
    // fixme: this is a dumb way to go.....
    //m_bd.erase(0, min_impact); 

    if (!nfound) {
        return Waveform::realseq_t(nsamples, 0.0);
    }
    
    auto waveform = Waveform::idft(spec);

    return waveform;
}

