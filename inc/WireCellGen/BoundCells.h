#ifndef WIRECELLGEN_BOUNDCELLS
#define WIRECELLGEN_BOUNDCELLS

#include "WireCellIface/IWire.h"
#include "WireCellIface/ICell.h"

namespace WireCell {

    /** A provider of cells using the bounded wire algorithm. */
    class BoundCells :
	public IWireSink,
	public ICellSequence
    {

    public:
	
	BoundCells();
	virtual ~BoundCells();

	/// Generate my cells (IWireSink interface).
	void sink(const IWire::iterator_range& wires);

	/// Return iterator to first cell provided.
	cell_iterator cells_begin();

	/// Return iterator to one past last cell provided.
	cell_iterator cells_end();

	size_t cells_size() const { return m_store.size(); }

    private:
	typedef std::vector<ICell::pointer> BoundCellStore;
	BoundCellStore m_store;
    };
}

#endif
