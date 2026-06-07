#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>
#include "TestHelper.h"
#include "../src/PluginProcessor.h"

class ProcessorChainModifierTest : public juce::UnitTest
{
public:
    ProcessorChainModifierTest() : UnitTest("ProcessorChainModifierTest", "Math")
    { }

    void runTest() override
    {
        AudioPluginAudioProcessor processor; // shouldn't be a field of this class as its static object is defined.
        prepareProcState(processor.activeState, channelNumber);

        performTest(
            "No roots",
            processor,
            {},
            1,
            0,
            {},
            { 1 });
    
        performTest(
            "Delay",
            processor,
            { { -3, 0.0, 0.0 } , { -4, 0.0, 0.0 } },
            0.2,
            7,
            {},
            { 1 });

        performTest(
            "1-order real pole",
            processor,
            { { -1, 0.5, 0 } },
            1,
            1,
            { {1, 0, -0.5 } },
            { 1 });

        performTest(
            "1-order complex pole",
            processor,
            { { -1, 0.5, 0.5 } },
            1,
            2,
            { { 1, 0, 0, -1, 0.5 } },
            { 1 });

        performTest(
            "1-order complex pole",
            processor,
            { { -1, -0.1, -0.1 } },
            1,
            2,
            { { 1, 0, 0, 0.2, 0.02 } },
            { 1 });

        performTest(
            "1-order complex zero, 1-order complex pole, delay",
            processor,
            { { 1, 0.5, 0.5 } , { -1, -0.1, -0.1 }, { -2, 0, 0 } },
            1,
            2,
            { { 1, -1, 0.5, 0.2, 0.02 } },
            { 1 });

        performTest(
            "1-order complex zero, 1-order real pole, delay",
            processor,
            { { 1, 0.5, 0.5 } , { -1, -0.1, 0 }, { -2, 0, 0 } },
            1,
            1,
            { { 1, 0, 0.1 } },
            { 1, -1, 0.5 });

        performTest(
            "1-order complex zero, 2-order real pole, delay",
            processor,
            { { 1, 0.5, 0.5 } , { -2, -0.1, 0 }, { -2, 0, 0 } },
            1,
            2, 
            { { 1, -1, 0.5, 0.2, 0.01 } },
            { 1 });

        performTest(
            "1-order real zero, 2-order complex pole",
            processor,
            { { -2, 0.5, 0.5 } , { 1, -0.1, 0 } },
            1,
            3,
            { { 1, 0, 0, -1, 0.5 }, { 1, 0.1, 0, -1, 0.5 } },
            { 1 });

        performTest(
            "Sorting poles",
            processor,
            { 
                { -2, 0.5, 0.5 }, // 0: mag - 0.707, |angle| = pi/4, Q = 1.333, res = 0.9705 
                { -2, 0, 0 },     // 1: delay
                { -2, -0.1, 0 },  // 2: mag - 0.1, |angle| = pi, Q = 0.682, res = 0.75335 
            },
            1,
            8,
            { 
                { 1, 0, 0, 0.2, 0.01 }, // 2
                { 1, 0, 0, -1, 0.5 },   // 0 
                { 1, 0, 0, -1, 0.5 },   // 0 
            },
            { 1 });
    }

private:
    juce::dsp::ProcessSpec spec{ 48000, 512, 1 }; // 1 channel for each mono processor
    const int channelNumber = 2;
    const float maxRelError = 1e-6;

    void prepareProcState(std::atomic<FullState<float>*>& procState, int channelNumber)
    {
        auto state = procState.load();
        state->clear(true);
        for (auto i = 0; i < channelNumber; i++)
            state->add(new ProcessorChain<float>);
    }

    void performTest(
        const juce::String testName,
        AudioPluginAudioProcessor& processor,
        std::vector<std::array<double, 3>> roots,
        float gain,
        int expectedDelay,
        std::vector<std::vector<float>> expectedIirCoefficients,
        std::vector<float> expectedFirCoefficients)
    {
        beginTest(testName);
        TestHelper::makeFilterState(processor.filterState.get(), roots, gain);
        ProcessorChainModifier::rootsToJuceCoeffs(processor.filterState.get(), processor.activeState, spec);
        checkMultichannel(processor.activeState, channelNumber);
        checkValues(processor.activeState, expectedDelay, gain, expectedIirCoefficients, expectedFirCoefficients);
    }

    void checkMultichannel(
        std::atomic<FullState<float>*>& procState,
        int channelNumber)
    {
        auto state = procState.load();
        expectEquals(state->size(), channelNumber);
        if (channelNumber < 2)
            return;
        auto state0 = (*state)[0];
        for (int i = 1; i < channelNumber; i++)
        {
            auto stateI = (*state)[i];
            expectEquals(state0->delay.getDelay(), stateI->delay.getDelay());
            expectEquals(state0->gain.getGainLinear(), stateI->gain.getGainLinear());
            expectEquals(state0->iirCascade.size(), stateI->iirCascade.size());
            for (int j = 0; j < state0->iirCascade.size(); j++)
            {
                auto& c0 = state0->iirCascade[j]->coefficients.get()->coefficients;
                auto& cI = stateI->iirCascade[j]->coefficients.get()->coefficients;
                expectEquals(c0.size(), cI.size());
                for (int k = 0; k < c0.size(); k++)
                    expectEquals(c0[k], cI[k]);
            }
            
            auto& c0 = state0->firFilter->coefficients.get()->coefficients;
            auto& cI = stateI->firFilter->coefficients.get()->coefficients;
            expectEquals(c0.size(), cI.size());
            for (int k = 0; k < c0.size(); k++)
                expectEquals(c0[k], cI[k]);
        }
    }

    void checkValues(
        std::atomic<FullState<float>*>& procState,
        int expectedDelay,
        float expectedGain,
        std::vector<std::vector<float>>& expectedIirCoefficients,
        std::vector<float>& expectedFirCoefficients)
    {
        auto state = (*(procState.load()))[0];
        expectEquals((int)state->delay.getDelay(), expectedDelay);
        expectWithinAbsoluteError(state->gain.getGainLinear(), expectedGain, maxRelError * expectedGain);
        expectEquals(state->iirCascade.size(), (int)expectedIirCoefficients.size());
        for (int i = 0; i < state->iirCascade.size(); i++)
        {
            auto& c = state->iirCascade[i]->coefficients.get()->coefficients;
            std::vector<float> rrr;
            for (int j = 0; j < c.size(); j++)
                rrr.push_back(c[j]);
            expectEquals(c.size(), (int)expectedIirCoefficients[i].size());
            for (int j = 0; j < c.size(); j++)
                expectWithinAbsoluteError(c[j], expectedIirCoefficients[i][j], maxRelError * std::abs(expectedIirCoefficients[i][j]));
        }

        auto& c = state->firFilter->coefficients.get()->coefficients;
            expectEquals(c.size(), (int)expectedFirCoefficients.size());
        std::vector<float> rrr;
        for (int j = 0; j < c.size(); j++)
            rrr.push_back(c[j]);
        for (int j = 0; j < c.size(); j++)
            expectWithinAbsoluteError(c[j], expectedFirCoefficients[j], maxRelError * std::abs(expectedFirCoefficients[j]));
    }
};

static ProcessorChainModifierTest processorChainModifierTest;