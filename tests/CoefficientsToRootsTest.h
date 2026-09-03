#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>
#include "TestHelper.h"
#include "../src/PluginProcessor.h"
#include "../src/CoefficientsToRoots.h"
#include "../src/RootsToCoefficients.h"


class CoefficientsToRootsTest : public juce::UnitTest
{
public:
    CoefficientsToRootsTest() : UnitTest("CoefficientsToRootsTest", "Math")
	{
	}

#define COEFFICIENTS_TO_ROOTS_TEST_XLIST\
  X("1:1 real pole order 1", { {-1, -0.5, 0} } )\
  X("2:1 real pole order 2", { {-2, -0.5, 0} } )\
  X("2:2 real poles order 1", { {-1, -0.5, 0}, {-1, 0.3, 0} } )\
  X("2:1 complex pole order 1 a", { {-1, 0.3, 0.4} } )\
  X("2:1 complex pole order 1 b", { {-1, 0.4, 0.5} } )\
  X("2:1 complex pole order 1 c", { {-1, 0.6, 0.7} } )\
  X("3:1 real pole order 2 + order 1", { {-2, -0.01, 0}, {-1, 0.3, 0} } )\
  X("3:1 real pole order 3 a", { {-3, -0.5, 0} } )\
  X("3:1 real pole order 3 b", { {-3, -0.9, 0} } )\
  X("3:3 real poles order 1", { {-1, -0.5, 0}, {-1, 0.3, 0}, {-1, 0.2, 0} } )\
  X("3:1 complex order 1 + 1 real order 1 a", { {-1, 0.3, 0.4}, {-1, -0.5, 0} } )\
  X("3:1 complex order 1 + 1 real order 1 b", { {-1, 0.3, 0.4}, {-1, 0.5, 0} } )\
  X("3:1 complex order 1 + 1 real order 1 c", { {-1, 0.3, 0.4}, {-1, -0.8, 0} } )\
  X("4:1 real order 4", { {-4, -0.33, 0} })\
  X("4:2 real order 1 + 1 real order 2", { {-1,-0.5,0}, {-1,0.3,0}, {-2,0.2,0} })\
  X("4:4 real poles order 1", { {-1,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0} } )\
  X("4:2 real poles order 2", { {-2,-0.5,0}, {-2,0.3,0} } )\
  X("4:2 complex poles order 1 a", { {-1,0.3,0.4}, {-1,-0.1,0.3} } )\
  X("4:2 complex poles order 1 b", { {-1,0.3,0.4}, {-1,0.1,-0.3} } )\
  X("4:1 real order 3 + 1 real order 1", { {-3,-0.5,0}, {-1,0.3,0} } )\
  X("5:1 real pole order 5", { {-5,-0.5,0} } )\
  X("5:1 real order 4 + 1 real order 1 a", { {-4,-0.5,0}, {-1,0.1,0} } )\
  X("5:1 real order 4 + 1 real order 1 b", { {-4,-0.6,0}, {-1,0.2,0} } )\
  X("5:1 real order 4 + 1 real order 1 c", { {-4,-0.7,0}, {-1,0.3,0} } )\
  X("5:1 real order 4 + 1 real order 1 d", { {-4,-0.8,0}, {-1,0.7,0} } )\
  X("5:1 real order 3 + 1 real order 2 a", { {-3,-0.1,0}, {-2,0.3,0} } )\
  X("5:1 real order 3 + 1 real order 2 b", { {-3,-0.9,0}, {-2,0.7,0} } )\
  X("5:1 real order 3 + 1 complex order 1 a", { {-3,-0.5,0}, {-1,0.1,0.4} } )\
  X("5:1 real order 3 + 1 complex order 1 b", { {-3,-0.1,0}, {-1,0.2,0.3} } )\
  X("5:1 real order 1 + 1 complex order 2 a", { {-1,-0.5,0}, {-2,0.3,0.1} } )\
  X("5:1 real order 1 + 1 complex order 2 b", { {-1,-0.5,0}, {-2,0.4,0.44} } )\
  X("5:2 real order 2 + 1 real order 1", { {-2,-0.5,0}, {-2,0.3,0}, {-1,0.2,0} } )\
  X("5:1 real order 2 + 3 real order 1", { {-2,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0} } )\
  X("5:5 real poles order 1 a", { {-1,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0}, {-1,-0.3,0} } )\
  X("5:5 real poles order 1 b", { {-1,-0.1,0}, {-1,0.2,0}, {-1,0.3,0}, {-1,0.4,0}, {-1,-0.5,0} } )\
  X("5:1 real order 1 + 1 complex + 2 real order 1 a", { {-1,-0.5,0}, {-1,0.3,0.4}, {-1,0.2,0}, {-1,0.1,0} } )\
  X("5:1 real order 1 + 1 complex + 2 real order 1 b", { {-1,-0.2,0}, {-1,0.1,0.4}, {-1,0.4,0}, {-1,0.8,0} } )\
  X("5:1 real order 1 + 2 complex order 1 a", { {-1,-0.1,0}, {-1,0.3,0.4}, {-1,-0.1,0.3} } )\
  X("5:1 real order 1 + 2 complex order 1 b", { {-1,-0.9,0}, {-1,0.13,0.4}, {-1,-0.13,0.53} } )\
  X("6:1 real pole order 6 a", { {-6,-0.01,0} } )\
  X("6:1 real pole order 6 b", { {-6,-0.99,0} } )\
  X("6:1 real order 5 + 1 real order 1", { {-5,-0.5,0}, {-1,0.3,0} } )\
  X("6:1 real order 4 + 1 complex order 1 a", { {-4,-0.05,0}, {-1,0.13,0.14} } )\
  X("6:1 real order 4 + 1 complex order 1 b", { {-4,-0.95,0}, {-1,0.31,0.41} } )\
  X("6:1 real order 4 + 2 real order 1 a", { {-4,-0.1,0}, {-1,0.35,0}, {-1,0.12,0} } )\
  X("6:1 real order 4 + 2 real order 1 b", { {-4,-0.6,0}, {-1,0.63,0}, {-1,0.21,0} } )\
  X("6:1 real order 3 + 3 real order 1", { {-3,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0} } )\
  X("6:1 real order 3 + 1 complex + 1 real order 1", { {-3,-0.5,0}, {-1,0.3,0.4}, {-1,0.2,0} } )\
  X("6:1 complex pole order 3", { {-3,0.99,0.4} } )\
  X("6:1 complex pole order 3 b", { {-3,0.1,0.4} } )\
  X("6:3 real poles order 2", { {-2,-0.5,0}, {-2,0.3,0}, {-2,0.2,0} } )\
  X("6:2 real order 2 + 1 complex order 1", { {-2,-0.5,0}, {-2,0.3,0}, {-1,0.2,0.3} } )\
  X("6:1 real order 2 + 2 complex order 1", { {-2,-0.5,0}, {-1,0.3,0.4}, {-1,-0.1,0.3} } )\
  X("6:3 complex poles order 1", { {-1,0.3,0.4}, {-1,-0.1,0.3}, {-1,0.1,0.2} } )\
  X("7:1 real pole order 7 a", { {-7,-0.9,0} } )\
  X("7:1 real pole order 7 b", { {-7,-0.1,0} } )\
  X("7:1 real order 6 + 1 real order 1", { {-6,-0.5,0}, {-1,0.3,0} } )\
  X("7:1 real order 5 + 1 complex order 1 a", { {-5,-0.5,0}, {-1,0.9,0.1} } )\
  X("7:1 real order 5 + 1 complex order 1 b", { {-5,-0.5,0}, {-1,0.3,0.4} } )\
  X("7:1 real order 4 + 1 complex + 1 real order 1", { {-4,-0.5,0}, {-1,0.3,0.4}, {-1,0.2,0} } )\
  X("7:1 real order 4 + 3 real order 1", { {-4,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0} } )\
  X("7:1 real order 3 + 1 complex order 2", { {-3,-0.5,0}, {-2,0.3,0.4} } )\
  X("7:1 complex order 3 + 1 real order 1", { {-3,0.3,0.4}, {-1,-0.5,0} } )\
  X("7:3 complex + 1 real order 1", { {-1,0.3,0.4}, {-1,-0.1,0.3}, {-1,0.1,0.2}, {-1,-0.5,0} } )\
  X("7:1 complex + 5 real order 1", { {-1,0.3,0.4}, {-1,-0.5,0}, {-1,0.2,0}, {-1,0.1,0}, {-1,-0.3,0}, {-1,-0.2,0} } )\
  X("7:7 real poles order 1", { {-1,-0.5,0},{-1,0.3,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0} } )\
  X("8:1 real pole order 8", { {-8,-0.5,0} } )\
  X("8:1 real order 7 + 1 real order 1", { {-7,-0.5,0}, {-1,0.3,0} } )\
  X("8:1 real order 6 + 1 complex order 1", { {-6,-0.5,0}, {-1,0.3,0.4} } )\
  X("8:8 real poles order 1", { {-1,-0.5,0},{-1,0.3,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0},{-1,-0.4,0} } )\
  X("8:1 complex + 6 real order 1", { {-1,0.3,0.4},{-1,-0.5,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0} } )\
  X("8:2 complex + 4 real order 1", { {-1,0.3,0.4},{-1,-0.1,0.3},{-1,-0.5,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0} } )\
  X("8:4 complex poles order 1", { {-1,0.3,0.4},{-1,-0.1,0.3},{-1,0.1,0.2},{-1,-0.2,0.1} } )\
  X("9:1 real pole order 9", { {-9,-0.5,0} } )\
  X("9 real poles order 1", { {-1,-0.5,0},{-1,0.3,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0},{-1,-0.4,0},{-1,0.45,0} } )\
  X("9:1 complex + 7 real order 1", { {-1,0.3,0.4},{-1,-0.5,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0},{-1,-0.4,0} } )\
  X("9:1 real order 5 + 1 real order 4", { {-5,-0.5,0}, {-4,0.3,0} } )\
  X("9:1 real order 5 + 1 complex order 2", { {-5,-0.5,0}, {-2,0.3,0.4} } )\
  X("10:1 real pole order 10", { {-10,-0.5,0} } )\
  X("10:2 real poles order 5", { {-5,-0.5,0}, {-5,0.3,0} } )\
  X("10:1 real order 5 + order 4 + order 1", { {-5,-0.5,0}, {-4,0.3,0}, {-1,0.2,0} } )\
  X("10:1 real order 4 + order 3 + order 2 + order 1", { {-4,-0.5,0},{-3,0.3,0},{-2,0.2,0},{-1,0.1,0} } )\
  X("10:1 real order 4 + order 3 + complex + order 1", { {-4,-0.5,0},{-3,0.3,0},{-1,0.2,0.3},{-1,0.1,0} } )\
  X("14:1 real pole order 14", { {-14,-0.5,0} } )\
  X("20:1 real pole order 20", { {-20,-0.5,0} } )\
  X("28:1 real pole order 28", { {-28,-0.5,0} } )\
  X("32:1 real pole order 32", { {-32,-0.5,0} } )\

