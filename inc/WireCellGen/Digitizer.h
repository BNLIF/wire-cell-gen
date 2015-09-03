#ifndef WIRECELL_DIGITIZER
#define WIRECELL_DIGITIZER


#include "WireCellIface/IDigitizer.h"

namespace WireCell {

    /** This simple digitizer takes plane slices and reorganizes their
     * contents into channel slices.  Charges on wires going to the
     * same channel are simply summed.
     *
     * Internal buffering of inputs and outputs is of depth 1.
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
	virtual bool sink(const IWireVector& wires);

	/** Feed plane slices.  
	 *
	 * Ordering of vector is not important.  
	 *
	 * Only one input is buffered and it invalidates any
	 * previously cached channel slice.
	 */
	virtual bool sink(const IPlaneSliceVector& plane_slices);

	/** Return the next channel slice.
	 *
	 * Only one (current) output is buffered.  Subsequent calls
	 * will return the same.
	 */
	virtual bool source(IChannelSlice::pointer& returned_channel_slice);


    private:

	// wires, organized by plane
	IWireVector m_wires[3];

	// Current plane slices
	IPlaneSliceVector m_plane_slices;
    };

}
#endif
