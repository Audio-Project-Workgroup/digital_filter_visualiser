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

    void runTest() override
    {
		AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.

		performTest("1:1 real pole order 1", 							processor, { {-1, -0.5, 0} } );
		performTest("2:1 real pole order 2", 							processor, { {-2, -0.5, 0} } );
		performTest("2:2 real poles order 1",  							processor, { {-1, -0.5, 0}, {-1, 0.3, 0} } );
		performTest("2:1 complex pole order 1 a", 						processor, { {-1, 0.3, 0.4} } );
		performTest("2:1 complex pole order 1 b",						processor, { {-1, 0.3, 0.4} } );
		performTest("2:1 complex pole order 1 c",						processor, { {-1, 0.3, 0.4} } );
		performTest("3:1 real pole order 2 + order 1",					processor, { {-2, -0.01, 0}, {-1, 0.3, 0} } );
		performTest("3:1 real pole order 3 a",							processor, { {-3, -0.5, 0} } );
		performTest("3:1 real pole order 3 b",							processor, { {-3, -0.9, 0} } );
		performTest("3:3 real poles order 1", 							processor, { {-1, -0.5, 0}, {-1, 0.3, 0}, {-1, 0.2, 0} } );
		performTest("3:1 complex order 1 + 1 real order 1", 			processor, { {-1, 0.3, 0.4}, {-1, -0.5, 0} } );
		performTest("3:1 complex order 1 + 1 real order 1 b",			processor, { {-1, 0.3, 0.4}, {-1, -0.5, 0} } );
		performTest("3:1 complex order 1 + 1 real order 2",				processor, { {-1, 0.3, 0.4}, {-2, -0.5, 0} } );
		performTest("3:2 real order 1 + 1 real order 2",				processor, { {-1,-0.5,0}, {-1,0.3,0}, {-2,0.2,0} });
		performTest("4:4 real poles order 1",							processor, { {-1,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0} } );
		performTest("4:2 real poles order 2",							processor, { {-2,-0.5,0}, {-2,0.3,0} } );
		performTest("4:2 complex poles order 1 a",						processor, { {-1,0.3,0.4}, {-1,-0.1,0.3} } );
		performTest("4:2 complex poles order 1 b",						processor, { {-1,0.3,0.4}, {-1,-0.1,0.3} } );
		performTest("4:1 real order 3 + 1 real order 1",				processor, { {-3,-0.5,0}, {-1,0.3,0} } );
		performTest("5:1 real pole order 5",							processor, { {-5,-0.5,0} } );
		performTest("5:1 real order 4 + 1 real order 1 a",				processor, { {-4,-0.5,0}, {-1,0.1,0} } );
		performTest("5:1 real order 4 + 1 real order 1 b",				processor, { {-4,-0.5,0}, {-1,0.2,0} } );
		performTest("5:1 real order 4 + 1 real order 1 c",				processor, { {-4,-0.5,0}, {-1,0.3,0} } );
		performTest("5:1 real order 4 + 1 real order 1 d",				processor, { {-4,-0.5,0}, {-1,0.3,0} } );
		performTest("5:1 real order 3 + 1 real order 2 a",				processor, { {-3,-0.5,0}, {-2,0.3,0} } );
		performTest("5:1 real order 3 + 1 real order 2 b",				processor, { {-3,-0.5,0}, {-2,0.3,0} } );
		performTest("5:1 real order 3 + 1 complex order 1 a",			processor, { {-3,-0.5,0}, {-1,0.1,0.4} } );
		performTest("5:1 real order 3 + 1 complex order 1 b",			processor, { {-3,-0.5,0}, {-1,0.2,0.3} } );
		performTest("5:1 real order 1 + 1 complex order 2 a",			processor, { {-1,-0.5,0}, {-2,0.3,0.1} } );
		performTest("5:1 real order 1 + 1 complex order 2 b",			processor, { {-1,-0.5,0}, {-2,0.4,0.44} } );
		performTest("5:2 real order 2 + 1 real order 1",				processor, { {-2,-0.5,0}, {-2,0.3,0}, {-1,0.2,0} } );
		performTest("5:1 real order 2 + 3 real order 1",				processor, { {-2,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0} } );
		performTest("5:5 real poles order 1 a",							processor, { {-1,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0}, {-1,-0.3,0} } );
		performTest("5:5 real poles order 1 b", 						processor, { {-1,-0.1,0}, {-1,0.2,0}, {-1,0.3,0}, {-1,0.4,0}, {-1,-0.5,0} } );
		performTest("5:1 real order 1 + 1 complex + 2 real order 1 a", 	processor, { {-1,-0.5,0}, {-1,0.3,0.4}, {-1,0.2,0}, {-1,0.1,0} } );
		performTest("5:1 real order 1 + 1 complex + 2 real order 1 b", 	processor, { {-1,-0.2,0}, {-1,0.1,0.4}, {-1,0.4,0}, {-1,0.8,0} } );
		performTest("5:1 real order 1 + 2 complex order 1 a",			processor, { {-1,-0.1,0}, {-1,0.3,0.4}, {-1,-0.1,0.3} } );
		performTest("5:1 real order 1 + 2 complex order 1 b",			processor, { {-1,-0.9,0}, {-1,0.13,0.4}, {-1,-0.13,0.53} } );
		performTest("6:1 real pole order 6 a",							processor, { {-6,-0.01,0} } );
		performTest("6:1 real pole order 6 b",							processor, { {-6,-0.99,0} } );
		performTest("6:1 real order 5 + 1 real order 1",				processor, { {-5,-0.5,0}, {-1,0.3,0} } );
		performTest("6:1 real order 4 + 1 complex order 1 a",			processor, { {-4,-0.05,0}, {-1,0.13,0.14} } );
		performTest("6:1 real order 4 + 1 complex order 1 b",			processor, { {-4,-0.95,0}, {-1,0.31,0.41} } );
		performTest("6:1 real order 4 + 2 real order 1 a",				processor, { {-4,-0.1,0}, {-1,0.35,0}, {-1,0.12,0} } );
		performTest("6:1 real order 4 + 2 real order 1 b",				processor, { {-4,-0.6,0}, {-1,0.63,0}, {-1,0.21,0} } );
		performTest("6:1 real order 3 + 3 real order 1",				processor, { {-3,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0} } );
		performTest("6:1 real order 3 + 1 complex + 1 real order 1",	processor, { {-3,-0.5,0}, {-1,0.3,0.4}, {-1,0.2,0} } );
		performTest("6:1 complex pole order 3",							processor, { {-3,0.99,0.4} } );
		performTest("6:1 complex pole order 3 b",						processor, { {-3,0.1,0.4} } );
		performTest("6:3 real poles order 2",							processor, { {-2,-0.5,0}, {-2,0.3,0}, {-2,0.2,0} } );
		performTest("6:2 real order 2 + 1 complex order 1",				processor, { {-2,-0.5,0}, {-2,0.3,0}, {-1,0.2,0.3} } );
		performTest("6:1 real order 2 + 2 complex order 1",				processor, { {-2,-0.5,0}, {-1,0.3,0.4}, {-1,-0.1,0.3} } );
		performTest("6:3 complex poles order 1",						processor, { {-1,0.3,0.4}, {-1,-0.1,0.3}, {-1,0.1,0.2} } );
		performTest("7:1 real pole order 7 a",							processor, { {-7,-0.9,0} } );
		performTest("7:1 real pole order 7 b",							processor, { {-7,-0.1,0} } );
		performTest("7:1 real order 6 + 1 real order 1",				processor, { {-6,-0.5,0}, {-1,0.3,0} } );
		performTest("7:1 real order 5 + 1 complex order 1 a",			processor, { {-5,-0.5,0}, {-1,0.9,0.1} } );
		performTest("7:1 real order 5 + 1 complex order 1 b",			processor, { {-5,-0.5,0}, {-1,0.3,0.4} } );
		performTest("7:1 real order 4 + 1 complex + 1 real order 1",	processor, { {-4,-0.5,0}, {-1,0.3,0.4}, {-1,0.2,0} } );
		performTest("7:1 real order 4 + 3 real order 1",				processor, { {-4,-0.5,0}, {-1,0.3,0}, {-1,0.2,0}, {-1,0.1,0} } );
		performTest("7:1 real order 3 + 1 complex order 2",				processor, { {-3,-0.5,0}, {-2,0.3,0.4} } );
		performTest("7:1 complex order 3 + 1 real order 1",	 			processor, { {-3,0.3,0.4}, {-1,-0.5,0} } );
		performTest("7:3 complex + 1 real order 1",						processor, { {-1,0.3,0.4}, {-1,-0.1,0.3}, {-1,0.1,0.2}, {-1,-0.5,0} } );
		performTest("7:1 complex + 5 real order 1",						processor, { {-1,0.3,0.4}, {-1,-0.5,0}, {-1,0.2,0}, {-1,0.1,0}, {-1,-0.3,0}, {-1,-0.2,0} } );
		performTest("7:7 real poles order 1",							processor, { {-1,-0.5,0},{-1,0.3,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0} } );
		performTest("8:1 real pole order 8",							processor, { {-8,-0.5,0} } );
		performTest("8:1 real order 7 + 1 real order 1",				processor, { {-7,-0.5,0}, {-1,0.3,0} } );
		performTest("8:1 real order 6 + 1 complex order 1",				processor, { {-6,-0.5,0}, {-1,0.3,0.4} } );
		performTest("8:8 real poles order 1",							processor, { {-1,-0.5,0},{-1,0.3,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0},{-1,-0.4,0} } );
		performTest("8:1 complex + 6 real order 1",						processor, { {-1,0.3,0.4},{-1,-0.5,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0} } );
		performTest("8:2 complex + 4 real order 1",						processor, { {-1,0.3,0.4},{-1,-0.1,0.3},{-1,-0.5,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0} } );
		performTest("8:4 complex poles order 1", 						processor, { {-1,0.3,0.4},{-1,-0.1,0.3},{-1,0.1,0.2},{-1,-0.2,0.1} } );
		performTest("9:1 real pole order 9",							processor, { {-9,-0.5,0} } );
		performTest("9 real poles order 1",								processor, { {-1,-0.5,0},{-1,0.3,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0},{-1,-0.4,0},{-1,0.45,0} } );
		performTest("9:1 complex + 7 real order 1", 					processor, { {-1,0.3,0.4},{-1,-0.5,0},{-1,0.2,0},{-1,0.1,0},{-1,-0.3,0},{-1,-0.2,0},{-1,0.4,0},{-1,-0.4,0} } );
		performTest("9:1 real order 5 + 1 real order 4",  				processor, { {-5,-0.5,0}, {-4,0.3,0} } );
		performTest("9:1 real order 5 + 1 complex order 2", 			processor, { {-5,-0.5,0}, {-2,0.3,0.4} } );
		performTest("10:1 real pole order 10",							processor, { {-10,-0.5,0} } );
		performTest("10:2 real poles order 5",							processor, { {-5,-0.5,0}, {-5,0.3,0} } );
		performTest("10:1 real order 5 + order 4 + order 1", 			processor, { {-5,-0.5,0}, {-4,0.3,0}, {-1,0.2,0} } );
		performTest("10:1 real order 4 + order 3 + order 2 + order 1", 	processor, { {-4,-0.5,0},{-3,0.3,0},{-2,0.2,0},{-1,0.1,0} } );
		performTest("10:1 real order 4 + order 3 + complex + order 1", 	processor, { {-4,-0.5,0},{-3,0.3,0},{-1,0.2,0.3},{-1,0.1,0} } );
		performTest("14:1 real pole order 14",							processor, { {-14,-0.5,0} } );
		performTest("20:1 real pole order 20",							processor, { {-20,-0.5,0} } );
		performTest("28:1 real pole order 28",							processor, { {-28,-0.5,0} } );
		performTest("32:1 real pole order 32",							processor, { {-32,-0.5,0} } );
	}

