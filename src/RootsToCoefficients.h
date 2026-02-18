#pragma once
#include "FilterState.h"

class RootsToCoefficients
{
public:
	static std::vector<double> CalculatePolynomialCoefficientsFrom(juce::OwnedArray<FilterRoot>& roots)
	{
		// Determine polinomial order

		int order = 0;
		for (const auto* root : roots)
			order += std::abs(root->order.get());

		// Sort roots by magnitude for reducing numerical mistakes

		struct RootIndexes
		{
			int index;
			double magnitude;
		};

		std::vector<RootIndexes> rootIndexes(roots.size());
		for (int i = 0; i < roots.size(); i++)
		{
			rootIndexes[i].index = i;
			rootIndexes[i].magnitude = std::abs(roots[i]->value.get());
		}

		std::sort(rootIndexes.begin(), rootIndexes.end(),
			[](const RootIndexes& a, const RootIndexes& b)
			{
				return a.magnitude < b.magnitude;
			});

		// Calculate coefficients

		std::vector<double> res(order + 1);
		res[0] = 1;
		int nonZeroCoeffCount = 1;

		for (int i = 0; i < roots.size(); i++)
		{
			auto* root = roots[rootIndexes[i].index];
			const double re = root->value.re.get();
			const double im = root->value.im.get();
			const int rootOrder = std::abs(root->order.get());
			if (im == 0) // One real root
			{
				const double a0 = -re;
				const double a1 = 1.0;
				for (int i = 0; i < rootOrder; i++)
				{
					for (int j = nonZeroCoeffCount - 1; j >= 0; j--)
					{
						double v = res[j];
						res[j + 1] += v * a1;
						res[j] = v * a0;
					}
					nonZeroCoeffCount += 1;
				}
			}
			else //(im != 0) => Two conjugate roots
			{
				const double a0 = re * re + im * im;
				const double a1 = -2.0 * re;
				const double a2 = 1.0;
				for (int i = 0; i < rootOrder; i++)
				{
					for (int j = nonZeroCoeffCount - 1; j >= 0; j--)
					{
						double v = res[j];
						res[j + 2] += v * a2;
						res[j + 1] += v * a1;
						res[j] = v * a0;
					}
					nonZeroCoeffCount += 2;
				}
			}
		}

		return res;
	}
};