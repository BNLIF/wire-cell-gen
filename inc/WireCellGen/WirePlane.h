/** An IWirePlane.
 */

#ifndef WIRECELLGEN_WIREPLANE
#define WIRECELLGEN_WIREPLANE

#include "WireCellIface/IWirePlane.h"

namespace WireCell {
    namespace Gen {

        class WirePlane : public IWirePlane {
        public:

            WirePlane(int ident, IWire::vector wires, Pimpos* pimpos, PlaneImpactResponse* pir);
            virtual ~WirePlane();

            virtual int ident() const { return m_ident; }

            virtual const Pimpos* pimpos() const { return m_pimpos; }
            
            /// Access response functions.
            virtual const PlaneImpactResponse* pir() const { return m_pir; }
            
            /// Return vector of wire objects ordered by increasing Z.
            virtual const IWire::vector& wires() const { return m_wires; }
            
        private:

            int m_ident;
            Pimpos* m_pimpos;
            PlaneImpactResponse* m_pir;
            IWire::vector m_wires;

            Ray m_bbox;         // bounding box of wire planes in external coordinates

        };
    }

}


#endif
