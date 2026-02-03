#pragma once

#include "PluginProcessor.h"

//==============================================================================
class RootSliderComponent final : public juce::Component, private juce::ValueTree::Listener
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

  void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override
  {
    sliders.removeLast();
    sliders.removeLast();
  }

  juce::TextButton addRoot;
  juce::TextButton delRoot;
  juce::TextButton undo;
  juce::TextButton redo;
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
