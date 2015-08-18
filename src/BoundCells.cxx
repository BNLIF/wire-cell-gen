#include "WireCellIface/IWireSelectors.h"

#include "WireCellNav/BoundCells.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/BoundingBox.h"
#include "WireCellUtil/Units.h"

#include <boost/range.hpp>
#include <iterator>

#include <iostream>		// debug
using namespace std;

using namespace WireCell;

WIRECELL_NAMEDFACTORY(BoundCells);
WIRECELL_NAMEDFACTORY_ASSOCIATE(BoundCells, IWireSink);
WIRECELL_NAMEDFACTORY_ASSOCIATE(BoundCells, ICellSequence);


struct AngularSort {
    Point center;
    AngularSort(const Point& center) : center(center) { }

    bool operator()(const Point& lhs, const Point& rhs) {
	const Vector a = (lhs-center).norm();
	const Vector b = (rhs-center).norm();
	double theta_a = atan2(a.y(), a.z());
	double theta_b = atan2(b.y(), b.z());
	return theta_a < theta_b;
    }

};

// This is the actual implementation of the cell we provide.  It is
// buried in this implementation for a reason.  It's not a publicly
// accessible class.
namespace WireCell {
    class BoundCell : public ICell {
    
	int m_ident;
	PointVector m_corners;	// this is likely a source of a lot of memory usage
	const IWireVector m_wires;

    public:
	BoundCell(int id, const PointVector& pcell, const IWireVector& wires)
	    : m_ident(id), m_corners(pcell), m_wires(wires) {
	    std::sort(m_corners.begin(), m_corners.end(), AngularSort(center()));
	}
	virtual ~BoundCell() {
	}	// do not further inherit.

	virtual int ident() const { return m_ident; }

	virtual double area() const { return 0.0; /*fixme*/}

	virtual WireCell::Point center() const {
	    /// fixme

	    WireCell::Point center;
	    size_t npos = m_corners.size();
	    for (int ind=0; ind<npos; ++ind) {
		center = center + m_corners[ind];
	    }
	    double norm = 1.0/npos;
	    return  norm * center;
	}

	virtual WireCell::PointVector corners() const {
	    return m_corners;
	}

	virtual WireCell::IWireVector wires() const {
	    return m_wires;
	}
    };
} // namespace WireCell;



//
// fixme: some of this code is more generically available in WireSummary
// 


// Return a Ray going from the center point of wires[0] to a point on
// wire[1] and perpendicular to both.
static Ray pitch2(const std::vector<IWire::pointer>& wires)
{
    // Use two consecutive wires from the center to determine the pitch. 
    int ind = wires.size()/2;
    IWire::pointer one = wires[ind];
    IWire::pointer two = wires[ind+1];
    const Ray p = ray_pitch(one->ray(), two->ray());

    // Move the pitch to the first wire
    IWire::pointer zero = wires[0];
    const Vector center0 = zero->center();
    return Ray(center0, center0 + ray_vector(p));
}

static Vector axis(const std::vector<IWire::pointer>& wires)
{
    int ind = wires.size()/2;
    return ray_vector(wires[ind]->ray()).norm();
}

struct WireByIndex {
    bool operator() (IWire::pointer lhs, IWire::pointer rhs) const {
	if (lhs->plane() == rhs->plane()) {
	    return lhs->index() < rhs->index();
	}
	return lhs->plane() < lhs->plane();
    }
};

Vector origin_cross(const std::vector<IWire::pointer>& ones,
		    const std::vector<IWire::pointer>& twos)
{
    // double A1 = r1.second.y()-r1.first.y();
    // double B1 = r1.second.z()-r1.first.z();
    // double C1 = A1*r1.first.z() + B1*r.first.y();

    // double A2 = r2.second.y()-r2.first.y();
    // double B2 = r2.second.z()-r2.first.z();
    // double C2 = A2*r2.first.z() + B2*r.first.y();

    const Ray& ray1 = ones[0]->ray();
    const Ray& ray2 = twos[0]->ray();
    const Ray cross = ray_pitch(ray1, ray2);

    // cerr << "Ray1 = " << ray1.first << " --> " << ray1.second << endl;
    // cerr << "Ray2 = " << ray2.first << " --> " << ray2.second << endl;
    // cerr << "Cross: " << cross.first << " --> " << cross.second << endl;
    return cross.first;
}


bool is_point_inside_w_lane(const Point& point, const double& w_lane_center, const double& w_lane_half_width)
{
    return fabs(w_lane_center - point.z()) < w_lane_half_width;
}

