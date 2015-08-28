#ifndef WIRECELL_PARAMGEOMETRY
#define WIRECELL_PARAMGEOMETRY

#include "WireCellIface/IGeometry.h"
#include "WireCellIface/IConfigurable.h"


namespace WireCell {

    /** Full geometry class predicated on parameterization of the wires. */
    class ParamGeometry : public IGeometry, public IConfigurable {

    public:

	ParamGeometry() ; 
	virtual ~ParamGeometry();

	virtual void configure(const WireCell::Configuration& config);
	virtual WireCell::Configuration default_configuration() const;

	virtual IWireSequence::pointer wires() { return m_wires; }
	virtual ICellSequence::pointer cells() { return m_cells; }
	virtual ITiling::pointer tiling() { return m_tiling; }
	virtual IWireSummary::pointer wire_summary() { return m_wiresummary; }

    private:
	IWireSequence::pointer m_wires;
	ICellSequence::pointer m_cells;
	IWireSummary::pointer m_wiresummary;
	ITiling::pointer m_tiling;

    };
}


#endif
