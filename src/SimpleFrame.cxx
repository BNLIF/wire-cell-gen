#include "WireCellGen/SimpleFrame.h"

#include <iostream>

using namespace WireCell;
using namespace std;

SimpleFrame::SimpleFrame(int ident, double time, const ITraceVector& traces)
    : m_ident(ident), m_time(time), m_traces(traces)
{
//    cerr << "SimpleFrame(" << ident << " , " << time << " , " << traces.size() << ")" << endl;
}
SimpleFrame::~SimpleFrame() {

}
int SimpleFrame::ident() const { return m_ident; }
double SimpleFrame::time() const { return m_time; }
    
SimpleFrame::iterator SimpleFrame::begin() { return adapt(m_traces.begin()); }
SimpleFrame::iterator SimpleFrame::end() { return adapt(m_traces.end()); }


