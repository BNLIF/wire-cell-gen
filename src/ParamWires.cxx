#include "WireCellGen/ParamWires.h"
#include "WireCellUtil/Intersection.h"
#include "WireCellUtil/NamedFactory.h"

#include <cmath>
#include <iostream> 		// debugging
#include <vector>

using namespace WireCell;
using namespace std;

WIRECELL_NAMEDFACTORY(ParamWires);
WIRECELL_NAMEDFACTORY_ASSOCIATE(ParamWires, IWireGenerator);
WIRECELL_NAMEDFACTORY_ASSOCIATE(ParamWires, IWireSequence);

static int number_of_wires = 0;

namespace WireCell {
class ParamWire : public IWire {
    WirePlaneId m_wpid;
    int m_index;
    Ray m_ray;

public:
    ParamWire(WirePlaneId wpid, int index, const Ray& ray)
	: m_wpid(wpid), m_index(index), m_ray(ray)
    {
	++number_of_wires;
    }
    virtual ~ParamWire()
    {
	--number_of_wires;
    }

    int ident() const {
	int iplane = m_wpid.index();
	++iplane;
	return iplane*100000 + m_index;
    }

    WirePlaneId planeid() const { return m_wpid; }
	
    int index() const { return m_index; }

    int channel() const { return ident(); }

    WireCell::Ray ray() const { return m_ray; }

    /// fixme: supply
    int segment() const {return 0;}
    int face() const {return 0;}
    int apa() const {return 0;}

    void set_index(int ind) { m_index = ind; }
    void set_planeid(WirePlaneId wpid) { m_wpid = wpid; }

};
}

static ParamWire* make_wire(int index, const Point& point,
			    const Point& proto, const Ray& bounds)
{
    double number = index;
    const Point pt1 = point;
    const Point pt2  = pt1 + proto;
    const Ray wireray(pt1, pt2);

    Ray hits;
    int hitmask = box_intersection(bounds, wireray, hits);
    if (3 != hitmask) {
	return 0;
    }
    // ray should point generally towards +Y
    if (hits.first.y() > hits.second.y()) {
	hits = Ray(hits.second, hits.first);
    }
    return new ParamWire(WirePlaneId(kUnknownLayer), index, hits);
}

struct SortByIndex {
    inline bool operator()(const ParamWire* lhs, const ParamWire* rhs) {
	return lhs->index() < rhs->index();
    }
};    


void ParamWires::make_one_plane(WirePlaneId wpid, const Ray& bounds, const Ray& step)
{
    const Vector xaxis(1,0,0);
    const Point starting_point = step.first;
    const Vector pitch = step.second - starting_point;
    const Vector proto = pitch.cross(xaxis).norm();
    
    std::vector<ParamWire*> these_wires;

    int pos_index = 0;
    Point offset = starting_point;
    while (true) {		// go in positive pitch direction
	ParamWire* wire = make_wire(pos_index, offset, proto, bounds);
	if (! wire) { break; }
	these_wires.push_back(wire);
	offset = wire->center() + pitch;
	pos_index += 1;
    }

    int neg_index = -1;		// now go in negative pitch direction
    const Vector neg_pitch = -1.0 * pitch;
    offset = these_wires[0]->center() + neg_pitch; // start one below first upward going one
    while (true) {		// go in negative pitch direction
	ParamWire* wire = make_wire(neg_index, offset, proto, bounds);
	if (! wire) { break; }
	these_wires.push_back(wire);
	offset = wire->center() + neg_pitch;
	neg_index -= 1;
    }

    // order by index
    std::sort(these_wires.begin(), these_wires.end(), SortByIndex());

    // load in to store and fix up index and plane
    for (int ind=0; ind<these_wires.size(); ++ind) {
	ParamWire* pwire = these_wires[ind];
	pwire->set_index(ind);
	pwire->set_planeid(wpid);
	m_wire_store.push_back(IWire::pointer(pwire));
    }

    //std::cerr << "Made "<<store.size()<<" wires for plane " << plane << std::endl;
    //std::cerr << "step = " << step << std::endl;
    //std::cerr << "bounds = " << bounds << std::endl;
}


void ParamWires::generate(IWireParameters::pointer params)

{
    this->clear();

    make_one_plane(WirePlaneId(kUlayer), params->bounds(), params->pitchU());
    make_one_plane(WirePlaneId(kVlayer), params->bounds(), params->pitchV());
    make_one_plane(WirePlaneId(kWlayer), params->bounds(), params->pitchW());
}


IWireSequence::wire_iterator ParamWires::wires_begin() 
{
    return wire_iterator(adapt(m_wire_store.cbegin()));
}
IWireSequence::wire_iterator ParamWires::wires_end() 
{
    return wire_iterator(adapt(m_wire_store.cend()));
}

void ParamWires::clear() {
    m_wire_store.clear();
}

ParamWires::ParamWires()
{
}

ParamWires::~ParamWires()
{
    this->clear();
}
