#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

template<typename SampleType>
struct ProcessorChain
{
    juce::dsp::DelayLine<SampleType> delay;
    juce::OwnedArray<juce::dsp::IIR::Filter<SampleType>> iirCascade;
    juce::OwnedArray<juce::dsp::FIR::Filter<SampleType>> firCascade;

    void prepare(const juce::dsp::ProcessSpec& spec)
    { // spec.numChannels is always 1 as it is a mono-processor
        delay.prepare(spec);
        for (auto* f : iirCascade) f->prepare(spec);
        for (auto* f : firCascade) f->prepare(spec);
    }

    void process(juce::dsp::ProcessContextReplacing<float>& context)
    {
        delay.process(context);
        for (auto* f : iirCascade) f->process(context);
        for (auto* f : firCascade) f->process(context);
    }
};

template<typename SampleType>
using FullState = juce::OwnedArray<ProcessorChain<SampleType>>;