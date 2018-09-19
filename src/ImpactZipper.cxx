#include "WireCellGen/ImpactZipper.h"
#include "WireCellUtil/Testing.h"

#include <iostream>             // debugging.
using namespace std;

using namespace WireCell;
Gen::ImpactZipper::ImpactZipper(IPlaneImpactResponse::pointer pir, BinnedDiffusion& bd)
  :m_pir(pir), m_bd(bd), m_flag(0)
{

  // for (int i=0;i!=210;i++){
  //   double pos = -31.5 + 0.3*i+1e-9;0
  //   m_pir->closest(pos);
  // }
  
  
  if (m_flag==1){
    // arrange the field response (210 in total, pitch_range/impact)
    // number of wires nwires ... 
    m_num_group = std::round(m_pir->pitch()/m_pir->impact())+1; // 11
    m_num_pad_wire = std::round((m_pir->nwires()-1)/2.); // 10

    
    
    
    //std::cout << m_num_group << " " << m_num_pad_wire << std::endl;
    
    for (int i=0;i!=m_num_group;i++){
      double rel_cen_imp_pos ;
      if (i!=m_num_group-1){
	rel_cen_imp_pos = -m_pir->pitch()/2.+m_pir->impact()*i+1e-9;
      }else{
	rel_cen_imp_pos = -m_pir->pitch()/2.+m_pir->impact()*i-1e-9;
      }
      m_vec_impact.push_back(std::round(rel_cen_imp_pos/m_pir->impact()));
      std::map<int, IImpactResponse::pointer> map_resp; // already in freq domain

      for (int j=0;j!=m_pir->nwires();j++){
	map_resp[j-m_num_pad_wire] = m_pir->closest(rel_cen_imp_pos - (j-m_num_pad_wire)*m_pir->pitch());
	Waveform::compseq_t response_spectrum = map_resp[j-m_num_pad_wire]->spectrum();

	//	std::cout << i << " " << j << " " << rel_cen_imp_pos - (j-m_num_pad_wire)*m_pir->pitch()<< " " << response_spectrum.size() << std::endl;
      }
      //std::cout << m_vec_impact.back() << std::endl;
      // std::cout << rel_cen_imp_pos << std::endl;
      // std::cout << map_resp.size() << std::endl;
      m_vec_map_resp.push_back(map_resp);
      
      std::vector<std::tuple<int,int, double> > vec_charge; // ch, time, charge
      m_vec_vec_charge.push_back(vec_charge);
    }
    
    
    // now work on the charge part ...
    // trying to sampling ...
    m_bd.get_charge_vec(m_vec_vec_charge, m_vec_impact);
    
    // for (size_t i=0;i!=m_vec_vec_charge.size();i++){
    //   std::cout << m_vec_vec_charge[i].size() << std::endl;
    // }
    
    // length and width ...
    const int nsamples = m_bd.tbins().nbins();
    const auto pimpos = m_bd.pimpos();
    const auto rb = pimpos.region_binning();
    const int nwires = rb.nbins();
    
    //    std::cout << nwires << " " << nsamples << std::endl;
    std::pair<int,int> impact_range = m_bd.impact_bin_range(m_bd.get_nsigma());
    std::pair<int,int> time_range = m_bd.time_bin_range(m_bd.get_nsigma());
    // std::cout << impact_range.first << " " << impact_range.second << " " << time_range.first << " " << time_range.second << std::endl;
    int start_ch = std::floor(impact_range.first*1.0/(m_num_group-1))-1;
    int end_ch = std::ceil(impact_range.second*1.0/(m_num_group-1))+2;
    if ( (end_ch-start_ch)%2==1) end_ch += 1;
    int start_tick = time_range.first-1;
    int end_tick = time_range.second+2;
    if ( (end_tick-start_tick)%2==1 ) end_tick += 1;
    
    
    //  m_decon_data = Array::array_xxf::Zero(nwires+2*m_num_pad_wire,nsamples);    
    // for saving the accumulated wire data in the time frequency domain ...
    // adding no padding now, it make the FFT slower, need some other methods ... 
    
    int npad_wire =0;
    int ntotal_wires = pow(2,std::ceil(log(end_ch - start_ch + 2 * m_num_pad_wire)/log(2)));
    
    if (nwires == 2400){
      if (ntotal_wires > 2500)
	ntotal_wires = 2500;
      npad_wire= (ntotal_wires -end_ch + start_ch)/2;
    }else if (nwires ==3456){
      if (ntotal_wires > 3600)
	ntotal_wires = 3600;
      npad_wire= (ntotal_wires -end_ch + start_ch)/2;
      //      npad_wire=72; //3600
    }
    m_start_ch = start_ch - npad_wire;
    m_end_ch = end_ch + npad_wire;
    //std::cout << start_ch << " " << end_ch << " " << npad_wire << " " << start_tick << " " << end_tick << " " << m_start_ch << " " << m_end_ch << std::endl;
    
    int npad_time = 3000; // considering the RCRC 500 us 
    int ntotal_ticks = pow(2,std::ceil(log(end_tick - start_tick + npad_time)/log(2)));
    if (ntotal_ticks >9800 && nsamples <9800)
      ntotal_ticks = 9800;
    npad_time = ntotal_ticks - end_tick + start_tick;
    m_start_tick = start_tick;
    m_end_tick = end_tick + npad_time;

    // m_end_tick = 16384;//nsamples;
    // m_start_tick = 0;
    // // std::cout << m_start_tick << " " << m_end_tick << std::endl;
    // int npad_time = 0;
    // int ntotal_ticks = pow(2,std::ceil(log(nsamples + npad_time)/log(2)));
    // if (ntotal_ticks >9800 && nsamples <9800)
    //   ntotal_ticks = 9800
    // npad_time = ntotal_ticks - nsamples;
    // m_start_tick = 0;
    // m_end_tick = ntotal_ticks;
    
    
    Array::array_xxc acc_data_f_w = Array::array_xxc::Zero(end_ch-start_ch+2*npad_wire, m_end_tick - m_start_tick); 

    int num_double = (m_vec_vec_charge.size()-1)/2;
    
    // speed up version , first five
    for (int i=0;i!=num_double;i++){
      //if (i!=0) continue;
      // std::cout << i << std::endl;
      Array::array_xxc c_data = Array::array_xxc::Zero(end_ch-start_ch+2*npad_wire,m_end_tick - m_start_tick);

      // fill normal order
      for (size_t j=0;j!=m_vec_vec_charge.at(i).size();j++){
      	c_data(std::get<0>(m_vec_vec_charge.at(i).at(j))+npad_wire-start_ch,std::get<1>(m_vec_vec_charge.at(i).at(j))-m_start_tick) +=  std::get<2>(m_vec_vec_charge.at(i).at(j));
      }
      m_vec_vec_charge.at(i).clear();
      
      // fill reverse order
      int ii=num_double*2-i;
      for (size_t j=0;j!=m_vec_vec_charge.at(ii).size();j++){
    	c_data(end_ch+npad_wire-1-std::get<0>(m_vec_vec_charge.at(ii).at(j)),std::get<1>(m_vec_vec_charge.at(ii).at(j))-m_start_tick) +=  std::complex<float>(0,std::get<2>(m_vec_vec_charge.at(ii).at(j)));
      }
      m_vec_vec_charge.at(ii).clear();

      // Do FFT on time
      c_data = Array::dft_cc(c_data,0);
      // Do FFT on wire
      c_data = Array::dft_cc(c_data,1);

      // std::cout << i << std::endl;
      {
    	Array::array_xxc resp_f_w = Array::array_xxc::Zero(end_ch-start_ch+2*npad_wire,m_end_tick - m_start_tick);
    	{
    	  Waveform::compseq_t rs1 = m_vec_map_resp.at(i)[0]->spectrum();
    	  // do a inverse FFT
    	  Waveform::realseq_t rs1_t = Waveform::idft(rs1);
    	  // pick the first xxx ticks
    	  Waveform::realseq_t rs1_reduced(m_end_tick-m_start_tick,0);
    	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
    	    if (icol >= int(rs1_t.size())) break;
    	    rs1_reduced.at(icol) = rs1_t[icol];
    	  }
    	  // do a FFT
    	  rs1 = Waveform::dft(rs1_reduced);

    	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
    	    resp_f_w(0,icol) = rs1[icol];
    	  }
    	}
	
	 
    	for (int irow = 0; irow!=m_num_pad_wire;irow++){
    	  Waveform::compseq_t rs1 = m_vec_map_resp.at(i)[irow+1]->spectrum();
    	  Waveform::realseq_t rs1_t = Waveform::idft(rs1);
    	  Waveform::realseq_t rs1_reduced(m_end_tick-m_start_tick,0);
    	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
    	    if (icol >= int(rs1_t.size())) break;
    	    rs1_reduced.at(icol) = rs1_t[icol];
    	  }
    	  rs1 = Waveform::dft(rs1_reduced);
    	  Waveform::compseq_t rs2 = m_vec_map_resp.at(i)[-irow-1]->spectrum();
    	  Waveform::realseq_t rs2_t = Waveform::idft(rs2);
    	  Waveform::realseq_t rs2_reduced(m_end_tick-m_start_tick,0);
    	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
    	    if (icol >= int(rs2_t.size())) break;
    	    rs2_reduced.at(icol) = rs2_t[icol];
    	  }
    	  rs2 = Waveform::dft(rs2_reduced);
    	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
    	    resp_f_w(irow+1,icol) = rs1[icol];
    	    resp_f_w(end_ch-start_ch-1-irow+2*npad_wire,icol) = rs2[icol];
    	  }
    	}
    	//std::cout << i << std::endl;
	
    	// Do FFT on wire for response // slight larger
    	resp_f_w = Array::dft_cc(resp_f_w,1); // Now becomes the f and f in both time and wire domain ...
    	// multiply them together
    	c_data = c_data * resp_f_w;
      }
     

      // Do inverse FFT on wire
      c_data = Array::idft_cc(c_data,1);
      
      // Add to wire result in frequency
      acc_data_f_w += c_data;
      
    }

    // std::cout << "ABC : " << std::endl;
    
    // central region ...
    {
      int i = num_double;
       // fill response array in frequency domain

      Array::array_xxc data_f_w;
      {
    	Array::array_xxf data_t_w = Array::array_xxf::Zero(end_ch-start_ch+2*npad_wire,m_end_tick-m_start_tick);
    	// fill charge array in time-wire domain // slightly larger
    	for (size_t j=0;j!=m_vec_vec_charge.at(i).size();j++){
    	  data_t_w(std::get<0>(m_vec_vec_charge.at(i).at(j))+npad_wire-start_ch,std::get<1>(m_vec_vec_charge.at(i).at(j))-m_start_tick) +=  std::get<2>(m_vec_vec_charge.at(i).at(j));
	  // std::cout << std::get<1>(m_vec_vec_charge.at(i).at(j)) << std::endl;
	}
    	m_vec_vec_charge.at(i).clear();


	
    	// Do FFT on time
    	data_f_w = Array::dft_rc(data_t_w,0);
    	// Do FFT on wire
    	data_f_w = Array::dft_cc(data_f_w,1);
      }
      
      {
    	Array::array_xxc resp_f_w = Array::array_xxc::Zero(end_ch-start_ch+2*npad_wire,m_end_tick-m_start_tick);
	
    	{
    	  Waveform::compseq_t rs1 = m_vec_map_resp.at(i)[0]->spectrum();
	  // Array::array_xxc temp_resp_f_w = Array::array_xxc::Zero(2*m_num_pad_wire+1,nsamples);
	  // for (int icol = 0; icol != nsamples; icol++){
	  //   temp_resp_f_w(0,icol) = rs1[icol];
	  // }
	  // Array::array_xxf temp_resp_t_w = Array::idft_cr(temp_resp_f_w,0).block(0,0,2*m_num_pad_wire+1,m_end_tick-m_start_tick);
	  // temp_resp_f_w = Array::dft_rc(temp_resp_t_w,0);
	    
	  // do a inverse FFT
	  Waveform::realseq_t rs1_t = Waveform::idft(rs1);
	  // pick the first xxx ticks
	  Waveform::realseq_t rs1_reduced(m_end_tick-m_start_tick,0);
	  // std::cout << rs1.size() << " " << nsamples << " " << m_end_tick << " " <<  m_start_tick << std::endl;
	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
	    if (icol >= int(rs1_t.size())) break;
	    rs1_reduced.at(icol) = rs1_t[icol];
	    //  std::cout << icol << " " << rs1_t[icol] << std::endl;
	  }
	  // do a FFT
	  rs1 = Waveform::dft(rs1_reduced);

	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
	    //   std::cout << icol << " " << rs1[icol] << " " << temp_resp_f_w(0,icol) << std::endl;
	    resp_f_w(0,icol) = rs1[icol];
    	  }
	}
    	for (int irow = 0; irow!=m_num_pad_wire;irow++){
    	  Waveform::compseq_t rs1 = m_vec_map_resp.at(i)[irow+1]->spectrum();
	  Waveform::realseq_t rs1_t = Waveform::idft(rs1);
	  Waveform::realseq_t rs1_reduced(m_end_tick-m_start_tick,0);
	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
	    if (icol >= int(rs1_t.size())) break;
	    rs1_reduced.at(icol) = rs1_t[icol];
	  }
	  rs1 = Waveform::dft(rs1_reduced);
    	  Waveform::compseq_t rs2 = m_vec_map_resp.at(i)[-irow-1]->spectrum();
	  Waveform::realseq_t rs2_t = Waveform::idft(rs2);
	  Waveform::realseq_t rs2_reduced(m_end_tick-m_start_tick,0);
	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
	    if (icol >= int(rs2_t.size())) break;
	    rs2_reduced.at(icol) = rs2_t[icol];
	  }
	  rs2 = Waveform::dft(rs2_reduced);
	  for (int icol = 0; icol != m_end_tick-m_start_tick; icol++){
    	    resp_f_w(irow+1,icol) = rs1[icol];
    	    resp_f_w(end_ch-start_ch-1-irow+2*npad_wire,icol) = rs2[icol];
    	  }
    	  // for (int icol = 0; icol != nsamples; icol++){
    	  //   resp_f_w(irow+1,icol) = rs1[icol];
    	  //   resp_f_w(end_ch-start_ch-1-irow+2*npad_wire,icol) = rs2[icol];
    	  // }
    	}
    	// Do FFT on wire for response // slight larger
    	resp_f_w = Array::dft_cc(resp_f_w,1); // Now becomes the f and f in both time and wire domain ...
    	// multiply them together
    	data_f_w = data_f_w * resp_f_w;
      }
      
      // Do inverse FFT on wire
      data_f_w = Array::idft_cc(data_f_w,1);
      
      // Add to wire result in frequency
      acc_data_f_w += data_f_w;
    }
    
    //m_decon_data = Array::array_xxc::Zero(nwires,nsamples);
    //    if (npad_wire!=0){
    Array::array_xxc temp_m_decon_data = Array::idft_cc(acc_data_f_w,0);//.block(npad_wire,0,nwires,nsamples);
    Array::array_xxf real_m_decon_data = temp_m_decon_data.real();
    Array::array_xxf img_m_decon_data = temp_m_decon_data.imag().colwise().reverse();
    m_decon_data = real_m_decon_data + img_m_decon_data;
    
    // std::cout << real_m_decon_data(40,5182) << " " << img_m_decon_data(40,5182) << std::endl;
    //    std::cout << real_m_decon_data(40,5182-m_start_tick) << " " << img_m_decon_data(40,5182-m_start_tick) << std::endl;
    
    //}else{
    // Array::array_xxc temp_m_decon_data = Array::idft_cc(acc_data_f_w,0);
    //   Array::array_xxf real_m_decon_data = temp_m_decon_data.real();
    //   Array::array_xxf img_m_decon_data = temp_m_decon_data.imag().rowwise().reverse();
    //   m_decon_data = real_m_decon_data + img_m_decon_data;
    // }
        
    // // prepare FFT, loop 11 of them ... (older version)
    // for (size_t i=0;i!=m_vec_vec_charge.size();i++){
    //   // fill response array in frequency domain
    //   if (i!=10) continue;
      
    //   Array::array_xxc data_f_w;
    //   {
    // 	Array::array_xxf data_t_w = Array::array_xxf::Zero(nwires+2*npad_wire,nsamples);
    // 	// fill charge array in time-wire domain // slightly larger
    // 	for (size_t j=0;j!=m_vec_vec_charge.at(i).size();j++){
    // 	  data_t_w(std::get<0>(m_vec_vec_charge.at(i).at(j))+npad_wire,std::get<1>(m_vec_vec_charge.at(i).at(j))) +=  std::get<2>(m_vec_vec_charge.at(i).at(j));
    // 	}
    // 	m_vec_vec_charge.at(i).clear();
	
    // 	// Do FFT on time
    // 	data_f_w = Array::dft_rc(data_t_w,0);
    // 	// Do FFT on wire
    // 	data_f_w = Array::dft_cc(data_f_w,1);
    //   }
      
    //   {
    // 	Array::array_xxc resp_f_w = Array::array_xxc::Zero(nwires+2*npad_wire,nsamples);
    // 	{
    // 	  Waveform::compseq_t rs1 = m_vec_map_resp.at(i)[0]->spectrum();
    // 	  for (int icol = 0; icol != nsamples; icol++){
    // 	    resp_f_w(0,icol) = rs1[icol];
    // 	  }
    // 	}
    // 	for (int irow = 0; irow!=m_num_pad_wire;irow++){
    // 	  Waveform::compseq_t rs1 = m_vec_map_resp.at(i)[irow+1]->spectrum();
    // 	  Waveform::compseq_t rs2 = m_vec_map_resp.at(i)[-irow-1]->spectrum();
    // 	  for (int icol = 0; icol != nsamples; icol++){
    // 	    resp_f_w(irow+1,icol) = rs1[icol];
    // 	    resp_f_w(nwires-1-irow+2*npad_wire,icol) = rs2[icol];
    // 	  }
    // 	}
    // 	// Do FFT on wire for response // slight larger
    // 	resp_f_w = Array::dft_cc(resp_f_w,1); // Now becomes the f and f in both time and wire domain ...
    // 	// multiply them together
    // 	data_f_w = data_f_w * resp_f_w;
    //   }
      
    //   // Do inverse FFT on wire
    //   data_f_w = Array::idft_cc(data_f_w,1);
      
    //   // Add to wire result in frequency
    //   acc_data_f_w += data_f_w;
    // }
    // m_vec_vec_charge.clear();
    
    // // do inverse FFT on time for the final results ... 
    
    // if (npad_wire!=0){
    //   Array::array_xxf temp_m_decon_data = Array::idft_cr(acc_data_f_w,0);
    //   m_decon_data = temp_m_decon_data.block(npad_wire,0,nwires,nsamples);
    // }else{
    //   m_decon_data = Array::idft_cr(acc_data_f_w,0);
    // }
    
    
    //    std::cout << m_decon_data(40,5195-m_start_tick)/units::mV << " " << m_decon_data(40,5195-m_start_tick)/units::mV << std::endl;
    
      


    
    // int nrows = resp_f_w.rows();
    // int ncols = resp_f_w.cols();
    std::cout << m_decon_data.rows() << " " << m_decon_data.cols() << std::endl;
  }
}



