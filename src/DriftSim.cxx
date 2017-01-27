#include "WireCellGen/DriftSim.h"
#include "WireCellGen/ImpactZipper.h"

#include <iostream>

using namespace WireCell;
using namespace std;

Gen::DriftSim::DriftSim(const std::vector<Pimpos>& plane_descriptors,
                        const std::vector<PlaneImpactResponse>& plane_responses,
                        double ndiffision_sigma, bool fluctuate)
    : m_pimpos(plane_descriptors)
    , m_pir(plane_responses)
    , m_ndiffision_sigma(ndiffision_sigma)
    , m_fluctuate(fluctuate)
{
}

Gen::DriftSim::~DriftSim()
{
}

IDepo::pointer Gen::DriftSim::add(IDepo::pointer depo)
{
}

std::vector<Gen::DriftSim::block_t> Gen::DriftSim::latch(Binning tbins)
{
    std::vector<block_t> ret;

    for (int iplane=0; iplane < m_pimpos.size(); ++iplane) {
        Gen::BinnedDiffusion bdw(m_pimpos[iplane], tbins, m_ndiffision_sigma, m_fluctuate);
        Gen::ImpactZipper zipper(m_pir[iplane], bdw);

        auto pitch_mm = bdw.pitch_range();
        cerr << "Pitch sample range: ["<<pitch_mm.first/units::mm <<","<< pitch_mm.second/units::mm<<"]mm\n";

        const auto rb = m_pimpos[iplane].region_binning();
        const int minwire = rb.bin(pitch_mm.first);
        const int maxwire = rb.bin(pitch_mm.second);
        cerr << "Wires: [" << minwire << "," << maxwire << "]\n";

        int nwires = rb.nbins();
        int nticks = tbins.nbins();

        block_t block(nwires, nticks); // fixme

        for (int iwire=minwire; iwire <= maxwire; ++iwire) {
            auto wave = zipper.waveform(iwire);
            // fixme: set_wire(block[iwire], wave);
        }
        ret.push_back(block);
    }
    return ret;

}
