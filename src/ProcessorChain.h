#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

template<typename SampleType>
struct ProcessorChain
{
    juce::dsp::DelayLine<SampleType> delay;
    juce::OwnedArray<juce::dsp::IIR::Filter<SampleType>> iirCascade;
    std::unique_ptr<juce::dsp::FIR::Filter<SampleType>> firFilter;
    juce::dsp::Gain<SampleType> gain;

    ProcessorChain()
    {
        std::array<float, 1> c{ 1.f };
        firFilter.reset(new juce::dsp::FIR::Filter<float>{ new juce::dsp::FIR::Coefficients<float> { c.data(), 1} });
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        jassert(spec.numChannels == 1); // ProcessorChain is a mono-processor
        delay.prepare(spec);
        for (auto* f : iirCascade) f->prepare(spec);
        firFilter->prepare(spec);
        gain.prepare(spec);
    }

    void process(juce::dsp::ProcessContextReplacing<float>& context)
    {
        delay.process(context);
        for (auto* f : iirCascade) f->process(context);
        firFilter->process(context);
        gain.process(context);
    }
};

template<typename SampleType>
using FullState = juce::OwnedArray<ProcessorChain<SampleType>>;