Gen::ImpactZipper::~ImpactZipper()
{
  
}


Waveform::realseq_t Gen::ImpactZipper::waveform(int iwire) const
{

  if (m_flag==1){
    const int nsamples = m_bd.tbins().nbins();
    if (iwire < m_start_ch || iwire >= m_end_ch){
      return Waveform::realseq_t(nsamples, 0.0);
    }else{
      Waveform::realseq_t wf(nsamples, 0.0);
      for (int i=0;i!=nsamples;i++){
	if (i>=m_start_tick && i < m_end_tick){
	  wf.at(i) = m_decon_data(iwire-m_start_ch,i-m_start_tick);
	}else{
	  wf.at(i) = 1e-25;
	}
	//std::cout << m_decon_data(iwire-m_start_ch,i-m_start_tick) << std::endl;
      }
      return wf;
    }
  }else{
  //  std::cout << m_pir->pitch() << " " << m_pir->pitch_range() << " " << m_pir->nwires() << " " << m_pir->impact() << " " << std::endl;  
  const double pitch_range = m_pir->pitch_range();
  
  const auto pimpos = m_bd.pimpos();
  const auto rb = pimpos.region_binning();
  const auto ib = pimpos.impact_binning();
  const double wire_pos = rb.center(iwire);
  
  const int min_impact = ib.edge_index(wire_pos - 0.5*pitch_range);
  const int max_impact = ib.edge_index(wire_pos + 0.5*pitch_range);
  const int nsamples = m_bd.tbins().nbins();
  Waveform::compseq_t total_spectrum(nsamples, Waveform::complex_t(0.0,0.0));
  
  
  int nfound=0;
  const bool share=true;
  const Waveform::complex_t complex_one_half(0.5,0.0);
  
  // The BinnedDiffusion is indexed by absolute impact and the
  // PlaneImpactResponse relative impact.
  for (int imp = min_impact; imp <= max_impact; ++imp) {

    
    
      // ImpactData
      auto id = m_bd.impact_data(imp);
      if (!id) {
          // common as we are scanning all impacts covering a wire
          // fixme: is there a way to predict this to avoid the query?
          //std::cerr << "ImpactZipper: no data for absolute impact number: " << imp << std::endl;
          continue;
      }

      //      std::cout << "qx " << iwire << " " << imp << std::endl;
      
      const Waveform::compseq_t& charge_spectrum = id->spectrum();
      // for interpolation
      const Waveform::compseq_t& weightcharge_spectrum = id->weight_spectrum();
  
      if (charge_spectrum.empty()) {
          // should not happen
          std::cerr << "ImpactZipper: no charge for absolute impact number: " << imp << std::endl;
            continue;
        }
        if (weightcharge_spectrum.empty()) {
            // weight == 0, should not happen
            std::cerr << "ImpactZipper: no weight charge for absolute impact number: " << imp << std::endl;
            continue;
        }

        const double imp_pos = ib.center(imp);
        const double rel_imp_pos = imp_pos - wire_pos;
	//	std::cout << "IZ: " << " imp=" << imp << " imp_pos=" << imp_pos << " wire_pos=" << wire_pos << " rel_imp_pos=" << rel_imp_pos << std::endl;

        Waveform::compseq_t conv_spectrum(nsamples, Waveform::complex_t(0.0,0.0));
        if (share) {            // fixme: make a configurable option
            TwoImpactResponses two_ir = m_pir->bounded(rel_imp_pos);
            if (!two_ir.first || !two_ir.second) {
                //std::cerr << "ImpactZipper: no impact response for absolute impact number: " << imp << std::endl;
                continue;
            }
            // fixme: this is average, not interpolation.
            Waveform::compseq_t rs1 = two_ir.first->spectrum();            
            Waveform::compseq_t rs2 = two_ir.second->spectrum();
	    // std::cout << rs1.size() << " " << rs2.size() << std::endl;
            
            for (int ind=0; ind < nsamples; ++ind) {
                //conv_spectrum[ind] = complex_one_half*(rs1[ind]+rs2[ind])*charge_spectrum[ind];
            
                // linear interpolation: wQ*rs1 + (Q-wQ)*rs2
                conv_spectrum[ind] = weightcharge_spectrum[ind]*rs1[ind]+(charge_spectrum[ind]-weightcharge_spectrum[ind])*rs2[ind];
                /* debugging */
                /* if(iwire == 1000 && ind>1000 && ind<2000) { */
                /* std::cerr<<"rs1 spectrum: "<<imp<<"|"<<ind<<": "<<std::abs(rs1[ind])<<std::endl; */
                /* std::cerr<<"rs2 spectrum: "<<imp<<"|"<<ind<<": "<<std::abs(rs2[ind])<<std::endl; */
                /* std::cerr<<"rs1 charge spectrum "<<ind<<": "<<weightcharge_spectrum[ind]<<std::endl; */
                /* std::cerr<<"rs2 charge spectrum "<<ind<<": "<<charge_spectrum[ind]-weightcharge_spectrum[ind]<<std::endl; */
              /* //std::cerr<<"rs1 charge spectrum "<<ind<<": "<<complex_one_half*charge_spectrum[ind]<<std::endl; */
              /* //std::cerr<<"rs2 charge spectrum "<<ind<<": "<<complex_one_half*charge_spectrum[ind]<<std::endl; */
                /* } */
            }
        }
        else {
            auto ir = m_pir->closest(rel_imp_pos);
            if (! ir) {
                // std::cerr << "ImpactZipper: no impact response for absolute impact number: " << imp << std::endl;
                continue;
            }
            Waveform::compseq_t response_spectrum = ir->spectrum();
            for (int ind=0; ind < nsamples; ++ind) {
                conv_spectrum[ind] = response_spectrum[ind]*charge_spectrum[ind];
            }
        }

        ++nfound;
        // std::cerr << "ImpactZipper: found:"<<nfound<<" for absolute impact number " << imp
        //           << " csize=" << csize << " rsize=" << rsize << " rebin=" << rebinfactor
        //           << std::endl;

        Waveform::increase(total_spectrum, conv_spectrum);
    }
    //std::cerr << "ImpactZipper: found " << nfound << " in abs impact: ["  << min_impact << ","<< max_impact << "]\n";

    // Clear memory assuming next call is iwire+1.
    // fixme: this is a dumb way to go. Better to make an iterator.
    m_bd.erase(0, min_impact); 

    if (!nfound) {
    return Waveform::realseq_t(nsamples, 0.0);
    }
    
    auto waveform = Waveform::idft(total_spectrum);
    
    return waveform;
  }
}

