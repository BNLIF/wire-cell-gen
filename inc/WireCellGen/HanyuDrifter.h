// direct copy of Brett's Drifter to play
// 
#ifndef WIRECELL_HANYU_DRIFTER
#define WIRECELL_HANYU_DRIFTER

#include "WireCellIface/IDrifter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellUtil/Units.h"

#include <deque>

namespace WireCell {

    namespace Gen {

        /** This IDrifter accepts inserted point depositions of some
         * number of electrons, drifts them to a plane near an
         * IAnodePlane and buffers long enough to assure they can be
         * delivered in time order.
         *
         * Note that the input electrons are assumed to be already
         * free from Fano and Recombination.
         */
        class hyDrifter : public IDrifter, public IConfigurable {
        public:
            hyDrifter(const std::string& anode_plane_component="");

            /// WireCell::IDrifter interface.
            virtual void reset();
            virtual bool operator()(const input_pointer& depo, output_queue& outq);

            /// WireCell::IConfigurable interface.
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;


            // Implementation methods.

            // Do actual transport, producing a new depo
            IDepo::pointer transport(IDepo::pointer depo);

            // Return the "proper time" for a deposition
            double proper_time(IDepo::pointer depo);

        private:

            std::string m_anode_tn;

            // Longitudinal and Transverse coefficients of diffusion
            // in units of [length^2]/[time].
            double m_DL, m_DT;

            // Electron absorption lifetime.
            double m_lifetime;          

            // If true, fluctuate by number of absorbed electrons.
            bool m_fluctuate;

            // cached, get from anode object.
            double m_location, m_speed; 
            
            // fano factor to fluctuate ionization electrons 
            double m_fano; 

            // recombination probability
            double m_recomb;

            // Input buffer sorted by proper time
            DepoTauSortedSet m_input;

            // mark reaching end of stream 
            bool m_eos;

            // Set anode plane component title:name.
            void set_anode();


        };

    }

}

#endif