    void runTest() override
    {
		AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.

#define X(name, ...)\
		performTest("QR: " name, processor, QRSolve, __VA_ARGS__);\

		COEFFICIENTS_TO_ROOTS_TEST_XLIST;
#undef X
#define X(name, ...)\
		performTest("Aberth: " name, processor, AberthSolve, __VA_ARGS__);\

		COEFFICIENTS_TO_ROOTS_TEST_XLIST;
#undef X
    }

	static void printReport()
	{
		std::cout<<"CoefficientsToRootsTest Report:"<<std::endl;
		std::cout<<"Total tests : "<<totalNumAllTests<<std::endl;
		std::cout<<"Total failing unit-tests : "<<totalFailingUnitTests<<"/"<<totalNumAllTests<<std::endl;
		std::cout<<"Total missed root orders - all tests : "<<totalMissedRootOrdersAllTests<<std::endl;
	}

private:
	void performTest(
		const juce::String testName,
		AudioPluginAudioProcessor& processor,
		CoefficientsToRoots::SolverFn solve,
		std::vector<TestRootSpecification> givenRoots)
	{
		beginTest(testName);

		int zeros_order{0}, poles_order{0};
		int distinctZeroCount{0}, distinctPoleCount{0};
		for (auto r : givenRoots)
		{
		    int order = static_cast<int>(r.order) * (juce::exactlyEqual(r.valIm, 0.0) ? 1 : 2);
		    jassert(std::abs(order) > 0);
		    if (order>0)
		    {
		        zeros_order+=order;
			distinctZeroCount+=1;
		    }
		    else
		    {
		        poles_order+=order;
			distinctPoleCount+=1;
		    }
		}
		// causality
		if (zeros_order > -poles_order)
			poles_order = -zeros_order;

		auto* state = processor.filterState.get();
		TestHelper::makeFilterState(state, givenRoots, 1);

		if (!state->zeros.isEmpty())
		{
		    auto ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros);
		    auto newZeros = solve(ffcoeffs);

		    // Expect to have the same order
		    int curr_order{0};
		    for (auto r : newZeros)
		    {
		      curr_order += r.order * (juce::exactlyEqual(r.value.imag(), 0.0) ? 1 : 2);
		    }

		    bool failed{false};

		    const size_t absDiff{static_cast<size_t>(std::abs(static_cast<int>(zeros_order) - curr_order))};
		    expectEquals(absDiff, size_t(0));
		    if (absDiff)
		    {
		        std::cout<<"Error { total order = "<<zeros_order<<" != "<<curr_order<< " by "<<absDiff<<"}"<<std::endl;
		        totalMissedRootOrdersAllTests += absDiff;
		        failed = true;
		    }

		    const auto newZerosCount{static_cast<int>(newZeros.size())};
		    const int countDiff{std::abs(newZerosCount - distinctZeroCount)};
		    if (countDiff)
		    {
		        std::cout<<"Error { got "<<newZerosCount<<" distinct zeros, expected "<<distinctZeroCount<<" }"<<std::endl;
		        failed = true;
		    }

		    if (failed)
		    {
		        ++totalFailingUnitTests;
		    }
		    totalNumAllTests++;
		}

