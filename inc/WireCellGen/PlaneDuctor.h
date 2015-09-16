#ifndef WIRECELL_PLANEDUCTOR
#define WIRECELL_PLANEDUCTOR

#include "WireCellIface/IPlaneDuctor.h"
#include "WireCellIface/IDiffusion.h"

#include <deque>

namespace WireCell {

    class BufferedHistogram2D;
    class Diffuser;

    // IConverter<IDiffusion::pointer, IPlaneSlice::pointer>
    class PlaneDuctor : public IPlaneDuctor {
    public:


	/** Create a PlaneDuctor that produces plane slices from diffusions.
	 *
	 * \param wpid is the wire plane ID given to the resulting
	 * plane slice.
	 *
	 * \param nwires gives the number of wires in the plane.
	 *
	 * \param lbin is the longitudinal (time) bin width.
	 *
	 * \param tbin is the transverse (pitch) bin width.
	 *
	 * \param lpos0 is the longitudinal (time) origin (starting
	 * time of zero'th latch).  Time bins are assumed to cover one
	 * tick, inclusive of the tick start time and exclusive of the
	 * tick end time.
	 * 
	 * \param tpos0 is the transverse (pitch) origin (location of
	 * wire zero along the pitch direction).  Transverse bins are
	 * assumed to be centered on the wire with the low/high edges
	 * extending to a half pitch distance below/above the wire
	 * inclusive/exclusive, respectively.
	 * 
	 * Note: the origin and bin widths should be sync'ed,
	 * quantitatively and conceptually to be the same as used to
	 * form the input diffusion patches.
	 */
	PlaneDuctor(WirePlaneId wpid, int nwires,
		    double lbin = 0.5*units::microsecond,
		    double tbin = 5.0*units::millimeter,
		    double lpos0 = 0.0*units::microsecond,
		    double tpos0 = 0.0*units::millimeter);



	virtual ~PlaneDuctor();

	virtual void reset();
	virtual void flush();
	virtual bool insert(const input_type& diffusion);
	virtual bool extract(output_type& plane_slice);


	// internal

	// remove any diffusions that are fully past the planeductor's "now"
	void purge_bygone();
	// produce a plane slice from all diffusions that overlap "now"
	IPlaneSlice::pointer latch_hits();
	// purge + latch
	void latch_one();
	// return true if we have not buffered enough to assure we see
	// all diffusion patches for the current "now" time.
	bool starved();

	// debugging
	int ninput() const { return m_input.size(); }
	int noutput() const { return m_output.size(); }

    private:
	WirePlaneId m_wpid;
	int m_nwires;
	double m_lbin, m_tbin, m_lpos;
	const double m_tpos0;

	// internal buffering
	IDiffusionSet m_input;
	std::deque<IPlaneSlice::pointer> m_output;

    };

}

#endif
