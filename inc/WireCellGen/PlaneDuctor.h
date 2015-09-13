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
	 * \param lbin is the longitudinal (time) bin width.
	 *
	 * \param tbin is the transverse (pitch) bin width.
	 *
	 * \param lpos0 is the longitudinal (time) origin (starting
	 * time of zero'th latch).
	 * 
	 * \param tpos0 is the transverse (pitch) origin (location of
	 * wire zero along the pitch direction).
	 * 
	 * Note: the origin and bin widths should be sync'ed to be the
	 * same as used to form the input diffusion patches.
	 */
	PlaneDuctor(WirePlaneId wpid,
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
	void purge_bygone();
	IPlaneSlice::pointer latch_hits();
	bool latch_one();

    private:
	WirePlaneId m_wpid;
	double m_lbin, m_tbin, m_lpos;
	const double m_tpos0;

	// internal buffering
	IDiffusionSet m_input;
	std::deque<IPlaneSlice::pointer> m_output;

    };

}

#endif
