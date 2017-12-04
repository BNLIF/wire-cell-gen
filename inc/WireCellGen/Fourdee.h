#ifndef WIRECELL_FOURDEE_H
#define WIRECELL_FOURDEE_H

#include "WireCellIface/IApplication.h"
#include "WireCellIface/IConfigurable.h"

#include "WireCellIface/IDepoSource.h"
#include "WireCellIface/IDrifter.h"
#include "WireCellIface/IDuctor.h"
#include "WireCellIface/IFrameSource.h"
#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IFrameSink.h"

#include "WireCellUtil/Configuration.h"


namespace WireCell {
    namespace Gen {

        /**

           Fourdee is a Wire Cell Toolkit application class which
           handles a chain of "the 4 D's": deposition, drifting,
           "ducting" (induction response) and digitization.  There is
           an optional "5th D": "dissonance" which provides a source
           of noise to be summed.

           Each step provided by a model implemented as an instance of the
           associated component class.  A final sink is also provided if
           the digitization output is provided.

        */
        class Fourdee : public WireCell::IApplication, public WireCell::IConfigurable {

        public:
            Fourdee();
            virtual ~Fourdee();

            virtual void execute();

            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

        private:

            WireCell::IDepoSource::pointer m_depos;
            WireCell::IDrifter::pointer m_drifter;
            WireCell::IDuctor::pointer m_ductor;
            WireCell::IFrameSource::pointer m_dissonance;
            WireCell::IFrameFilter::pointer m_digitizer;
            WireCell::IFrameFilter::pointer m_filter;
            WireCell::IFrameSink::pointer m_output;
        };
    }
}

#endif
