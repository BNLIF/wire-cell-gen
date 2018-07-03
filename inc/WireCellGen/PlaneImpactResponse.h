#ifndef WIRECELLGEN_PLANEIMPACTRESPONSE
#define WIRECELLGEN_PLANEIMPACTRESPONSE

#include "WireCellIface/IPlaneImpactResponse.h"
#include "WireCellIface/IConfigurable.h"

#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Units.h"

namespace WireCell {

    namespace Gen {

    /** The information about detector response at a particular impact
     * position (discrete position along the pitch direction of a
     * plane on which a response function is defined).  Note,
     * different physical positions may share the same ImpactResponse.
     */
    class ImpactResponse : public IImpactResponse {
        int m_impact;
	Waveform::compseq_t m_spectrum;

    public:
	ImpactResponse(int impact, const Waveform::compseq_t& spec)
            : m_impact(impact), m_spectrum(spec) {}

	/// Frequency-domain spectrum of response
	const Waveform::compseq_t& spectrum() const { return m_spectrum; }

        /// Corresponding impact number
        int impact() const { return m_impact; }

    };

    /** Collection of all impact responses for a plane */
    class PlaneImpactResponse : public IPlaneImpactResponse, public IConfigurable {
    public:

        /** Create a PlaneImpactResponse.

            Field response is assumed to be normalized in units of current.

            Pre-amplifier gain and peaking time is that of the FE
            electronics.  The preamp gain should be in units
            consistent with the field response.  If 0.0 then no
            electronics response will be convolved.

            A flat post-FEE amplifier gain can also be given to
            provide a global scaling of the output of the electronics.

            Fixme: field response should be provided by a component.
         */
	PlaneImpactResponse(int plane_ident = 0,
                            size_t nbins = 10000,
                            double tick = 0.5*units::us);
	~PlaneImpactResponse();

        // IConfigurable interface
        virtual void configure(const WireCell::Configuration& cfg);
        virtual WireCell::Configuration default_configuration() const;

	/// Return the response at the impact position closest to
	/// the given relative pitch location (measured relative
	/// to the wire of interest).
        virtual IImpactResponse::pointer closest(double relpitch) const;

	/// Return the two responses which are associated with the
	/// impact positions on either side of the given pitch
	/// location (measured relative to the wire of interest).
	virtual TwoImpactResponses bounded(double relpitch) const;

	double pitch_range() const { return 2.0*m_half_extent; }
	double pitch() const { return m_pitch; }
	double impact() const { return m_impact; }

	int nwires() const { return m_bywire.size(); }

        size_t nbins() const { return m_nbins; }

        /// not in the interface
	int nimp_per_wire() const { return m_bywire[0].size(); }
	typedef std::vector<int> region_indices_t;
	typedef std::vector<region_indices_t> wire_region_indicies_t;
	const wire_region_indicies_t& bywire_map() const { return m_bywire; }
	std::pair<int,int> closest_wire_impact(double relpitch) const;


    private:
        std::string m_frname;
        std::vector<std::string> m_others;

	int m_plane_ident;
        size_t m_nbins;
        double m_tick;

	wire_region_indicies_t m_bywire;

	std::vector<IImpactResponse::pointer> m_ir;
	double m_half_extent, m_pitch, m_impact;

        void build_responses();

    };

}}

#endif

