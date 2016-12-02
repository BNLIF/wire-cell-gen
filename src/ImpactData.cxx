#include "WireCellGen/ImpactData.h"

using namespace WireCell;

Gen::ImpactData::ImpactData(int number, double distance)
    : m_number(number)
    , m_distance(distance)
{
}
void Gen::ImpactData::add(GaussianDiffusion::pointer diffusion)
{
    m_diffusions.push_back(diffusion);
}

Waveform::realseq_t Gen::ImpactData::waveform()
{
    if (m_waveform.size() == 0) {
	calculate();
    }
    return m_waveform;
}

Waveform::compseq_t Gen::ImpactData::spectrum()
{
    if (m_spectrum.size() == 0) {
	calculate();
    }
}

void Gen::ImpactData::calculate()
{
    if (m_diffusions.size() == 0) {
	return;			// fixme: this is a logic error
    }
    auto first = m_diffusions.front();
    auto tdesc = first->time_desc();
    auto arange = tdesc.absolute_range();
    auto nticks = arange.second - arange.first;

    m_waveform.clear();
    m_waveform.resize(nticks, 0.0);

    for (auto diff : m_diffusions) {
	auto pd = diff->pitch_desc();
	auto td = diff->time_desc();
	auto p = diff->patch();

	int relimpact = m_number - pd.sample_begin;
	for (int ind=0; ind < td.nsamples; ++ind) {
	    const int absind = td.sample_begin + ind;
	    m_waveform[absind] += p(relimpact, ind);
	}
    }

    m_spectrum = Waveform::dft(m_waveform);
}

