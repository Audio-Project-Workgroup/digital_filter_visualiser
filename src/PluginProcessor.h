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

namespace IDs
{
  const juce::Identifier FilterState("FILTER_STATE");
  const juce::Identifier Root("Root");
  const juce::Identifier Zeros("Zeros");
  const juce::Identifier Poles("Poles");
  const juce::Identifier Conjugate("Conjugate");
  const juce::Identifier ValueRe("ValueReal");
  const juce::Identifier ValueIm("ValueImag");
  const juce::Identifier Order("Order");
}

struct FilterRoot
{
  using Ptr = juce::WeakReference<FilterRoot>;

  FilterRoot(juce::ValueTree v) : node(v)
  {
    conjugate.referTo(node, IDs::Conjugate, nullptr);
    value.re.referTo(node, IDs::ValueRe, nullptr);
    value.im.referTo(node, IDs::ValueIm, nullptr);
    order.referTo(node, IDs::Order, nullptr);
  }

  juce::ValueTree node; // each root manages its own node in the state tree

  juce::CachedValue<int> conjugate; // the index of this root's conjugate node in the state tree
  struct {
    juce::CachedValue<double> re;
    juce::CachedValue<double> im;
    void setValue(c128 v, juce::UndoManager *um)
    {
      re.setValue(v.real(), um);
      im.setValue(v.imag(), um);
    }
  } value;
  juce::CachedValue<int> order;

  // Ptr conjugate;
  // c128 value;
  // s32 order;

private:
  JUCE_DECLARE_WEAK_REFERENCEABLE(FilterRoot);
};

struct FilterState
{
  FilterState(juce::AudioProcessor &p, juce::UndoManager *um)
    : apvts(p, um)
  {
    if(!apvts.state.isValid())
    {
      apvts.state = juce::ValueTree(IDs::FilterState);
    }
    auto zerosNode = apvts.state.getOrCreateChildWithName(IDs::Zeros, um);
    auto polesNode = apvts.state.getOrCreateChildWithName(IDs::Poles, um);
  }

  FilterRoot::Ptr add(s32 newOrder)
  {
    juce::ValueTree newNode(IDs::Root);
    newNode.setProperty(IDs::Order, newOrder, nullptr);
    newNode.setProperty(IDs::ValueRe, 0.0, nullptr);
    newNode.setProperty(IDs::ValueIm, 0.0, nullptr);
    newNode.setProperty(IDs::Conjugate, -1, nullptr);

    FilterRoot *root = nullptr;
    if(newOrder > 0)
    {
      // NOTE: adding a zero
      apvts.state.getChildWithName(IDs::Zeros).appendChild(newNode, apvts.undoManager);
      root = zeros.add(new FilterRoot(newNode));
    }
    else if(newOrder < 0)
    {
      // NOTE: adding a pole
      apvts.state.getChildWithName(IDs::Poles).appendChild(newNode, apvts.undoManager);
      root = poles.add(new FilterRoot(newNode));
    }
    else
    {
      jassertfalse; // order must be nonzero;
    }
    order += u32(std::abs(newOrder));
    FilterRoot::Ptr result(root);
    return(result);
  }

  void remove(FilterRoot::Ptr rootRef)
  {
    // NOTE: if the root has already been deleted, we don't need to do anything
    if(auto root = rootRef.get())
    {
      auto node = root->node;
      auto parent = node.getParent();
      parent.removeChild(node, apvts.undoManager);

      // NOTE: `removeObject` does nothing if the passed object is not in the array
      zeros.removeObject(root);
      poles.removeObject(root);
    }
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
