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

            // Accept new frames into the buffer
            void merge(const output_queue& newframes);

            // Maybe extract output frames from the buffer.  If the
            // depo is past the next scheduled readout or if a nullptr
            // depo (EOS) then sub-ductors are flushed with EOS and
            // the outframes queue is filled with one or more frames.
            // If extraction occurs not by EOS then any carryover is
            // kept.
            bool maybe_extract(const input_pointer& depo, output_queue& outframes);

        private:

            std::string m_anode_tn;
            IAnodePlane::pointer m_anode;
            double m_start_time;
            double m_readout_time;
            int m_frame_count;

            struct SubDuctor {
                std::string name;
                std::function<bool(IDepo::pointer depo)> check;
                IDuctor::pointer ductor;
                SubDuctor(const std::string& tn,
                          std::function<bool(IDepo::pointer depo)> f,
                          IDuctor::pointer d) : name(tn), check(f), ductor(d) {}
            };
            typedef std::vector<SubDuctor> ductorchain_t;
            std::vector<ductorchain_t> m_chains;            
            
            /// As sub ductors are called they will each return frames
            /// which are not in general synchronized with the others.
            /// Their frames must be buffered here and released as a
            /// merged frame in order for MultiDuctor to behave just
            /// like a monolithic ductor.
            output_queue m_frame_buffer;

        };
    }
}

#endif
