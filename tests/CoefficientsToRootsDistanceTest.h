#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>
#include "TestHelper.h"
#include "../src/PluginProcessor.h"
#include "../src/CoefficientsToRoots.h"
#include "../src/RootsToCoefficients.h"


class CoefficientsToRootsDistanceTest : public juce::UnitTest
{
public:
    CoefficientsToRootsDistanceTest() : UnitTest("CoefficientsToRootsDistanceTest", "Math")
	{
	}

  #define COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_POLES_XLIST\
    X("{distance} 1:1 real pole order 1 a", { {-1, -0.5, 0} } )\
    X("{distance} 1:1 real pole order 1 b", { {-1, 0.5, 0} } )\
    X("{distance} 1:1 real pole order 1 c", { {-1, -0.1, 0} } )\
    X("{distance} 1:1 real pole order 1 d", { {-1, 0.9, 0} } )\
    X("{distance} 2:1 real pole order 2", { {-2, -0.5, 0} } )\
    X("{distance} 2:1 complex pole order 1 a", { {-2, 0.3, 0.4} } )\
    X("{distance} 2:1 complex pole order 1 b", { {-2, 0.4, 0.5} } )\
    X("{distance} 2:1 complex pole order 1 c", { {-2, 0.6, 0.7} } )\
    X("{distance} 3:1 real pole order 3 a", { {-3, -0.5, 0} } )\
    X("{distance} 3:1 real pole order 3 b", { {-3, -0.9, 0} } )\
    X("{distance} 3:1 real pole order 3 c", { {-3, 0.9, 0} } )\
    X("{distance} 3:1 real pole order 3 d", { {-3, 0.1, 0} } )\
    X("{distance} 4:1 real pole order 4 a", { {-4,-0.9,0} } )\
    X("{distance} 4:1 real pole order 4 b", { {-4,-0.5,0} } )\
    X("{distance} 4:1 real pole order 4 c", { {-4,-0.1,0} } )\
    X("{distance} 5:1 real pole order 5 a", { {-5,-0.5,0} } )\
    X("{distance} 5:1 real pole order 5 b", { {-5,-0.1,0} } )\
    X("{distance} 5:1 real pole order 5 c", { {-5,-0.9,0} } )\
    X("{distance} 6:1 real pole order 6 a", { {-6,-0.01,0} } )\
    X("{distance} 6:1 real pole order 6 b", { {-6,-0.99,0} } )\
    X("{distance} 6:1 complex pole order 3", { {-3,0.99,0.4} } )\
    X("{distance} 6:1 complex pole order 3 b", { {-3,0.1,0.4} } )\
    X("{distance} 7:1 real pole order 7 a", { {-7,-0.9,0} } )\
    X("{distance} 7:1 real pole order 7 b", { {-7,-0.1,0} } )\
    X("{distance} 8:1 real pole order 8", { {-8,-0.5,0} } )\
    X("{distance} 9:1 real pole order 9", { {-9,-0.5,0} } )\
    X("{distance} 10:1 real pole order 10", { {-10,-0.5,0} } )\
    X("{distance} 14:1 real pole order 14", { {-14,-0.5,0} } )\
    X("{distance} 20:1 real pole order 20", { {-20,-0.5,0} } )\
    X("{distance} 28:1 real pole order 28", { {-28,-0.5,0} } )\
    X("{distance} 32:1 real pole order 32", { {-32,-0.5,0} } )\

  #define COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_ZEROS_XLIST\
    X("{distance} 1:1 real zero order 1", { {1, -0.5, 0} } )\
    X("{distance} 1:1 real zero order 1", { {1, 0.5, 0} } )\
    X("{distance} 1:1 real zero order 1", { {1, -0.1, 0} } )\
    X("{distance} 1:1 real zero order 1", { {1, 0.9, 0} } )\
    X("{distance} 2:1 real zero order 2", { {2, -0.5, 0} } )\
    X("{distance} 2:1 complex zero order 1 a", { {2, 0.3, 0.4} } )\
    X("{distance} 2:1 complex zero order 1 b", { {2, 0.4, 0.5} } )\
    X("{distance} 2:1 complex zero order 1 c", { {2, 0.6, 0.7} } )\
    X("{distance} 3:1 real zero order 3 a", { {3, -0.5, 0} } )\
    X("{distance} 3:1 real zero order 3 b", { {3, -0.9, 0} } )\
    X("{distance} 3:1 real zero order 3 c", { {3, 0.9, 0} } )\
    X("{distance} 3:1 real zero order 3 d", { {3, 0.1, 0} } )\
    X("{distance} 4:1 real zero order 4 a", { {4,-0.9,0} } )\
    X("{distance} 4:1 real zero order 4 b", { {4,-0.5,0} } )\
    X("{distance} 4:1 real zero order 4 c", { {4,-0.1,0} } )\
    X("{distance} 5:1 real zero order 5", { {5,-0.5,0} } )\
    X("{distance} 5:1 real zero order 5 b", { {5,-0.1,0} } )\
    X("{distance} 5:1 real zero order 5 c", { {5,-0.9,0} } )\
    X("{distance} 6:1 real zero order 6 a", { {6,-0.01,0} } )\
    X("{distance} 6:1 real zero order 6 b", { {6,-0.99,0} } )\
    X("{distance} 6:1 complex zero order 3", { {3,0.99,0.4} } )\
    X("{distance} 6:1 complex zero order 3 b", { {3,0.1,0.4} } )\
    X("{distance} 7:1 real zero order 7 a", { {7,-0.9,0} } )\
    X("{distance} 7:1 real zero order 7 b", { {7,-0.1,0} } )\
    X("{distance} 8:1 real zero order 8", { {8,-0.5,0} } )\
    X("{distance} 9:1 real zero order 9", { {9,-0.5,0} } )\
    X("{distance} 10:1 real zero order 10", { {10,-0.5,0} } )\
    X("{distance} 14:1 real zero order 14", { {14,-0.5,0} } )\
    X("{distance} 20:1 real zero order 20", { {20,-0.5,0} } )\
    X("{distance} 28:1 real zero order 28", { {28,-0.5,0} } )\
    X("{distance} 32:1 real zero order 32", { {32,-0.5,0} } )\

