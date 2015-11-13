#ifndef WIRECELLGEN_WIRECELL_H
#define WIRECELLGEN_WIRECELL_H

#include "WireCellIface/ITiling.h"

#include <deque>

namespace WireCell {


    /** The reference implementation of WireCell::ITiling.
     */
    class Tiling : public ITiling
    {
    public:
	Tiling();
	virtual ~Tiling();

	virtual bool operator()(const input_pointer& cell_vector, output_pointer& cell_summary);

    };

} // namespace WireCell
#endif
