#include "WireCellGen/AnodePlane.h"
#include "WireCellGen/AnodeFace.h"
#include "WireCellGen/WirePlane.h"

#include "WireCellUtil/WireSchema.h"
#include "WireCellUtil/BoundingBox.h"
#include "WireCellUtil/Testing.h"

#include "WireCellIface/SimpleWire.h"
#include "WireCellUtil/NamedFactory.h"


#include <string>

WIRECELL_FACTORY(AnodePlane, WireCell::Gen::AnodePlane, WireCell::IAnodePlane, WireCell::IConfigurable);


using namespace WireCell;
using namespace std;

Gen::AnodePlane::AnodePlane()
    : m_ident(0)
{
}



const double default_gain = 14.0*units::mV/units::fC;
const double default_shaping = 2.0*units::us;
const double default_postgain = 1.0;
const double default_readout = 5.0*units::ms;
const double default_tick = 0.5*units::us;


WireCell::Configuration Gen::AnodePlane::default_configuration() const
{
    Configuration cfg;

    /// Path to (possibly compressed) JSON file holding wire geometry
    /// which follows wirecell.util.wire.schema.  
    put(cfg, "wires", "");

    /// This number is used to take from the wire file the anode
    put(cfg, "ident", 0);

    /// Path to (possibly compressed) JSON file holding field responses
    put(cfg, "fields", "");

    /// Electronics gain
    put(cfg, "gain", default_gain);

    /// Post FEE relative gain
    put(cfg, "postgain", default_postgain);

    /// Electronics shaping time 
    put(cfg, "shaping", default_shaping);

    /// The period over which to latch responses
    put(cfg, "readout_time", default_readout);

    /// The sample period
    put(cfg, "tick", default_tick);

    return cfg;
}


