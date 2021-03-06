// Created 13-Jan-2014 by David Kirkby (University of California, Irvine) <dkirkby@uci.edu>

#ifndef COSMO_TABULATED_POWER
#define COSMO_TABULATED_POWER

#include "cosmo/types.h"
#include "likely/types.h"

#include "boost/smart_ptr.hpp"

#include <iosfwd>

namespace cosmo {
	class TabulatedPower {
	// Represents a power spectrum P(k) derived from tabulated values of P(k) that
	// are assumed (but not required) to be (approximately) logarithmically spaced
	// in k. Supports optional power-law extrapolation above and below the limits
	// of the tabulated range of k.
	public:
		// Creates a new tabulated power object using the specified vectors of k
		// and P(k) which must be of the same size, with k > 0 and increasing.
		// Uses spline interpolation in log(k) and P(k) to evaluate P(k) at values
		// within the tabulated range of k. Use the boolean options to enable
		// power-law extrapolations below and above the range of tabulated k values.
		// Extrapolates below k0 using power-law coefficients obtained with k0 and k2
		// and checks that the resulting interpolation to k1 is sufficiently accurate.
		// Similarly for extrapolation above the maximum k.
		TabulatedPower(std::vector<double> const &k, std::vector<double> const &Pk,
			bool extrapolateBelow = false, bool extrapolateAbove = false,
			double maxRelError = 1e-3, bool verbose = false);
		virtual ~TabulatedPower();
		// Evaluates P(k) for the specified k. Always returns 0 for k <= 0.
		double operator()(double k) const;
		// Returns the interpolation limits
		double getKMin() const;
		double getKMax() const;
		// Creates a new tabulated power object for the difference between this power
		// and another power. In case the other power is tabulated on a different k grid,
		// our grid will be used for the result. Uses the same options to create the new
		// object that we were created with. The returned object has no dependencies on
		// this object or the other object.
		TabulatedPowerCPtr createDelta(TabulatedPowerCPtr other, bool verbose = false) const;

	private:
		double _kmin, _kmax, _maxRelError;
		class PowerLawExtrapolator;
		boost::scoped_ptr<PowerLawExtrapolator> _extrapolateBelow, _extrapolateAbove;
		likely::InterpolatorPtr _interpolator;
	}; // TabulatedPower

	inline double TabulatedPower::getKMin() const { return _kmin; }
	inline double TabulatedPower::getKMax() const { return _kmax; }

	// Creates a new tabulated power object using k and P(k) vectors read from
	// the specified filename. Additional options are as described above.
	TabulatedPowerCPtr createTabulatedPower(std::string const &filename,
			bool extrapolateBelow = false, bool extrapolateAbove = false,
			double maxRelError = 1e-3, bool verbose = false);

} // cosmo

#endif // COSMO_TABULATED_POWER
