#ifndef WIRECELLNAV_WIRECELL_H
#define WIRECELLNAV_WIRECELL_H

#include "WireCellIface/ICell.h"
#include "WireCellIface/ITiling.h"


namespace WireCell {


    // Considered opaque.
    class TilingGraph;

    /** A default implementation of WireCell::ITiling.
     *
     */
    class Tiling : public ICellSink, public ITiling
    {
    public:
	Tiling();
	virtual ~Tiling();

	/// Provide access to the collection of cells. (ICellSink)
	virtual void sink(const ICell::iterator_range& cells);

	/// The rest are ITiling interface methods.

	/// Return a sequence of wires which are associated with the
	/// given cell.
	virtual IWireVector wires(ICell::pointer cell) const;
	
	/// Return a sequence of cells associated with the given wire.
	virtual ICellVector cells(IWire::pointer wire) const;

	/// Return the cell associated with the collection of wires,
	/// if any.
	virtual ICell::pointer cell(const IWireVector& wires) const;

	/// Return collection of nearest neighbor cells.
	virtual ICellVector neighbors(ICell::pointer cell) const;


    private:
	
	mutable TilingGraph* m_graph;
    };

} // namespace WireCell
#endif
