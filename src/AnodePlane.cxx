#include "WireCellGen/AnodePlane.h"
#include "WireCellGen/AnodeFace.h"
#include "WireCellGen/WirePlane.h"

#include "WireCellUtil/WireSchema.h"
#include "WireCellUtil/BoundingBox.h"
#include "WireCellUtil/Exceptions.h"

#include "WireCellIface/IFieldResponse.h"
#include "WireCellIface/IWireSchema.h"
#include "WireCellIface/SimpleWire.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"

#include <string>

WIRECELL_FACTORY(AnodePlane, WireCell::Gen::AnodePlane, WireCell::IAnodePlane, WireCell::IConfigurable);


using namespace WireCell;
using namespace std;
using WireCell::String::format;


Gen::AnodePlane::AnodePlane()
    : m_ident(0)
{
}



const int default_nimpacts = 10;
const double default_response_plane = 10*units::cm;

WireCell::Configuration Gen::AnodePlane::default_configuration() const
{
    Configuration cfg;

    /// This number is used to take from the wire file the anode
    put(cfg, "ident", 0);

    /// Name of a IWireSchema component.
    // note: this used to be a "wires" parameter directly specifying a filename.
    put(cfg, "wire_schema", "");

    // Number of impact positions per wire pitch.  This should likely
    // match exactly what the field response functions use.
    put(cfg, "nimpacts", default_nimpacts);

    // The AnodePlane has to two faces, one of which may be "null".
    // Face 0 or "front" is the one that is toward the positive X-axis
    // (wire coordinate system).  A face consists of wires and some
    // bounds on the X-coordinate in the form of three planes:
    //
    // - response :: The response plane demarks the location where the
    // complex fields near the wires are considered sufficiently
    // regular.
    //
    // - anode and cathode :: These two planes bracket the region in X
    // - for the volume associated with the face.  The transverse
    // - range is determined by the wires.  If anode is not given then
    // - response will be used instead.  Along with wires, these
    // - determine the sensitive bounding box of the AnodeFaces.
    //
    // eg: faces: [
    //       {response: 10*wc.cm - 6*wc.mm, 2.5604*wc.m},
    //       null,
    //     ]

    cfg["faces"][0] = Json::nullValue;
    cfg["faces"][1] = Json::nullValue;

    return cfg;
}


