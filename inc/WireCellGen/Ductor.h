#ifndef WIRECELLGEN_DUCTOR
#define WIRECELLGEN_DUCTOR

#include "WireCellUtil/Pimpos.h"
#include "WireCellUtil/Response.h"

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IDuctor.h"

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IRandom.h"

namespace WireCell {
    namespace Gen {

        /** This IDuctor needs a Garfield2D field calculation data
         * file in compressed JSON format as produced by Python module
         * wirecell.sigproc.garfield.
         */
        class Ductor : public IDuctor, public IConfigurable {
        public:
            
            Ductor();

            virtual void reset();
            virtual bool operator()(const input_pointer& depo, output_queue& frames);

            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

        private:

            // The "Type:Name" of the IAnodePlane (default is "AnodePlane")
            std::string m_anode_tn;
            std::string m_rng_tn;

            IAnodePlane::pointer m_anode;
            IRandom::pointer m_rng;
            IDepo::vector m_depos;

            double m_start_time;
            double m_readout_time;
            double m_drift_speed;
            double m_nsigma;
            bool m_fluctuate;

            int m_frame_count;
            bool m_eos;

            void process(output_queue& frames);


        };
    }
}

#endif
