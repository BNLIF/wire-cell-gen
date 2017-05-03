#ifndef WIRECELLGEN_NOISESOURCE
#define WIRECELLGEN_NOISESOURCE

#include "WireCellIface/IFrameSource.h"
#include "WireCellIface/IConfigurable.h"

#include <vector>

namespace WireCell {
    namespace Gen {

        class NoiseSource : public IFrameSource, public IConfigurable {
        public:
            NoiseSource();
            NoiseSource(const std::string& noise_spectrum_data_filename, double scale);
            virtual ~NoiseSource();

            /// IFrameSource 
            virtual bool operator()(IFrame::pointer& frame);

            /// IConfigurable
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

        private:
            std::string m_filename;
            double m_scale;

            std::vector<double> m_spectra;
            double m_nyquist;
        };
    }
}

#endif
