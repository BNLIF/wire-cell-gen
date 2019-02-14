#include "WireCellGen/AnodeFace.h"

using namespace WireCell;
using namespace WireCell::Gen;


static
ray_pair_vector_t get_raypairs(const IWirePlane::vector& planes)
{
    ray_pair_vector_t raypairs;
    for (const auto& plane : planes) {
        const auto& wires = plane->wires();
        const auto wray0 = wires[0]->ray();
        const auto wray1 = wires[1]->ray();
        const auto pitray = ray_pitch(wray0, wray1);
        const auto pitvec = ray_vector(pitray);
        Ray r1(wray0.first - 0.5*pitvec, wray0.second - 0.5*pitvec);
        Ray r2(wray0.first + 0.5*pitvec, wray0.second + 0.5*pitvec);
        raypairs.push_back(ray_pair_t(r1,r2));
    }
    return raypairs;
}


AnodeFace::AnodeFace(int ident, IWirePlane::vector planes, const BoundingBox& bb)
    : m_ident(ident)
    , m_planes(planes)
    , m_bb(bb)
    , m_coords(get_raypairs(planes))
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

