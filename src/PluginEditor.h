#pragma once

#include "PluginProcessor.h"

#include "ComplexPlaneEditor.h"
#include "PhaseFrequencyResponseViewer.h"
#include "CoeffComponents.h"

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
  PhaseFrequencyResponseViewer phaseFrequencyResponseViewer;
  CoefficientsComponent coefficients;
  juce::TextButton exportButton;
  juce::PopupMenu exportPopupMenu;
  std::unique_ptr<juce::FileChooser> chooser;
  void chooseFileAndSave(std::shared_ptr<juce::XmlElement>);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
