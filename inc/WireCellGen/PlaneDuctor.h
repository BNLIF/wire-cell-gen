/**
   The PlaneDuctor ("ductor" = either an inductor or a conductor)
   produces digitized charge signals on its plane of wires from
   depositions that have been drifted to the plane.  It handles the
   buffering required for diffusion to be applied, as well as actually
   applying it.  There is one PlaneDuctor per plane.  See also
   WireCell::Digitizer.
 */

#ifndef WIRECELL_PLANEDUCTOR
#define WIRECELL_PLANEDUCTOR

#include "WireCellIface/IPlaneDuctor.h"

#include "WireCellIface/IDepo.h"
#include "WireCellIface/IPlaneSlice.h"

#include <deque>

namespace WireCell {

    class BufferedHistogram2D;
    class Diffuser;

    class PlaneDuctor : public IPlaneDuctor {
    public:


	/// This giant constructor needs help.
	PlaneDuctor(WirePlaneId wpid, // the plane ID to stamp on the slices
		    const Ray& pitch, /// location, pitch and angle of wires
		    double tick = 0.5*units::microsecond, /// digitized time bin size
		    double tstart = 0.0, /// absolute time of first tick
		    double toffset = 0.0, /// arbitrary time offset added to deposition time
		    double drift_velocity = 1.6*units::millimeter/units::microsecond, 
		    double tbuffer = 5.0*units::microsecond, /// buffer long enough to get leading diffusion
		    double DL=5.3*units::centimeter2/units::second, /// long. diffusion coefficient
		    double DT=12.8*units::centimeter2/units::second, /// trans. diffusion coefficient
		    int nsigma=3); /// number of sigma of diffusion to keep

	virtual ~PlaneDuctor();

	/// Accept depositions.
	virtual bool sink(const IDepo::pointer& depo);

	/// Return the next available plane slice.
	virtual bool source(IPlaneSlice::pointer& plane_slice);
	
	// internal methods below.

	// Internal method for calculating the "proper time" of an
	// event.  This is the time for a point like event occurring
	// at t,x to drift to the location of our wire plane (as
	// determined by the tail of <pitch>).
	double proper_tau(double event_time, double event_xloc);

	// Internal method for calculating the distance along the
	// pitch from the origin of the wires (as determined by the
	// <pitch> ray) that the projection of the 3D point makes on
	// the wire plane.
	double pitch_dist(const Point& p);

    private:
	WirePlaneId m_wpid;
	const Vector m_pitch_origin;
	const Vector m_pitch_direction;
	const double m_toffset;
	const double m_drift_velocity;
	const double m_tbuffer;
	const double m_DL;
	const double m_DT;

	BufferedHistogram2D* m_hist;
	Diffuser* m_diff;
	double m_high_water_tau;
	bool m_eoi;


	// internal buffering
	std::deque<IDepo::pointer> m_input;
	std::deque<IPlaneSlice::pointer> m_output;
    };

}

#endif
