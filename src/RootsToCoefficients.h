#pragma once
#include "FilterState.h"

class RootsToCoefficients
{
public:
	static std::vector<double> CalculatePolynomialCoefficientsFrom(
		juce::OwnedArray<FilterRoot>& roots,
		int minimalLength = 1,
		std::vector<int>* usedRootsPtr = nullptr);
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
