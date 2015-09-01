#ifndef WIRECELL_DRIFTER
#define WIRECELL_DRIFTER

#include "WireCellIface/IDrifter.h"
#include "WireCellUtil/Units.h"

#include <deque>

namespace WireCell {

    /** A Drifter takes depositions from the connected slot and
     * produces new depositions drifted to a given location.  It
     * properly manages the time/distance ordering by reading ahead
     * from its input stream just far enough in space and time.
     */
    class Drifter : public IDrifter {
    public:
	/// Create a drifter that will drift charge to a given
	/// location at a given drift velocity.
	
	Drifter(double x_location=0*units::meter,
		double drift_velocity = 1.6*units::mm/units::microsecond);
	// fixme: make configureable

	/// This drifter internally buffers all input 
	bool sink(const IDepo::pointer& depo);

	/// Drift one deposition
	bool process();

	/// If the buffer is deep enough, this pops the next available
	/// deposition.  The depth of the buffer is maintained such
	/// that time at origin of the last expected (of all currently
	/// buffered) deposition is after the arrive time of the
	/// earliest expected (of all currently buffered).
	bool source(IDepo::pointer& depo);

    private:
	double m_location, m_drift_velocity;
	DepoTauSortedSet m_input;
	std::deque<IDepo::pointer> m_output;

	double proper_time(IDepo::pointer depo);
    };


}

#endif
