#ifndef WIRECELL_DRIFTER
#define WIRECELL_DRIFTER

#include "WireCellIface/IDrifter.h"
#include "WireCellUtil/Units.h"

#include <deque>

namespace WireCell {

    /** A Drifter takes inserted depositions, drifts and buffers as
     * needed and reproduces new depositions drifted to a new
     * location.
     */
    class Drifter : public IDrifter {
    public:
	/// Create a drifter that will drift charge to a given
	/// location at a given drift velocity.

	Drifter(double x_location=0*units::meter,
		double drift_velocity = 1.6*units::mm/units::microsecond);
	// fixme: make configureable

	/// WireCell::IDrifter interface.
	virtual void reset();
	virtual bool insert(const input_pointer& depo);
	virtual bool extract(output_pointer& depo);

	int ninput() const { return m_input.size(); }
	int noutput() const { return m_output.size(); }

    private:
	void flush();

	double m_location, m_drift_velocity;
	bool m_eoi;		// end of input

	// Input buffer sorted by proper time
	DepoTauSortedSet m_input;

	// Output buffer of depos safe to remove from input
	std::deque<IDepo::pointer> m_output;

	double proper_time(IDepo::pointer depo);
    };


}

#endif
