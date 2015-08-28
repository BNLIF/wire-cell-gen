#ifndef WIRECELL_WIRESUMMARY
#define WIRECELL_WIRESUMMARY

#include "WireCellIface/IWireSummary.h"
#include "WireCellUtil/BoundingBox.h"

namespace WireCell {

    /** Default WireSummary which is also a wire sink and a wire sequence.
     */
    class WireSummary : public IWireSummary, public IWireSink, public IWireSequence {
    public:
	WireSummary();
	virtual ~WireSummary() {}

	/// Set the wires to use. (IWireSink)
	virtual void sink(const IWire::iterator_range& wires);

	/// Access the sequence of wires. (IWireSequence)
	virtual wire_iterator wires_begin();
	virtual wire_iterator wires_end();

	/// Return the bounding box of the wire planes.
	virtual const WireCell::BoundingBox& box() const;

	/// Return the closest wire along the pitch direction to the
	/// given point in the given wire plane.  It is assumed the
	/// point is in the (Y-Z) bounding box of the wire plane.
	virtual IWire::pointer closest(const Point& point, WirePlaneId wpid) const;
	
	/// Return a pair of adjacent wires from the given plane which
	/// bound the given point along the pitch direction.  The pair
	/// is ordered by increasing wire index number.  If one or
	/// both sides of the point are unbound by wire (segments) the
	/// associated pointer will be zero.  It is assumed the point
	/// is in the (Y-Z) bounding box of the wire plane.
	virtual IWirePair bounding_wires(const Point& point, WirePlaneId wpid) const;

	/// Return the distance along the pitch of the given wire
	/// plane to the given point as measured from the zeroth wire.
	virtual double pitch_distance(const Point& point, WirePlaneId wpid) const;

	/// Return a unit vector along the direction of the pitch.
	virtual const Vector& pitch_direction(WirePlaneId wpid) const;

    private:
	class WireSummaryCache;
	WireSummaryCache* m_cache;

    };
}

#endif
