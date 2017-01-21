#ifndef WIRECELLGEN_DRIFTSIM_H
#define WIRECELLGEN_DRIFTSIM_H

#include "WireCellUtil/ImpactResponse.h"
#include "WireCellUtil/Pimpos.h"
#include "WireCellUtil/Array.h"
#include "WireCellIface/IDepo.h"

#include "WireCellGen/BinnedDiffusion.h"

#include <vector>

namespace WireCell {

    namespace Gen {

        class DriftSim
        {
        public:
            /** Make a DriftSim.

                A DriftSim takes IDepo deposition objects and produces
                2D response for planes of wires binned in wire and
                time.

                It must be given a geometrical descriptions of some
                number of wire planes (the vector of Pimpos objects)
                and corresponding information about the plane response
                (the vector of PlaneImpactResponse objects).

             */
            DriftSim(const std::vector<Pimpos>& plane_descriptors,
                     const std::vector<PlaneImpactResponse>& plane_responses,
                     double ndiffision_sigma = 3.0, bool fluctuate = true);
            virtual ~DriftSim();

            /// Add a deposition for consideration.  The depo is
            /// expected to be drifted to the FieldResponse "origin"
            /// (the location in the nominal drift direction where
            /// field response paths begin).  The deposition extent
            /// (longitudinal and transverse) are interpreted as
            /// Gaussian sigma.  If the depo is not at the origin, it
            /// will be drifted to a new depo at the origin.  This
            /// drift will be trivial in that no diffusion nor
            /// absorption will be applied.  

            IDepo::pointer add(IDepo::pointer depo);


            /// A block is data from one plane as a 2D (nwires,nticks) array
	    typedef Array::array_xxf block_t;

            /// Return a vector of blocks covering the given time
            /// binning with the response for the planes.  The time
            /// binning is taken w.r.t. the time defined at the drift
            /// "origin".  It is assumed that a subsequent call to
            /// latch() covers a time period which begins no earlier
            /// than the last period did.  It may otherwise overlap.
            /// It is up to the caller to add sufficient depos as
            /// desired before latching.
            std::vector<block_t> latch(Binning tbins);

        private:
            const std::vector<Pimpos>& m_pimpos;
            const std::vector<PlaneImpactResponse>& m_pirw;
            double m_ndiffision_sigma;
            bool m_fluctuate;

        };


    }  // Gen

}  // WireCell

#endif /* WIRECELLGEN_DRIFTSIM_H */
