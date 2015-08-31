#ifndef WIRECELLGEN_BOUNDCELLS
#define WIRECELLGEN_BOUNDCELLS

#include "WireCellIface/ICellMaker.h"

namespace WireCell {

    /** 
     * A provider of cells using the bounded wire pair algorithm.
     */
    class BoundCells : public ICellMaker
    {
    public:
	
	BoundCells();
	virtual ~BoundCells();

	/** Process input, make output (IProcessor).
	 *
	 * This is actually a no-op (all work is done in sink()).
	 */
	virtual bool process() { return false; }

	/** Accept input wires (ISink).
	 *
	 * The input queue is zero length, adding new wires will
	 * invalidate any existing output and regenerate it.
	 */ 
	virtual bool sink(const IWireVector& wires);

	/** Produce any output cells (ISource).
	 *
	 * Repeated calls will return the same cells.  If no input
	 * wires have yet been given then an empty cell vector will be
	 * set.
	 */
	virtual bool source(ICellVector& cells) {
	    cells = m_cells;
	    return true;
	}

    private:
	ICellVector m_cells;
    };
}

#endif
