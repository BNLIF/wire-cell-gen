#include "WireCellGen/AnodeFace.h"

using namespace WireCell;
using namespace WireCell::Gen;




AnodeFace::AnodeFace(int ident, IWirePlane::vector planes)
    : m_ident(ident)
    , m_planes(planes)
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

