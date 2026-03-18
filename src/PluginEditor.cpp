#include "PluginEditor.h"

// NOTE(ry): Don't delete this. I know what I'm doing
#include "ComplexPlaneEditor.cpp"

//==============================================================================
CoefficientsComponent::CoefficientsComponent()
{}

CoefficientsComponent::~CoefficientsComponent()
{}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
  : AudioProcessorEditor (&p), processorRef (p), complexPlaneEditor(*p.filterState), phaseFrequencyResponseViewer(&p)
{
  juce::ignoreUnused (processorRef);

  addAndMakeVisible(complexPlaneEditor);
  addAndMakeVisible(coefficients);
  addAndMakeVisible(phaseFrequencyResponseViewer);

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
  phaseFrequencyResponseViewer.paint(g);
}

void AudioPluginAudioProcessorEditor::resized()
{
  coefficients.setBounds(0, 0, getWidth() / 2, getHeight() / 2);
  phaseFrequencyResponseViewer.setBounds(0, getHeight() / 2, getWidth() / 2, getHeight() / 2);
  complexPlaneEditor.setBounds(getWidth() / 2, 0, getWidth() / 2, getHeight());
}
