#ifndef PDH_H
#define PDH_H

#include <cmath>
#include <complex>
#include "generalmath.h"

const std::complex<double> imaginary(0.0, 1.0);

class PDH {
	public:
		double getError(std::vector<double> data, std::vector<double> reference) {
			
			std::vector<double> tmp; 
			tmp.resize(data.size());

			for (int j(0); j < data.size(); j++) {
				tmp[j] = data[j] * reference[j];
			}
			
			return generalmath::mean(tmp);
		};

		int32_t getError(std::vector<int32_t> data, std::vector<int32_t> reference) {

			std::vector<int32_t> tmp;
			tmp.resize(data.size());

			for (int j(0); j < data.size(); j++) {
				tmp[j] = data[j] * reference[j];
			}

			return generalmath::mean(tmp);
		};
};

#endif // PDH_H