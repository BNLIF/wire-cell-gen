/*
  Implementation notes:

  Implementation of Random uses actually another indirection
  (pointer-to-implementation pattern).  This is done in order to allow
  for use of different C++ std engines and because the standard does
  not deem it necessary to have these engines follow an inheritance
  hierarchy.

  Also, added is "binormal" which requires a bit extra tooling.

 */

#include "WireCellGen/Random.h"

#include "WireCellUtil/NamedFactory.h"

#include <random>

WIRECELL_FACTORY(Random, WireCell::Gen::Random, WireCell::IRandom, WireCell::IConfigurable);

using namespace WireCell;

Gen::Random::Random(const std::string& generator,
                    const std::vector<unsigned int> seeds)
    : m_generator(generator)
    , m_seeds(seeds.begin(), seeds.end())
    , m_pimpl(nullptr)
{
}


template<typename URNG>
struct Binormal {
    URNG& m_rng;
    std::normal_distribution<double> m_n1, m_n2;
    Binormal(URNG& rng, double m1, double s1, double m2, double s2)
        : m_rng(rng), m_n1(m1,s1), m_n2(m2,s2) {
    }

    std::complex<double> operator()() {
        double r = m_n1(m_rng);
        double i = m_n2(m_rng);
        return std::complex<double>(r,i);
    }
        
};

template<typename Distro, typename URNG>
struct Bindit {
    URNG& m_rng;     // must be by reference to share common RN stream
    Distro m_dist;
    typedef typename Distro::result_type result_type;
    Bindit(Distro distro, URNG& rng) : m_rng(rng), m_dist(distro) {}
    result_type operator()() {
        return m_dist(m_rng);
    }
};

template<typename URNG>
class RandomT : public IRandom {
    URNG m_rng;
public:
    RandomT(std::vector<unsigned int> seeds) {
        std::seed_seq seed(seeds.begin(), seeds.end());
        m_rng.seed(seed);
    }

    virtual intfunc_t binomial(int max, double prob) {
        std::binomial_distribution<int> distribution(max, prob);
        return Bindit<std::binomial_distribution<int>, URNG>(distribution, m_rng);
    }
    virtual intfunc_t poisson(double mean) {
        std::poisson_distribution<int> distribution(mean);
        return Bindit<std::poisson_distribution<int>, URNG>(distribution, m_rng);
    }
    virtual realfunc_t normal(double mean, double sigma) {
        std::normal_distribution<double> distribution(mean, sigma);
        return Bindit<std::normal_distribution<double>, URNG>(distribution, m_rng);
        /* Note, this causes a compile warning which I think is a cosmetic bug in gcc.
../gen/src/Random.cxx: In member function ‘WireCell::IRandom::realfunc_t RandomT<URNG>::normal(double, double) [with URNG = std::mersenne_twister_engine<long unsigned int, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>; WireCell::IRandom::realfunc_t = std::function<double()>]’:
../gen/src/Random.cxx:79:42: warning: ‘distribution.std::normal_distribution<double>::_M_saved’ may be used uninitialized in this function [-Wmaybe-uninitialized]
         std::normal_distribution<double> distribution(mean, sigma);

         see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59800

         */

    }
    virtual realfunc_t uniform(double begin, double end) {
        std::uniform_real_distribution<double> distribution(begin, end);
        return Bindit<std::uniform_real_distribution<double>, URNG>(distribution, m_rng);
    }
    virtual intfunc_t range(int first, int last) {
        std::uniform_int_distribution<int> distribution(first, last);
        return Bindit<std::uniform_int_distribution<int>, URNG>(distribution, m_rng);
    }
    virtual compfunc_t binormal(double mean1, double sigma1,
                                double mean2, double sigma2) {
        return Binormal<URNG>(m_rng, mean1, sigma1, mean2, sigma2);
    }
};

void Gen::Random::configure(const WireCell::Configuration& cfg)
{
    auto jseeds = cfg["seeds"];
    if (not jseeds.isNull()) {
        std::vector<unsigned int> seeds;
        for (auto jseed : jseeds) {
            seeds.push_back(jseed.asInt());
        }
        m_seeds = seeds;
    }
    auto gen = get(cfg,"generator",m_generator);
    if (m_pimpl) {
        delete m_pimpl;
    }
    if (gen == "default") {
        m_pimpl = new RandomT<std::default_random_engine>(m_seeds);
    }
    else if (gen == "twister") {
        m_pimpl = new RandomT<std::mt19937>(m_seeds);
    }
    else {
        std::cerr << "Gen::Random::configure: warning: unknown random engine: \"" << gen << "\" using default\n";
        m_pimpl = new RandomT<std::default_random_engine>(m_seeds);
    }
}

WireCell::Configuration Gen::Random::default_configuration() const
{
    Configuration cfg;
    cfg["generator"] = m_generator;
    Json::Value jseeds(Json::arrayValue);
    for (auto seed : m_seeds) {
        jseeds.append(seed);
    }
    cfg["seeds"] = jseeds;
    return cfg;
}

IRandom::intfunc_t Gen::Random::binomial(int max, double prob)
{
    return m_pimpl->binomial(max, prob);
}
IRandom::intfunc_t Gen::Random::poisson(double mean)
{
    return m_pimpl->poisson(mean);
}

IRandom::realfunc_t Gen::Random::normal(double mean, double sigma)
{
    return m_pimpl->normal(mean, sigma);
}

IRandom::realfunc_t Gen::Random::uniform(double begin, double end)
{
    return m_pimpl->uniform(begin, end);
}

IRandom::intfunc_t Gen::Random::range(int first, int last)
{
    return m_pimpl->range(first, last);
}

IRandom::compfunc_t Gen::Random::binormal(double mean1, double sigma1,
                                        double mean2, double sigma2)
{
    return m_pimpl->binormal(mean1, sigma1, mean2, sigma2);
}