void Gen::AnodePlane::configure(const WireCell::Configuration& cfg)
{
    m_faces.clear();
    m_ident = get<int>(cfg, "ident", 0);

    // check for obsolete config params:
    if (!cfg["wires"].isNull()) {
        cerr << "\nWarning: your configuration is obsolete.\n"
             << "Use \"wire_store\" to name components instead of directly giving file names\n"
             << "This job will likely throw an exception next\n\n";
    }

    // AnodePlane used to have the mistake of tight coupling with
    // field response functions.  
    if (!cfg["fields"].isNull() or !cfg["field_response"].isNull()) {
        cerr << "\nWarning: AnodePlane does not require any field response functions.\n";
    }

    auto jfaces = cfg["faces"];
    if (jfaces.isNull() or jfaces.empty() or (jfaces[0].isNull() and jfaces[1].isNull())) {
        cerr << "At least two faces need to be defined, got:\n";
        cerr << jfaces << endl;
        THROW(ValueError() << errmsg{"AnodePlane: error in configuration"});        
    }


    const int nimpacts = get<int>(cfg, "nimpacts", default_nimpacts);


    // get wire schema
    const string ws_name = get<string>(cfg, "wire_schema");
    if (ws_name.empty()) {
        THROW(ValueError() << errmsg{"\"wire_schema\" parameter must specify an IWireSchema component"});
    }
    auto iws = Factory::find_tn<IWireSchema>(ws_name); // throws if not found
    const WireSchema::Store& ws_store = iws->wire_schema_store();

    // keep track which channels we know about in this anode.
    m_channels.clear();

    const WireSchema::Anode& ws_anode = ws_store.anode(m_ident);

    std::vector<WireSchema::Face> ws_faces = ws_store.faces(ws_anode);
    const size_t nfaces = ws_faces.size();
    
    m_faces.resize(nfaces);
    for (size_t iface=0; iface<nfaces; ++iface) { // note, WireSchema requires front/back face ordering in an anode
        const auto& ws_face = ws_faces[iface];
        std::vector<WireSchema::Plane> ws_planes = ws_store.planes(ws_face);
        const size_t nplanes = ws_planes.size();
        
        // location of imaginary boundary planes
        auto jface = jfaces[(int)iface];
        const double response_x = jface["response"].asDouble();
        const double anode_x = get(jface, "anode", response_x);
        const double cathode_x = jface["cathode"].asDouble();

        IWirePlane::vector planes(nplanes);
        for (size_t iplane=0; iplane<nplanes; ++iplane) { // note, WireSchema requires U/V/W plane ordering in a face.
            const auto& ws_plane = ws_planes[iplane];

            WirePlaneId wire_plane_id(iplane2layer[iplane], iface, m_ident);
            if (wire_plane_id.index() < 0) {
                THROW(ValueError() << errmsg{format("bad wire plane id: %d", wire_plane_id.ident())});
            }

            // (wire,pitch) directions
            auto wire_pitch_dirs = ws_store.wire_pitch(ws_plane);
            auto ecks_dir = wire_pitch_dirs.first.cross(wire_pitch_dirs.second);

            std::vector<int> plane_chans = ws_store.channels(ws_plane);
            m_channels.insert(m_channels.end(), plane_chans.begin(), plane_chans.end());

            std::vector<WireSchema::Wire> ws_wires = ws_store.wires(ws_plane);
            const size_t nwires = ws_wires.size();
            IWire::vector wires(nwires);

            for (size_t iwire=0; iwire<nwires; ++iwire) { // note, WireSchema requires wire-in-plane ordering
                const auto& ws_wire = ws_wires[iwire];
                const int chanid = plane_chans[iwire];
                    
                Ray ray(ws_wire.tail, ws_wire.head);
                auto iwireptr = make_shared<SimpleWire>(wire_plane_id, ws_wire.ident,
                                                        iwire, chanid, ray,
                                                        ws_wire.segment);
                wires[iwire] = iwireptr;
                m_c2wires[chanid].push_back(iwireptr);
                m_c2wpid[chanid] = wire_plane_id.ident();
            } // wire

            const BoundingBox bb = ws_store.bounding_box(ws_plane);
            const Ray bb_ray = bb.bounds();
            const Vector plane_center = 0.5*(bb_ray.first + bb_ray.second);

            const double pitchmin = wire_pitch_dirs.second.dot(wires[0]->center() - plane_center);
            const double pitchmax = wire_pitch_dirs.second.dot(wires[nwires-1]->center() - plane_center);
            const Vector pimpos_origin(response_x, plane_center.y(), plane_center.z());

            cerr << "AnodePlane: face:" << iface <<", plane:"<<iplane
                 << " origin:" << pimpos_origin/units::mm
                 << "mm\n";

            Pimpos* pimpos = new Pimpos(nwires, pitchmin, pitchmax,
                                        wire_pitch_dirs.first, wire_pitch_dirs.second,
                                        pimpos_origin, nimpacts);

            planes[iplane] = make_shared<WirePlane>(ws_plane.ident, wires, pimpos);


            // Last iteration, use W plane to define volume
            if (iplane == nplanes-1) { 
                const double mean_pitch = (pitchmax - pitchmin) / (nwires-1);

                auto v1 = bb_ray.first;
                auto v2 = bb_ray.second;
                // Enlarge to anode/cathode planes in X and by 1/2 pitch in Z.
                Point p1(  anode_x, v1.y(), std::min(v1.z(), v2.z()) - 0.5*mean_pitch);
                Point p2(cathode_x, v2.y(), std::max(v1.z(), v2.z()) + 0.5*mean_pitch);
                const BoundingBox sensvol(Ray(p1,p2));
                m_faces[iface] = make_shared<AnodeFace>(ws_face.ident, planes, sensvol);
                cerr << "AnodePlane: face:"<<iface<<" sensvol: " << p1/units::mm << "mm --> "
                     << p2/units::mm << "mm\n";
            }
        } // plane
    } // face

    // remove any duplicate channels
    std::sort(m_channels.begin(), m_channels.end());
    auto chend = std::unique(m_channels.begin(), m_channels.end());
    m_channels.resize( std::distance(m_channels.begin(), chend) );
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

IChannel::pointer Gen::AnodePlane::channel(int ident) const
{
    return nullptr;             // not yet
}

std::vector<int> Gen::AnodePlane::channels() const
{
    return m_channels;
}

IWire::vector Gen::AnodePlane::wires(int channel) const
{
    auto it = m_c2wires.find(channel);
    if (it == m_c2wires.end()) {
        return IWire::vector();
    }
    return it->second;
}
