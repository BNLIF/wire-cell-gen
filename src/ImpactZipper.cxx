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
    const int nimpacts = max_impact-min_impact+1;
    const int nsamples = m_bd.tbins().nbins();
    Waveform::compseq_t spec(nsamples, Waveform::complex_t(0.0,0.0));

    
    // std::cerr << "IZ: wire="<<iwire<<" @"<<wire_pos
    //           <<", imps:["<<min_impact<<","<<max_impact<<"] n="<<nimpacts<<" pitch range:" << pitch_range 
    //           <<", nwires=" << m_pir.nwires() << " nimp_per_wire="<<m_pir.nimp_per_wire()
    //           << std::endl;

    int nfound=0;

    // The BinnedDiffusion is indexed by absolute impact and the
    // PlaneImpactResponse relative impact.
    for (int imp = min_impact; imp <= max_impact; ++imp) {

        // ImpactData
        auto id = m_bd.impact_data(imp);
        if (!id) {
            // common as we are scanning all impacts covering a wire
            // fixme: is there a way to predict this to avoid the query?
            //std::cerr << "ImpactZipper: no data for absolute impact number: " << imp << std::endl;
            continue;
        }
        
        const Waveform::compseq_t& charge_spectrum = id->spectrum();
        if (charge_spectrum.empty()) {
            std::cerr << "ImpactZipper: no charge for absolute impact number: " << imp << std::endl;
            continue;
        }

        const double imp_pos = ib.center(imp);
        const double rel_imp_pos = imp_pos - wire_pos;
        //std::cerr << "IZ: imp=" << imp << " imp_pos=" << imp_pos << " rel_imp_pos=" << rel_imp_pos << std::endl;

        auto ir = m_pir.closest(rel_imp_pos);
        if (! ir) {
            // std::cerr << "ImpactZipper: no impact response for absolute impact number: " << imp << std::endl;
            continue;
        }
	/// frequency-domain spectrum of response
	const Waveform::compseq_t& response_spectrum = ir->spectrum();

        
        // fixme: this should not be done inside this loop.
        // fixme: it shouldn't also make so many assumptions about these numbers!
        const int csize = charge_spectrum.size(); // 10000
        const int chalf = csize/2;
        const int rsize = response_spectrum.size(); // 1000
        const int rhalf = rsize/2;
        
        const double cbin = m_bd.tbins().binsize(); // 500 ns
        const double rbin = m_pir.field_response().period*units::us; // 100 ns
        // must rebin field response by this much.
        // fixme: assumes we know which one is more finely binned!
        const int rebinfactor = int(float(cbin)/float(rbin)); // 5

        const int shalf = rhalf / rebinfactor; // 100
        
        Waveform::compseq_t conv(nsamples, Waveform::complex_t(0.0,0.0));
        for (int ind=0; ind < shalf; ++ind) {
            Waveform::complex_t pval(0.0,0.0);
            Waveform::complex_t mval(0.0,0.0);
            for (int ii=0; ii<rebinfactor; ++ii) {
                pval += response_spectrum[ind+ii];
                mval += response_spectrum[rsize-rebinfactor+ii];
            }
            conv[ind] += pval * charge_spectrum[ind];
            const int rind = csize-1-ind;
            conv[rind] += mval * charge_spectrum[rind];
        }
        

        ++nfound;
        // std::cerr << "ImpactZipper: found:"<<nfound<<" for absolute impact number " << imp
        //           << " csize=" << csize << " rsize=" << rsize << " rebin=" << rebinfactor
        //           << std::endl;

        Waveform::increase(spec, conv);
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

