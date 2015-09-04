#ifndef WIRECELL_DIGITIZER
#define WIRECELL_DIGITIZER

#include "WireCellIface/IDigitizer.h"
#include <deque>

namespace WireCell {
    
    /** This simple digitizer takes plane slices and reorganizes their
     * contents into channel slices.  Charges on wires going to the
     * same channel are simply summed.
     */
    class Digitizer : public IDigitizer {
    public:
	Digitizer();       
	virtual ~Digitizer();       

        /** Feed new wires to use.  
	 *
	 * This must be called once and before any output can be expected.
	 *
	 * Calling it does not invalidate a previously sunk plane
	 * slice vector but will invalidate any previously sunk wires.
	 */
        virtual bool set_wires(const IWireVector& wires);
	virtual void reset();
	virtual void flush();
	virtual bool insert(const input_type& plane_slice_vector);
	virtual bool extract(output_type& channel_slice);


    private:

	// wires, organized by plane
	IWireVector m_wires[3];

	// no input buffering
	std::deque<output_type> m_output;
    };

}
#endif
