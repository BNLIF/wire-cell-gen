// this is a private header for Tiling.cxx.

#ifndef WIRECELL_TILINGGRAPH
#define WIRECELL_TILINGGRAPH

#include "WireCellIface/IWire.h"
#include "WireCellIface/ICellSummary.h"
#include "WireCellUtil/IndexedSet.h"

#include <boost/graph/adjacency_list.hpp>


namespace WireCell {



    class TilingGraph : public ICellSummary {
    public:
	TilingGraph(const ICellVector& the_cells);
	virtual ~TilingGraph();


	/// ICellSummary

	// return wires associated with cell.  This should be
	// identical to cell->wires() but does query the graph.
	virtual IWireVector wires(ICell::pointer cell) const;

	/// Return cells associated with the wires.
	virtual ICellVector cells(IWire::pointer wire) const;

	/// Return a single cell if it is associated with all wires.
	virtual ICell::pointer cell(const IWireVector& wires) const;
    
	/// Return collection of nearest neighbor cells.
	virtual ICellVector neighbors(ICell::pointer cell) const;

    private:

	void record(ICell::pointer thecell);

	/// Each vertex of the graph has a "type" and an index into a
	/// vector of objects of corresponding type.:
	/// - cell indexes into an array of Cell
	/// - wire indexes into an array of Wire
	/// - point index into an array of Point 
	struct Property {
	    enum VertexType_t { unknown=0, cell, wire, point};
	    VertexType_t vtype;
	    int index;
	    Property(VertexType_t vt=unknown, int ind=-1) : vtype(vt), index(ind) {}
	
	    bool operator<(const Property& rhs) const {
		if (vtype == rhs.vtype) {
		    return index < rhs.index;
		}
		return vtype < rhs.vtype;
	    }
	};


	/// The edges are implicitly of these four types:
	/// - wirecell :: cell <--> wire
	/// - neighbor :: cell <--> cell
	/// - boundary :: point <--> point
	/// - internal :: cell <--> point

	typedef boost::adjacency_list<boost::setS, boost::vecS, 
				      boost::undirectedS,
				      Property > BoostTilingGraph;	
	typedef boost::graph_traits<BoostTilingGraph>::vertex_descriptor Vertex;
	typedef boost::graph_traits<BoostTilingGraph>::edge_descriptor Edge;



	typedef std::pair<float,float> Point2D;
	Point2D threetotwo(const WireCell::Point& p3);

	/// BGL does not appear to support lookups by property, so
	/// DIY.  Note, these are actually TilingGraphCell and
	/// TilingGraphWire
	Vertex cell_vertex_make(ICell::pointer cell);
	Vertex wire_vertex_make(IWire::pointer wire);
	Vertex point_vertex_make(const Point2D& p2d);
    
	bool cell_vertex_lookup(ICell::pointer cell, Vertex& vtx) const;
	bool wire_vertex_lookup(IWire::pointer wire, Vertex& vtx) const;

	/// A graph vertex may be a wire a cell or a point.  The graph
	/// stores this as a type and a key into an index for that type.
	/// For Wire and Cell, they also carry a unique ident() but that
	/// is ignored.
	WireCell::IndexedSet<Point2D>        point_index;
	WireCell::IndexedSet<ICell::pointer> cell_index;
	WireCell::IndexedSet<IWire::pointer> wire_index;
    
	Property point_property(const Point2D& point);
	Property point_property(const Point2D& point) const;
	Property cell_property(ICell::pointer cell);
	Property cell_property(ICell::pointer cell) const;
	Property wire_property(IWire::pointer wire);
	Property wire_property(IWire::pointer wire) const;

    private:
	BoostTilingGraph m_graph;
	mutable std::map<Property, Vertex> m_cellVertex, m_wireVertex, m_pointVertex;


    };
    

} // namespace WireCell


#endif
