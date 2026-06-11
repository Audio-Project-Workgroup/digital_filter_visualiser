#include "../src/JUCE/modules/juce_core/juce_core.h"
#include "RootsToCoefficientsTest.h"
#include "ProcessorChainModifierTest.h"
#include "PhaseFrequencyResponseTest.h"

//==============================================================================
int main (int argc, char* argv[])
{
    juce::ignoreUnused(argc);
    juce::ignoreUnused(argv);

    juce::ScopedJuceInitialiser_GUI libraryInitialiser; // for proper processor initialization in test classes
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
