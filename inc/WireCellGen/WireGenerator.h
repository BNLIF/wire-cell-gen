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

    class WireGenerator : public IWireGenerator {
    public:
	WireGenerator();
	virtual ~WireGenerator();

	// Feed the parameters, triggering the generation of wires.
	virtual bool sink(const IWireParameters::pointer& wp);

	// Return the most recently produced wires.
	virtual bool source(IWireVector& ret);


    private:

	IWireVector m_wires;
    };


}

#endif
