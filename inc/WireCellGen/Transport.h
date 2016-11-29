#ifndef WIRECELLGEN_TRANSPORT
#define WIRECELLGEN_TRANSPORT

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"
#include "WireCellIface/IDepo.h"

#include <deque>

namespace WireCell {
    namespace Gen {

	/** A DepoPlaneX collects depositions and drifts them to the
	 * given plane assuming a uniform drift velocity which is in
	 * the negative X direction.
	 *
	 * It is assumed new depositions are added strictly in time
	 * order.  They will then be drifted and maintained in the
	 * order of their time at the plane.
	 *
	 * The time of the most recently added depo sets a high water
	 * mark in time at the plane such that all newly added depos
	 * must (causally) come later.  Any depos older than this time
	 * are considered "frozen out" as nothing can change their
	 * ordering.
	 */

	class DepoPlaneX {
	    double m_planex, m_speed;
	    DepoTauSortedSet m_queue;
	    std::deque<IDepo::pointer> m_frozen;
	public:
	    DepoPlaneX(double planex = 0.0*units::cm,
		       double speed = 1.6*units::millimeter/units::microsecond);

	    /// Add a deposition and drift it into the queue at the
	    /// plane.  Depos must be strictly added in (local) time
	    /// order.
	    IDepo::pointer add(const IDepo::pointer& depo);

	    /// The time a deposition would have if it drifts to the plane
	    double proper_time(IDepo::pointer depo) const;

	    /// Return the time at the plane before which the order of
	    /// collected depositions are guaranteed (causally) to
	    /// remain unchanged as any new depos are added.
	    double freezeout_time() const;

	    /// Force any remaining "thawed" depos in the queue to be frozen out.
	    void freezeout();

	private:
	    /// Move all froze-out depos to the frozen queue
	    void drain(double time);
	};



	// /// A DepoPitchStack maintains a binned and ordered collection of
	// /// IDepo objects ordered by pitch direction.
	// class DepoPitchStack {
	// public:

	//     /** Create a DepoStack

	// 	\param pitch Ray with tail on center of wire zero,
	// 	head on wire 1, perpendicular to both.

	// 	\param impacts_per_pitch Number of "impact positions"
	// 	per pitch distance.

	// 	\param impact_offset Distance from a wire to where the
	// 	first impact position begins.
	//     */
	//     DepoStack(const Ray& pitch,
	// 	      int impacts_per_pitch=10,
	// 	      double impact_offset = 0.0*units::mm);

	//     /** Add a deposition.
	//      */
	//     void add(const IDepo::pointer& depo);


	// };

    }

}

#endif
