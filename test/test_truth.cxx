#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellGen/ImpactZipper.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/IRandom.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellUtil/ExecMon.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Response.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include <iostream>

using namespace WireCell;
using namespace std;

const double drift_speed = 1.114*units::mm/units::us;
const int nticks = 9600;
const double trk_time = 100*units::us;
const double tick = 0.5*units::us;
const double readout_time= nticks*tick;
Binning tbins(nticks,trk_time,trk_time+readout_time);

const double DL = 7.2*units::centimeter2/units::second;
const double DT = 12.0*units::centimeter2/units::second;
//const double DL = 0.0*units::centimeter2/units::second;
//const double DT = 0.0*units::centimeter2/units::second;
const int ndiffusion_sigma = 3.0;

TH2F *U = new TH2F("U","U Plane; Time[#mus]; Channel",tbins.nbins(),tbins.min()/units::us,tbins.max()/units::us,2400,0,2400);
TH2F *V = new TH2F("V","V Plane; Time[#mus]; Channel",tbins.nbins(),tbins.min()/units::us,tbins.max()/units::us,2400,2400,4800);
TH2F *Y = new TH2F("Y","Y Plane; Time[#mus]; Channel",tbins.nbins(),tbins.min()/units::us,tbins.max()/units::us,3456,4800,8256);

// --- induction filter ---
double nIndWireBins = 2400.0;
double indMaxFreq = 1.0;
double indSigma = 1./std::sqrt(3.1415926)*1.4;
double indPower = 2.0;
bool indFlag = false;
Binning indBins(nIndWireBins,0.0,indMaxFreq);
Response::HfFilter hf_ind(indSigma,indPower,indFlag);
auto indTruth = hf_ind.generate(indBins);

// --- collection filter ---
double nColWireBins = 3456.0;
double colMaxFreq = 1.0;
double colSigma = 1./std::sqrt(3.1415926)*3.0;
double colPower = 2.0;
bool colFlag = false;
Binning colBins(nColWireBins,0.0,colMaxFreq);
Response::HfFilter hf_col(colSigma,colPower,colFlag);
auto colTruth = hf_col.generate(colBins);

// --- time filter ---
double timeMaxFreq = 1.0*units::megahertz;
Binning timeBins(tbins.nbins()/2.0,0.0,timeMaxFreq);
double timeSigma = 0.1141*units::megahertz;
double timePower = 2.0;
bool timeFlag = true;
Response::HfFilter hf_time(timeSigma,timePower,timeFlag);
auto timeTruth = hf_time.generate(timeBins);

