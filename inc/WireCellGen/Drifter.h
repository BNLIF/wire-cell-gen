#ifndef WIRECELL_GEN_DRIFTER
#define WIRECELL_GEN_DRIFTER

#include "WireCellIface/IDrifter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IRandom.h"
#include "WireCellUtil/Units.h"

#include <deque>

namespace WireCell {

    namespace Gen {

        /** This component drifts depos bounded by planes
         * perpendicular to the X-axis.  The boundary planes are
         * specified with the "xregions" list.  Each list is an object
         * with "cathode" and "anode" attributes giving their X
         * location.  Drifting ends when the (negative) deposition
         * reaches the anode plane.
         * 
         * Input depositions must be ordered in absolute time (their
         * current time) and output depositions are produced ordered
         * by their time at the anode plane.
         * 
         * Diffusion and absorption effects and also, optionally,
         * fluctuations are applied.  Fano factor and Recombination
         * are not applied in this component.
         *
         * Typically a drifter is used just prior to a ductor and in
         * such cases the "anode" boundary plane should be coincident
         * with the non-physical "response plane" which defines the
         * starting point for the field response functions.  The
         * location of the response plane *realtive* to the wire
         * planes can be found using:
         *
         * $ wriecell-sigproc response-info garfield-1d-3planes-21wires-6impacts-dune-v1.json.bz2 
         * origin:10.00 cm, period:0.10 us, tstart:0.00 us, speed:1.60 mm/us, axis:(1.00,0.00,0.00)
         *    plane:0, location:9.4200mm, pitch:4.7100mm
	 *    plane:1, location:4.7100mm, pitch:4.7100mm
         *    plane:2, location:0.0000mm, pitch:4.7100mm
         * 
         * Here, "origin" gives the location of the response plane.
         * The location of the wire planes according to wire geometry
         * can be similarly dumped.
         *
         * $ wirecell-util wires-info protodune-wires-larsoft-v3.json.bz2 
         * anode:0 face:0 X=[-3584.63,-3584.63]mm Y=[6066.70,6066.70]mm Z=[7.92,7.92]mm
         *     0: x=-3584.63mm dx=9.5250mm
         *     1: x=-3589.39mm dx=4.7620mm
         *     2: x=-3594.16mm dx=0.0000mm
         * ....
         * anode:5 face:1 X=[3584.63,3584.63]mm Y=[6066.70,6066.70]mm Z=[6940.01,6940.01]mm
         *     0: x=3584.63mm dx=-9.5250mm
         *     1: x=3589.39mm dx=-4.7620mm
         *     2: x=3594.16mm dx=0.0000mm
         * 
         * Note, as can see, these two sources of information may not
         * be consistent w.r.t. the inter-plane separation distance
         * (4.71mm and 4.76mm, respectively).  This mismatch will
         * result in a relative shift in time between the planes for
         * various waveform features (eg induction zero crossings and
         * collection peak).
         *
         * For the example above, likely candidates for "anode" X
         * locations are:
         *
         *    x = -3594.16mm + 10cm
         * 
         * and
         *
         *    x = +3594.16mm - 10cm
         */
        class Drifter : public IDrifter, public IConfigurable {
        public:
            Drifter();
            virtual ~Drifter();

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

            bool insert(const input_pointer& depo);
            void flush(output_queue& outq);
            void flush_ripe(output_queue& outq, double now);

        private:

            IRandom::pointer m_rng;
            std::string m_rng_tn;

            // Longitudinal and Transverse coefficients of diffusion
            // in units of [length^2]/[time].
            double m_DL, m_DT;

            // Electron absorption lifetime.
            double m_lifetime;          

            // If true, fluctuate by number of absorbed electrons.
            bool m_fluctuate;

            double m_speed;   // drift speeds
            int n_dropped, n_drifted;

            // A little helper to carry the region extent and depo buffers.
            struct Xregion {
                Xregion(double ax, double cx);
                double anode, cathode;
                std::vector<IDepo::pointer> depos; // buffer depos

                bool inside(double x) const;

            };
            std::vector<Xregion> m_xregions;  

            struct IsInside {
                const input_pointer& depo;
                IsInside(const input_pointer& depo) : depo(depo) {}
                bool operator()(const Xregion& xr) const { return xr.inside(depo->pos().x()); }
            };

        };

    }

}

#endif
