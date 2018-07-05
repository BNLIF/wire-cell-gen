#include "WireCellGen/WirePlane.h"

using namespace WireCell;

Gen::WirePlane::WirePlane(int ident, IWire::vector wires, Pimpos* pimpos)
    : m_ident(ident)
    , m_pimpos(pimpos)
    , m_wires(wires)
{

}


Gen::WirePlane::~WirePlane()
{
}

