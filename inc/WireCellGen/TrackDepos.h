#ifndef WIRECELL_TRACKDEPOS
#define WIRECELL_TRACKDEPOS

#include "WireCellIface/IDepoSource.h"
#include "WireCellIface/ISource.h"
#include "WireCellUtil/Units.h"

namespace WireCell {
/// A producer of depositions created from some number of simple, linear tracks.
    class TrackDepos 
	: public IDepoSource
    {
    public:    
	/// Create tracks with depositions every stepsize and assumed
	/// to be traveling at clight.
	TrackDepos(double stepsize=1.0*units::millimeter,
		   double clight=units::clight);
	virtual ~TrackDepos();

	/// Add track starting at given <time> and stretching across given
	/// ray.  The <dedx> gives a uniform charge/distance and if < 0
	/// then it gives the (negative of) absolute amount of charge per
	/// deposition.
	void add_track(double time, const WireCell::Ray& ray, double dedx=-1.0);

	/// Pop the next available deposition in time-order.
	WireCell::IDepo::pointer operator()();

	/// ISource
	bool extract(output_type& out);

	/// Utility: access entire deposition store.
	//WireCell::IDepo::shared_vector depositions();

    private:
	const double m_stepsize;
	const double m_clight;
	WireCell::IDepo::vector m_depos;
    
    };

}

#endif
