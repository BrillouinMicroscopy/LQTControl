#ifndef DFT_H
#define DFT_H

#include <cmath>
#include <complex>

typedef struct {
	std::vector<std::complex<double>> A1;	// amplitudes of the first harmonic 
	std::vector<std::complex<double>> A2;	// amplitudes of the second harmonic
} AMPLITUDES;

const std::complex<double> imaginary(0.0, 1.0);

class DFT {
	public:
		AMPLITUDES getAmplitudes(std::vector<double> data, double fa, double fm) {
			AMPLITUDES amplitudes;
			amplitudes.A1.resize(1);
			amplitudes.A2.resize(1);

			double t;
			std::complex<double> e1;
			std::complex<double> e2;

			for (int j(0); j < data.size(); j++) {
				t = j / fa;
				e1 = exp(-2.0*imaginary*M_PI*fm*t);
				e2 = exp(-4.0*imaginary*M_PI*fm*t);
				amplitudes.A1[0] += data[j] * e1;
				amplitudes.A2[0] += data[j] * e2;
			}
			amplitudes.A1[0] = 2.0 * amplitudes.A1[0] / std::complex<double>(data.size(), 0);
			amplitudes.A2[0] = 2.0 * amplitudes.A2[0] / std::complex<double>(data.size(), 0);

			return amplitudes;
		};
};

#endif // DFT_H