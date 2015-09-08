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

	virtual void reset();
	virtual void flush();
	virtual bool insert(const input_type& wire_vector);
	virtual bool extract(output_type& cell_vector);

    private:
	std::deque<ICellVector> m_output;

    };
}

#endif
