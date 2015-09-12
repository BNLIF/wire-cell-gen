#ifndef WIRECELL_DIFUSER
#define WIRECELL_DIFUSER

#include <boost/multi_array.hpp>

#include <vector>

namespace WireCell {

    /** A simple 2D histogram which is an array and its 2D bounds. */
    struct Diffusion {
	typedef boost::multi_array<double, 2> array_type;
	typedef array_type::index index;
	typedef std::shared_ptr<Diffusion> pointer;

	array_type array;
	double lmin, tmin, lmax, tmax, lbin, tbin;

	Diffusion(int nlong, int ntrans, double lmin, double tmin, double lmax, double tmax)
	    : array(boost::extents[nlong][ntrans])
	    , lmin(lmin), tmin(tmin), lmax(lmax), tmax(tmax)
	{
	    lbin = (lmax-lmin) / nlong;
	    tbin = (tmax-tmin) / ntrans;
	}
	
	// Size of diffusion patch in both directions.
	int lsize() const { return array.shape()[0]; }
	int tsize() const { return array.shape()[1]; }

	// Longitudinal position at index with extra offset 0.5 is bin center.
	double lpos(index ind, double offset=0.5) const {
	    return lmin + (ind+offset)*lbin; 
	}
	// Transverse position at index with extra offset 0.5 is bin center.
	double tpos(index ind, double offset=0.5) const {
	    return tmin + (ind+offset)*tbin; 
	}
    };

    /** Model longitudinal and transverse diffusion of drifted charge.
     *
     * The result is returned as a truncated 2D Gaussian distribution
     * defined on a grid covering a rectangular domain.  Each
     * direction is considered independently.
     */

    class Diffuser {
    public:

	typedef std::pair<double,double> bounds_type;


	/** Create a diffuser.
	 *
	 * \param lbinsize defines the grid spacing in the longitudinal direction.
	 * \param tbinsize defines the grid spacing in the transverse direction.
	 * \param lorigin defines a grid line in the longitudinal direction.
	 * \param torigin defines a grid line in the transverse direction.
	 * \param nsigma defines the number of sigma at which to truncate the Gaussian.
	 */
	Diffuser(double binsize_l, double binsize_t,
		 double origin_l=0.0, double origin_t=0.0,
		 double nsigma=3.0);

	~Diffuser();
	
	/** Diffuse a point charge.
	 *
	 * \param mean_l is the position in the longitudinal direction.
	 * \param mean_t is the position in the transverse direction.
	 * \param sigma_l is the Gaussian sigma in the longitudinal direction.
	 * \param sigma_t is the Gaussian sigma in the transverse direction.
	 * \param weight is the normalization (eg, amount of charge)
	 *
	 * Returns a 2D array of the diffused distribution.
	 *
	 * Use like:
	 *
	 * ```c++
	 *
	 * Diffuser diff(...);
	 * Diffusion::pointer smear = diff(mean_l,mean_t,sigma_l,sigma_t);
	 *
	 * Histogram hist; 
	 * for (int l_ind=0; l_ind < smear->lsize(); ++l_ind) {
	 *   for (int t_ind=0; t_ind < smear->tsize(); ++t_ind) {
	 *      hist.fill(smear->lval(l_ind), smear->tval(t_ind), smear->array[l_ind][t_ind]);
	 *   }
	 * } 
	 *
	 * ```
	 */
	Diffusion::pointer operator()(double mean_l, double mean_t,
				      double sigma_l, double sigma_t,
				      double weight = 1.0);

	/** Return the +/- nsigma*sigma bounds around mean enlarged to fall on bin edges.
	 */
	bounds_type bounds(double mean, double sigma, double binsize, double origin=0.0);


	/// Internal function, return a 1D distribution of mean/sigma
	/// binned with zeroth bin at origin and given binsize.
	std::vector<double> oned(double mean, double sigma,
				 double binsize,
				 const Diffuser::bounds_type& bounds);



    private:
	const double m_origin_l, m_origin_t, m_binsize_l, m_binsize_t;
	const double m_nsigma;

    };

}

#endif

