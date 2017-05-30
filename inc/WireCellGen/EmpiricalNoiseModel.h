/** 
    A noise model based on empirically measured noise spectra. 

    It requires configuration file holding a list of dictionary which
    provide association between wire length and the noise spectrum.

    TBD: document JSON file format for providing spectra and any other parameters.

*/
    

#ifndef WIRECELLGEN_EMPERICALNOISEMODEL
#define WIRECELLGEN_EMPERICALNOISEMODEL

#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"

#include "WireCellUtil/Units.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace WireCell {
    namespace Gen {

        class EmpiricalNoiseModel : public IChannelSpectrum , public IConfigurable {
        public:
            EmpiricalNoiseModel(const std::string& spectra_file = "",
                                const double minium_frequency = 2.0*units::megahertz / 10000, // assuming 10k samples
                                const double nyquist_frequency = 2.0*units::megahertz / 2.0,
                                const double wire_length_scale = 1.0*units::cm,
                                const std::string anode_tn = "AnodePlane");

            virtual ~EmpiricalNoiseModel();

            /// IChannelSpectrum
            virtual const amplitude_t& operator()(int chid) const;

            /// IConfigurable
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;


            // Local methods

            // Resample the frequency space amplitude taken with the
            // given sampling frequency parameters and return a new
            // one that matches those for the class.
            amplitude_t resample(double minfreq, double nyqfreq, const amplitude_t& amp) const;

            // Return a new amplitude which is the interpolation
            // between those given in the spectra file.
            amplitude_t interpolate(double wire_length) const;


        private:
            IAnodePlane::pointer m_anode;

            std::string m_spectra_file;
            double m_minfreq, m_nyqfreq, m_wlres;
            std::string m_anode_tn;
            
            // associate wire length and its noise amplitude spectrum 
            typedef std::pair<double, amplitude_t> wire_amplitude_t;
            std::vector<wire_amplitude_t> m_wire_amplitudes;

            // cache amplitudes to the nearest integral distance.
            mutable std::unordered_map<int, int> m_chid_to_intlen;
            mutable std::unordered_map<int, amplitude_t> m_amp_cache;

        };

    }
}


#endif
