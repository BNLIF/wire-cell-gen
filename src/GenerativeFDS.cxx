#include "WireCellNav/GenerativeFDS.h"

using namespace WireCell;
using namespace std;

GenerativeFDS::GenerativeFDS(const Depositor& dep, const GeomDataSource& gds, 
			     int bins_per_frame, int nframes_total, float drift_speed)
    : dep(dep)
    , gds(gds)
    , bins_per_frame(bins_per_frame)
    , max_frames(nframes_total)
    , drift_speed(drift_speed)
{
}


GenerativeFDS::~GenerativeFDS()
{
}

int GenerativeFDS::size() const { return max_frames; }

WireCell::SimTruthSelection GenerativeFDS::truth() const
{
    WireCell::SimTruthSelection ret;

    for (auto it = simtruth.begin(); it != simtruth.end(); ++it) {
	ret.push_back(& (*it));
    }
    return ret;
}


int GenerativeFDS::jump(int frame_number)
{
    if (frame_number < 0) {
	return -1;
    }
    if (max_frames >=0 && frame_number >= max_frames) {
	return -1;
    }

    if (frame.index == frame_number) {
	return frame_number;
    }

    frame.clear();

    const PointValueVector& hits = dep.depositions(frame_number);

    size_t nhits = hits.size();


   
    

    simtruth.clear();


    for (size_t ind=0; ind<nhits; ++ind) {
	const Point& pt = hits[ind].first;
	float charge = hits[ind].second;
	int tbin = pt.x;
	
	if (tbin >= bins_per_frame) {
	    continue;
	}

	if (!gds.contained_yz(pt)) {
	    continue;
	}
	  
	//SimTruth(float x, float y, float z, float q, int tdc, int trackid=-1);
	WireCell::SimTruth st(drift_speed*tbin, pt.y, pt.z, charge, tbin, simtruth.size());
	simtruth.insert(st);
	//cerr << "SimTruth: " << st.trackid() << " q=" << st.charge() << endl;

	for (int iplane=0; iplane < 3; ++iplane) {
	  //	  std::cout << nhits << " " << ind << std::endl;

	    WirePlaneType_t plane = static_cast<WirePlaneType_t>(iplane); // annoying
	    const GeomWire* wire = gds.closest(pt, plane);
	    int chid = wire->channel();

	    // make little one hit traces
	    Trace trace;
	    trace.chid = chid;
	    trace.tbin = tbin;
	    trace.charge.push_back(charge);
	    frame.traces.push_back(trace);
	}	
    }
    
    frame.index = frame_number; 
    return frame.index;
}

