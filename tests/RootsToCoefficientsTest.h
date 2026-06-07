#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>
#include "TestHelper.h"
#include "../src/PluginProcessor.h"

class RootsToCoefficientsTest : public juce::UnitTest
{
public:
    RootsToCoefficientsTest() : UnitTest("RootsToCoefficientsTest", "Math") 
	{
	}

    void runTest() override
    {
		AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.

		performTest(
			"No roots test",
			processor,
			{},
			{ 1 });

		performTest(
			"1 1-order real root",
			processor,
			{ {1, 5, 0} },
			{ 1, -5 });

		performTest(
			"1 3-order real root",
			processor,
			{ {3, -5, 0} },
			{ 1, 15, 75, 125 });

		performTest(
			"2 1-order real roots",
			processor,
			{ {1, 5, 0}, {1, -2, 0} },
			{ 1, -3, -10 });
		
		performTest(
			"1 1-order complex roots",
			processor,
			{ {1, 2, -3} },
			{ 1, -4, 13 });

		performTest(
			"1 1-order complex root, 1 1-order real root",
			processor,
			{ {1, 2, -3}, {1, -2, 0} },
			{ 1, -2, 5, 26 });
	}

private:
	void performTest(
		const juce::String testName,
		AudioPluginAudioProcessor& processor,
		std::vector<std::array<double, 3>> roots,
		std::vector<double> expectedCoefficients)
	{
		beginTest(testName);
		
		for (auto r : roots)
			jassert(r[0] > 0);

		auto* state = processor.filterState.get();
		TestHelper::makeFilterState(state, roots, 1);
		auto res = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros);
		
		expectEquals(res.size(), expectedCoefficients.size());
		for (int i = 0; i < res.size(); i++)
			expectEquals(res[i], expectedCoefficients[i]);
	}
};

static RootsToCoefficientsTest rootsToCoefficientsTest;