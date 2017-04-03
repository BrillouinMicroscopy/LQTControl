#ifndef FPI_H
#define FPI_H

#include <cmath>

typedef struct {
	double R = 0.875;	// [1] reflectivity
	double n = 1.0;		// [1] refractive index
	double d0 = 0.01;	// [m] mirror spacing
	double piezoConstant = 780.24e-9 / 8;	// [m/V] constant of the piezo element
} FPI_PARAMS;

const int c = 299792458;	// [m/s] speed of light in vacuum

typedef struct {
	double f0 = 299792458 / 780.24e-9;	// current laser frequency
	double fa = 100e6;	// [Hz] frequency modulation amplitude
	double fm = 5000;	// [Hz] frequency modulation frequency
	int nrPeriods = 10;
} LASER_PARAMS;

FPI_PARAMS fpiParams;

LASER_PARAMS laserParams;

class FPI {
	public:
		double tau(double frequency, double voltage) {

			double d = fpiParams.d0 + fpiParams.piezoConstant * (voltage + 2.4);

			double delta = 4 * M_PI*frequency*fpiParams.n*d / c;

			return pow((1 - fpiParams.R), 2) / (1 + pow(fpiParams.R, 2) - 2*fpiParams.R*cos(delta));
		};

		std::vector<double> getFrequencies(int nrSamples) {
			std::vector<double> frequencies;
			frequencies.resize(nrSamples);

			for (int kk(0); kk < nrSamples; kk++) {
				frequencies[kk] = laserParams.f0 + laserParams.fa  * cos(2 * M_PI * laserParams.nrPeriods * kk / (nrSamples - 1));
			}
			return frequencies;
		};
};

#endif // FPI_H