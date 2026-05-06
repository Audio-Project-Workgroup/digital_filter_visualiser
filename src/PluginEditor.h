#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "FilterState.h"

#include "ComplexPlaneEditor.h"
#include "PhaseFrequencyResponseViewer.h"
#include "CoeffComponents.h"
#include "ButtonPanel.h"
#include "StateSerializer.h"

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
  ComplexPlaneEditor complexPlaneEditor;
  PhaseFrequencyResponseViewer phaseFrequencyResponseViewer;
  CoefficientsComponent coefficients;
  ButtonPanel buttonPanel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
