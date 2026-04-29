#pragma once
#include "FilterState.h"
#include "ProcessorChain.h"
#include "PluginProcessor.h"

class ProcessorChainModifier
{
public:
	static void rootsToJuceCoeffs(
		FilterState* state,
		FullState<float>* processorState,
		juce::dsp::ProcessSpec& spec);
	static void process(AudioPluginAudioProcessor& processor);

private:
	static inline double evaluatePole(const FilterRoot* pole);
	static int findBestZeroIndexPairForPole(
		const FilterRoot* pole,
		juce::OwnedArray<FilterRoot>& zeros,
		std::vector<int>& usedZeros,
		bool doesEqualPoleExist);
	static juce::dsp::IIR::Coefficients<float>* calculateIirCoefficients(
		FilterRoot* pole,
		juce::OwnedArray<FilterRoot>& zeros,
		int zeroIndex,
		bool shouldEqualPoleBeTaken,
		bool shouldEqualZeroBeTaken);
	static inline void calculatePolynomialCoefficients(
		FilterRoot* root,
		bool shouldBeTakenTwice,
		float& c0, float& c1, float& c2);
	static inline double angleDiffAbs(double a, double b);

	static constexpr double pi = juce::MathConstants<double>::pi;
	static constexpr double angleSimilarityThreshold = 0.1 * pi;
	static constexpr double magnitudeThreshold = 1e-5;
	static constexpr double qCoeff = 0.6;
	static constexpr double magCoeff = 0.3;
	static constexpr double angleCoeff = 0.1;
	static constexpr juce::uint32 processBlockMaxPause = 100;
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 8 */
/* c-basic-offset: 8 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