void Gen::AnodePlane::configure(const WireCell::Configuration& cfg)
{
    double gain = get<double>(cfg, "gain", default_gain);
    double postgain = get<double>(cfg, "postgain", default_postgain);
    double shaping = get<double>(cfg, "shaping", default_shaping);
    double readout = get<double>(cfg, "readout_time", default_readout);
    double tick = get<double>(cfg, "tick", default_tick);
    const int nticks = readout/tick;

    cerr << "Gen::AnodePlane: "
         << "gain=" << gain/(units::mV/units::fC) << " mV/fC, "
         << "peaking=" << shaping/units::us << " us, "
         << "postgain=" << postgain << ", "
         << "readout=" << readout/units::ms << " ms, "
         << "tick=" << tick/units::us << " us "
         << endl;

    const double t0 = 0.0; // fixme: t0 not actually used, PIR interface needs reduction
    const Binning tbins(nticks, t0, t0+readout);

    m_faces.clear();
    m_ident = get<int>(cfg, "ident", 0);

    const string frfname = get<string>(cfg, "fields");
    if (frfname.empty()) {
        cerr << "Gen::AnodePlane::configure() must have a field response file name\n";
    }
    Assert(!frfname.empty());
    m_fr = Response::Schema::load(frfname.c_str());
    Assert(m_fr.speed > 0);


    const string wfname = get<string>(cfg, "wires");
    if (wfname.empty()) {
        cerr << "Gen::AnodePlane::configure() must have a wire geometry file name\n";
    }
    Assert(!wfname.empty());
    WireSchema::Store store = WireSchema::load(wfname.c_str());
    Assert(!store.anodes.empty());

    const Vector Xaxis(1.0,0,0);



    for (int ianode=0; ianode<store.anodes.size(); ++ianode) {
        const int other_ident = store.anodes[ianode].ident;
        if (other_ident != m_ident) {
            cerr << "Gen::AnodePlane: my ident is " << m_ident
                 << " not " << other_ident << " (" << ianode << "/" << store.anodes.size() << ")\n";
            continue;
        }

        auto& s_anode = store.anodes[ianode];
    
        const int nfaces = s_anode.faces.size();
        m_faces.resize(nfaces);
        cerr << "Gen::AnodePlane: found my ident " << other_ident << " with " << nfaces << " faces\n";

        for (int iface=0; iface<nfaces; ++iface) {
            auto& s_face = store.faces[s_anode.faces[iface]];
            const int nplanes = s_face.planes.size();


            IWirePlane::vector planes(nplanes);
            for (int iplane=0; iplane<nplanes; ++iplane) {
                auto& s_plane = store.planes[s_face.planes[iplane]];

                // fixme: the WireSchema data is SUPPOSED to encode
                // the ident as packed data but first version stores
                // just an index.  So we play ball for now.

                // WirePlaneId wire_plane_id(s_plane.ident); // dubious
                WirePlaneId wire_plane_id(iplane2layer[s_plane.ident]);
                cerr << wire_plane_id << endl;
                Assert(wire_plane_id.index() >= 0); 

                Vector total_wire;
                const int nwires = s_plane.wires.size();
                IWire::vector wires(nwires);
                BoundingBox bb;

                for (int iwire=0; iwire<nwires; ++iwire) {
                    auto& s_wire = store.wires[s_plane.wires[iwire]];
                    Ray ray(s_wire.tail, s_wire.head);
                    wires[iwire] = make_shared<SimpleWire>(wire_plane_id, s_wire.ident,
                                                           iwire, s_wire.channel,
                                                           ray, s_wire.segment);
                    total_wire += ray_vector(ray);
                    bb(ray);
                    m_c2wpid[s_wire.channel] = wire_plane_id.ident();
                    // if (iwire == 0) {
                    //     cerr << "nwires=" << nwires << " ch=" << s_wire.channel
                    //      << ", wpid=" << wire_plane_id
                    //      << " index=" << wire_plane_id.index()
                    //      << " ident=" << wire_plane_id.ident()
                    //      << endl;
                    // }
                } // wire

                const Vector wire_dir = total_wire.norm();
                const Vector pitch_dir = Xaxis.cross(wire_dir);
                auto pr = m_fr.plane(s_plane.ident);
                Response::Schema::lie(*pr, pitch_dir, wire_dir);

                // Calculate transverse center of wire plane and
                // pushed to a point longitudinally where the field
                // response starts.
                Ray bb_ray = bb.bounds();
                Vector field_origin = 0.5*(bb_ray.first + bb_ray.second);
                const double to_response_plane = m_fr.origin - pr->location;
                field_origin[0] += to_response_plane;

                const double wire_pitch = pr->pitch;
                const double impact_pitch = pr->paths[1].pitchpos - pr->paths[0].pitchpos;
                const int nregion_bins = round(wire_pitch/impact_pitch);
                const double halfwireextent = wire_pitch * (nwires/2);

                Pimpos* pimpos = new Pimpos(nwires, -halfwireextent, halfwireextent,
                                            wire_dir, pitch_dir,
                                            field_origin, nregion_bins);
                
                PlaneImpactResponse* pir = new PlaneImpactResponse(m_fr, s_plane.ident, tbins,
                                                                   gain, shaping, postgain);

                planes[iplane] = make_shared<WirePlane>(s_plane.ident, wires, pimpos, pir);

            } // plane

            m_faces[iface] = make_shared<AnodeFace>(s_face.ident, planes);

        } // face

        break; // this class only handles single-anode plane detectors
    }          // anode
}


IAnodeFace::pointer Gen::AnodePlane::face(int ident) const
{
    for (auto ptr : m_faces) {
        if (ptr->ident() == ident) {
            return ptr;
        }
    }
    return nullptr;
}

 
WirePlaneId Gen::AnodePlane::resolve(int channel) const
{
    const WirePlaneId bogus(0xFFFFFFFF);

    auto got = m_c2wpid.find(channel);
    if (got == m_c2wpid.end()) {
        return bogus;
    }
    return WirePlaneId(got->second);
}


