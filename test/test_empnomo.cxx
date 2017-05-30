#include "WireCellGen/EmpiricalNoiseModel.h"
#include "WireCellUtil/Persist.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Units.h"

#include <cstdlib>

using namespace WireCell;

const std::string xins_data = R"XINDATA(
0		250.36781
0.009739706	250.3474
0.020698916	252.27959
0.031658128	254.21178
0.048886355	261.99835
0.06142439	273.70712
0.07709013	283.4531
0.09588356	291.23627
0.11310363	293.15485
0.13344322	287.24265
0.14595132	277.43542
0.16315505	267.618
0.17566042	255.85481
0.18659785	242.139
0.1991005	228.41982
0.20847544	216.6634
0.21785039	204.907
0.22722805	195.1066
0.23503233	179.4416
0.24910292	167.675
0.25690994	153.966
0.26785007	142.20619
0.28191522	126.527596
0.3022412	110.83538
0.31631178	99.06877
0.33038238	87.30217
0.34601817	75.53216
0.3616594	67.67415
0.372605	59.82634
0.39137667	51.961525
0.41015103	46.05271
0.42579228	38.1947
0.4508275	32.272274
0.46647692	30.282259
0.4852513	24.373442
0.50716156	22.36982
0.5321995	18.403395
0.5588054	16.389566
0.5901015	10.453537
0.6167074	8.439709
0.64331603	8.381879
0.68557405	6.3340335
0.7450467	2.2927704
0.77791613	2.221334
0.8686985	2.0240333
0.9015707	3.9085953
1		0
)XINDATA";

int main()
{
    std::vector<double> raw_frequency, raw_amplitude;
    for (auto line : String::split(xins_data, "\n")) {
        if (line.empty()) {
            continue;
        }
        auto fa = String::split(line, "\t");
        const double f = std::atof(fa[0].c_str()) * units::megahertz;
        const double a = std::atof(fa[1].c_str()) * units::volt/units::hertz; // is it?
        raw_frequency.push_back(f);
        raw_amplitude.push_back(a);
    }

    // slow and ugly constant resampling via interpolation
    const double nyqfreq = raw_frequency.back();
    const int nfreq = 50;
    std::vector<double> amplitude_mid(nfreq), amplitude_lo(nfreq), amplitude_hi(nfreq);
    const double freqstep = nyqfreq/(nfreq-1);
    for (int ind=0; ind<nfreq; ++ind) {
        const double f = freqstep*ind;
        for (size_t ri=1; ri<raw_frequency.size(); ++ri) {
            if (raw_frequency[ri] < f) { continue; }
            const double delta = raw_frequency[ri] - raw_frequency[ri-1];
            const double diff = f - raw_frequency[ri-1];
            const double mu = diff/delta;
            const double amp = raw_amplitude[ri-1]*(1-mu) + raw_amplitude[ri]*mu;
            amplitude_mid[ind] = amp;
            amplitude_hi[ind] = 2.0*amp;
            amplitude_lo[ind] = 0.5*amp;
            break;
        }
    }

    
    Json::Value jtop(Json::arrayValue);
    
    const double wire_length = 5*units::meter;

    Json::Value jlo(Json::objectValue);
    jlo["wire_length"] = wire_length;
    jlo["nyquist"] = nyqfreq;
    jlo["amplitude"] = Persist::iterable2json(amplitude_lo);
    jtop.append(jlo);

    Json::Value jmid(Json::objectValue);
    jmid["wire_length"] = wire_length;
    jmid["nyquist"] = nyqfreq;
    jmid["amplitude"] = Persist::iterable2json(amplitude_mid);
    jtop.append(jmid);

    Json::Value jhi(Json::objectValue);
    jhi["wire_length"] = wire_length;
    jhi["nyquist"] = nyqfreq;
    jhi["amplitude"] = Persist::iterable2json(amplitude_hi);
    jtop.append(jhi);

    Persist::dump("test_empnomo_noise_spectra.json", jtop, true);
    Gen::EmpiricalNoiseModel empnomo("test_empnomo_noise_spectra");

    return 0;
}
