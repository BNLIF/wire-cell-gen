#ifndef WIRECELL_DIFUSER
#define WIRECELL_DIFUSER

#include "WireCellUtil/BufferedHistogram2D.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Units.h"
#include "WireCellIface/IDepo.h"

#include <vector>

namespace WireCell {


    /** Model diffusion of drifted charge.
     *
     * Each point deposition is spread out in the longitudinal (X,
     * drift) and transverse (Y, wire pitch) directions.
     * 
     * The spread in either direction is taken to be independent and
     * Gaussian but truncated to the number of sigma.  
     *
     * The spread is placed on to a BufferedHistogram2D. 
     *
     * Charge is conserved unless it underflows the histogram.
     *
     * Note: caller must take care to sync the BufferedHistogram2D
     * with the depositions and to select suitable binning of the
     * BufferedHistogram2D.
     */

    class Diffuser { 		// fixme: make interface?
    public:


	/// Create a diffuser on the given buffered histogram (as
	/// described above).  Truncate the diffusion at the given
	/// number of sigma.
	Diffuser(BufferedHistogram2D& hist, int nsigma);
	~Diffuser();
	
	/// Diffuse the deposition onto the buffered histogram, return
	/// the low-X bound of the diffusion patch.
	double diffuse(double x, double y, double sigma_x, double sigma_y,
		       double charge = 1.0);

	/// Internal function, return a 1D distribution of mean/sigma
	/// binned with zeroth bin at origin and given binsize.
	std::vector<double> oned(double mean, double sigma,
				 double origin, double binsize) const;


    private:
	BufferedHistogram2D& m_hist;
	const int m_nsigma;
    };

}

#endif