		// TODO : fix this condition. In case that a test contains only zeros, or the order of zeros exceeds order of poles, then default poles at 0,0 will be added.
		if (!state->poles.isEmpty())
		{
		    auto fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->poles);
		    auto newPoles = solve(fbcoeffs);

		    // Expect to have the same order
		    int curr_order{0};
		    for (auto r : newPoles)
		    {
		      curr_order -= r.order * (juce::exactlyEqual(r.value.imag(), 0.0) ? 1 : 2); // have to reverse sign, since pole-logic is outside GramSchmidt function
		    }

		    bool failed{false};

		    const size_t absDiff{static_cast<size_t>(std::abs(static_cast<int>(poles_order) - curr_order))};
		    expectEquals(absDiff, size_t(0));
		    if (absDiff)
		    {
		        std::cout<<"Error { total order = "<<poles_order<<" != "<<curr_order<< " by "<<absDiff<<"}"<<std::endl;
		        totalMissedRootOrdersAllTests += absDiff;
			failed = true;
		    }

		    const auto newPolesCount{static_cast<int>(newPoles.size())};
		    const int countDiff{std::abs(newPolesCount - distinctPoleCount)};
		    if (countDiff)
		    {
		        std::cout<<"Error { got "<<newPolesCount<<" distinct poles, expected "<<distinctPoleCount<<" }"<<std::endl;
		        failed = true;
		    }

		    if (failed)
		    {
		        ++totalFailingUnitTests;
		    }
		    totalNumAllTests++;
		}
	}

    static inline int totalMissedRootOrdersAllTests, totalNumAllTests {0}, totalFailingUnitTests {0};
};

static CoefficientsToRootsTest coeffsToRootsTest;
