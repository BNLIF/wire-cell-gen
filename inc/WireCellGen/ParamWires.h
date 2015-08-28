#ifndef WIRECELLGEN_PARAMWIRES_H
#define WIRECELLGEN_PARAMWIRES_H

#include "WireCellIface/IWireGenerator.h"
#include "WireCellIface/IWire.h"

#include <vector>

namespace WireCell {

    // internal
    class ParamWire;

    /** A provider of wire (segment) geometry as generated from parameters.
     *
     * All wires in one plane are constructed to be parallel to
     * one-another and to be equally spaced between neighbors and
     * perpendicular to the drift direction.
     */

    class ParamWires :	public IWireGenerator,	public IWireSequence {
    public:
	ParamWires();
	virtual ~ParamWires();

	void generate(WireCell::IWireParameters::pointer params);

	/// Access the wires
	wire_iterator wires_begin();
	wire_iterator wires_end();
	size_t wires_size() const { return m_wire_store.size(); }

    private:

	void clear();

	void make_one_plane(WirePlaneId wpid, const Ray& bounds, const Ray& step);


	typedef std::vector<IWire::pointer> ParamWireStore;
	ParamWireStore m_wire_store;

    };


}

#endif
