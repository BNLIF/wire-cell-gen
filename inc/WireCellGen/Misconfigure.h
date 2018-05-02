/** This component will output a "misconfigured" trace for each input
 * trace.
 * 
 * It does this by filtering out an assumed electronics response
 * function and applying a new one.  
 *
 * Note, traces are "misconfigured" independently even if multiple
 * traces exist on the same channel.  Output traces will be sized to
 * the larger of the input trace length and the length of the response
 * function ("nsamples" configuration parameter).  Caller must take
 * care to properly handle any overlaps in time.
 *
 * This component does not honor frame/trace tags.  No tags will be
 * considered on input and none are placed on output.
 *
 */

#ifndef WIRECELLGEN_MISCONFIGURE
#define WIRECELLGEN_MISCONFIGURE

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellUtil/Waveform.h"

#include <unordered_set>

namespace WireCell {
    namespace Gen {

        class Misconfigure : public IFrameFilter, public IConfigurable {
        public:
            Misconfigure();
            virtual ~Misconfigure();

            // IFrameFilter
            virtual bool operator()(const input_pointer& in, output_pointer& out);

            // IConfigurable
            virtual WireCell::Configuration default_configuration() const;
            virtual void configure(const WireCell::Configuration& cfg);

        private:
            // The ratio of the FFTs of the misconfigured and nominal
            // electronics response functions.
            Waveform::compseq_t m_filter;


        };
    }
}

#endif
