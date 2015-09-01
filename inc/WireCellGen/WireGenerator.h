#ifndef WIRECELLGEN_WIREGENERATOR
#define WIRECELLGEN_WIREGENERATOR

#include "WireCellIface/IWireGenerator.h"
#include "WireCellIface/IWire.h"

namespace WireCell {

    /** A source of wire (segment) geometry as generated from parameters.
     *
     * All wires in one plane are constructed to be parallel to
     * one-another and to be equally spaced between neighbors and
     * perpendicular to the drift direction.
     */

    class WireGenerator :	public IWireGenerator {
    public:
	WireGenerator();
	virtual ~WireGenerator();

	// No buffering, all the action is in sink().
	bool process() { return false; }

	// Feed the parameters, triggering the generation of wires.
	bool sink(const IWireParameters::pointer& wp);

	// Return the most recently produced wires.
	bool source(IWireVector& ret) {
	    ret = m_wires;
	    return true;
	}


    private:

	IWireVector m_wires;
    };


}

#endif
