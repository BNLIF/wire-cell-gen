/** Digitizer converts voltage waveforms to integer ADC ones.
 * 
 * Resulting waveforms are still in floating-point form and should be
 * round()'ed and truncated to whatever integer representation is
 * wanted by some subsequent node.
 */

#ifndef WIRECELL_DIGITIZER
#define WIRECELL_DIGITIZER

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellUtil/Units.h"
#include <deque>

namespace WireCell {
    
    namespace Gen {

        class Digitizer : public IFrameFilter, public IConfigurable {
        public:
            Digitizer(int maxsamp=4095, float baseline=0.0*units::volt,
                      float vperadc=2.0*units::volt/4096);
            virtual ~Digitizer();       

            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

            // Voltage frame goes in, ADC frame comes out.
            virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);

        private:
            int m_max_sample;   // eg, 4095.
            float m_voltage_baseline;  // this number is subtracted to all voltage samples.
            float m_volts_per_adc;     // how many volts corresponds to one ADC count.
        };

    }
}
#endif
