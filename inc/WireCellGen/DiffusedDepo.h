#ifndef WIRECELL_NAV_DIFFUSEDDEPO
#define WIRECELL_NAV_DIFFUSEDDEPO

#include "WireCellIface/IDepo.h"

namespace WireCell {
    namespace Gen {

	// This is like SimpleDepo but requires a prior from which it
	// gets the charge.
	class DiffusedDepo : public WireCell::IDepo {
	    WireCell::IDepo::pointer m_prior;
	    WireCell::Point m_pos;
	    double m_time;
            double m_extent_long, m_extent_tran;
	public:

	    DiffusedDepo(const WireCell::IDepo::pointer& prior,
                         const Point& here, double now,
                         double sigma_long, double sigma_tran)
		: m_prior(prior), m_pos(here), m_time(now), m_sigma_log(sigma_long), m_sigma_tran(sigma_tran)
            {
	    }
	    virtual ~DiffusedDepo() {};

	    virtual const WireCell::Point& pos() const { return m_pos; }
	    virtual double time() const { return m_time; }
	    virtual double charge() const { return m_from->charge(); }
	    virtual WireCell::IDepo::pointer prior() const { return m_from; }
            virtual double extent_long() const { return m_sigma_long; }
            virtual double extent_tran() const { return m_sigma_tran; }

	};

    }
}
#endif
