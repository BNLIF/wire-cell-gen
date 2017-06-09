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
    URNG m_rng;
    std::normal_distribution<double> m_n1, m_n2;
    Binormal(URNG rng, double m1, double s1, double m2, double s2)
        : m_rng(rng), m_n1(m1,s1), m_n2(m2,s2) {
    }

    std::complex<double> operator()() {
        double r = m_n1(m_rng);
        double i = m_n2(m_rng);
        return std::complex<double>(r,i);
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
        return std::bind(distribution, m_rng);
    }
    virtual realfunc_t normal(double mean, double sigma) {
        std::normal_distribution<double> distribution(mean, sigma);
        return std::bind(distribution, m_rng);
    }
    virtual realfunc_t uniform(double begin, double end) {
        std::uniform_real_distribution<double> distribution(begin, end);
        return std::bind(distribution, m_rng);
    }
    virtual intfunc_t range(int first, int last) {
        std::uniform_int_distribution<int> distribution(first, last);
        return std::bind(distribution, m_rng);
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


