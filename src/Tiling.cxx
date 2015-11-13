#include "WireCellGen/Tiling.h"
#include "WireCellGen/TilingGraph.h"

#include "WireCellUtil/NamedFactory.h"

#include <vector>
#include <iostream>

using namespace WireCell;

WIRECELL_NAMEDFACTORY_BEGIN(Tiling) 
WIRECELL_NAMEDFACTORY_INTERFACE(Tiling, ITiling);
WIRECELL_NAMEDFACTORY_END(Tiling) 


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

