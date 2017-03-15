#ifndef WIRECELLGEN_DUCTOR
#define WIRECELLGEN_DUCTOR

#include "WireCellUtil/Pimpos.h"
#include "WireCellUtil/Response.h"

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IDuctor.h"

namespace WireCell {
    namespace Gen {

        /** This IDuctor needs a Garfield2D field calculation data
         * file in compressed JSON format as produced by Python module
         * wirecell.sigproc.garfield.
         */
        class Ductor : public IDuctor, public IConfigurable {
        public:
            
            Ductor();

            virtual bool operator()(const input_pointer& depo, output_queue& frames);

            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

        private:

            std::vector<Pimpos*> m_pimpos;
            Response::Schema::FieldResponse m_fr;

            double m_start_time, m_readout_period, m_sample_period;
            double m_drift_speed;
            double m_nsigma;
            bool m_fluctuate;
            double m_gain, m_shaping;


        };
    }
}

#endif
