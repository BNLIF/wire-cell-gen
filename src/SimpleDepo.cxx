#include "WireCellGen/SimpleDepo.h"

using namespace WireCell;

SimpleDepo::SimpleDepo(double t, const WireCell::Point& pos,
		       double charge, IDepo::pointer prior)
    : m_time(t), m_pos(pos), m_charge(charge), m_prior(prior)
{
}

const WireCell::Point& SimpleDepo::pos() const
{
    return m_pos; 
}
double SimpleDepo::time() const 
{ 
    return m_time; 
}
double SimpleDepo::charge() const
{
    return m_charge;
}
WireCell::IDepo::pointer SimpleDepo::prior() const
{
    return m_prior;
}
