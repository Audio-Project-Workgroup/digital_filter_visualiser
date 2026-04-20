#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "FilterState.h"

class RootsToCoefficients
{
public:
	static std::vector<double> CalculatePolynomialCoefficientsFrom(
		juce::OwnedArray<FilterRoot>& roots,
		int minimalLength = 1,
		std::vector<int>* usedRootsPtr = nullptr)
	{
		std::vector<int> usedRoots;
		if (usedRootsPtr == nullptr)
			usedRoots.resize(static_cast<size_t>(roots.size()));
		else
			usedRoots = *usedRootsPtr;
		jassert(usedRoots.size() == static_cast<size_t>(roots.size()));
		jassert(minimalLength > 0);

		// Determine polinomial order

		int order = 0;
		for (int i = 0; i <roots.size(); i++)
		{
			auto* root = roots[i];
			int rootOrder = std::abs(root->order.get());
			bool isReal = root->isReal();//root->value.im.get() == 0;
			order += (isReal ? 1 : 2) * (rootOrder - usedRoots[static_cast<size_t>(i)]);
		}

		size_t resLength = static_cast<size_t>(std::max(order + 1, minimalLength));
		std::vector<double> res(resLength);
		res[resLength - 1] = 1;

		if (order == 0)
			return res;

		// Sort roots by magnitude for reducing numerical mistakes

		struct RootIndexes
		{
			int index;
			double magnitude;
		};

		std::vector<RootIndexes> rootIndexes(static_cast<size_t>(roots.size()));
		for (int i = 0; i < roots.size(); i++)
		{
			rootIndexes[static_cast<size_t>(i)].index = i;
			rootIndexes[static_cast<size_t>(i)].magnitude = std::abs(roots[i]->value.get());
		}

		std::sort(rootIndexes.begin(), rootIndexes.end(),
			[](const RootIndexes& a, const RootIndexes& b)
			{
				return a.magnitude < b.magnitude;
			});

		// Calculate coefficients

		int nonZeroCoeffCount = 1;

		for (int i = 0; i < roots.size(); i++)
		{
			int index = rootIndexes[static_cast<size_t>(i)].index;
			auto* root = roots[index];
			const double re = root->value.re.get();
			const double im = root->value.im.get();
			const int rootOrder = std::abs(root->order.get()) - usedRoots[static_cast<size_t>(index)];
			const bool isReal = root->isReal();
			if (rootOrder == 0)
				continue;
			//if (im == 0) // One real root
			if (isReal)
			{
				const double a0 = -re;
				const double a1 = 1.0;
				for (int j = 0; j < rootOrder; j++)
				{
					for (size_t k = static_cast<size_t>(resLength - nonZeroCoeffCount); k < resLength; k++)
					{
						double v = res[k];
						res[k - 1] += v * a1;
						res[k] = v * a0;
					}
					nonZeroCoeffCount += 1;
				}
			}
			else //(im != 0) => Two conjugate roots
			{
				const double a0 = re * re + im * im;
				const double a1 = -2.0 * re;
				const double a2 = 1.0;
				for (int j = 0; j < rootOrder; j++)
				{
					for (size_t k = static_cast<size_t>(resLength - nonZeroCoeffCount); k < resLength; k++)
					{
						double v = res[k];
						res[k - 2] += v * a2;
						res[k - 1] += v * a1;
						res[k] = v * a0;
					}
					nonZeroCoeffCount += 2;
				}
			}
		}

		return res;
	}
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
