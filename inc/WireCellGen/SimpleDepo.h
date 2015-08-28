#include "WireCellIface/IDepo.h"

namespace WireCell {

    // A deposition that simply holds all the data it presents.  If
    // you have something that makes depositions immediately and needs
    // no "smarts" just use this to hold their info outright.
    class SimpleDepo : public WireCell::IDepo {
    public:
	SimpleDepo(double t, const WireCell::Point& pos,
		   double charge = 1.0, IDepo::pointer prior = nullptr);

	virtual const WireCell::Point& pos() const;
	virtual double time() const;
	virtual double charge() const;
	virtual WireCell::IDepo::pointer prior() const;


    private:
	// bag o' data
	double m_time;
	WireCell::Point m_pos;
	double m_charge;
	IDepo::pointer m_prior;

    };

}
