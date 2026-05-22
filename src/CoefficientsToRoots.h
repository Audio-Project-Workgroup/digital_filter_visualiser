#pragma once

#include "FilterState.h"
#include <vector>

class CoefficientsToRoots
{
	public:
		static std::vector<c128> GramSchmidt(std::vector<double> coefs);
	private:

		// TODO Finetune these parameters
	    static constexpr double Epsilon = 1e-3;
	    static constexpr int MaxIterations = 100000;

		static void printCheck(const std::vector<std::vector<double>> &matrix);
		static std::vector<c128> extractRoots(const std::vector<std::vector<double>>& M, int n);
};