void test_track(double charge, double track_time, const Ray& track_ray,
		double stepsize, IRandom::pointer fluctuate, IAnodePlane::pointer iap){

  // --- truth type ---
  int type = 1; // 0=bare; 1=unit; 2=fractional
  const double pitch_range = 20*3*units::mm;
  
  for(auto face : iap->faces()){
    for(auto plane : face->planes()){
      const Pimpos* pimp = plane->pimpos();

      // ### drift & diffuse charge ###
      
      Gen::BinnedDiffusion bd(*pimp, tbins, ndiffusion_sigma, fluctuate);
      auto track_start = track_ray.first;
      auto track_dir = ray_unit(track_ray);
      auto track_length = ray_length(track_ray);
      const double xstop = pimp->origin()[0];
      const auto rbins = pimp->region_binning();
      const auto ibins = pimp->impact_binning();

      int stepCtr = 0;
      for(double dist=0.0; dist<track_length; dist+=stepsize){
	stepCtr++;
	auto pt = track_start + dist*track_dir;
	const double delta_x = pt.x() - xstop;
	const double drift_time = delta_x / drift_speed;
	pt.x(xstop);
	const double time = track_time+drift_time;
	const double pitch = pimp->distance(pt);
	Assert(rbins.inside(pitch));
	Assert(ibins.inside(pitch));
	Assert(tbins.inside(time));
	const double tmpcm2 = 2*DL*drift_time/units::centimeter2;
	const double sigmaL = sqrt(tmpcm2)*units::centimeter / drift_speed;
	const double sigmaT = sqrt(2*DT*drift_time/units::centimeter2)*units::centimeter2;
	auto depo = std::make_shared<SimpleDepo>(time,pt,charge);
	bd.add(depo,sigmaL,sigmaT);
      }

      // ### charge at anode plane ###

      auto& wires = plane->wires();
      const int numwires = pimp->region_binning().nbins();
      for(int iwire = 0; iwire<numwires; iwire++){
	const double wire_pos = rbins.center(iwire);
	const int min_impact = ibins.edge_index(wire_pos - 0.5*pitch_range);
	const int max_impact = ibins.edge_index(wire_pos + 0.5*pitch_range);
	const int nsamples = tbins.nbins();
	Waveform::compseq_t total_spectrum(nsamples, Waveform::complex_t(0.0,0.0));

	for(int imp = min_impact; imp<=max_impact; imp++){
	  auto id = bd.impact_data(imp);
	  if(!id){ continue; }

	  if(type == 0){
	    const Waveform::compseq_t& charge_spectrum = id->spectrum();
	    if(charge_spectrum.empty()){ continue; }
	    Waveform::increase(total_spectrum, charge_spectrum);
	  }
	  if(type == 1){
	    const Waveform::compseq_t& charge_spectrum = id->spectrum();
	    if(charge_spectrum.empty()){ continue; }
	    Waveform::compseq_t conv_spectrum(nsamples, Waveform::complex_t(0.0,0.0));
	    for(int ind=0; ind<nsamples; ind++){
	      if(wires[iwire]->channel()<4800){
		conv_spectrum[ind] = charge_spectrum[ind]*timeTruth[ind]*indTruth[iwire];
	      }
	      else{
		conv_spectrum[ind] = charge_spectrum[ind]*timeTruth[ind]*colTruth[iwire];
	      }
	    }
	    Waveform::increase(total_spectrum, conv_spectrum);
	  }
	  if(type == 2){
	    Waveform::compseq_t& weightcharge_spectrum = id->weight_spectrum();
	    if(weightcharge_spectrum.empty()){ continue; }
	    Waveform::compseq_t conv_spectrum(nsamples, Waveform::complex_t(0.0,0.0));
	    for(int ind=0; ind<nsamples; ind++){
	      if(wires[iwire]->channel()<4800){
		conv_spectrum[ind] = weightcharge_spectrum[ind]*timeTruth[ind]*indTruth[iwire];
	      }
	      else{
		conv_spectrum[ind] = weightcharge_spectrum[ind]*timeTruth[ind]*colTruth[iwire];
	      }
	    }
	    Waveform::increase(total_spectrum, conv_spectrum);
	  }
	}
	bd.erase(0,min_impact);
	Waveform::realseq_t wave(nsamples,0.0);
	wave = Waveform::idft(total_spectrum);
	auto mm = Waveform::edge(wave);
	if(mm.first == (int)wave.size()){ continue; }
	ITrace::ChargeSequence charge(wave.begin()+mm.first,
				      wave.begin()+mm.second);

	// ### fill histograms ###

	int chid = wires[iwire]->channel();
	for(int i=0; i<(int)charge.size(); i++){
	  if(chid<2400){ U->SetBinContent(mm.first+i, chid, charge[i]); }
	  else if(chid >=2400 && chid<4800){ V->SetBinContent(mm.first+i, chid-2400, charge[i]); }
	  else{ Y->SetBinContent(mm.first+i, chid-4800, charge[i]); }
	}
      }      
    }
  }

  U->Write();
  V->Write();
  Y->Write();  
} // end test_track


#include "anode_loader.h"       // do not use

int main(int argc, char* argv[]){

    // ### configuration ###
    auto anode_tns = anode_loader("uboone");
    {
        auto icfg = Factory::lookup<IConfigurable>("Random");
        auto cfg = icfg->default_configuration();
        icfg->configure(cfg);
    }

    auto rng = Factory::lookup<IRandom>("Random");
    const char* me = argv[0];
    const std::string outfile = Form("%s.root", me);
    cerr << "Writing file: " << outfile << endl;
    TFile* rootfile = TFile::Open(outfile.c_str(),"RECREATE");

    auto iap = Factory::find_tn<IAnodePlane>(anode_tns[0]);

    // ### simulate track ###

    //Ray track_ray(Point(101*units::mm,0,1000*units::mm),
    //		Point(102*units::mm,0,1000*units::mm));
    Ray track_ray(Point(1000*units::mm,0,1000*units::mm),
                  Point(1000*units::mm,0,2000*units::mm));
    const double track_time = 1*units::ms;
    const double stepsize = 1*units::mm;
    const double charge = 5000*units::eplus;

    test_track(charge, track_time, track_ray, stepsize, rng, iap);
  
    rootfile->Close();
    return 0;
}
