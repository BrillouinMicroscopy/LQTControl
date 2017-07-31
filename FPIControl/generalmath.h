#ifndef GENERALMATH_H
#define GENERALMATH_H

#include <cstdlib>
#include <numeric>
#include <vector>
#include <array>
#include <complex>
#include <cmath>

class generalmath {
public:
	static double mean(std::vector<int> vector) {
		return std::accumulate(std::begin(vector), std::end(vector), 0.0) / vector.size();
	}

	static double mean(std::vector<double> vector) {
		return std::accumulate(std::begin(vector), std::end(vector), 0.0) / vector.size();
	}

	static std::complex<double> mean(std::vector<std::complex<double>> vector) {
		return std::accumulate(std::begin(vector), std::end(vector), std::complex<double>(0.0, 0.0)) / std::complex<double>(vector.size(), 0);
	}

	static double max(std::vector<double> vector) {
		double max = vector[0];
		for (int jj(0); jj < vector.size(); jj++) {
			if (vector[jj] > max) {
				max = vector[jj];
			}
		}
		return max;
	}

	static double absSum(std::vector<double> vector) {
		double sum = 0;
		for (int jj(0); jj < vector.size(); jj++) {
			sum += abs(vector[jj]);
		}
		return sum;
	}
};

#endif // GENERALMATH_H