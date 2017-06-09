/**
   Gen::Random is an IRandom which is implemented with standard C++ <random>.
 */

#ifndef WIRECELLGEN_RANDOM
#define WIRECELLGEN_RANDOM

#include "WireCellIface/IRandom.h"
#include "WireCellIface/IConfigurable.h"

namespace WireCell {
    namespace Gen {

        class Random : public IRandom, public IConfigurable {
        public:
            Random(const std::string& generator = "default",
                   const std::vector<unsigned int> seeds = {0,0,0,0,0});
            virtual ~Random() {}
            
            // IConfigurable interface
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

            /// Return a function sampling a binomial distribution
            virtual intfunc_t binomial(int max, double prob);

            /// Return a function sampling a Poisson distribution. 
            virtual intfunc_t poisson(double mean);

            /// Return a function sampling a normal distribution.
            virtual realfunc_t normal(double mean, double sigma);

            /// Return a function sampling a uniform real distribution in
            /// the semi-open [begin,end).
            virtual realfunc_t uniform(double begin, double end);

            /// Return a function sampling a uniform integer range between
            /// [first,last] inclusive.
            virtual intfunc_t range(int first, int last);

            /// Return a function sampling a 2D normal distribution.  Real
            /// part is first, imaginary part is second.  Note, the abs()
            /// gives a Rayleigh and the arg() gives a phase.
            virtual compfunc_t binormal(double mean1, double sigma1,
                                        double mean2, double sigma2);

        private:
            std::string m_generator;
            std::vector<unsigned int> m_seeds;
            IRandom* m_pimpl;
        };

    }
}
#endif
