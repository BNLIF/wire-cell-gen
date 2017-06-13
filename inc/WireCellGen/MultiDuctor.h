#ifndef WIRECELLGEN_MULTIDUCTOR
#define WIRECELLGEN_MULTIDUCTOR

#include "WireCellIface/IDuctor.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"

#include <functional>

namespace WireCell {
    namespace Gen {

        class MultiDuctor : public IDuctor, public IConfigurable {

        public:
            
            MultiDuctor(const std::string anode = "AnodePlane");
            virtual ~MultiDuctor();

            // IDuctor
            virtual void reset();
            virtual bool operator()(const input_pointer& depo, output_queue& frames);

            // IConfigurable
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

            // local

        private:

            std::string m_anode_tn;
            IAnodePlane::pointer m_anode;

            struct SubDuctor {
                std::function<bool(IDepo::pointer depo)> check;
                IDuctor::pointer ductor;
                SubDuctor(std::function<bool(IDepo::pointer depo)> f, IDuctor::pointer d) : check(f), ductor(d) {}
            };
            std::vector<SubDuctor> m_subductors;            
            

        };
    }
}

#endif
