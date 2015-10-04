#include "WireCellGen/TilingGraph.h"


using namespace std;
using namespace WireCell;

TilingGraph::TilingGraph(const ICell::vector& the_cells)
{
    for (auto acell: the_cells) {
	this->record(acell);
    }
    this->join_neighbors();
}
TilingGraph::~TilingGraph()
{
}


WireCell::TilingGraph::Point2D WireCell::TilingGraph::threetotwo(const WireCell::Point& p3)
{
    return WireCell::TilingGraph::Point2D(p3.z(), p3.y());
}

TilingGraph::Property WireCell::TilingGraph::point_property(const Point2D& point)
{
    return Property(Property::point, point_index(point));
}
TilingGraph::Property WireCell::TilingGraph::point_property(const Point2D& point) const
{
    return Property(Property::point, point_index(point));
}

TilingGraph::Property WireCell::TilingGraph::cell_property(ICell::pointer cell)
{
    return Property(Property::cell, cell_index(cell));
}

TilingGraph::Property WireCell::TilingGraph::cell_property(ICell::pointer cell) const
{
    return Property(Property::cell, cell_index(cell));
}

TilingGraph::Property WireCell::TilingGraph::wire_property(IWire::pointer wire)
{
    return Property(Property::wire, wire_index(wire));
}
TilingGraph::Property WireCell::TilingGraph::wire_property(IWire::pointer wire) const
{
    return Property(Property::wire, wire_index(wire));
}



WireCell::TilingGraph::Vertex WireCell::TilingGraph::cell_vertex_make(ICell::pointer cell)
{
    Property prop = cell_property(cell);
    auto found = m_cellVertex.find(prop);
    if (found == m_cellVertex.end()) {
	Vertex vertex = boost::add_vertex(prop, m_graph);
	m_cellVertex[prop] = vertex;
	return vertex;
    }
    return found->second;
}
WireCell::TilingGraph::Vertex WireCell::TilingGraph::wire_vertex_make(IWire::pointer wire)
{
    Property prop = wire_property(wire);
    auto found = m_wireVertex.find(prop);
    if (found == m_wireVertex.end()) {
	Vertex vertex = boost::add_vertex(prop, m_graph);
	m_wireVertex[prop] = vertex;
	return vertex;
    }
    return found->second;
}
WireCell::TilingGraph::Vertex WireCell::TilingGraph::point_vertex_make(const TilingGraph::Point2D& p2d)
{
    Property prop = point_property(p2d);
    auto found = m_pointVertex.find(prop);
    if (found == m_pointVertex.end()) {
	Vertex vertex = boost::add_vertex(prop, m_graph);
	m_pointVertex[prop] = vertex;
	return vertex;
    }
    return found->second;
}

bool WireCell::TilingGraph::cell_vertex_lookup(ICell::pointer cell,
					       WireCell::TilingGraph::Vertex& vtx) const
{
    Property prop = cell_property(cell);
    auto found = m_cellVertex.find(prop);
    if (found == m_cellVertex.end()) {
	return false;
    }
    vtx = found->second;
    return true;
}
bool WireCell::TilingGraph::wire_vertex_lookup(IWire::pointer wire,
					       WireCell::TilingGraph::Vertex& vtx) const
{
    Property prop = wire_property(wire);
    auto found = m_wireVertex.find(prop);
    if (found == m_wireVertex.end()) {
	return false;
    }
    vtx = found->second;
    return true;
}


void WireCell::TilingGraph::record(WireCell::ICell::pointer thecell) 
{
    Vertex cv = cell_vertex_make(thecell);
    //cerr << "Recording cell: " << thecell->ident() << " ("<<cv<<") with wires: " << endl;;

    // wire vertices and wire-cell edges
    const IWire::vector uvw_wires = thecell->wires();
    for (auto wire : uvw_wires) {
	Vertex wv = wire_vertex_make(wire);
	auto the_edge = boost::add_edge(cv, wv, m_graph);
	// cerr << "\tedge " << the_edge.first << " ok? " << the_edge.second
	//      <<  " to wire id=" << wire->ident()
	//      << " graphind=" << wv
	//      << endl;
    }

    // corners
    const WireCell::PointVector pcell = thecell->corners();
    size_t npos = pcell.size();
    std::vector<Vertex> corner_vertices;

    // corner vertices and internal edges
    for (int ind=0; ind < npos; ++ind) {
	Vertex pv = point_vertex_make(threetotwo(pcell[ind]));
	corner_vertices.push_back(pv);

	// center to corner internal edges
	boost::add_edge(cv, pv, m_graph);
    }

    // cell boundary edges
    for (int ind=0; ind<npos; ++ind) {
	boost::add_edge(corner_vertices[ind], corner_vertices[(ind+1)%npos], m_graph);
    }
    
    // done except nearest neighbors


}


