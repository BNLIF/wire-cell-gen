#include "WireCellGen/WirePlane.h"

using namespace WireCell;

//Gen::WirePlane::WirePlane(int ident, IWire::vector wires, Pimpos* pimpos, PlaneImpactResponse* pir)
Gen::WirePlane::WirePlane(int ident, IWire::vector wires, Pimpos* pimpos)
    : m_ident(ident)
    , m_pimpos(pimpos)
//    , m_pir(pir)
    , m_wires(wires)
{

}


Gen::WirePlane::~WirePlane()
{
}

