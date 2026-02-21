#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "FilterState.h"
#include "ProcessorChain.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor, juce::ChangeListener
{
public:

  using SampleType = float;

  //==============================================================================
  AudioPluginAudioProcessor();
  ~AudioPluginAudioProcessor() override;

  //==============================================================================
  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

  void processBlock (juce::AudioBuffer<SampleType>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  //==============================================================================
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram (int index) override;
  const juce::String getProgramName (int index) override;
  void changeProgramName (int index, const juce::String& newName) override;

  //==============================================================================
  void getStateInformation (juce::MemoryBlock& destData) override;
  void setStateInformation (const void* data, int sizeInBytes) override;
  
  //==============================================================================
  void 	changeListenerCallback(juce::ChangeBroadcaster* source) override;

  //juce::AudioProcessorValueTreeState state;
  FilterState state;
  juce::UndoManager um;
  std::atomic<FullState<SampleType>*> activeState;
  FullState<SampleType>* pendingState;
  std::atomic<bool> isActiveStateUsed{ false };
  std::atomic<bool> isPendingStateUsed{ false };
  std::atomic<bool> isNewStateReady{ false };

  // This mutex avoids races when reading spec in ProcessorChainModifier class 
  // while writing it in prepareToPlay.
  std::mutex stateMutex;
  bool isPrepared = false;
  juce::dsp::ProcessSpec spec;

private:
	juce::AudioBuffer<float> crossFadeBuffer;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
