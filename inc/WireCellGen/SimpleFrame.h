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

	SimpleFrame(int ident, double time, const ITrace::vector& traces);
	~SimpleFrame();
	virtual int ident() const;
	virtual double time() const;
    
	virtual ITrace::shared_vector traces() const;

    private:
	int m_ident;
	double m_time;
	ITrace::shared_vector m_traces;
    };

}

#endif
