#ifndef WIRECELL_GEN_PIPEGRAPH_WRAPPERS
#define WIRECELL_GEN_PIPEGRAPH_WRAPPERS

#include "WireCellUtil/PipeGraph.h"

// fixme: this is a rather monolithic file that should be broken out
// into its own package.  It needs to depend on util and iface but NOT
// gen.
#include "WireCellIface/ISourceNode.h"
#include "WireCellIface/ISinkNode.h"

#include <map>
#include <iostream>             // debug

namespace WireCell { namespace PipeGraph { namespace Wrapper {

// Node wrappers are constructed with just an INode::pointer and adapt
// it to PipeGraph::Node.  They are constructed with a type-erasing
// factory below.

class PortedNode : public PipeGraph::Node
{
public:
    PortedNode(int nin=0, int nout=0) { 
        using PipeGraph::Port;
        for (int ind=0; ind<nin; ++ind) {
            m_ports[Port::input].push_back(
                PipeGraph::Port(this, PipeGraph::Port::input));
        }
        for (int ind=0; ind<nout; ++ind) {
            m_ports[Port::output].push_back(
                PipeGraph::Port(this, PipeGraph::Port::output));
        }
    }        
};


class Source : public PortedNode {
    ISourceNodeBase::pointer m_wcnode;
    bool m_ok;
public:
    Source(INode::pointer wcnode) : PortedNode(0,1), m_ok(true) {
        m_wcnode = std::dynamic_pointer_cast<ISourceNodeBase>(wcnode);
    }
    virtual ~Source() {}
    virtual bool ready() { return m_ok; }
    virtual bool operator()() {
        boost::any obj;
        m_ok = (*m_wcnode)(obj);
        if (!m_ok) {
            // fixme: need to clean up the node protocol.  When using
            // queues, the protocol is simply: don't send anything out
            // when you have nothing to send out.  But in the
            // functional interface with output arguments there is not
            // general type safe way to tell if the output argument
            // was set to NULL.  So, we must wait to go one past EOS
            // to see the railure returned.  What I *should* do is use
            // excpetions to indicate failures or read-past-EOS.
            return true;
        }
        oport().put(obj);
        return true;
    }
};

class Sink : public PortedNode {
    ISinkNodeBase::pointer m_wcnode;
public:
    Sink(INode::pointer wcnode) : PortedNode(1,0) {
        m_wcnode = std::dynamic_pointer_cast<ISinkNodeBase>(wcnode);
    }
    virtual ~Sink() {}
    virtual bool operator()() {
        auto obj = iport().get();
        bool ok = (*m_wcnode)(obj);
        std::cerr << "Sink returns: " << ok << std::endl;
        return ok;
    }
};


// Makers make an appropriate PipeGraph::Node from an INode
struct Maker {
    virtual ~Maker() {}
    virtual Node* operator()(INode::pointer wcnode) = 0;
};
template<class Wrapper>
struct MakerT : public Maker {
    virtual ~MakerT() {}
    virtual Node* operator()(INode::pointer wcnode) {
        return new Wrapper(wcnode);
    }
};
class Factory {
public:
    // here we bind all concrete types 
    Factory() {
        bind_maker<Source>(INode::sourceNode);
        bind_maker<Sink>(INode::sinkNode);
        // ...
    }

    typedef std::map<INode::pointer, Node*> WCNodeWrapperMap;

    template<class Wrapper>
    void bind_maker(INode::NodeCategory cat) {
        m_factory[cat] = new MakerT<Wrapper>;
    }

    Node* operator()(WireCell::INode::pointer wcnode) {
        auto nit = m_nodes.find(wcnode);
        if (nit != m_nodes.end()) {
            return nit->second;
        }
        auto mit = m_factory.find(wcnode->category());
        if (mit == m_factory.end()) {
            return nullptr;
        }
        auto maker = mit->second;

        Node* node = (*maker)(wcnode);
        m_nodes[wcnode] = node;
        return node;
    }


private:
    typedef std::map<INode::NodeCategory, Maker*> NodeMakers;
    NodeMakers m_factory;
    WCNodeWrapperMap m_nodes;
};
        
}}}

#endif
