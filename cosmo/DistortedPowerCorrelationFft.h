// Created 20-Mar-2014 by Michael Blomqvist (University of California, Irvine) <cblomqvi@uci.edu>

#ifndef COSMO_DISTORTED_POWER_CORRELATION_FFT
#define COSMO_DISTORTED_POWER_CORRELATION_FFT

#include "cosmo/types.h"
#include "likely/types.h"
#include "likely/function.h"

#include "boost/smart_ptr.hpp"

#include <vector>
#include <iosfwd>

namespace likely{class BiCubicInterpolator;}
namespace cosmo {
	class DistortedPowerCorrelationFft {
	// Represents the 3D correlation function corresponding to an isotropic power
	// spectrum P(k) that is distorted by a multiplicative function D(k,mu_k).
	// This class is optimized for the case where D(k,mu_k) is allowed to
	// change (e.g., as its internal parameters are changed) but that after
	// each change, the correlation function xi(r,mu) needs to be evaluated
	// many times. The normal usage is:
	//
	//    - transform() each time D(k,mu_k) changes internally
	//      - call getCorrelation(r,mu) many times
	//
	public:
		// Creates a new distorted power correlation function using the specified
		// isotropic power P(k) and distortion function D(k,mu).
		DistortedPowerCorrelationFft(likely::GenericFunctionPtr power, KMuPkFunctionCPtr distortion, KMuPkFunctionCPtr imdistortion, bool imagpart,
			double spacing, int nx, int ny, int nz);
		virtual ~DistortedPowerCorrelationFft();
		// Returns the value of P(k,mu) = P(k)*D(k,mu).
		double getPower(double k, double mu) const;
        double getImPower(double k, double mu) const;
		// Returns the correlation function xi(r,mu).
		double getCorrelation(double r, double mu) const;
		// Transforms the k-space power spectrum to r space.
		void transform();
		// Returns the memory size in bytes required for this transform or zero if this
        // information is not available.
        virtual std::size_t getMemorySize() const;
	private:
		class Implementation;
		boost::scoped_ptr<Implementation> _pimpl;
		likely::GenericFunctionPtr _power;
		KMuPkFunctionCPtr _distortion, _imdistortion;
        bool _imagpart;
		std::vector<double> _kxgrid, _kygrid, _kzgrid;
		boost::shared_array<double> _xi;
		double _spacing, _norm;
		int _nx, _ny, _nz;
		likely::BiCubicInterpolator *_bicubicinterpolator;
	}; // DistortedPowerCorrelationFft

} // cosmo

#endif // COSMO_DISTORTED_POWER_CORRELATION_FFT
