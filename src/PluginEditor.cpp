#include "PluginEditor.h"

//==============================================================================
CoefficientsComponent::CoefficientsComponent()
{}

CoefficientsComponent::~CoefficientsComponent()
{}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
  : AudioProcessorEditor (&p), processorRef (p), complexPlaneEditor(p)
{
  juce::ignoreUnused (processorRef);

  addAndMakeVisible(complexPlaneEditor);
  addAndMakeVisible(coefficients);

  setSize(640, 480);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

  // g.setColour (juce::Colours::white);
  // g.setFont (15.0f);
  // g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
  complexPlaneEditor.paint(g);
  coefficients.paint(g);
}

void AudioPluginAudioProcessorEditor::resized()
{
  coefficients.setBounds(0, 0, getWidth() / 2, getHeight());
  complexPlaneEditor.setBounds(getWidth() / 2, 0, getWidth() / 2, getHeight());
}
