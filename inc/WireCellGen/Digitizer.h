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
	virtual void reset();
	virtual bool insert(const input_pointer& plane_slice_vector);
	virtual bool extract(output_pointer& channel_slice);

    private:
	void flush();

	// wires, organized by plane
	IWire::vector m_wires[3];

	// no input buffering
	std::deque<output_pointer> m_output;
    };

}
#endif
