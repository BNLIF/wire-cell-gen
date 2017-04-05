#ifndef WIRECELL_DRIFTER
#define WIRECELL_DRIFTER

#include "WireCellIface/IDrifter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellUtil/Units.h"

#include <deque>

namespace WireCell {

    /** This IDrifter accepts inserted depositions, drifts them to a
     * plane near an IAnodePlane and buffers long enough to assure
     * they can be delivered in time order.
     */
    class Drifter : public IDrifter, public IConfigurable {
    public:
	Drifter(const std::string& anode_plane_component="");

	/// WireCell::IDrifter interface.
	virtual void reset();
	virtual bool operator()(const input_pointer& depo, output_queue& outq);

	/// WireCell::IConfigurable interface.
	virtual void configure(const WireCell::Configuration& config);
	virtual WireCell::Configuration default_configuration() const;


        void set_anode(const std::string& anode_tn);

    private:

	double m_location, m_speed; // get from anode.

	// Input buffer sorted by proper time
	DepoTauSortedSet m_input;

	double proper_time(IDepo::pointer depo);

	bool m_eos;
    };


}

#endif
