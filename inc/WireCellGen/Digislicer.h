/** fixme: this is an obsolete class that produces one sample by N
 * channels.  See IFrameFilter for more general.
 */

#ifndef WIRECELL_DIGISLICER
#define WIRECELL_DIGISLICER

#include "WireCellIface/IDigislicer.h"
#include <deque>

namespace WireCell {
    
    /** This simple digislicer takes plane slices and reorganizes their
     * contents into channel slices.  Charges on wires going to the
     * same channel are simply summed.
     */
    class Digislicer : public IDigislicer {
    public:
	Digislicer();       
	virtual ~Digislicer();       

        /** Feed new wires to use.  
	 *
	 * This must be called once and before any output can be expected.
	 *
	 * Calling it does not invalidate a previously sunk plane
	 * slice vector but will invalidate any previously sunk wires.
	 */
        //virtual bool set_wires(const IWire::shared_vector& wires);

	virtual bool operator()(const input_tuple_type& intup, output_pointer& out);

//	virtual bool operator()(const input_pointer& plane_slice_vector,
//				output_pointer& channel_slice);

    private:
	// wires, organized by plane
//	IWire::vector m_wires[3];
    };

}
#endif
