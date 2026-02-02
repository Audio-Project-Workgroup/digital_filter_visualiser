#pragma once

#include "PluginProcessor.h"

//==============================================================================
class RootSliderComponent final : public juce::Component
{
public:
  RootSliderComponent(AudioPluginAudioProcessor &p);
  ~RootSliderComponent();

  struct Slider : juce::Slider
  {
    Slider(FilterState &s, FilterRoot::Ptr r)
      : juce::Slider(), stateRef(s), root(r)
    {}
    ~Slider()
    {
      stateRef.remove(root);
    }

  private:
    FilterState &stateRef;
    FilterRoot::Ptr root;
  };

  //void paint(juce::Graphics&) override;
  void resized() override;

private:
  juce::TextButton addRoot;
  juce::TextButton delRoot;
  juce::OwnedArray<Slider> sliders;
  AudioPluginAudioProcessor &processorRef;
};

//==============================================================================
class CoefficientsComponent final : public juce::Component
{
public:
  CoefficientsComponent();
  ~CoefficientsComponent();

  //void paint(juce::Graphics&) override;
private:
};

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
  explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
  ~AudioPluginAudioProcessorEditor() override;

  //==============================================================================
  void paint (juce::Graphics&) override;
  void resized() override;

private:
  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  AudioPluginAudioProcessor& processorRef;

  RootSliderComponent rootSliders;
  CoefficientsComponent coefficients;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
