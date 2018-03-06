#include "WireCellGen/DepoMerger.h"

#include "WireCellUtil/NamedFactory.h"


WIRECELL_FACTORY(DepoMerger, WireCell::Gen::DepoMerger,
                 WireCell::IDepoMerger, WireCell::IConfigurable);


using namespace WireCell;

Gen::DepoMerger::DepoMerger()
{
    get<0>(m_eos) = false;
    get<1>(m_eos) = false;
}
Gen::DepoMerger::~DepoMerger()
{
}


bool Gen::DepoMerger::operator()(input_queues_type& inqs,
                                 output_queues_type& outqs)
{
    if (get<0>(m_eos) and get<1>(m_eos)) { // both reach end of stream
        return false;
    }

    auto& inq0 = get<0>(inqs);
    auto& inq1 = get<1>(inqs);
    auto& outq = get<0>(outqs);

    // both are streaming so take the newest
    if (!get<0>(m_eos) and !get<1>(m_eos)) { 
        auto& d0 = inq0.front();
        auto& d1 = inq1.front();

        if (!d0) {              // hit eos on stream 0
            inq0.pop_front();
            inq1.pop_front();
            get<0>(m_eos) = true;
            outq.push_back(d1);
            return true;
        }
        if (!d1) {              // hit eos on stream 1
            inq0.pop_front();
            inq1.pop_front();
            get<1>(m_eos) = true;
            outq.push_back(d0);
            return true;
        }

        // both good depos.

        double t0 = d0->time();        
        double t1 = d1->time();        
        if (t0 <= t1) {
            outq.push_back(d0);
            inq0.pop_front();
        }
        if (t1 <= t0) {
            outq.push_back(d1);
            inq1.pop_front();
        }
        return true;
    }   

    // One stream is already ended.

    if (!get<0>(m_eos)) {
        auto d0 = inq0.front();
        if (!d0) {
            get<0>(m_eos) = true;
        }
        outq.push_back(d0);     // pass on
        inq0.pop_front();
        return true;
    }

    if (!get<1>(m_eos)) {
        auto d1 = inq1.front();
        if (!d1) {
            get<1>(m_eos) = true;
        }
        outq.push_back(d1);     // pass on
        inq1.pop_front();
        return true;
    }

    // should not get here.
    return false;
}


void Gen::DepoMerger::configure(const WireCell::Configuration& cfg)
{
}

WireCell::Configuration Gen::DepoMerger::default_configuration() const
{
    Configuration cfg;
    return cfg;
}
