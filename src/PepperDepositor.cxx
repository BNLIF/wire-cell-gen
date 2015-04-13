#include "WireCellNav/PepperDepositor.h"

#include <random>

using namespace WireCell;
using namespace std;

PepperDepositor::PepperDepositor(MinMax ymm, MinMax zmm, MinMax qmm, int hits_per_frame)
    : ymm(ymm), zmm(zmm), qmm(qmm)
    , hits_per_frame(hits_per_frame)
{
}
PepperDepositor::~PepperDepositor()
{
}

const PointValueVector& PepperDepositor::depositions(int frame_number, int bins_per_frame) const
{
    // fixme: we need to get a handle on rng usage across all code!
    std::default_random_engine gen(frame_number);
    std::uniform_real_distribution<float> trand(0,bins_per_frame);
    std::uniform_real_distribution<float> yrand(ymm.first, ymm.second);
    std::uniform_real_distribution<float> zrand(zmm.first, zmm.second);
    std::uniform_real_distribution<float> qrand(qmm.first, qmm.second);

    hits.clear();
    for (int ind=0; ind < hits_per_frame; ++ind) {
	hits.push_back(PointValue(Point(trand(gen), yrand(gen), zrand(gen)), qrand(gen)));
    }
    return hits;
}
