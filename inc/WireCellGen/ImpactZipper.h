// fixme: this should be made a component

#ifndef WIRECELL_IMPACTZIPPER
#define WIRECELL_IMPACTZIPPER

#include "WireCellIface/IPlaneImpactResponse.h"
#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellUtil/Array.h"


namespace WireCell {
    namespace Gen {

    
        /** An ImpactZipper "zips" up through all the impact positions
         * along a wire plane convolving the response functions and
         * the local drifted charge distribution producing a waveform
         * on each central wire.
         */
        class ImpactZipper
        {
            IPlaneImpactResponse::pointer m_pir;
            BinnedDiffusion& m_bd;
	    
	    int m_flag;
	    int m_num_group;  // how many 2D convolution is needed
	    int m_num_pad_wire; // how many wires are needed to pad on each side
	    std::vector<std::map<int, IImpactResponse::pointer> > m_vec_map_resp;
	    std::vector<std::vector<std::tuple<int,int,double> > > m_vec_vec_charge; // ch, time, charge
	    std::vector<int> m_vec_impact;
	    Array::array_xxf m_decon_data;
	    int m_start_ch;
	    int m_end_ch;
	    int m_start_tick;
	    int m_end_tick;
	    
        public:

            ImpactZipper(IPlaneImpactResponse::pointer pir, BinnedDiffusion& bd);
            virtual ~ImpactZipper();

            /// Return the wire's waveform.  If the response functions
            /// are just field response (ie, instantaneous current)
            /// then the waveforms are expressed as current integrated
            /// over each sample bin and thus in units of charge.  If
            /// the response functions include electronics response
            /// then the waveforms are in units of voltage
            /// representing the sampling of the output of the FEE
            /// amplifiers.
 
            // fixme: this should be a forward iterator so that it may cal bd.erase() safely to conserve memory
            Waveform::realseq_t waveform(int wire) const;


	    
        };

    }  // Gen
}  // WireCell
#endif /* WIRECELL_IMPACTZIPPER */
