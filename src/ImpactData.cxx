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
    m_waveform.clear();
    m_waveform.resize(nticks, 0.0);

    for (auto diff : m_diffusions) {
	auto pd = diff->pitch_desc();
	auto td = diff->time_desc();
	auto p = diff->patch();

	int relimpact = m_impact - pd.sample_begin;
	for (int ind=0; ind < td.nsamples; ++ind) {
	    const int absind = td.sample_begin + ind;
	    if (absind >= nticks) {
		break;
	    }
	    m_waveform[absind] += p(relimpact, ind);
	}
    }

    m_spectrum = Waveform::dft(m_waveform);
}

std::pair<int,int> Gen::ImpactData::tick_bounds() const
{
    int mint=0, maxt=0;
    const int ndiff = m_diffusions.size();
    for (int ind=0; ind<ndiff; ++ind) {
	auto td = m_diffusions[ind]->time_desc();
	const int tmp = td.sample_begin + td.nsamples;
	if (!ind) {
	    mint = td.sample_begin;
	    maxt = tmp;
	    continue;
	}
	if (td.sample_begin < mint) {
	    mint = td.sample_begin;
	}
	if (tmp > maxt) {
	    maxt = tmp;
	}
    }
    return std::make_pair(mint,maxt);
}



double Gen::ImpactData::pitch_distance() const
{
    auto pd = m_diffusions.front()->pitch_desc();
    return m_impact * pd.sample_size;
}
