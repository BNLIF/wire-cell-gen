#ifndef WIRECELL_SIMPLEFRAME
#define  WIRECELL_SIMPLEFRAME

#include "WireCellIface/IFrame.h"
#include <vector>

namespace WireCell {

    /** A simple frame.
     *
     * This is is nothing more than a bag of data.
     */ 
    class SimpleFrame : public IFrame {
    public:

	SimpleFrame(int ident, double time, const ITraceVector& traces);
	~SimpleFrame();
	virtual int ident() const;
	virtual double time() const;
    
	virtual iterator begin();
	virtual iterator end();
    private:
	int m_ident;
	double m_time;
	ITraceVector m_traces;
    };

}

#endif
