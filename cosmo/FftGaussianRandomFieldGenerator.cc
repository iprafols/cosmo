// Created 12-Aug-2011 by David Kirkby (University of California, Irvine) <dkirkby@uci.edu>

#include "cosmo/FftGaussianRandomFieldGenerator.h"

#include "config.h"
#ifdef HAVE_LIBFFTW3
#include "fftw3.h"
//#define FFTW(X) fftw_ ## X // double transforms
#define FFTW(X) fftwf_ ## X // float transforms
//#define FFTW(X) fftwl_ ## X // long double transforms
typedef float FftwReal;
#endif

namespace local = cosmo;

namespace cosmo {
    struct FftGaussianRandomFieldGenerator::Implementation {
#ifdef HAVE_LIBFFTW3
        FFTW(complex) *data;
        FFTW(plan) plan;
#endif
    };
} // cosmo::

local::FftGaussianRandomFieldGenerator::FftGaussianRandomFieldGenerator(
PowerSpectrumPtr powerSpectrum, double spacing, int nx, int ny, int nz)
: AbsGaussianRandomFieldGenerator(powerSpectrum,spacing,nx,ny,nz),
_pimpl(new Implementation()), _halfz(nz/2+1)
{
#ifdef HAVE_LIBFFTW3
    _pimpl->data = 0;
#endif
}

local::FftGaussianRandomFieldGenerator::~FftGaussianRandomFieldGenerator() {
#ifdef HAVE_LIBFFTW3
    if(0 != _pimpl->data) {
        FFTW(destroy_plan)(_pimpl->plan);
        FFTW(free)(_pimpl->data);
    }
#endif
}

void local::FftGaussianRandomFieldGenerator::_generate(int seed) {
#ifdef HAVE_LIBFFTW3
    if(0 == _pimpl->data) {
        std::size_t nbuf((std::size_t)getNx()*getNy()*_halfz);
        _pimpl->data = (FFTW(complex)*)FFTW(malloc)(nbuf*sizeof(FFTW(complex)));
        FftwReal *realData = (FftwReal*)(_pimpl->data);
        _pimpl->plan = FFTW(plan_dft_c2r_3d)(getNx(),getNy(),getNz(),_pimpl->data,realData,FFTW_ESTIMATE);
    }
    // Fill the complex transform data here, making sure that the inverse FT is real, i.e.
    // data[(nx-x0)%nx,(ny-y)%ny,(nz-z)%nz] = data[x,y,z]*. Since we only provide transform
    // data for z < nz/2+1, this only constrains values with z=0 or (for nz even) z=nz/2,
    // which are unchanged under z -> (nz-z)%nz. I'm not sure if actually matters if we don't
    // bother symmetrizing the transform data -- should test this...
    for(int x = 0; x < getNx(); ++x) {
        for(int y = 0; y < getNy(); ++y) {
            for(int z = 0; z < _halfz; ++z) {
                std::size_t index(z+_halfz*(y+getNy()*x));
                _pimpl->data[index][0] = 0;
                _pimpl->data[index][1] = 0;
            }
        }
    }
    FFTW(execute)(_pimpl->plan);
#else
    throw RuntimeError("FftGaussianRandomFieldGenerator: package was not built with FFTW3.");
#endif    
}

double local::FftGaussianRandomFieldGenerator::_getFieldUnchecked(int x, int y, int z) const {
    FftwReal const *realData = (FftwReal*)(_pimpl->data);
    int index(z+2*_halfz*(y+getNy()*x));
    return (double)realData[index];
}

std::size_t local::FftGaussianRandomFieldGenerator::getMemorySize() const {
    return sizeof(*this) + (std::size_t)getNx()*getNy()*_halfz;
}
