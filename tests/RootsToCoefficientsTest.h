#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>
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
		auto* state = processor.filterState.get();
		auto& roots = state->zeros;

        beginTest("No roots test");
        auto res = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(roots);
        expect(res.size() == 1);
        expect(res[0] == 1);

        beginTest("1 1-order real root");
		roots.clear();
		auto r1 = state->add(1);
		r1->value.re.setValue(5, &processor.um);
		r1->value.im.setValue(0, &processor.um);
        res = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(roots);
        expect(res.size() == 2);
        expect(res[0] == 1);
		expect(res[1] == -5);

		beginTest("1 3-order real root");
		r1->order.setValue(3, &processor.um);
		r1->value.re.setValue(-5, &processor.um);
		r1->value.im.setValue(0, &processor.um);
		res = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(roots);
		expect(res.size() == 4);
		expect(res[0] == 1);
		expect(res[1] == 15);
		expect(res[2] == 75);
		expect(res[3] == 125);

		beginTest("2 1-order real roots");
		r1->order.setValue(1, &processor.um);
		r1->value.re.setValue(5, &processor.um);
		r1->value.im.setValue(0, &processor.um);
		auto r2 = state->add(1);
		r2->value.re.setValue(-2, &processor.um);
		r2->value.im.setValue(0, &processor.um);
		res = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(roots);
		expect(res.size() == 3);
		expect(res[0] == 1);
		expect(res[1] == -3);
		expect(res[2] == -10);

		beginTest("1 1-order complex roots");
		state->remove(r2);
		r1->order.setValue(1, &processor.um);
		r1->value.re.setValue(2, &processor.um);
		r1->value.im.setValue(-3, &processor.um);
		res = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(roots);
		expect(res.size() == 3);
		expect(res[0] == 1);
		expect(res[1] == -4);
		expect(res[2] == 13);

		beginTest("1 1-order complex root, 1 1-order real root");
		r2 = state->add(1);
		r2->value.re.setValue(-2, &processor.um);
		r2->value.im.setValue(0, &processor.um);
		res = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(roots);
		expect(res.size() == 4);
		expect(res[0] == 1);
		expect(res[1] == -2);
		expect(res[2] == 5);
		expect(res[3] == 26);
	}
};

static RootsToCoefficientsTest rootsToCoefficientsTest;