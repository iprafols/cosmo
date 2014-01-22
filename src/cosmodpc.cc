// Created 22-Jan-2014 by David Kirkby (University of California, Irvine) <dkirkby@uci.edu>
// A driver and test program for the DistortedPowerCorrelation class.
// Calculates the 3D correlation function corresponding to a distorted power spectrum.

#include "cosmo/cosmo.h"
#include "likely/likely.h"
#include "likely/function_impl.h"

#include "boost/program_options.hpp"
#include "boost/bind.hpp"

#include <iostream>
#include <fstream>

namespace po = boost::program_options;
namespace lk = likely;

class RedshiftSpaceDistortion {
public:
    RedshiftSpaceDistortion(double beta, double bias = 1) : _beta(beta), _bias(bias) { }
    double operator()(double k, double mu) const {
        double tmp = _bias*(1 + _beta*mu*mu);
        return tmp*tmp;
    }
private:
    double _beta,_bias;
};

int main(int argc, char **argv) {
    
    // Configure command-line option processing
    po::options_description cli("Cosmology distorted power correlation function");
    std::string input,output;
    int ellMax,nr,repeat;
    double rmin,rmax,relerr,abserr,abspow,margin,maxRelError;
    double beta,bias;
    cli.add_options()
        ("help,h", "prints this info and exits.")
        ("verbose", "prints additional information.")
        ("input,i", po::value<std::string>(&input)->default_value(""),
            "name of filename to read k,P(k) values from")
        ("output,o", po::value<std::string>(&output)->default_value(""),
            "base name for saving results")
        ("rmin", po::value<double>(&rmin)->default_value(10.),
            "minimum value of comoving separation to use")
        ("rmax", po::value<double>(&rmax)->default_value(200.),
            "maximum value of comoving separation to use")
        ("nr", po::value<int>(&nr)->default_value(191),
            "number of points spanning [rmin,rmax] to use")
        ("ell-max", po::value<int>(&ellMax)->default_value(4),
            "maximum multipole to use for transforms")
        ("symmetric", "distortion is symmetric in mu")
        ("beta", po::value<double>(&beta)->default_value(1.4),
            "redshift-space distortion parameter")
        ("bias", po::value<double>(&bias)->default_value(1.0),
            "linear tracer bias")
        ("relerr", po::value<double>(&relerr)->default_value(1e-2),
            "relative error termination goal")
        ("abserr", po::value<double>(&abserr)->default_value(1e-3),
            "absolute error termination goal")
        ("abspow", po::value<double>(&abspow)->default_value(0.),
            "absolute error weighting power")
        ("max-rel-error", po::value<double>(&maxRelError)->default_value(1e-3),
            "maximum allowed relative error for power-law extrapolation of input P(k)")
        ("optimize", "optimizes transform FFTs")
        ("bypass", "bypasses the termination test for transforms")
        ("repeat", po::value<int>(&repeat)->default_value(1),
            "number of times to repeat identical transform")
        ;
    // do the command line parsing now
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, cli), vm);
        po::notify(vm);
    }
    catch(std::exception const &e) {
        std::cerr << "Unable to parse command line options: " << e.what() << std::endl;
        return -1;
    }
    if(vm.count("help")) {
        std::cout << cli << std::endl;
        return 1;
    }
    bool verbose(vm.count("verbose")), symmetric(vm.count("symmetric")),
        optimize(vm.count("optimize")), bypass(vm.count("bypass"));

    if(input.length() == 0) {
        std::cerr << "Missing input filename." << std::endl;
        return 1;
    }

    cosmo::TabulatedPowerCPtr power =
        cosmo::createTabulatedPower(input,true,true,maxRelError,verbose);
    lk::GenericFunctionPtr PkPtr =
        lk::createFunctionPtr<const cosmo::TabulatedPower>(power);

    boost::shared_ptr<RedshiftSpaceDistortion> rsd(new RedshiftSpaceDistortion(beta,bias));
    cosmo::RMuFunctionCPtr distPtr(new cosmo::RMuFunction(boost::bind(&RedshiftSpaceDistortion::operator(),rsd,_1,_2)));

    try {
    	cosmo::DistortedPowerCorrelation dpc(PkPtr,distPtr,rmin,rmax,nr,ellMax,
            symmetric,relerr,abserr,abspow);
        std::cout << "ctor done" << std::endl;
        dpc.initialize();
        if(verbose) {
            std::cout << "initialized" << std::endl;
        }
        bool ok;
        for(int i = 0; i < repeat; ++i) {
            ok = dpc.transform(bypass);
        }
        if(!ok) {
            std::cerr << "Transform fails termination test." << std::endl;
        }
        if(output.length() > 0) {
            std::ofstream out(output.c_str());
            // ...
            out.close();
        }
    }
    catch(std::runtime_error const &e) {
        std::cerr << "ERROR: exiting with an exception:\n  " << e.what() << std::endl;
        return -1;
    }

    return 0;
}