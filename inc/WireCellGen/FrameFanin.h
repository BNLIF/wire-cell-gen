#ifndef WIRECELL_GEN_FRAMEFANIN
#define WIRECELL_GEN_FRAMEFANIN

#include "WireCellIface/IFrameFanin.h"
#include "WireCellIface/IConfigurable.h"

namespace WireCell {
    namespace Gen {

        // Fan in N frames to one.
        class FrameFanin : public IFrameFanin , public IConfigurable {
        public:
            FrameFanin(size_t multiplicity = 2);
            virtual ~FrameFanin();

            virtual std::vector<std::string> input_types();

            virtual bool operator()(const input_vector& invec, output_pointer& out);

            // IConfigurable
            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

        private:
            size_t m_multiplicity;
        };
    }
}

#endif
