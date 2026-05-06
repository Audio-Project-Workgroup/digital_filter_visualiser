#include "PluginEditor.h"

#include "ComplexPlaneEditor.cpp"
#include "PhaseFrequencyResponseViewer.cpp"
#include "CoeffComponents.cpp"
#include "StateSerializer.cpp"
#include "ButtonPanel.cpp"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
  :AudioProcessorEditor (&p)
  ,complexPlaneEditor(&p)
  ,phaseFrequencyResponseViewer(&p)
  ,coefficients(&p)
  ,buttonPanel(p)
{
  addAndMakeVisible(complexPlaneEditor);
  addAndMakeVisible(coefficients);
  addAndMakeVisible(phaseFrequencyResponseViewer);
  addAndMakeVisible(buttonPanel);

  setSize(960, 520);
  setResizable(true, true);
}

AudioPluginAudioProcessorEditor::
~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::
paint(juce::Graphics& g)
{
  juce::ignoreUnused(g);
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

  // g.setColour (juce::Colours::white);
  // g.setFont (15.0f);
  // g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::
resized()
{
    constexpr int padding = 5;
    auto bounds = getLocalBounds();
    auto buttonPanelBounds = bounds.removeFromTop(40);
    auto phaseFrequencyResponseViewerBounds = bounds.removeFromRight(bounds.getWidth() / 3);
    auto complexPlaneEditorBounds = bounds.removeFromRight(bounds.getWidth() / 2);
    buttonPanel.setBounds(buttonPanelBounds);
    phaseFrequencyResponseViewer.setBounds(phaseFrequencyResponseViewerBounds.reduced(padding));
    complexPlaneEditor.setBounds(complexPlaneEditorBounds.reduced(padding));
    coefficients.setBounds(bounds.reduced(padding));
}
