/*
  Test out the IRandom implementation Gen::Random.
 */


#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IRandom.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellUtil/ExecMon.h"

#include <iostream>
#include <complex>

using namespace std;
using namespace WireCell;

double norm(std::complex<double> val) {
    return std::abs(val);
}
double norm(double val) {
    return val;
}
double norm(int val) {
    return (double)val;
}

template<typename NumType, int nbins=10, int nstars = 100, int ntries=100000>
void histify(std::function<NumType()> gen) {
    int hist[nbins+2]={};
    for (int count=0; count<ntries; ++count) {
        double num = norm(gen());
        ++num;                  // shift to accommodate under/overflow
        if (num<=0) num=0;
        if (num>nbins) num = nbins+1;
        ++hist[(int)(0.5+num)];
    }
    for (int bin=0; bin<nbins+2; ++bin) {
        int bar = (hist[bin]*nstars)/ntries;
        

        char sbin = '0'+(bin-1);
        if (bin == 0) {
            sbin = '-';
        }
        if (bin == nbins+1) {
            sbin = '+';
        }
        cout << sbin << ": " << std::string(bar,'*') << std::endl;
    }
}

void test_named(std::string generator_name)
{
    const std::string gen_random_name = "Random";
    auto rnd = Factory::lookup<IRandom>(gen_random_name);
    auto rndcfg = Factory::lookup<IConfigurable>(gen_random_name);

    {
        auto cfg = rndcfg->default_configuration();
        cfg["generator"] = generator_name;
        rndcfg->configure(cfg);
    }

    cout << "binomial(9,0.5)\n";
    histify(rnd->binomial(9,0.5));
        
    cout << "normal(5.0,3.0)\n";
    histify(rnd->normal(5.0, 3.0));

    cout << "uniform(0.0,10.0)\n";
    histify(rnd->uniform(0.0,10.0));
    cout << "uniform(2.0,5.0)\n";
    histify(rnd->uniform(2.0,5.0));

    cout << "range(0,10)\n";
    histify(rnd->range(0,10));
    cout << "range(2,4)\n";
    histify(rnd->range(2,4));

    cout << "binormal(2,1,2,1) (magnitude)\n";
    histify(rnd->binormal(2,1,2,1));
}

int main()
{
    ExecMon em("starting");
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellGen");
    em("plugged in");

    cout << "DEFAULT:\n";
    test_named("default");
    em("default generator");

    cout << "\nTWISTER:\n";
    test_named("twister");
    em("twister generator");

    cout << "\nBOGUS:\n";
    test_named("bogus");
    em("bogus generator");

    cout << em.summary() << endl;
    return 0;
}

