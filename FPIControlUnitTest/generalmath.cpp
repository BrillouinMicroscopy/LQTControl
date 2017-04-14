#include "stdafx.h"
#include "..\FPIControl\generalmath.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FPIControlUnitTest {		
	TEST_CLASS(UnitTest1) {
		public:
		
			TEST_METHOD(TestMethodMeanInt) {
				std::vector<int> vector;
				vector.push_back(1);
				vector.push_back(2);
				vector.push_back(3);
				Assert::AreEqual(2.0, generalmath::mean(vector));
			}

			TEST_METHOD(TestMethodMeanDouble) {
				std::vector<double> vector;
				vector.push_back(1.0);
				vector.push_back(2.0);
				vector.push_back(3.0);
				Assert::AreEqual(2.0, generalmath::mean(vector));
			}

			TEST_METHOD(TestMethodMeanComplex) {
				std::vector<std::complex<double>> vector;
				vector.push_back(std::complex<double>(1.0, 0.0));
				vector.push_back(std::complex<double>(2.0, 1.0));
				vector.push_back(std::complex<double>(3.0, 2.0));
				std::complex<double> expected = std::complex<double>(2.0, 1.0);
				std::complex<double> actual = generalmath::mean(vector);
				Assert::AreEqual(expected.real(), actual.real());
				Assert::AreEqual(expected.imag(), actual.imag());
			}
	};
}