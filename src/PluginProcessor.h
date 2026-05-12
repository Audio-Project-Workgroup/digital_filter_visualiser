#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "FilterState.h"
#include "ValueChangeBroadcaster.h"

#include "RootsToCoefficients.h"
#include "ProcessorChain.h"
#include "ProcessorChainModifier.h"

//==============================================================================
enum class PlayerState
{
	Empty,
	Stopped,
	Playing,
	Paused
};

class AudioPluginAudioProcessor final :
	public juce::AudioProcessor,
	public juce::ChangeBroadcaster,
	juce::ChangeListener
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
  void changeListenerCallback(juce::ChangeBroadcaster* source) override;
  void setTransportSourceFromFile(juce::File file);

  juce::UndoManager um;
  juce::AudioProcessorValueTreeState apvts;
  std::unique_ptr<FilterState> filterState;

  // TODO(ry): We should be able to simplify the active/pending state swap
  std::atomic<FullState<SampleType>*> activeState;
  FullState<SampleType>* pendingState;
  std::atomic<bool> isPendingStateReady{ false };
  std::atomic<juce::uint32> lastProcessTime;

  bool isPrepared = false;
  juce::dsp::ProcessSpec spec;

  // for standalone version
  juce::AudioFormatManager formatManager;
  std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
  std::unique_ptr<juce::ResamplingAudioSource> resamplerSource;   // for the case when DAW or audio card sampling rate doesn't match file sampling rate
  juce::AudioTransportSource transportSource;
  ValueChangeBroadcaster<PlayerState> playerState;

private:
	juce::AudioBuffer<SampleType> crossFadeBuffer;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
