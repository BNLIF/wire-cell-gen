#include "WireCellGen/ImpactData.h"

using namespace WireCell;

Gen::ImpactData::ImpactData(int impact)
    : m_impact(impact)
{
}
void Gen::ImpactData::add(GaussianDiffusion::pointer diffusion)
{
    m_diffusions.push_back(diffusion);
}

Waveform::realseq_t& Gen::ImpactData::waveform() const
{
    return m_waveform;
}

Waveform::compseq_t& Gen::ImpactData::spectrum() const
{
    return m_spectrum;
}

void Gen::ImpactData::calculate(int nticks) const
{
    if (m_waveform.size() > 0) {
        return;
    }
    m_waveform.resize(nticks, 0.0);

    for (auto diff : m_diffusions) {
        const int toffset = diff->toffset();
        const int poffset = diff->poffset();
        const int pind = m_impact - poffset;

	auto patch = diff->patch();
        const int nt = patch.cols();

        for (int tind=0; tind<nt; ++tind) {
            const int absind = tind+toffset;
            m_waveform[absind] += patch(pind, tind);
        }
    }

    m_spectrum = Waveform::dft(m_waveform);
}



std::pair<int,int> Gen::ImpactData::strip() const
{
    int imin=-1, imax = -1;
    for (int ind=0; ind<m_waveform.size(); ++ind) {
        const double val = m_waveform[ind];
        if (imin < 0 && val > 0) { imin = ind; }
        if (val > 0) { imax = ind; }
    }
    return std::make_pair(imin, imax+1);
}