private:
	void performTest(
		const juce::String testName,
		AudioPluginAudioProcessor& processor,
		std::vector<std::array<double, 3>> givenRoots)
	{
		beginTest(testName);
		
		int zeros_order{0}, poles_order{0};
		size_t zeros_orderTolerance{0}, poles_orderTolerance{0};
		for (auto r : givenRoots)
		{
			int order = static_cast<int>(r[0]) * (r[2] == 0 ? 1 : 2);
			jassert(std::abs(order) > 0);
			if (order>0)
			{
				zeros_order+=order;
				zeros_orderTolerance+= (r[0] / RootOrderToleranceStep);	// TODO: consider log scale growth instead of linear
			}
			else
			{
				poles_order+=order;
				poles_orderTolerance+= (std::abs(r[0]) / RootOrderToleranceStep);	// TODO: consider log scale growth instead of linear
			}
		}
		// causality
		if (zeros_order > -poles_order)
			poles_order = -zeros_order;

		std::cout<<"zeros_orderTolerance "<<zeros_orderTolerance<<std::endl;
		std::cout<<"poles_orderTolerance "<<poles_orderTolerance<<std::endl;
		
		auto* state = processor.filterState.get();
		TestHelper::makeFilterState(state, givenRoots, 1);

        if (!state->zeros.isEmpty())
        {
            auto ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros);
            auto newZeros = CoefficientsToRoots::QR(ffcoeffs);

            // Expect to have the same order
			int curr_order{0};
			for (auto r : newZeros)
			{
				curr_order += r.second * (r.first.imag() == 0 ? 1 : 2);
			}
			
			// expectEquals( static_cast<int>(zeros_order), curr_order );
			const size_t absDiff {static_cast<size_t>(std::abs(static_cast<int>(zeros_order) - curr_order))};
		    expectLessOrEqual( absDiff, zeros_orderTolerance);
			if (absDiff)
				std::cout<<"Warning : computation error { total order = "<<zeros_order<<" != "<<curr_order<< " by "<<absDiff<<"}"<<std::endl;
        }
		
		if (!state->poles.isEmpty())
        {
            auto fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->poles);
			auto newPoles = CoefficientsToRoots::QR(fbcoeffs);

            // Expect to have the same order
			int curr_order{0};
			for (auto r : newPoles)
			{
				curr_order -= r.second * (r.first.imag() == 0 ? 1 : 2); // have to reverse sign, since pole-logic is outside GramSchmidt function
			}

			// expectEquals( static_cast<int>(poles_order), curr_order );
			const size_t absDiff {static_cast<size_t>(std::abs(static_cast<int>(poles_order) - curr_order))};
		    expectLessOrEqual( absDiff , poles_orderTolerance);
			if (absDiff)
				std::cout<<"Warning : computation error { total order = "<<poles_order<<" != "<<curr_order<< " by "<<absDiff<<"}"<<std::endl;
			

        }
	}

	/* Empirical value : Root computatation accuracy degrades for roots of higher-order as their order increases. This step allows 1 order of error per 4 orders.*/
	static constexpr int RootOrderToleranceStep {4};
};

static CoefficientsToRootsTest CoefficientsToRootsTest;