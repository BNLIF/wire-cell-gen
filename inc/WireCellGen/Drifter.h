#ifndef WIRECELL_DRIFTER
#define WIRECELL_DRIFTER

#include "WireCellIface/IDepo.h"
#include "WireCellUtil/Units.h"

#include <boost/signals2.hpp>

namespace WireCell {

    /** A Drifter takes depositions from the connected slot and
     * produces new depositions drifted to a given location.  It
     * properly manages the time/distance ordering by reading ahead
     * from its input stream just far enough in space and time.
     */
    class Drifter {
    public:
	/// Create a drifter that will drift charge to a given
	/// location at a given drift velocity.
	
	Drifter(double x_location=0*units::meter,
		double drift_velocity = 1.6*units::mm/units::microsecond);
	// fixme: make configureable

	/// Produce the next drifted deposition. 
	IDepo::pointer operator()();

	void connect(const IDepo::source_slot& s) { m_input.connect(s); }

    private:
	double m_location, m_drift_velocity;
	DepoTauSortedSet m_buffer;
	IDepo::source_signal m_input;


	double proper_time(IDepo::pointer depo);

	// buffer management
	bool buffer();
	IDepo::pointer top();
	IDepo::pointer bot();
	IDepo::pointer pop();

    };


}

#endif
