#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using b32 = s32;

using r32 = float;
using r64 = double;

using c64  = juce::dsp::Complex<float>;
using c128 = juce::dsp::Complex<double>;

//#include "filterstate.h"

struct FilterRoot
{
  using Ptr = juce::WeakReference<FilterRoot>;

  Ptr conjugate;
  c128 value;
  s32 order;

private:
  JUCE_DECLARE_WEAK_REFERENCEABLE(FilterRoot);
};

struct FilterState
{
  FilterState(juce::AudioProcessor &p, juce::UndoManager *um)
    : apvts(p, um)
  {}

  FilterRoot::Ptr add(s32 newOrder)
  {
    FilterRoot *root = nullptr;
    if(newOrder > 0)
    {
      root = zeros.add(new FilterRoot());
      root->order = newOrder;
    }
    else if(newOrder < 0)
    {
      root = poles.add(new FilterRoot());
      root->order = newOrder;
    }
    else
    {
      jassertfalse; // order must be nonzero;
    }
    FilterRoot::Ptr result(root);
    order += u32(std::abs(newOrder));
    return(result);
  }

  void remove(FilterRoot::Ptr root)
  {
    // NOTE: `removeObject` does nothing if the passed object is not in the array
    zeros.removeObject(root.get());
    poles.removeObject(root.get());
  }

  juce::OwnedArray<FilterRoot> zeros;
  juce::OwnedArray<FilterRoot> poles;
  u32 order;
private:
  juce::AudioProcessorValueTreeState apvts;
};

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:

  using SampleType = float;
  //using CoefficientType = juce::SmoothedValue<SampleType, juce::ValueSmoothingTypes::Linear>;

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

  //juce::AudioProcessorValueTreeState state;
  FilterState state;

private:

  juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<SampleType>, juce::dsp::IIR::Coefficients<SampleType>> filter;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
