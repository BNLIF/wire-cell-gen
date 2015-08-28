#ifndef WIRECELL_DEPOBUFFER
#define WIRECELL_DEPOBUFFER

#include "WireCellIface/IDepo.h"
#include <boost/range.hpp>

#include <deque>


namespace WireCell {
    // Maintain a buffer of depositions which span a window of time
    // centered on "now".

    class DepoBuffer {
	std::deque<IDepo::pointer> m_depos;
	const double m_window;
	double m_now;
    public:
	typedef std::deque<IDepo::pointer> buffer_type;
	typedef buffer_type::iterator iterator;
	typedef boost::iterator_range<iterator> iterator_range;

	DepoBuffer(double window, double now = 0);
	~DepoBuffer();
    
	/// Advance "now" by a time interval and return any
	/// depositions that have fallen out of the window.
	IDepoVector advance(double deltat);

	/** Access the current buffer as a range.
	 *
	 * for (auto depo : mydepobuffer()) { depo->....; }
	 */
	iterator_range operator()();

	void connect(const IDepo::source_slot& s) { m_input.connect(s); }

    private:
	// add to deque
	void add(IDepo::pointer depo);

	IDepo::source_signal m_input;
    };

}

#endif