bool WireCell::TilingGraph::vertex_cell_lookup(Vertex vtx,
					       WireCell::ICell::pointer& cell) const
{
    auto res = cell_index.collection[m_graph[vtx].index];
    if (!res) return false;
    cell = res;
    return true;
}
bool WireCell::TilingGraph::vertex_wire_lookup(Vertex vtx,
					       WireCell::IWire::pointer& wire) const
{
    auto res = wire_index.collection[m_graph[vtx].index];
    if (!res) return false;
    wire = res;
    return true;
}


// return cells associated with wire
WireCell::ICell::vector WireCell::TilingGraph::cells(WireCell::IWire::pointer wire) const
{
    WireCell::ICell::vector res;
    Vertex vwire;
    if (!wire_vertex_lookup(wire, vwire)) {
	return res;
    }

    auto verts = boost::adjacent_vertices(vwire, m_graph);
    for (auto vit = verts.first; vit != verts.second; ++vit) {
	Vertex other = *vit;
	if (m_graph[other].vtype == Property::cell) {
	    res.push_back(cell_index.collection[m_graph[other].index]);
	}
    }

    return res;
}



// return wires associated with cell
WireCell::IWire::vector WireCell::TilingGraph::wires(WireCell::ICell::pointer cell) const
{
    WireCell::IWire::vector res;
    Vertex vcell;
    if (!cell_vertex_lookup(cell, vcell)) {
	return res;
    }

    auto verts = boost::adjacent_vertices(vcell, m_graph);
    for (auto vit = verts.first; vit != verts.second; ++vit) {
	Vertex other = *vit;
	if (m_graph[other].vtype == Property::wire) {
	    res.push_back(wire_index.collection[m_graph[other].index]);
	}
    }
    return res;
}


// originally from tiling/src/GraphTiling
WireCell::ICell::pointer WireCell::TilingGraph::cell(const IWire::vector& given) const
{
    // This method returns the first cell associated with one wire in
    // the given collection of wires that is also associated with the
    // other wires in the selection.

    IWire::pointer first_wire = given[0];

    WireCell::ICell::vector thecells = this->cells(first_wire);

    if (!thecells.size()) {
        return 0;
    }

    IWireSet wanted(given.begin(), given.end());

    for (auto thecell : thecells) {
	IWire::vector got = thecell->wires();
	IWireSet found(got.begin(), got.end()); // for sort

	// Fill <missing> with <wanted> wires not in <found>.
	IWire::vector missing;
        std::set_difference(wanted.begin(), wanted.end(),
                            found.begin(), found.end(),
                            std::back_inserter(missing));

        if (0 == missing.size()) {
            return thecell;
        }
    }

    return 0;
    
}



bool WireCell::TilingGraph::is_cell(const Vertex& vtx) const
{
    return m_graph[vtx].vtype == Property::cell;
}
bool WireCell::TilingGraph::is_wire(const Vertex& vtx) const
{
    return m_graph[vtx].vtype == Property::wire;
}
bool WireCell::TilingGraph::is_point(const Vertex& vtx) const
{
    return m_graph[vtx].vtype == Property::point;
}

std::set<TilingGraph::Vertex>
TilingGraph::connected_of_type(Vertex vtx,
			       Property::VertexType_t typ) const
{
    std::set<Vertex> ret;
    auto adjacent = boost::adjacent_vertices(vtx, m_graph);
    std::copy_if(adjacent.first, adjacent.second,
		 std::inserter(ret, ret.end()),
		 [=](const Vertex& v) { return m_graph[v].vtype == typ; });
    return ret;
}

std::set<TilingGraph::Vertex>
TilingGraph::all_of_type(Property::VertexType_t typ) const
{
    std::set<Vertex> ret;
    auto adjacent = boost::vertices(m_graph);
    std::copy_if(adjacent.first, adjacent.second,
		 std::inserter(ret, ret.end()),
		 [=](const Vertex& v) { return m_graph[v].vtype == typ; });
    return ret;
}



void WireCell::TilingGraph::join_neighbors()
{
    auto cell_verts = all_of_type(Property::cell);

    for (auto the_cell_vert : cell_verts) {

	std::set<Vertex> the_neighbors;

	auto corner_verts = connected_of_type(the_cell_vert, Property::point);
	for (auto cv : corner_verts) {
	    auto next_door = connected_of_type(cv, Property::cell);
	    the_neighbors.insert(next_door.begin(), next_door.end());
	}

	for (auto neighbor : the_neighbors) {
	    if (neighbor == the_cell_vert) { continue; }
	    add_edge(the_cell_vert, neighbor, m_graph);
	}
	    
    } // all cells

}

WireCell::ICell::vector WireCell::TilingGraph::neighbors(ICell::pointer cell) const
{
    WireCell::ICell::vector ret;
    Vertex cell_vtx;
    cell_vertex_lookup(cell, cell_vtx);
    auto neighbors = connected_of_type(cell_vtx, Property::cell);
    for (auto nc : neighbors) {
	WireCell::ICell::pointer np;
	if (!vertex_cell_lookup(nc, np)) {
	    cerr << "Failed to find cell for vertex " << nc << endl;
	    continue;
	}
	ret.push_back(np);
    }
    return ret;
}
