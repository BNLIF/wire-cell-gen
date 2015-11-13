#ifndef WIRECELLGEN_BOUNDCELLS
#define WIRECELLGEN_BOUNDCELLS

#include "WireCellIface/ICellMaker.h"

#include <deque>

namespace WireCell {

    /** 
     * A provider of cells using the bounded wire pair algorithm.
     */
    class BoundCells : public ICellMaker
    {
    public:
	
	BoundCells();
	virtual ~BoundCells();

	virtual bool operator()(const input_pointer& wires, output_pointer& cells);


    };
}

#endif
