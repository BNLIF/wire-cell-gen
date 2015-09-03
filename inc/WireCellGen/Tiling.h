#ifndef WIRECELLGEN_WIRECELL_H
#define WIRECELLGEN_WIRECELL_H

#include "WireCellIface/ITiling.h"

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

	/** Provide input vector of cells build summary.
	 */
	virtual bool sink(const ICellVector& cells);

	/** Return the most recent cell summary.
	 */
	virtual bool source(ICellSummary::pointer& summary) {
	    summary = m_summary;
	    return true;
	}

    private:
	ICellSummary::pointer m_summary;
	
    };

} // namespace WireCell
#endif
