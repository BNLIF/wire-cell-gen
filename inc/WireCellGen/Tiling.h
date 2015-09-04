#ifndef WIRECELLGEN_WIRECELL_H
#define WIRECELLGEN_WIRECELL_H

#include "WireCellIface/ITiling.h"

#include <deque>

namespace WireCell {


    /** The reference implementation of WireCell::ITiling.
     *
     * This the internal buffering of this processor is of depth 1.
     * Any newly sunk cells produce a new summary.  The most recent
     * summary is kept for any subsequent sourcing.
     */
    class Tiling : public ITiling
    {
    public:
	Tiling();
	virtual ~Tiling();

	virtual void reset();
	virtual void flush();
	virtual bool insert(const input_type& cell_vector);
	virtual bool extract(output_type& cell_summary);

    private:
	std::deque<ICellSummary::pointer> m_output;
	
    };

} // namespace WireCell
#endif
