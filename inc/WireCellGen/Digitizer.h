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
        virtual bool set_wires(const IWire::shared_vector& wires);

	virtual bool operator()(const input_pointer& plane_slice_vector,
				output_pointer& channel_slice);

	virtual int nin() { return m_count; }
	virtual int nout() { return m_count; }

    private:
	// wires, organized by plane
	IWire::vector m_wires[3];

	// no input buffering
	std::deque<output_pointer> m_output;

    	int m_count;
    };

}
#endif
