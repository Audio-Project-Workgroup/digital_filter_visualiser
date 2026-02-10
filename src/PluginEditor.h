#pragma once

#include "PluginProcessor.h"

#include "ComplexPlaneEditor.h"

class CoefficientsComponent final : public juce::Component
{
public:
  CoefficientsComponent();
  ~CoefficientsComponent();

  void paint(juce::Graphics& g) override
  {
    g.fillAll(juce::Colours::white);
  }
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

  ComplexPlaneEditor complexPlaneEditor;
  CoefficientsComponent coefficients;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
