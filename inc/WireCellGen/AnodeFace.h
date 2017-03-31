#ifndef WIRECELLGEN_ANODEFACE
#define WIRECELLGEN_ANODEFACE

#include "WireCellIface/IAnodeFace.h"


namespace WireCell {
    namespace Gen {

        class AnodeFace : public IAnodeFace {
        public:
            
            AnodeFace(int ident, IWirePlane::vector planes);

            /// Return the ident number of this face.
            virtual int ident() const { return m_ident; }

            /// Return the number of wire planes in the given side
            virtual int nplanes() const { return m_planes.size(); }

            /// Return the wire plane with the given ident or nullptr if unknown.
            IWirePlane::pointer plane(int ident) const;

        private:
            int m_ident;
            IWirePlane::vector m_planes;
        };
    }
}

#endif
