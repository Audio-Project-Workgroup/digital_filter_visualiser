#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RootSliderComponent::RootSliderComponent(AudioPluginAudioProcessor &p)
  : addRoot("+"), delRoot("-"), undo("undo"), redo("redo"), processorRef(p)
{
  processorRef.state.addListener(this);

  addAndMakeVisible(addRoot);
  addAndMakeVisible(delRoot);

  addRoot.setBounds(100, 100, 100, 50);
  delRoot.setBounds(100, 150, 100, 50);

  addRoot.onClick = [this]{
    processorRef.state.add(1);
    processorRef.um.beginNewTransaction();
  };
  delRoot.onClick = [this]{
    sliders.removeLast();
    sliders.removeLast();
  };

  addAndMakeVisible(undo);
  addAndMakeVisible(redo);

  undo.setBounds(200, 100, 100, 50);
  redo.setBounds(200, 150, 100, 50);

  undo.onClick = [this]{
    DBG(processorRef.um.getUndoDescription());
    processorRef.um.undo();
  };
  redo.onClick = [this]{
    processorRef.um.redo();
  };
}

RootSliderComponent::~RootSliderComponent()
{
  processorRef.state.removeListener(this);
}

void RootSliderComponent::resized()
{
  auto area = getLocalBounds();
  addRoot.setBounds(area.removeFromTop(50).removeFromLeft(50));
  delRoot.setBounds(area.removeFromTop(50).removeFromLeft(50));
  for(auto *slider : sliders)
  {
    slider->setBounds(area.removeFromTop(50).reduced(5));
  }
  undo.setBounds(area.removeFromTop(50).removeFromLeft(100));
  redo.setBounds(area.removeFromTop(50).removeFromLeft(100));
}

//==============================================================================
CoefficientsComponent::CoefficientsComponent()
{}

CoefficientsComponent::~CoefficientsComponent()
{}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
  : AudioProcessorEditor (&p), processorRef (p), rootSliders(p)
{
  juce::ignoreUnused (processorRef);

  addAndMakeVisible(rootSliders);
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
  g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

  // g.setColour (juce::Colours::white);
  // g.setFont (15.0f);
  // g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
  rootSliders.paint(g);
  coefficients.paint(g);
}

void AudioPluginAudioProcessorEditor::resized()
{
  rootSliders.setBounds(0, 0, getWidth() / 2, getHeight());
  coefficients.setBounds(getWidth() / 2, 0, getWidth() / 2, getHeight());
}
