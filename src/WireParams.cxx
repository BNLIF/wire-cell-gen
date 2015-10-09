#include "WireCellGen/WireParams.h"

#include "WireCellUtil/NamedFactory.h"

#include <iostream>
using namespace WireCell;
using namespace std;

WIRECELL_NAMEDFACTORY_BEGIN(WireParams)
WIRECELL_NAMEDFACTORY_INTERFACE(WireParams, IConfigurable);
WIRECELL_NAMEDFACTORY_INTERFACE(WireParams, IWireParameters);
WIRECELL_NAMEDFACTORY_END(WireParams)




//static int n_wireparams = 0;

WireParams::WireParams()
{
    //++n_wireparams;
    //cerr << "WireParams(" << n_wireparams << ")" << endl;
    this->set();
}
WireParams::~WireParams()
{
    //--n_wireparams;
    //cerr << "~WireParams(" << n_wireparams << ")" << endl;
}

Configuration WireParams::default_configuration() const
{
    std::string json = R"(
{
"center_mm":{"x":0.0, "y":0.0, "z":0.0},
"size_mm":{"x":10.0, "y":1000.0, "z":1000.0},
"pitch_mm":{"u":3.0, "v":3.0, "w":3.0},
"angle_deg":{"u":60.0, "v":-60.0, "w":0.0},
"offset_mm":{"u":0.0, "v":0.0, "w":0.0},
"plane_mm":{"u":3.0, "v":2.0, "w":1.0}
}
)";
    return configuration_loads(json, "json");
}

void WireParams::configure(const Configuration& cfg)
{
    // The local origin about which all else is measured.
    double cx = cfg.get<double>("center_mm.x")*units::mm;
    double cy = cfg.get<double>("center_mm.y")*units::mm;
    double cz = cfg.get<double>("center_mm.z")*units::mm;
    const Point center(cx,cy,cz);
    
    // The full width sizes
    double dx = cfg.get<double>("size_mm.x")*units::mm;
    double dy = cfg.get<double>("size_mm.y")*units::mm;
    double dz = cfg.get<double>("size_mm.z")*units::mm;
    const Point deltabb(dx,dy,dz);
    const Point bbmax = center + 0.5*deltabb;
    const Point bbmin = center - 0.5*deltabb;

    // The angles of the wires w.r.t. the positive Y axis
    double angU = cfg.get<double>("angle_deg.u")*units::degree;
    double angV = cfg.get<double>("angle_deg.v")*units::degree;
    double angW = cfg.get<double>("angle_deg.w")*units::degree;

    const Vector wU(0, std::cos(angU), std::sin(angU));
    const Vector wV(0, std::cos(angV), std::sin(angV));
    const Vector wW(0, std::cos(angW), std::sin(angW));

    // The pitch magnitudes.
    double pitU = cfg.get<double>("pitch_mm.u")*units::mm;
    double pitV = cfg.get<double>("pitch_mm.v")*units::mm;
    double pitW = cfg.get<double>("pitch_mm.w")*units::mm;

    // Pitch vectors
    const Vector xaxis(1,0,0);
    const Vector pU = pitU*xaxis.cross(wU);
    const Vector pV = pitV*xaxis.cross(wV);
    const Vector pW = pitW*xaxis.cross(wW);

    // The offset in the pitch direction from the center to a wire
    double offU = cfg.get<double>("offset_mm.u")*units::mm;
    double offV = cfg.get<double>("offset_mm.v")*units::mm;
    double offW = cfg.get<double>("offset_mm.w")*units::mm;

    Point oU = center + pU.norm() * offU;
    Point oV = center + pV.norm() * offV;
    Point oW = center + pW.norm() * offW;

    // Force X location of plane along the X axis.
    oU.x(cfg.get<double>("plane_mm.u")*units::mm);
    oV.x(cfg.get<double>("plane_mm.v")*units::mm);
    oW.x(cfg.get<double>("plane_mm.w")*units::mm);


    const Ray bounds(bbmin, bbmax);
    const Ray U(oU, oU+pU), V(oV, oV+pV), W(oW, oW+pW);
    this->set(bounds, U, V, W);
}

void WireParams::set(const Ray& bounds, 
		     const Ray& U, const Ray& V, const Ray& W)
{
    // save for posterity
    m_bounds = bounds;
    m_pitchU = U;
    m_pitchV = V;
    m_pitchW = W;
}

void WireParams::set(double dx, double dy, double dz,
		     double pitch, double angle)
{
    // The local origin about which all else is measured.
    const Point center;
    const Point deltabb(dx,dy,dz);
    const Point bbmax = center + 0.5*deltabb;
    const Point bbmin = center - 0.5*deltabb;

    double angU = +angle;
    double angV = -angle;
    double angW = 0.0;

    // Wire directions
    const Vector wU(0, std::cos(angU), std::sin(angU));
    const Vector wV(0, std::cos(angV), std::sin(angV));
    const Vector wW(0,              1,              0);

    const Vector xaxis(1,0,0);
    const Vector pU = pitch*xaxis.cross(wU);
    const Vector pV = pitch*xaxis.cross(wV);
    const Vector pW = pitch*xaxis.cross(wW);

    // cerr << "pU=" << pU << endl;
    // cerr << "pV=" << pV << endl;
    // cerr << "pW=" << pW << endl;

    const Point oU(0.375*dx, 0.0, 0.0);
    const Point oV(0.250*dx, 0.0, 0.0);
    const Point oW(0.125*dx, 0.0, 0.0);

    const Ray bounds(bbmin, bbmax);
    const Ray U(oU, oU+pU);
    const Ray V(oV, oV+pV);
    const Ray W(oW, oW+pW);
    this->set(bounds, U, V, W);

}

const Ray& WireParams::bounds() const { return m_bounds; }
const Ray& WireParams::pitchU() const { return m_pitchU; }
const Ray& WireParams::pitchV() const { return m_pitchV; }
const Ray& WireParams::pitchW() const { return m_pitchW; }

