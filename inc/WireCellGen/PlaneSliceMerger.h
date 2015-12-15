#ifndef WIRECELL_PLANESLICEMERGER
#define WIRECELL_PLANESLICEMERGER

#include "WireCellIface/IPlaneSliceMerger.h"

#include <deque>

namespace WireCell {

    class PlaneSliceMerger : public IPlaneSliceMerger {
    public:
	PlaneSliceMerger();
	virtual ~PlaneSliceMerger() {}

	virtual bool operator()(const input_vector& invec, output_pointer& out);
    };
}

#endif
