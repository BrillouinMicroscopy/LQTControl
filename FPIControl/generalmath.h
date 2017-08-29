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

	static int32_t max(std::vector<int32_t> vector) {
		int32_t max = vector[0];
		for (int jj(0); jj < vector.size(); jj++) {
			if (vector[jj] > max) {
				max = vector[jj];
			}
		}
		return max;
	}

	static double min(std::vector<double> vector) {
		double min = vector[0];
		for (int jj(0); jj < vector.size(); jj++) {
			if (vector[jj] < min) {
				min = vector[jj];
			}
		}
		return min;
	}

	static int32_t min(std::vector<int32_t> vector) {
		int32_t min = vector[0];
		for (int jj(0); jj < vector.size(); jj++) {
			if (vector[jj] < min) {
				min = vector[jj];
			}
		}
		return min;
	}

	static double absSum(std::vector<double> vector) {
		double sum = 0;
		for (int jj(0); jj < vector.size(); jj++) {
			sum += abs(vector[jj]);
		}
		return sum;
	}

	static int32_t absSum(std::vector<int32_t> vector) {
		int32_t sum = 0;
		for (int jj(0); jj < vector.size(); jj++) {
			sum += abs(vector[jj]);
		}
		return sum;
	}

	static double floatingMean(std::vector<double> vector, size_t nrValues) {
		nrValues = (nrValues > vector.size()) ? vector.size() : nrValues;
		if (nrValues == 0) {
			return nan("1");
		}
		return std::accumulate(std::prev(std::end(vector), nrValues), std::end(vector), 0.0) / nrValues;
	}
};

#endif // GENERALMATH_H