void BoundCells::sink(const IWire::iterator_range& wr)
{
    m_store.clear();

    /* This was originally Xin's cell algorithm but only the concept
       remains.

       The idea is based on "crossing points" and "wire half-way
       lines".  Crossing points are the points in the Y-Z plane where
       lines which are parallel to that plane cross.  Wire half-way
       lines are the lines running parallel to a given wire and within
       the wire plane which are 1/2 the of a pitch distance away.

       The algorithm visits each U and V wire pair and determines
       their crossing point.  If this point is outside the bounding
       box of the wire planes it is discarded.  Otherwise, the four
       crossing points of each wire half-way lines are found.  Any
       that are inside the bounding box are saved as candidate cell
       corner points.

       Then, for each W wire, these four points are checked and any
       which are withing 1/2 pitch of the wire are kept as cell corner
       points.

       In addition the wire half-way crossing points for the W wire
       and the U wire are found and checked to be within the wire
       half-way distance with the current V wires.  This is repeated
       with U and V swapped.  Surviving points are added to the list
       of cell corner points.

    */
    
    vector<IWire::pointer> u_wires, v_wires, w_wires;
    copy_if(boost::begin(wr), boost::end(wr), back_inserter(u_wires), select_u_wires);
    copy_if(boost::begin(wr), boost::end(wr), back_inserter(v_wires), select_v_wires);
    copy_if(boost::begin(wr), boost::end(wr), back_inserter(w_wires), select_w_wires);

    WireByIndex wbi_sorter;
    std::sort(u_wires.begin(), u_wires.end(), wbi_sorter);
    std::sort(v_wires.begin(), v_wires.end(), wbi_sorter);
    std::sort(w_wires.begin(), w_wires.end(), wbi_sorter);

    // cerr << "#wires: "
    // 	 << u_wires.size() << ", "
    // 	 << v_wires.size() << ", "
    // 	 << w_wires.size() <<endl;

    const Ray pitch_u_ray = pitch2(u_wires);
    const Ray pitch_v_ray = pitch2(v_wires);
    const Ray pitch_w_ray = pitch2(w_wires);

    const double pitch_u = ray_length(pitch_u_ray);
    const double pitch_v = ray_length(pitch_v_ray);
    const double pitch_w = ray_length(pitch_w_ray);

    // cerr << "pitch vectors:\n"
    // 	 << "\tU: " << pitch_u_ray.first << "-->" << pitch_u_ray.second << " |" << pitch_u << "|\n"
    // 	 << "\tV: " << pitch_v_ray.first << "-->" << pitch_v_ray.second << " |" << pitch_v << "|\n"
    // 	 << "\tW: " << pitch_w_ray.first << "-->" << pitch_w_ray.second << " |" << pitch_w << "|\n";

    
    const Vector axis_u = axis(u_wires);
    const Vector axis_v = axis(v_wires);
    const Vector axis_w = axis(w_wires);
    // cerr << "u_wires[0] = " << u_wires[0]->ray().first << " --> " << u_wires[0]->ray().second << endl;
    // cerr << "v_wires[0] = " << v_wires[0]->ray().first << " --> " << v_wires[0]->ray().second << endl;
    // cerr << "w_wires[0] = " << w_wires[0]->ray().first << " --> " << w_wires[0]->ray().second << endl;
    // cerr << "axis_u = " << axis_u << " u0=" << ray_vector(u_wires[0]->ray()) << endl;
    // cerr << "axis_v = " << axis_v << " v0=" << ray_vector(v_wires[0]->ray()) << endl;
    // cerr << "axis_w = " << axis_w << " w0=" << ray_vector(w_wires[0]->ray()) << endl;

    const Vector jump_u = axis_u * (pitch_u / sin(2.0 * acos(axis_w.dot(axis_u))));
    const Vector jump_v = axis_v * (pitch_v / sin(2.0 * acos(axis_w.dot(axis_v))));
    // cerr << "jump_u = " << jump_u << endl;
    // cerr << "jump_v = " << jump_v << endl;

    // jumps from a wire crossing point to the four half-wire crossing
    // points, in cyclical order.
    const Vector cross_jump_uv[4] = {
	+ 0.5*jump_u + 0.5*jump_v, // +Y/Z=0
	+ 0.5*jump_u - 0.5*jump_v, // Y=0/+Z
	- 0.5*jump_u - 0.5*jump_v, // -Y/Z=0
	- 0.5*jump_u + 0.5*jump_v  // Y=0/-Z
    };
    // cerr << "cross jump:" << endl;
    // for(int cjind=0; cjind<4; ++cjind) {
    // 	cerr << "\t" << cjind << ": " << cross_jump_uv[cjind] << endl;
    // }

    const double y_buffer = 0.0; //cross_jump_uv[0].magnitude();

    // fixme: generate noisy wires and test this value
    const double tolerance = 0.0*units::mm;

    BoundingBox bb;
    for (auto wit = boost::begin(wr); wit != boost::end(wr); ++wit) {
	bb((*wit)->ray());
    }
    //cerr << "Cell bounding box: " << bb.bounds.first << " -->  " << bb.bounds.second  << endl;
    
    // pack it up and send it out
    std::vector<IWire::pointer> uvw_wires(3);

    const Vector origin_uv = origin_cross(u_wires, v_wires);

    const double first_w_z = w_wires[0]->ray().first.z();
    const double last_w_z = w_wires[w_wires.size()-1]->ray().first.z();    
    const double w_lane_half_width = 0.5*(pitch_w + tolerance);

    for (int u_ind = 0; u_ind < u_wires.size(); ++u_ind) {
	IWire::pointer u_wire = u_wires[u_ind];
	uvw_wires[0] = u_wire;

	for (int v_ind = 0; v_ind < v_wires.size(); ++v_ind) {
	    IWire::pointer v_wire = v_wires[v_ind];
	    uvw_wires[1] = v_wire;

	    // Are these jumps a source of precision loss?
	    Vector cross_uv = origin_uv - double(u_ind)*jump_v + double(v_ind)*jump_u;

	    if (cross_uv.z() < bb.bounds().first.z() - 0.5*pitch_w ||
		cross_uv.z() > bb.bounds().second.z() + 0.5*pitch_w ||
		cross_uv.y() < bb.bounds().first.y() - y_buffer ||
		cross_uv.y() > bb.bounds().second.y() + y_buffer)
	    {
		// cerr << "outside: " << u_ind << "/" << v_ind << " " << cross_uv
		//      << " U:" << u_wire.ray().first << " --> " << u_wire.ray().second << " "
		//      << " V:" << v_wire.ray().first << " --> " << v_wire.ray().second << " "
		//      << endl;
		continue;
	    }

            std::vector<Vector> puv(4);
	    for (int cjind=0; cjind<4; ++cjind) {
		puv[cjind] = cross_uv + cross_jump_uv[cjind];
	    }

	    // Prefilter W wires, only bother checking those nearby.
	    
	    int max_w_ind = min(int((puv[1].z() - first_w_z + 0.5*pitch_w)/pitch_w), int(w_wires.size()-1));
	    int min_w_ind = max(int((puv[3].z() - first_w_z - 0.5*pitch_w)/pitch_w), 0);

	    // cerr << "\nChecking W: " << u_ind << "/" << v_ind << " w_ind(max/min)=" << min_w_ind << "/" << max_w_ind
	    // 	 << " crossing=" << cross_uv
	    // 	 << endl;
	    for (int w_ind = min_w_ind; w_ind <= max_w_ind; ++w_ind) {
		IWire::pointer w_wire = w_wires[w_ind];
		uvw_wires[2] = w_wire;

		double target_w_z = w_ind * pitch_w + first_w_z;

		//cerr << "\n" << u_ind << "/" << v_ind << "/" << w_ind << " w=" << target_w_z << " " << cross_uv << endl;

                WireCell::PointVector pcell;

		bool inside_uv[4];
		// check which of the four U/V crossings are near this W wire.
                for (int cjind=0; cjind<4; ++cjind) {
		    bool is_inlane = is_point_inside_w_lane(puv[cjind], target_w_z, w_lane_half_width);
		    inside_uv[cjind] = is_inlane;
		    if (inside_uv[cjind]) {
			pcell.push_back(puv[cjind]);
                    }
                }

                if (!pcell.size()) {
                    continue;	
                }

		// go around the cross finding edges to break
		vector<std::pair<int,int> > to_break;
		for (int cjind=0; cjind<4; ++cjind) {
		    int other_ind = (cjind+1)%4;
		    if (inside_uv[cjind] && inside_uv[other_ind]) {
			continue; // both inside
		    }
		    if (!(inside_uv[cjind] || inside_uv[other_ind])) {
			continue; // both outside
		    }
		    if (inside_uv[cjind]) { // order inside-first
			to_break.push_back(std::pair<int,int>(cjind, other_ind));
		    }
		    else {
			to_break.push_back(std::pair<int,int>(other_ind, cjind));
		    }
		}

		for (int tbind = 0; tbind < to_break.size(); ++tbind) {
		    const Vector& p_in = puv[to_break[tbind].first];
		    const Vector& p_out = puv[to_break[tbind].second];

		    double break_z = 0.0;
		    if (p_out.z() > target_w_z) {
			break_z = target_w_z + 0.5*pitch_w - p_in.z();
		    }
		    else {
			break_z = target_w_z - 0.5*pitch_w - p_in.z();
		    }

		    Vector diff = p_out - p_in;
		    double rel = break_z / diff.z();
		    Vector corner = p_in + rel*diff;
		    // cerr << tbind << " breaking: " << to_break[tbind].first << "/" << to_break[tbind].second
		    //  	 << " = " << p_in << " --> " << p_out << " = " << corner
		    // 	 << " break_z=" << break_z << " rel=" << rel
		    // 	 << endl;
		    pcell.push_back(corner);
		}
			
		// result
		m_store.push_back(ICell::pointer(new BoundCell(m_store.size(), pcell, uvw_wires)));

            } // W wires
        } // v wires
    } // u wires
} // generate()



ICellSequence::cell_iterator BoundCells::cells_begin() 
{
    return cell_iterator(adapt(m_store.begin()));
}


ICellSequence::cell_iterator BoundCells::cells_end()
{
    return cell_iterator(adapt(m_store.end()));
}


BoundCells::BoundCells()
{
}

BoundCells::~BoundCells()
{
}

