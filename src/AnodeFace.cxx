#include "WireCellGen/AnodeFace.h"

using namespace WireCell;
using namespace WireCell::Gen;




AnodeFace::AnodeFace(int ident, IWirePlane::vector planes, const BoundingBox& bb)
    : m_ident(ident)
    , m_planes(planes)
    , m_bb(bb)
{
}

IWirePlane::pointer AnodeFace::plane(int ident) const
{
    for (auto ptr : m_planes) {
        if (ptr->ident() == ident) {
            return ptr;
        }
    }
    return nullptr;
}

