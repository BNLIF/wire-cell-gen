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



const double default_gain = 14.0*units::mV/units::fC;
const double default_shaping = 2.0*units::us;
const double default_rc_constant = 1.0*units::ms;
const double default_postgain = 1.0;

WireCell::Configuration Gen::AnodePlane::default_configuration() const
{
    Configuration cfg;

    /// This number is used to take from the wire file the anode
    put(cfg, "ident", 0);

    /// Name of a IWireSchema component.
    // note: this used to be a "wires" parameter directly specifying a filename.
    put(cfg, "wire_schema", "");

    /// Name fo a IFieldResponse component.
    // note: this used to be a "fields" parameter directly specifying a filename.
    put(cfg, "field_response", "");

    /// Electronics gain
    put(cfg, "gain", default_gain);

    /// Post FEE relative gain
    put(cfg, "postgain", default_postgain);

    /// Electronics shaping time 
    put(cfg, "shaping", default_shaping);

    /// RC constant of RC filters 
    put(cfg, "rc_constant", default_rc_constant);
    
    /// The period over which to latch responses
    //put(cfg, "readout_time", default_readout);

    /// The sample period
    //put(cfg, "tick", default_tick);

    /// A point on the cathode plane(s).  If this is a two faced anode
    /// then the second element is for the "back" face (the face which
    /// has (wire) (x) (pitch) direction in the global -X direction.
    /// If one face is not sensitive then a "null" value may be given.
    cfg["cathode"] = Json::arrayValue;

    return cfg;
}


void Gen::AnodePlane::configure(const WireCell::Configuration& cfg)
{
    double gain = get<double>(cfg, "gain", default_gain);
    double postgain = get<double>(cfg, "postgain", default_postgain);
    double shaping = get<double>(cfg, "shaping", default_shaping);
    double rc_constant = get<double>(cfg, "rc_constant", default_rc_constant);

    cerr << "Gen::AnodePlane: "
         << "gain=" << gain/(units::mV/units::fC) << " mV/fC, "
         << "peaking=" << shaping/units::us << " us, "
         << "RCconstant=" << rc_constant/units::us << " us, "
         << "postgain=" << postgain << ", "
         << endl;

    // pre-check
    {
        auto jcathode = cfg["cathode"];
        if (jcathode.isNull() or jcathode.size() == 0) {
            cerr << "Gen::AnodePlane: must be configured with at least one cathode point\n";
            THROW(ValueError() << errmsg{"Gen::AnodePlane: must be configured with at least one cathode point"});
        }
    }

    m_faces.clear();
    m_ident = get<int>(cfg, "ident", 0);

    // check for obsolete config params:
    if (!cfg["fields"].isNull() or !cfg["wires"].isNull()) {
        cerr << "\nWarning: your configuration is obsolete.\n"
             << "Use \"field_response\" and \"wire_store\" to name components instead of directly giving file names\n"
             << "This job will likely throw an exception next\n\n";
    }

    // get field responses
    const string fr_name = get<string>(cfg, "field_response");
    if (fr_name.empty()) {
        cerr << "Gen::AnodePlane::configure() must have an IFieldResponse component specified\n";
        THROW(ValueError() << errmsg{"\"field_response\" parameter must specify an IFieldResponse component"});
    }
    m_fr = Factory::find_tn<IFieldResponse>(fr_name); // throws if not found
    const auto& fr = m_fr->field_response();
    if (fr.speed <= 0) {
        THROW(ValueError() << errmsg{format("illegal drift speed: %f mm/us", fr.speed/(units::mm/units::us))});
    }

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

        // Only include the cathode in the bounding box if given.
        // Final sensvol will still include the span of the wires.
        BoundingBox sensvol;
        auto jcathode = cfg["cathode"][(int)iface];
        if (! jcathode.isNull()) {
            auto catpt = convert<Point>(jcathode);
            sensvol(catpt);
            cerr << "AnodePlane: adding cathod point: " << catpt << endl;
        }

        IWirePlane::vector planes(nplanes);
        for (size_t iplane=0; iplane<nplanes; ++iplane) { // note, WireSchema requires U/V/W plane ordering in a face.
            const auto& ws_plane = ws_planes[iplane];

            // WirePlaneId wire_plane_id(s_plane.ident); // dubious
            WirePlaneId wire_plane_id(iplane2layer[iplane], iface, m_ident);

            if (wire_plane_id.index() < 0) {
                THROW(ValueError() << errmsg{format("bad wire plane id: %d", wire_plane_id.ident())});
            }

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


            auto pr = fr.plane(ws_plane.ident);

            // Calculate transverse center of wire plane and pushed to
            // a point longitudinally where the field response starts.
            // Note, this requires EXQUISITE coincidence between the
            // location of wire planes in the wire data file and the
            // field response file.  There's currently absolutely
            // nothing that assures this!  If the resulting X origin
            // for the resuling Pimpos objects for the three plane
            // don't match, then the coincidence was violated!

            const BoundingBox bb = ws_store.bounding_box(ws_plane);
            const Ray bb_ray = bb.bounds();
            if (!sensvol.empty()) { // only extend the sensitive volume if user gave a cathode point.
                sensvol(bb_ray);
            }
            Vector field_origin = 0.5*(bb_ray.first + bb_ray.second);
            const double to_response_plane = fr.origin - pr->location;
            // cerr << "AnodePlane:: face:" << iface << " plane:" << iplane
            //      << " fr.origin:" << fr.origin/units::cm << "cm pr.location:" << pr->location/units::cm << "cm"
            //      << " delta:"<<to_response_plane/units::cm<<"cm center:" << field_origin/units::cm << "cm\n";
            // cerr << "\tX-axis: " << ecks_dir << endl;
            // cerr << "\tBB:" << bb_ray.first << " --> " << bb_ray.second << endl;
            field_origin += to_response_plane * ecks_dir;

            const double wire_pitch = pr->pitch;
            const double impact_pitch = pr->paths[1].pitchpos - pr->paths[0].pitchpos;
            const int nregion_bins = round(wire_pitch/impact_pitch);

            // Absolute locations in the pitch dimension of the
            // first/last wires measured relative to the
            // centered origin.
            const double edgewirepitch = wire_pitch * 0.5 * (nwires - 1);

            Pimpos* pimpos = new Pimpos(nwires, -edgewirepitch, edgewirepitch,
                                        wire_pitch_dirs.first, wire_pitch_dirs.second,
                                        field_origin, nregion_bins);

            planes[iplane] = make_shared<WirePlane>(ws_plane.ident, wires, pimpos);

        } // plane
        {
            auto ray = sensvol.bounds();
            cerr << "AnodePlane: ident: "<< m_ident << " making face " << iface << " with ident:" << ws_face.ident
                 << " sensitive: " << !sensvol.empty()
                 << " with bb: "<< ray.first << " --> " << ray.second <<"\n";
        }
        m_faces[iface] = make_shared<AnodeFace>(ws_face.ident, planes, sensvol);

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
