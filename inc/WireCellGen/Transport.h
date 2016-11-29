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
	public:
	    typedef std::deque<IDepo::pointer> frozen_queue_t;
	    typedef DepoTauSortedSet working_queue_t;

	    DepoPlaneX(double planex = 0.0*units::cm,
		       double speed = 1.6*units::millimeter/units::microsecond);

	    /// Add a deposition and drift it into the queue at the
	    /// plane.  Depos must be strictly added in (local) time
	    /// order.  The drifted depo is returned (and held).
	    IDepo::pointer add(const IDepo::pointer& depo);

	    /// The time a deposition would have if it drifts to the plane
	    double proper_time(IDepo::pointer depo) const;

	    /// Return the time at the plane before which the order of
	    /// collected depositions are guaranteed (causally) to
	    /// remain unchanged as any new depos are added.
	    double freezeout_time() const;

	    /// Force any remaining "thawed" depos in the queue to be frozen out.
	    void freezeout();

	    /// Return ordered vector of all depositions at the plane
	    /// with times not later than the given time.  The given
	    /// time is typically the freezeout time.  If a time later
	    /// than the freezout time is given it may cause depos to
	    /// be artificially frozen out.
	    IDepo::vector pop(double time);

	    // Access internal const data structures to assist in debugging
	    const frozen_queue_t& frozen_queue() const { return m_frozen; }
	    const working_queue_t& working_queue() const { return m_queue; }

	private:
	    double m_planex, m_speed;
	    working_queue_t m_queue;
	    frozen_queue_t m_frozen;

	    /// Move all froze-out depos to the frozen queue
	    void drain(double time);
	};



	/**  A BinnedDiffusion takes IDepo objects and "paints"
	 *  them onto bins in time and pitch direction.
	 *
	 *  Partial results are provided inside a window of fixed
	 *  width in the pitch direction.
	 *
	 *  Results are either provided in time or frequency domain.
	 *
	 */
	class BinnedDiffusion {
	public:
	    BinnedDiffusion(const Ray& pitch_bin,
			    int ntime_bins, double min_time, double max_time,
			    double nsigma=3.0);

	    // Add a deposition with given longitudinal (temporal) and
	    // transverse (spatial) widths (sigmas).  These should be
	    // calculated based on some diffusion model.
	    void add(const IDepo::pointer& depo, double sigmaL, double sigmaT);

	    // Set the window to be between two pitches.  This should
	    // be called after all IDepo objects have been added.
	    void set_window(double pitch_min, double pitch_max);

	    // Access the time domain waveform at the given pitch inside the pitch window.
	    //const Waveform::realseq_t& waveform(double pitch);
	    // Access the frequency domain spectrum at the given pitch inside the pitch window.
	    //const Waveform::compseq_t& spectrum(double pitch);	    

	private:

	    
		
			 
	};


    }

}

#endif
