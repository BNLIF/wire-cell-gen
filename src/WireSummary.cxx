#include "WireCellNav/WireSummary.h"
#include "WireCellIface/IWireSelectors.h"

#include <iterator>

using namespace WireCell;
using namespace std;

WireSummary::WireSummary()
    : m_cache(0)
{
}

// Return a Ray going from the center point of wires[0] to a point on
// wire[1] and perpendicular to both.
static Ray pitch2(const IWireVector& wires)
{
    // Use two consecutive wires from the center to determine the pitch. 
    int ind = wires.size()/2;
    IWire::pointer one = wires[ind];
    IWire::pointer two = wires[ind+1];
    const Ray p = ray_pitch(one->ray(), two->ray());

    // Move the pitch to the first wire
    IWire::pointer zero = wires[0];
    const Vector center0 = zero->center();
    return Ray(center0, center0 + ray_vector(p));
}

struct WirePlaneCache {
    WirePlaneType_t plane;
    int iplane;
    IWireVector wires;
    Ray pitch_ray;
    Vector pitch_vector;
    Vector pitch_unit;
    double pitch_mag;

    WirePlaneCache(WirePlaneType_t plane, IWire::iterator_range wr)
	: plane(plane), iplane(int(plane))
    {
	copy_if(boost::begin(wr), boost::end(wr), back_inserter(wires), select_uvw_wires[iplane]);
	pitch_ray = pitch2(wires);
	pitch_vector = ray_vector(pitch_ray); // cache
	pitch_unit = pitch_vector.norm();     // the
	pitch_mag = pitch_vector.magnitude(); // things!
    }

    IWire::pointer wire_by_index(int index) {
	if (index < 0 || index >= wires.size()) {
	    return 0;
	}
	return wires[index];
    }

    double pitch_distance(const Point& point) {
	return ray_dist(pitch_ray, point);
    }
	

};

struct WireSummary::WireSummaryCache {
    IWireVector wires;
    BoundingBox bb;
    WirePlaneCache *plane_cache[3];

    WireSummaryCache(IWire::iterator_range wr)
	: wires(boost::begin(wr), boost::end(wr))
   {
       for (auto it : wr) {
	   bb(it->ray());
       }

       plane_cache[0] = new WirePlaneCache(kUwire, wr);
       plane_cache[1] = new WirePlaneCache(kVwire, wr);
       plane_cache[2] = new WirePlaneCache(kWwire, wr);
   }

    ~WireSummaryCache() {
	for (int ind=0; ind<3; ++ind) {
	    delete plane_cache[ind];
	}
    }

    WirePlaneCache* plane(WirePlaneType_t p) {
	return plane(int(p));
    }
    WirePlaneCache* plane(int iplane) {
	if (iplane<0 ||iplane>2) return 0;
	return plane_cache[iplane];
    }
    
};

//typedef IteratorAdapter< IWireVector::const_iterator, WireCell::wire_base_iterator > adapted_iterator;


void WireSummary::sink(IWire::iterator_range wires)
{
    if (m_cache) delete m_cache;
    m_cache = new WireSummaryCache(wires);    
}

static IWireVector dummy_vector;

IWireSequence::wire_iterator WireSummary::wires_begin() const
{
    if (!m_cache) {
	return wire_iterator(adapt(dummy_vector.end()));
    }
    return wire_iterator(adapt(m_cache->wires.begin()));
}

IWireSequence::wire_iterator WireSummary::wires_end() const
{
    if (!m_cache) {
	return wire_iterator(adapt(dummy_vector.end()));
    }
    return wire_iterator(adapt(m_cache->wires.end()));	
}

const BoundingBox& WireSummary::box() const
{
    if (!m_cache) {
	static BoundingBox bbdummy;
	return bbdummy;
    }
    return m_cache->bb;
}

IWire::pointer WireSummary::closest(const Point& point, WirePlaneType_t plane) const
{
    if (!m_cache) {
	return 0;
    }
    WirePlaneCache* wpc = m_cache->plane(plane);
    if (!wpc) {
	return 0;
    }
    
    double dist = wpc->pitch_distance(point);
    return wpc->wire_by_index(dist/wpc->pitch_mag);
}
	
IWirePair WireSummary::bounding_wires(const Point& point, WirePlaneType_t plane) const
{
    if (!m_cache) {
	return IWirePair();
    }
    WirePlaneCache* wpc = m_cache->plane(plane);
    if (!wpc) {
	return IWirePair();
    }

    IWire::pointer wire = closest(point, plane);
    if (!wire) return IWirePair();

    int index = wire->index();

    Vector topoint = point - wire->ray().first;
    double dot = wpc->pitch_unit.dot(topoint);
    int other_index = index - 1;
    if (dot > 0) {
	other_index = index + 1;
    }

    IWire::pointer other_wire = wpc->wire_by_index(other_index);

    if (index < other_index) {
	return IWirePair(IWire::pointer(wire), IWire::pointer(other_wire));
    }
    return IWirePair(IWire::pointer(other_wire), IWire::pointer(wire));
}

double WireSummary::pitch_distance(const Point& point, WirePlaneType_t plane) const
{
    if (!m_cache) {
	return 0;
    }
    WirePlaneCache* wpc = m_cache->plane(plane);
    if (!wpc) {
	return 0;
    }
    return wpc->pitch_distance(point);
}

const Vector& WireSummary::pitch_direction(WirePlaneType_t plane) const
{
    static Vector dummy;
    if (!m_cache) {
	return dummy;
    }
    WirePlaneCache* wpc = m_cache->plane(plane);
    if (!wpc) {
	return dummy;
    }
    return wpc->pitch_unit;
}
