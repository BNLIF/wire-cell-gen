// larweirecell::FrameSaver needs a general channel-to-wireplane resolver
// fixme: this is a temporary solution, only the member function resolve()
// is useful, otherwise, please do NOT use the other virtual fuctions inherited
// from IAnodePlane

#ifndef WIRECELLGEN_MEGAANODEPLANE
#define WIRECELLGEN_MEGAANODEPLANE

#include "WireCellGen/AnodePlane.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IConfigurable.h"
#include <vector>

namespace WireCell {
	namespace Gen {

		class MegaAnodePlane: public IAnodePlane, public IConfigurable {
		public:
			// MegaAnodePlane();
			virtual ~MegaAnodePlane() {}

            // IConfigurable interface
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

            // IAnodePlane interface
            virtual int ident() const { return -1; } // fixme: should never use this func
            virtual int nfaces() const { return m_anodes[0]->nfaces(); } // fixme: should never use this func
            virtual IAnodeFace::pointer face(int ident) const { return m_anodes[0]->face(ident); } // fixme: should never use this func
            virtual IAnodeFace::vector faces() const { return m_anodes[0]->faces(); } // fixme: should never use this func
            virtual WirePlaneId resolve(int channel) const;
            virtual std::vector<int> channels() const { return m_anodes[0]->channels(); } // fixme: should never use this func
            virtual IChannel::pointer channel(int chident) const { return m_anodes[0]->channel(chident); } // fixme: should never use this func
        	virtual IWire::vector wires(int channel) const { return m_anodes[0]->wires(channel); } // fixme: should never use this func

        	//
        	// int AddAnodePlane(AnodePlane* anode){
        	// 	m_anodes.push_back(anode);
        	// 	return m_anodes.size();
        	// }
       	private:
       		std::vector<IAnodePlane::pointer> m_anodes;

		};
	}
}

#endif