    void runTest() override
    {
		AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.

#define X(name, ...)\
		performTest("QR: " name, processor, QRSolve, __VA_ARGS__);\

		COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_POLES_XLIST;
		COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_ZEROS_XLIST;
#undef X
#define X(name, ...)\
		performTest("Aberth: " name, processor, AberthSolve, __VA_ARGS__);\

		COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_POLES_XLIST;
		COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_ZEROS_XLIST;
#undef X
    }

	static void printReport()
	{
		std::cout<<"CoefficientsToRootsDistanceTest Report:"<<std::endl;
		std::cout<<"Total tests : "<<totalNumAllTests<<std::endl;
		std::cout<<"Total number of warnigns : "<<totalNumWarnings<<"/"<<totalNumAllTests<<std::endl;
		std::cout<<"Mean distance all tests : "<<totalDistanceAllTests/totalNumAllTests<<std::endl;
	}

private:
	void performTest(
		const juce::String testName,
		AudioPluginAudioProcessor& processor,
		CoefficientsToRoots::SolverFn solve,
		std::vector<TestRootSpecification> givenRoots)
	{
		beginTest(testName);

		auto* state = processor.filterState.get();
		TestHelper::makeFilterState(state, givenRoots, 1);

        if (!state->zeros.isEmpty())
        {
            auto ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros);
            auto newZeros = solve(ffcoeffs);

			double totalDistance {0}, worstDistance{0};
			auto zero = state->zeros[0]; // these are one-zero tests
			for (auto r : newZeros)
			{
				double distance = std::abs(r.value - std::complex<double>(zero->value.re.get(), zero->value.im.get()));
				totalDistance += distance;
				worstDistance = std::max(worstDistance, distance);
			}
			double meanDistance = totalDistance / newZeros.size();

			std::cout<<"Report Distance (zeros):"<<std::endl;
			std::cout<<"Mean distance :"<<meanDistance<<std::endl;
			std::cout<<"Worst distance :"<<worstDistance<<std::endl;

			if (meanDistance > DistanceTolerance)
			{
				std::cout<<"Warning : computation error { meanDistance > DistanceTolerance  : "<<meanDistance<<" > "<<DistanceTolerance<< " }"<<std::endl;
				totalNumWarnings++;
			}

			totalDistanceAllTests += meanDistance;
			totalNumAllTests++;
			std::cout<<"totalDistanceAllTests:" <<totalDistanceAllTests<<", totalNumAllTests:" <<totalNumAllTests<<std::endl;
        }

		else if (!state->poles.isEmpty()) // else if make these pole  / zero tests mutually exlusive. Cause these are one-root tests.
        {
            auto fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->poles);
	    auto newPoles = solve(fbcoeffs);

			double totalDistance {0}, worstDistance{0};
			auto pole = state->poles[0]; // these are one-pole tests
			for (auto r : newPoles)
			{
				double distance = std::abs(r.value - std::complex<double>(pole->value.re.get(), pole->value.im.get()));
				totalDistance += distance;
				worstDistance = std::max(worstDistance, distance);
			}
			double meanDistance = totalDistance / newPoles.size();

			std::cout<<"Report Distance (poles):"<<std::endl;
			std::cout<<"Mean distance :"<<meanDistance<<std::endl;
			std::cout<<"Worst distance :"<<worstDistance<<std::endl;

			if (meanDistance > DistanceTolerance)
			{
				std::cout<<"Warning : computation error { meanDistance > DistanceTolerance  : "<<meanDistance<<" > "<<DistanceTolerance<< " }"<<std::endl;
				totalNumWarnings++;
			}


			totalDistanceAllTests += meanDistance;
			totalNumAllTests++;
			std::cout<<"totalDistanceAllTests:" <<totalDistanceAllTests<<", totalNumAllTests:" <<totalNumAllTests<<std::endl;
        }
	}

	static constexpr double DistanceTolerance {5e-2}; // equal to CoefficientsToRoots::tolerance
	static inline double totalDistanceAllTests {0.0};
    static inline int totalNumAllTests {0}, totalNumWarnings {0};

};

static CoefficientsToRootsDistanceTest coeffsToRootsDistanceTest;
