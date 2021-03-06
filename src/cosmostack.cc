// Created 8-Oct-2012 by Daniel Margala (University of California, Irvine) <dmargala@uci.edu>
// Stacks many Gaussian random fields on the field maximum (or minimum).

#include "cosmo/cosmo.h"
#include "likely/likely.h"

#include "boost/program_options.hpp"
#include "boost/format.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

namespace po = boost::program_options;
namespace lk = likely;

// Return the distance between 'x0' and 'x1' in a periodic dimension of length 'period'
double distance(double x0, double x1, double period){
    double dx(x1 - x0);
    return (dx > 0 ? (dx > period/2 ? dx - period : dx) : (dx < -period/2 ? dx + period : dx) );
}
 
int main(int argc, char **argv) {
    // Configure command-line option processing
    double spacing, xlos, ylos, zlos, binsize, rmin;
    long npairs;
    int nx,ny,nz,seed,nfields,nbins;
    std::string loadPowerFile, prefix;
    po::options_description cli("Stacks many Gaussian random fields on the field maximum (or minimum).");
    cli.add_options()
        ("help,h", "Prints this info and exits.")
        ("verbose", "Prints additional information.")
        ("spacing", po::value<double>(&spacing)->default_value(4),
            "Grid spacing in Mpc/h.")
        ("nx", po::value<int>(&nx)->default_value(76),
            "Grid size along x-axis.")
        ("ny", po::value<int>(&ny)->default_value(0),
            "Grid size along y-axis (or zero for ny=nx).")
        ("nz", po::value<int>(&nz)->default_value(0),
            "Grid size along z-axis (or zero for nz=ny).")
        ("load-power", po::value<std::string>(&loadPowerFile)->default_value(""),
            "Reads k,P(k) values (in h/Mpc units) to interpolate from the specified filename.")
        ("seed", po::value<int>(&seed)->default_value(511),
            "Random seed to use for GRF.")
        ("prefix", po::value<std::string>(&prefix)->default_value("stack"),
            "Prefix for output file names.")
        ("nfields", po::value<int>(&nfields)->default_value(1000),
            "Number of fields to stack.")
        ("fiducial",
            "Stack relative to box center instead of local maximum.")
        ("minimum",
            "Stack relative to the local minimum instead of local maximum.")
        ("snapshot",
            "Save snapshot of 1d projection every 10%%.")
        ("xlos", po::value<double>(&xlos)->default_value(1),
            "Line-of-sight direction x-component.")
        ("ylos", po::value<double>(&ylos)->default_value(0),
            "Line-of-sight direction y-component.")
        ("zlos", po::value<double>(&zlos)->default_value(0),
            "Line-of-sight direction z-component.")
        ("bin-size", po::value<double>(&binsize)->default_value(4),
            "Histogram bin size in Mpc/h.")
        ("bin-n", po::value<int>(&nbins)->default_value(37),
            "Number of histogram bins.")
        ("bin-min", po::value<double>(&rmin)->default_value(2),
            "Minimum bin (left edge) in Mpc/h.")
        ("test","Use the test fft generator.")
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
    bool verbose(vm.count("verbose")), fiducial(vm.count("fiducial")), snapshot(vm.count("snapshot")),
        minimum(vm.count("minimum"));

    double normlos(std::sqrt(xlos*xlos + ylos*ylos + zlos*zlos));
    if(normlos <= 0){
        std::cerr << "Invalid line-of-sight specification: norm must be > 0." << std::endl;
        return -2;
    }

    // Fill in any missing grid dimensions.
    if(0 == ny) ny = nx;
    if(0 == nz) nz = ny;
    
    double pi(4*std::atan(1));

    if(verbose) {
        std::cout << "Will stack " << nfields << " GRFs with dimensions (x,y,z) = " 
            << boost::format("(%d,%d,%d)") % nx % ny % nz 
            << boost::format(" using %.2f Mpc/h grid spacing.") % spacing << std::endl;
    }

    // Load a tabulated power spectrum for interpolation.
    cosmo::PowerSpectrumPtr power;
    if(0 < loadPowerFile.length()) {
        std::vector<std::vector<double> > columns(2);
        std::ifstream in(loadPowerFile.c_str());
        lk::readVectors(in,columns);
        in.close();
        if(verbose) {
            std::cout << "Read " << columns[0].size() << " rows from " << loadPowerFile
                << std::endl;
        }
        double twopi2(2*pi*pi);
        // rescale to k^3/(2pi^2) P(k)
        for(int row = 0; row < columns[0].size(); ++row) {
            double k(columns[0][row]);
            columns[1][row] *= k*k*k/twopi2;
        }
        // Create an interpolator of this data.
        lk::InterpolatorPtr iptr(new lk::Interpolator(columns[0],columns[1],"cspline"));
        // Use the resulting interpolation function for future power calculations.
        power = lk::createFunctionPtr(iptr);
    }
    else {
        std::cerr << "Missing required load-power filename." << std::endl;
        return -2;
    }

    // Initialize the random number source.
    lk::Random::instance()->setSeed(seed);

    // Create the generator.
    cosmo::AbsGaussianRandomFieldGeneratorPtr generator;
    if(vm.count("test")) {
        generator.reset(new cosmo::TestFftGaussianRandomFieldGenerator(power, spacing, nx, ny, nz));
    }
    else {
        generator.reset(new cosmo::FftGaussianRandomFieldGenerator(power, spacing, nx, ny, nz));
    }
    if(verbose) {
        std::cout << "Memory size = "
            << boost::format("%.1f Mb") % (generator->getMemorySize()/1048576.) << std::endl;
    }

    // Initialize histogram
    double rmax(rmin + nbins*binsize);
    boost::scoped_array<lk::WeightedAccumulator> xi(new lk::WeightedAccumulator[nbins]);
    boost::scoped_array<lk::WeightedAccumulator> xi2d(new lk::WeightedAccumulator[nbins*nbins]);
    // Collect extreme values of grfs
    lk::WeightedAccumulator extremeValues;
    // Line-of-sight direction
    double xparl(xlos/normlos), yparl(ylos/normlos), zparl(zlos/normlos);

    if(verbose) {
        std::cout << boost::format("Preparing 1-d and 2-d histograms from %.2f to %.2f Mpc/h with a bin size of %.2f Mpc/h") 
            % rmin % rmax % binsize << std::endl;
        std::cout << boost::format("Line-of-sight unit vector components: (%.4f,%.4f,%.4f)") 
            % xparl % yparl % zparl << std::endl;
    }

    lk::RandomPtr random = lk::Random::instance();
    random->setSeed(seed);

    for(int ifield = 0; ifield < nfields; ++ifield){
        // Generate Gaussian random field
        generator->generate();
        double extremeValue(generator->getField(0,0,0));
        std::vector<int> extremeIndex(3,0);
        if(!fiducial) {
            for(int ix = 0; ix < nx; ++ix){
                for(int iy = 0; iy < ny; ++iy){
                    for(int iz = 0; iz < nz; ++iz){
                        double value = generator->getField(ix,iy,iz);
                        if((minimum ? value < extremeValue : value > extremeValue)){
                            extremeValue = value;
                            extremeIndex[0] = ix;
                            extremeIndex[1] = iy;
                            extremeIndex[2] = iz;
                        }
                    }
                }
            }
        }
        else {
            // extremeIndex[0] = random->getInteger(0,nx-1);
            // extremeIndex[1] = random->getInteger(0,ny-1);
            // extremeIndex[2] = random->getInteger(0,nz-1);
            extremeIndex[0] = 0;
            extremeIndex[1] = 0;
            extremeIndex[2] = 0;
        }
        // Accumulate extreme value
        extremeValues.accumulate(generator->getField(extremeIndex[0],extremeIndex[1],extremeIndex[2]));
        // Fill 1-d, 2-d histograms
        for(int ix = 0; ix < nx; ++ix){
            for(int iy = 0; iy < ny; ++iy){
                for(int iz = 0; iz < nz; ++iz){
                    // Calculate distance from extreme value point on grid (wrap-around)
                    double dx(distance(ix,extremeIndex[0],nx)), 
                        dy(distance(iy,extremeIndex[1],ny)), 
                        dz(distance(iz,extremeIndex[2],nz));
                    //double dx(ix-extremeIndex[0]), dy(iy-extremeIndex[1]), dz(iz-extremeIndex[2]);
                    double dr(std::sqrt(dx*dx + dy*dy + dz*dz));
                    // Apply grid spacing
                    double r(spacing*dr);
                    double rparl(spacing*std::fabs(dx*xparl + dy*yparl + dz*zparl));
                    double rperp(std::sqrt(r*r-rparl*rparl));
                    // Look up field value
                    double value(generator->getField(ix,iy,iz));
                    // Accumulate value in appropriate bin
                    if(r < rmax && r >= rmin) {
                        xi[std::floor((r-rmin)/binsize)].accumulate(value);
                    }
                    if(rparl < rmax && rperp < rmax && rparl >= rmin && rperp >= rmin){
                        xi2d[std::floor((rperp-rmin)/binsize)+nbins*std::floor((rparl-rmin)/binsize)].accumulate(value);
                    }
                }
            }
        }
        if(nfields > 10 && (ifield+1) % (nfields/10) == 0) {
            // Print status message in 10% intervals
            if(verbose) {
                std::cout << "Generating " << ifield+1 << "..." << std::endl;
            }
            // Save 1d stack to file every 10%
            if(snapshot) {
                std::string outFilename((boost::format("%s.snap%d.1d.dat") % prefix % int((ifield+1.)/nfields*10)).str());
                std::ofstream out(outFilename.c_str());
                boost::format outFormat("%.2f %.10f %.10f %d");
                for(int index = 0; index < nbins; ++index) {
                    out << (outFormat % ((index+.5)*binsize+rmin)
                        % xi[index].mean() % xi[index].variance() % xi[index].count()) << std::endl;
                }
                out.close();
            }
        }
    }

    // Print extreme value mean, variance, and count
    if(verbose) {
        std::cout << boost::format("Extreme value mean, variance, count: %.6f %.6f %d")
            % extremeValues.mean() % extremeValues.variance() % extremeValues.count() << std::endl;
    }

    // Save 1d stack to file.
    {
        std::string outFilename(prefix+".1d.dat");
        std::ofstream out(outFilename.c_str());
        boost::format outFormat("%.2f %.10f %.10f %d");
        for(int index = 0; index < nbins; ++index) {
            out << (outFormat % ((index+.5)*binsize+rmin)
                % xi[index].mean() % xi[index].variance() % xi[index].count()) << std::endl;
        }
        out.close();
    } 

    // Save 2d stack to file.
    {
        std::string outFilename(prefix+".2d.dat");
        std::ofstream out(outFilename.c_str());
        boost::format outFormat("%.2f %.2f %.10f %.10f %d");
        for(int index = 0; index < nbins*nbins; ++index) {
            out << (outFormat % ((index%nbins+.5)*binsize+rmin) % ((index/nbins+.5)*binsize+rmin)
                % xi2d[index].mean() % xi2d[index].variance() % xi2d[index].count()) << std::endl;
        }
        out.close();
    }

    // Done
    return 0;
}