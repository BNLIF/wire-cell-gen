#include "WireCellGen/DepoMerger.h"

#include "WireCellUtil/NamedFactory.h"


WIRECELL_FACTORY(DepoMerger, WireCell::Gen::DepoMerger,
                 WireCell::IDepoMerger, WireCell::IConfigurable);


using namespace WireCell;

Gen::DepoMerger::DepoMerger()
    : m_nin0(0), m_nin1(0), m_nout(0)
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
    if (get<0>(m_eos) and get<1>(m_eos)) { // both have already reached end of stream before
        std::cerr << "DepoMerger: called after EOS\n";
        return false;
    }

    auto& inq0 = get<0>(inqs);
    auto& inq1 = get<1>(inqs);
    auto& outq = get<0>(outqs);

    if (inq0.empty() and inq1.empty()) {
        // We shouldn't even have been called in this case.
        // It means the graph execution engine is broken
        std::cerr << "DepoMerger: called with both input queues empty\n";
        return false;
    }

    std::cerr << "DepoMerger queues: "
              << " inq0=" << demangle(typeid(inq0).name())
              << " inq1=" << demangle(typeid(inq1).name())
              << " outq=" << demangle(typeid(outq).name())
              << "\n";


    // keep track is we need to pop an input
    bool pop0=false, pop1=false;

    IDepo::pointer d0=nullptr;
    if (!get<0>(m_eos) and !inq0.empty()) {
        d0 = inq0.front();
        if (!d0) {              // just got EOS
            std::cerr << "DepoMerger: stream 0 got EOS\n"; 
            pop0 = true;
            get<0>(m_eos) = true;
        }
    }
    IDepo::pointer d1=nullptr;
    if (!get<1>(m_eos) and !inq1.empty()) {
        d1 = inq1.front();
        if (!d1) {              // just got EOS
            std::cerr << "DepoMerger: stream 1 got EOS\n"; 
            pop1 = true;
            get<1>(m_eos) = true;
        }
    }

    if (d0 and d1) {
        double t0 = d0->time(); // keep the newest one
        double t1 = d1->time(); // which may be both if they coincide
        if (t0 <= t1) {
            ++m_nout;
            outq.push_back(d0);
            pop0 = true;
            std::cerr << "DepoMerger: stream 0 output\n"; 
        }
        if (t1 <= t0) {
            ++m_nout;
            outq.push_back(d1);
            pop1 = true;
            std::cerr << "DepoMerger: stream 1 output\n"; 
        }
    }   
    else if (d0) {
        ++m_nout;
        outq.push_back(d0);
        pop0 = true;
        std::cerr << "DepoMerger: stream 0 only output\n"; 
    }
    else if (d1) {
        ++m_nout;
        outq.push_back(d1);
        pop1 = true;
        std::cerr << "DepoMerger: stream 1 only output\n"; 
    }
    else {                      // both nullptr, must have reached full EOS
        outq.push_back(nullptr);
        std::cerr << "DepoMerger: global EOS: in: "
                  << m_nin0 << " + " << m_nin1
                  << ", out: " << m_nout << " depos\n";
    }
        
    if (pop0) {
        inq0.pop_front();
        ++m_nin0;
    }
        
    if (pop1) {
        inq1.pop_front();
        ++m_nin1;
    }

    return true;
}


void Gen::DepoMerger::configure(const WireCell::Configuration& cfg)
{
}

WireCell::Configuration Gen::DepoMerger::default_configuration() const
{
    Configuration cfg;
    return cfg;
}
