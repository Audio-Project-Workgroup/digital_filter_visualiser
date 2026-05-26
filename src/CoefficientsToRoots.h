#pragma once

#include "FilterState.h"
#include <vector>
#include <utility>

#define DEBUG_C2R

class CoefficientsToRoots
{
	public:
		static std::vector<std::pair<c128, int>> GramSchmidt(std::vector<double> coefs); // TODO change return type to bool --> for when there is a pole outside the unit. Check will be done outside of this.
	private:

		// TODO Finetune these parameters
	    static constexpr double Epsilon = 1e-3;
	    static constexpr int MaxIterations = 100000;
		static constexpr double tolerance = 1e-4; // threshold for considering two roots with negligible diff the same. Expressed in % after scaling differences, since zeros can much more than 1.0.

		static void printCheck(const std::vector<std::vector<double>> &matrix);
		static std::vector<std::pair<c128, int>> extractRoots(const std::vector<std::vector<double>>& M, int n);
};