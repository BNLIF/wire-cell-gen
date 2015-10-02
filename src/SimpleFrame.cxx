#include "WireCellGen/SimpleFrame.h"

#include <iostream>

using namespace WireCell;
using namespace std;

SimpleFrame::SimpleFrame(int ident, double time, const ITrace::vector& traces)
    : m_ident(ident), m_time(time), m_traces(new ITrace::vector(traces.begin(), traces.end()))
{
//    cerr << "SimpleFrame(" << ident << " , " << time << " , " << traces.size() << ")" << endl;
}
SimpleFrame::~SimpleFrame() {

}
int SimpleFrame::ident() const { return m_ident; }
double SimpleFrame::time() const { return m_time; }
    
ITrace::shared_vector SimpleFrame::traces() const { return m_traces; }


