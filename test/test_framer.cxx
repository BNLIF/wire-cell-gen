//#include "WireCellGen/Digitizer.h"
#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Drifter.h"
#include "WireCellGen/Digitizer.h"
#include "WireCellGen/TrackDepos.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/WireGenerator.h"
#include "WireCellGen/Framer.h"

#include "WireCellIface/WirePlaneId.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Faninout.h"
#include "WireCellUtil/Testing.h"

#include <iostream>
#include <boost/signals2.hpp>

using namespace WireCell;
using namespace std;

TrackDepos make_tracks() {
    TrackDepos td;

    const double cm = units::cm;
    Ray same_point(Point(cm,-cm,cm), Point(cm,+cm,+cm));

    const double usec = units::microsecond;

    td.add_track(1*usec, same_point);
    td.add_track(10*usec, same_point);
    td.add_track(100*usec, same_point);
    td.add_track(1000*usec, same_point);
    td.add_track(10000*usec, same_point);

    return td;
}

template<class ProcessorType, typename InputType, typename OutputType>
class SignalExecutor {
public:
    typedef boost::signals2::signal<InputType ()> signal_type;
    typedef typename signal_type::slot_type slot_type;

    SignalExecutor(ProcessorType& proc) : m_proc(proc) {}
    OutputType operator()() {
	OutputType out;
	while (!m_proc.source(out)) {
	    InputType in = *m_sig();
	    m_proc.sink(in);
	}
	return out;	
    }

    void connect(const slot_type& s) { m_sig.connect(s); }

private:
    ProcessorType& m_proc;
    signal_type m_sig;
};




int main()
{
    const double tick = 2.0*units::microsecond; 
    const int nticks_per_frame = 100;
    double now = 0.0*units::microsecond;

    IWireParameters::pointer iwp(new WireParams);

    WireGenerator wg;

    // Build a "mini application" using a lambda to return the wire params.
    SignalExecutor<WireGenerator, IWireParameters::pointer, IWireVector> wgse(wg);
    auto blerg = [iwp]() {return iwp;};
    wgse.connect(blerg);
    IWireVector wires = wgse();
    Assert(wires.size());


    TrackDepos td = make_tracks();

    // drift
    WireCell::Drifter drifter0(iwp->pitchU().first.x());
    WireCell::Drifter drifter1(iwp->pitchV().first.x());
    WireCell::Drifter drifter2(iwp->pitchW().first.x());

    typedef SignalExecutor<Drifter, IDepo::pointer, IDepo::pointer>  DriftExecutor;
    DriftExecutor driftse0(drifter0);
    DriftExecutor driftse1(drifter1);
    DriftExecutor driftse2(drifter2);

    // fanout

    Fanout<IDepo::pointer> fandepo;
    fandepo.connect(td);
    for (int addr = 0; addr < 3; ++addr) {
	fandepo.address(addr);	// register
    }

    driftse0.connect([&fandepo]() { return fandepo(0); });
    driftse1.connect([&fandepo]() { return fandepo(1); });
    driftse2.connect([&fandepo]() { return fandepo(2); });

    // diffuse + collect/induce

    PlaneDuctor pd0(WirePlaneId(kUlayer), iwp->pitchU(), tick, now);
    PlaneDuctor pd1(WirePlaneId(kVlayer), iwp->pitchV(), tick, now);
    PlaneDuctor pd2(WirePlaneId(kWlayer), iwp->pitchW(), tick, now);

    typedef IPlaneSlice::pointer IPSptr;
    typedef SignalExecutor<PlaneDuctor, IDepo::pointer, IPSptr> PDExecutor;

    PDExecutor pdse0(pd0);
    PDExecutor pdse1(pd1);
    PDExecutor pdse2(pd2);

    pdse0.connect(boost::ref(driftse0));
    pdse1.connect(boost::ref(driftse1));
    pdse2.connect(boost::ref(driftse2));

    // fanin 

    typedef boost::signals2::signal<IPSptr (), Fanin< IPlaneSliceVector > > PlaneSliceFanin;
    PlaneSliceFanin fanin;
    fanin.connect(boost::ref(pdse0));
    fanin.connect(boost::ref(pdse1));
    fanin.connect(boost::ref(pdse2));
    

    // digitize
    
    Digitizer digitizer;
    typedef SignalExecutor<Digitizer, IPlaneSliceVector, IChannelSlice::pointer> DigiExecutor;
    DigiExecutor digise(digitizer);
    digise.connect(boost::ref(fanin));
    

    // frame

    Framer framer;
    typedef SignalExecutor<Framer, IChannelSlice::pointer, IFrame::pointer> FrameExecutor;
    FrameExecutor framese(framer);
    framese.connect(boost::ref(digise));


    // process
    while (true) {
    	auto frame = framese();
    	if (!frame) { break; }

    	int ntraces = boost::distance(frame->range());
    	cerr << "Frame: #" << frame->ident()
    	     << " at t=" << frame->time()/units::microsecond << " usec"
    	     << " with " << ntraces << " traces"
    	     << endl;
    	for (auto trace : *frame) {
    	    cerr << "\ttrace ch:" << trace->channel()
    		 << " start tbin=" << trace->tbin()
    		 << " #time bins=" << trace->charge().size()
    		 << endl;
    	}

    }

    return 0;
}
