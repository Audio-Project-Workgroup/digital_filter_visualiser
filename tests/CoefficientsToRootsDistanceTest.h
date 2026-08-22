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

    void runTest() override
    {
		AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.
		// poles
		performTest("{distance} 1:1 real pole order 1 a", 							processor, { {-1, -0.5, 0} } );
		performTest("{distance} 1:1 real pole order 1 b", 							processor, { {-1, 0.5, 0} } );
		performTest("{distance} 1:1 real pole order 1 c", 							processor, { {-1, -0.1, 0} } );
		performTest("{distance} 1:1 real pole order 1 d", 							processor, { {-1, 0.9, 0} } );
		performTest("{distance} 2:1 real pole order 2", 							processor, { {-2, -0.5, 0} } );
		performTest("{distance} 2:1 complex pole order 1 a", 						processor, { {-2, 0.3, 0.4} } );
		performTest("{distance} 2:1 complex pole order 1 b",						processor, { {-2, 0.4, 0.5} } );
		performTest("{distance} 2:1 complex pole order 1 c",						processor, { {-2, 0.6, 0.7} } );
		performTest("{distance} 3:1 real pole order 3 a",							processor, { {-3, -0.5, 0} } );
		performTest("{distance} 3:1 real pole order 3 b",							processor, { {-3, -0.9, 0} } );
		performTest("{distance} 3:1 real pole order 3 c",							processor, { {-3, 0.9, 0} } );
		performTest("{distance} 3:1 real pole order 3 d",							processor, { {-3, 0.1, 0} } );
		performTest("{distance} 4:1 real pole order 4 a",							processor, { {-4,-0.9,0} } );
		performTest("{distance} 4:1 real pole order 4 b",							processor, { {-4,-0.5,0} } );
		performTest("{distance} 4:1 real pole order 4 c",							processor, { {-4,-0.1,0} } );
		performTest("{distance} 5:1 real pole order 5 a",							processor, { {-5,-0.5,0} } );
		performTest("{distance} 5:1 real pole order 5 b",							processor, { {-5,-0.1,0} } );
		performTest("{distance} 5:1 real pole order 5 c",							processor, { {-5,-0.9,0} } );
		performTest("{distance} 6:1 real pole order 6 a",							processor, { {-6,-0.01,0} } );
		performTest("{distance} 6:1 real pole order 6 b",							processor, { {-6,-0.99,0} } );
		performTest("{distance} 6:1 complex pole order 3",							processor, { {-3,0.99,0.4} } );
		performTest("{distance} 6:1 complex pole order 3 b",						processor, { {-3,0.1,0.4} } );
		performTest("{distance} 7:1 real pole order 7 a",							processor, { {-7,-0.9,0} } );
		performTest("{distance} 7:1 real pole order 7 b",							processor, { {-7,-0.1,0} } );
		performTest("{distance} 8:1 real pole order 8",								processor, { {-8,-0.5,0} } );
		performTest("{distance} 9:1 real pole order 9",								processor, { {-9,-0.5,0} } );
		performTest("{distance} 10:1 real pole order 10",							processor, { {-10,-0.5,0} } );
		performTest("{distance} 14:1 real pole order 14",							processor, { {-14,-0.5,0} } );
		performTest("{distance} 20:1 real pole order 20",							processor, { {-20,-0.5,0} } );
		performTest("{distance} 28:1 real pole order 28",							processor, { {-28,-0.5,0} } );
		performTest("{distance} 32:1 real pole order 32",							processor, { {-32,-0.5,0} } );
		// zeroes
		performTest("{distance} 1:1 real zero order 1", 							processor, { {1, -0.5, 0} } );
		performTest("{distance} 1:1 real zero order 1", 							processor, { {1, 0.5, 0} } );
		performTest("{distance} 1:1 real zero order 1", 							processor, { {1, -0.1, 0} } );
		performTest("{distance} 1:1 real zero order 1", 							processor, { {1, 0.9, 0} } );
		performTest("{distance} 2:1 real zero order 2", 							processor, { {2, -0.5, 0} } );
		performTest("{distance} 2:1 complex zero order 1 a", 						processor, { {2, 0.3, 0.4} } );
		performTest("{distance} 2:1 complex zero order 1 b",						processor, { {2, 0.4, 0.5} } );
		performTest("{distance} 2:1 complex zero order 1 c",						processor, { {2, 0.6, 0.7} } );
		performTest("{distance} 3:1 real zero order 3 a",							processor, { {3, -0.5, 0} } );
		performTest("{distance} 3:1 real zero order 3 b",							processor, { {3, -0.9, 0} } );
		performTest("{distance} 3:1 real zero order 3 c",							processor, { {3, 0.9, 0} } );
		performTest("{distance} 3:1 real zero order 3 d",							processor, { {3, 0.1, 0} } );
		performTest("{distance} 4:1 real zero order 4 a",							processor, { {4,-0.9,0} } );
		performTest("{distance} 4:1 real zero order 4 b",							processor, { {4,-0.5,0} } );
		performTest("{distance} 4:1 real zero order 4 c",							processor, { {4,-0.1,0} } );
		performTest("{distance} 5:1 real zero order 5",								processor, { {5,-0.5,0} } );
		performTest("{distance} 5:1 real zero order 5 b",							processor, { {5,-0.1,0} } );
		performTest("{distance} 5:1 real zero order 5 c",							processor, { {5,-0.9,0} } );
		performTest("{distance} 6:1 real zero order 6 a",							processor, { {6,-0.01,0} } );
		performTest("{distance} 6:1 real zero order 6 b",							processor, { {6,-0.99,0} } );
		performTest("{distance} 6:1 complex zero order 3",							processor, { {3,0.99,0.4} } );
		performTest("{distance} 6:1 complex zero order 3 b",						processor, { {3,0.1,0.4} } );
		performTest("{distance} 7:1 real zero order 7 a",							processor, { {7,-0.9,0} } );
		performTest("{distance} 7:1 real zero order 7 b",							processor, { {7,-0.1,0} } );
		performTest("{distance} 8:1 real zero order 8",								processor, { {8,-0.5,0} } );
		performTest("{distance} 9:1 real zero order 9",								processor, { {9,-0.5,0} } );
		performTest("{distance} 10:1 real zero order 10",							processor, { {10,-0.5,0} } );
		performTest("{distance} 14:1 real zero order 14",							processor, { {14,-0.5,0} } );
		performTest("{distance} 20:1 real zero order 20",							processor, { {20,-0.5,0} } );
		performTest("{distance} 28:1 real zero order 28",							processor, { {28,-0.5,0} } );
		performTest("{distance} 32:1 real zero order 32",							processor, { {32,-0.5,0} } );
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
		std::vector<TestRootSpecification> givenRoots)
	{
		beginTest(testName);

		auto* state = processor.filterState.get();
		TestHelper::makeFilterState(state, givenRoots, 1);

        if (!state->zeros.isEmpty())
        {
            auto ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros);
            auto newZeros = QRSolve(ffcoeffs);

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
	    auto newPoles = QRSolve(fbcoeffs);

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
