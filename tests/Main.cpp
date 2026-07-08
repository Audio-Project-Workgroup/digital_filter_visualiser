#include "../src/JUCE/modules/juce_core/juce_core.h"
#include "RootsToCoefficientsTest.h"
#include "ProcessorChainModifierTest.h"
#include "PhaseFrequencyResponseTest.h"
#include "CoefficientsToRootsTest.h"
#include "CoefficientsToRootsDistanceTest.h"

//==============================================================================
int main (int argc, char* argv[])
{
    juce::ignoreUnused(argc);
    juce::ignoreUnused(argv);

    juce::ScopedJuceInitialiser_GUI libraryInitialiser; // for proper processor initialization in test classes
    juce::UnitTestRunner runner;
    runner.runAllTests();

    std::cout << "\n===== All tests complete =====\n"<<std::endl;

    CoefficientsToRootsTest::printReport();
    std::cout<<"\n------------------------------\n"<<std::endl;
    CoefficientsToRootsDistanceTest::printReport();

    return 0;
}
