#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Diffuser.h"

#include "WireCellUtil/BufferedHistogram2D.h"

#include <iostream>
using namespace std;		// debugging
using namespace WireCell;


// fixme: this big constructor is kind of a mess.  In any case, this
// class needs to implement a "plane ductor" interface and be an
// configurable.
PlaneDuctor::PlaneDuctor(WirePlaneId wpid, 
			 const Ray& pitch,
			 double tick,
			 double tstart,
			 double toffset,
			 double drift_velocity,
			 double tbuffer,
			 double DL,
			 double DT,
			 int nsigma)
    : m_wpid(wpid)
    , m_pitch_origin(pitch.first)
    , m_pitch_direction(ray_unit(pitch))
    , m_toffset(toffset)
    , m_drift_velocity(drift_velocity)
    , m_tbuffer(tbuffer)
    , m_DL(DL)
    , m_DT(DT)
    , m_hist(new BufferedHistogram2D(tick, ray_length(pitch), tstart, 0))
    , m_diff(new Diffuser(*m_hist, nsigma))
    , m_high_water_tau(tstart)

{
    // The histogram's X direction is in units of "proper time" (tau)
    // which is defined as the absolute time it is when a point event
    // drifts from where/when it occurred to the plane.  To handle
    // diffusion, the point deposition is placed at a proper time in
    // the future by the amount of buffering requested.

    // cerr << "PlaneDuctor: BufferedHistogram2D: "
    // 	 << "xmin=" << m_hist->xmin() << " xbinsize=" << m_hist->xbinsize() << ", "
    // 	 << "ymin=" << m_hist->ymin() << " ybinsize=" << m_hist->ybinsize() << endl;

}
PlaneDuctor::~PlaneDuctor()
{
    delete m_diff;
    delete m_hist;
}

double PlaneDuctor::proper_tau(double event_time, double event_xloc)
{
    return event_time + (event_xloc - m_pitch_origin.x()) / m_drift_velocity;
}
double PlaneDuctor::pitch_dist(const Point& p)
{
    Vector to_p = p - m_pitch_origin;
    return m_pitch_direction.dot(to_p);
}


void PlaneDuctor::buffer()
{
    int count = 0;
    double charge_buffered = 0;

    // consume depositions until tbuffer amount of time past "now".
    while (m_high_water_tau < clock() + m_tbuffer) {
	IDepo::pointer depo = *m_input();
	if (!depo) {
	    return;
	}

	++count;
	charge_buffered += depo->charge();

	// the time it will show up
	double tau = proper_tau(depo->time(), depo->pos().x());
	// allow for any arbitrary offset 
	tau += m_toffset;

	// how long the center will have drifted for calculating diffusion sigmas.
	IDepo::pointer initial_depo = *depo_chain(depo).rbegin();
	double drift_time = proper_tau(0, initial_depo->pos().x());
	double tmpcm2 = 2*m_DL*drift_time/units::centimeter2;
	double sigmaL = sqrt(tmpcm2)*units::centimeter / m_drift_velocity;
	double sigmaT = sqrt(2*m_DT*drift_time/units::centimeter2)*units::centimeter2;
	double trans = pitch_dist(depo->pos());
	m_diff->diffuse(tau, trans, sigmaL, sigmaT, depo->charge());

	if (tau > m_high_water_tau) {
	    m_high_water_tau = tau;
	}
    }
}


class PDPS : public IPlaneSlice {
    WirePlaneId m_wpid;
    double m_time;
    ChargeGrouping m_charge;

public:
    PDPS(WirePlaneId wpid, double time, const std::vector<double>& charge)
	: m_wpid(wpid), m_time(time)
    {
	if (!charge.size()) { return; }
	//cerr << "PDPS(" << wpid << "," << time << "," << charge.size() << ")" << endl;

	int group_ind = -1;
	std::vector<double> group;
	for (int ind=0; ind < charge.size(); ++ind) {
	    double q = charge[ind];
	    if (group_ind < 0) { // we were out of a group
		if (q > 0.0) {	// we just entered a group
		    group.push_back(q);
		    group_ind = ind;
		    continue;
		}
		continue;
	    }

	    // we were in a group		
	    if (q > 0.0) {	// we stay in the group
		group.push_back(q);
		continue;
	    }

	    // we just left a group, save.
	    m_charge[group_ind] = group;
	    group.clear();
	    group_ind = -1;
	}
	if (group.size()) {	// last group
	    m_charge[group_ind] = group;
	}
    }
	

    virtual WirePlaneId planeid() const { return m_wpid; }

    virtual double time() const { return m_time; }

    virtual ChargeGrouping charge_groups() const { return m_charge; }
};


IPlaneSlice::pointer PlaneDuctor::operator()()
{
    if (!buffer_size()) {
	return nullptr;
    }
    double t = clock();		// call first
    return IPlaneSlice::pointer(new PDPS(m_wpid, t, latch()));

}

double PlaneDuctor::clock()
{
    return m_hist->xmin();
}

std::vector<double> PlaneDuctor::latch()
{
    buffer();
    return m_hist->popx();    
}

int PlaneDuctor::buffer_size()
{
    buffer();
    return m_hist->size();
}
