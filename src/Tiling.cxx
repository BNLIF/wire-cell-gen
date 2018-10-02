#include "WireCellGen/Tiling.h"
#include "WireCellGen/TilingGraph.h"

#include "WireCellUtil/NamedFactory.h"

#include <vector>
#include <iostream>

WIRECELL_FACTORY(Tiling, WireCell::Tiling, WireCell::ITiling)

using namespace WireCell;



Tiling::Tiling()
{
}

Tiling::~Tiling()
{
}

bool Tiling::operator()(const input_pointer& cell_vector, output_pointer& cell_summary)
{
    if (!cell_vector) {
	cell_summary = nullptr;
	return true;
    }
    cell_summary = output_pointer(new TilingGraph(*cell_vector));
    return true;
}

