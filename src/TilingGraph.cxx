#include "TilingGraph.h"


using namespace std;
using namespace WireCell;

static WireCell::TilingGraph::Point2D threetotwo(const WireCell::Point& p3)
{
    return WireCell::TilingGraph::Point2D(p3.z(), p3.y());
}

TilingGraph::Property WireCell::TilingGraph::point_property(const Point2D& point)
{
    return Property(Property::point, point_index(point));
}

TilingGraph::Property WireCell::TilingGraph::cell_property(ICell::pointer cell)
{
    return Property(Property::cell, cell_index(cell));
}

TilingGraph::Property WireCell::TilingGraph::wire_property(IWire::pointer wire)
{
    return Property(Property::wire, wire_index(wire));
}

/*

  Make graph store wire/cell info by ident.

  Make TilingGraphCell/Wire.

 */



WireCell::TilingGraph::Vertex WireCell::TilingGraph::cell_vertex(ICell::pointer cell)
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
WireCell::TilingGraph::Vertex WireCell::TilingGraph::wire_vertex(IWire::pointer wire)
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
WireCell::TilingGraph::Vertex WireCell::TilingGraph::point_vertex(const TilingGraph::Point2D& p2d)
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



void WireCell::TilingGraph::record(WireCell::ICell::pointer thecell) 
{
    Vertex cv = cell_vertex(thecell);
    //cerr << "Recording cell: " << thecell->ident() << " ("<<cv<<") with wires: " << endl;;

    // wire vertices and wire-cell edges
    const IWireVector uvw_wires = thecell->wires();
    for (auto wire : uvw_wires) {
	Vertex wv = wire_vertex(wire);
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
	Vertex pv = point_vertex(threetotwo(pcell[ind]));
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


// return cells associated with wire
WireCell::ICellVector WireCell::TilingGraph::cells(WireCell::IWire::pointer wire)
{
    Vertex vwire = wire_vertex(wire);

    WireCell::ICellVector res;
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
WireCell::IWireVector WireCell::TilingGraph::wires(WireCell::ICell::pointer cell)
{
    Vertex vcell = cell_vertex(cell);

    WireCell::IWireVector res;
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
WireCell::ICell::pointer WireCell::TilingGraph::cell(const IWireVector& given)
{
    // This method returns the first cell associated with one wire in
    // the given collection of wires that is also associated with the
    // other wires in the selection.

    IWire::pointer first_wire = given[0];

    WireCell::ICellVector thecells = this->cells(first_wire);

    if (!thecells.size()) {
        return 0;
    }

    IWireSet wanted(given.begin(), given.end());

    for (auto thecell : thecells) {
	IWireVector got = thecell->wires();
	IWireSet found(got.begin(), got.end()); // for sort

	// Fill <missing> with <wanted> wires not in <found>.
	IWireVector missing;
        std::set_difference(wanted.begin(), wanted.end(),
                            found.begin(), found.end(),
                            std::back_inserter(missing));

        if (0 == missing.size()) {
            return thecell;
        }
    }

    return 0;
    
}


