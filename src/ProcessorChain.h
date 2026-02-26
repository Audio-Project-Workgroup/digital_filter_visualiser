#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

template<typename SampleType>
struct ProcessorChain
{
    juce::dsp::DelayLine<SampleType> delay;
    juce::OwnedArray<juce::dsp::IIR::Filter<SampleType>> iirCascade;
    juce::OwnedArray<juce::dsp::FIR::Filter<SampleType>> firCascade;
    juce::dsp::Gain<SampleType> gain;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        jassert(spec.numChannels == 1); // ProcessorChain is a mono-processor
        delay.prepare(spec);
        for (auto* f : iirCascade) f->prepare(spec);
        for (auto* f : firCascade) f->prepare(spec);
        gain.prepare(spec);
    }

    void process(juce::dsp::ProcessContextReplacing<float>& context)
    {
        delay.process(context);
        for (auto* f : iirCascade) f->process(context);
        for (auto* f : firCascade) f->process(context);
        gain.process(context);
    }
};

template<typename SampleType>
using FullState = juce::OwnedArray<ProcessorChain<SampleType>>;