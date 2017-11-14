#ifndef FPI_H
#define FPI_H

#include <cmath>
#include <DAQ.h>

typedef struct {
	double R = 0.875;	// [1] reflectivity
	double n = 1.0;		// [1] refractive index
	double d0 = 0.01;	// [m] mirror spacing
	double piezoConstant = 780.24e-9 / 16;	// [m/V] constant of the piezo element
} FPI_PARAMS;

const int c = 299792458;	// [m/s] speed of light in vacuum

typedef struct {
	double lambda = 780.24e-9;
	double f0 = c / lambda;	// current laser frequency
	double fa = 100e6;	// [Hz] frequency modulation amplitude
	double fm = 5000;	// [Hz] frequency modulation frequency
	int nrPeriods = 10;
} LASER_PARAMS;

FPI_PARAMS fpiParams;

LASER_PARAMS laserParams;

class FPI {
	public:
		double tau(double frequency, double voltage) {

			double d = fpiParams.d0 + fpiParams.piezoConstant * (voltage - 2.0);

			double delta = 4 * M_PI*frequency*fpiParams.n*d / c;

			return -1*pow((1 - fpiParams.R), 2) / (1 + pow(fpiParams.R, 2) - 2*fpiParams.R*cos(delta));
		};

		std::vector<double> getFrequencies(ACQUISITION_PARAMETERS acquisitionParameters) {
			std::vector<double> frequencies;
			frequencies.resize(acquisitionParameters.no_of_samples);

			for (int kk(0); kk < static_cast<int32_t>(acquisitionParameters.no_of_samples); kk++) {
				frequencies[kk] = laserParams.f0 + laserParams.fa  * cos(2 * M_PI * laserParams.fm * kk / (200e6 / pow(2.0, acquisitionParameters.timebase)));
			}
			return frequencies;
		};
};

#endif // FPI_H