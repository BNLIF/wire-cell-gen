/** The biggest unit is an "anode".
    
    This is a configurable object and from it faces of planes of wires
    can be accessed.
 */

#ifndef WIRECELLGEN_ANODEPLANE
#define WIRECELLGEN_ANODEPLANE

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IConfigurable.h"

namespace WireCell {
    namespace Gen {

        class AnodePlane : public IAnodePlane, public IConfigurable {
        public:

            AnodePlane();
            virtual ~AnodePlane() {}

            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;


            virtual int ident() const { return m_ident; }

            virtual int nfaces() const { return m_faces.size(); }

            virtual IAnodeFace::pointer face(int ident) const;


        private:

            int m_ident;
            IAnodeFace::vector m_faces;

            Response::Schema::FieldResponse m_fr;


        };
    }

}


#endif
