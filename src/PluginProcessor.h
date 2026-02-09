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
  const juce::Identifier ValueRe("ValueReal");
  const juce::Identifier ValueIm("ValueImag");
  const juce::Identifier Order("Order");
}

struct FilterRoot
{
  // NOTE(ry): This structure is meant to be used as a fast proxy object for
  // reading and updating the filter state. Using a value tree directly for
  // these purposes has two problems:
  // 1. Looking up a value by property requires a linear scan through the tree's
  //    properties, which is worst-case O(N) in the number of properties. You'd
  //    think the juce people would use some kind of accelerated O(1) lookup,
  //    like a hash map, but no. Not even after decades of development.
  // 2. Updating a property requires a lot of ceremony at the usage site (ie
  //    passing the property identifier, the new value, and the undo manager to
  //    use to a function).
  // We can lean on juce's CachedValue class template to get around these
  // problems. It stores the most recent value of the associated property, so
  // it can be read without a loop, and it allows the property to be modified
  // with a regular assignment operator (though we have to wrap it to pass the
  // undo manager).
  // The major drawback with the CachedValue is that it's value is updated in
  // a value tree listener, which is not synchronized with other value tree
  // listener callbacks. That means if you want the most recent value in a
  // `valueTreePropertyChanged()` callback, unfortunately you can't depend on
  // the cached value being up to date, and you have to do the O(N) property
  // lookup in the tree instead.
  // The major drawback with this structure itself is mapping a value tree node
  // to the FilterRoot structure that wraps it. You can't just write
  // std::unordered_map<juce::ValueTree, FilterRoot::Ptr> since ValueTree is not
  // a hashable object. For now, we do our own linear scan through the state
  // arrays to find which root wraps the given tree node, which is not good.
  // It's quite possible the disadvantages of this structure outweigh it's
  // benefits. If it turns out we only ever read from the value tree inside
  // property changed callbacks, then the convenience and flexibility of
  // "normal" update code does not justify the continued use of this structure.
  using Ptr = juce::WeakReference<FilterRoot>;

  struct CachedComplex
  {
    juce::CachedValue<double> re;
    juce::CachedValue<double> im;
    CachedComplex& operator=(const c128 &other)
    {
      re = other.real();
      im = other.imag();
      return *this;
    }

    c128 get(void)
    {
      return(c128(re.get(), im.get()));
    }
    c128 get(void) const
    {
      return(c128(re.get(), im.get()));
    }
    operator c128() const noexcept { return(get()); }
  };

  FilterRoot(juce::ValueTree v, juce::UndoManager *um) : node(v)
  {
    value.re.referTo(node, IDs::ValueRe, um);
    value.im.referTo(node, IDs::ValueIm, um);
    order.referTo(node, IDs::Order, um);
  }

  juce::ValueTree node; // each root manages its own node in the state tree

  CachedComplex value;
  juce::CachedValue<int> order;

private:
  JUCE_DECLARE_WEAK_REFERENCEABLE(FilterRoot);
};

struct FilterState : private juce::ValueTree::Listener
{
  FilterState(juce::AudioProcessor &p, juce::UndoManager *um)
    : apvts(p, um)
  {
    if(!apvts.state.isValid())
    {
      apvts.state = juce::ValueTree(IDs::FilterState);
    }

    auto zerosNode = apvts.state.getOrCreateChildWithName(IDs::Zeros, nullptr);
    auto polesNode = apvts.state.getOrCreateChildWithName(IDs::Poles, nullptr);

    apvts.state.addListener(this);
    zerosNode.addListener(this);
    polesNode.addListener(this);
  }

  FilterRoot::Ptr add(s32 newOrder)
  {
    juce::ValueTree newNode(IDs::Root);
    newNode.setProperty(IDs::Order, newOrder, apvts.undoManager);
    newNode.setProperty(IDs::ValueRe, 0.0, apvts.undoManager);
    newNode.setProperty(IDs::ValueIm, 0.0, apvts.undoManager);

    if(newOrder > 0)
    {
      apvts.state.getChildWithName(IDs::Zeros).appendChild(newNode, apvts.undoManager);
    }
    else if(newOrder < 0)
    {
      apvts.state.getChildWithName(IDs::Poles).appendChild(newNode, apvts.undoManager);
    }
    else
    {
      jassertfalse; // order must be nonzero;
    }
    FilterRoot::Ptr result = getRootFromTreeNode(newNode);
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
    }
  }

  void addListener(juce::ValueTree::Listener *listener)
  {
    apvts.state.addListener(listener);
  }

  void removeListener(juce::ValueTree::Listener *listener)
  {
    apvts.state.removeListener(listener);
  }

  // TODO(ry): I'd love to not have to do a linear scan to associate value tree
  // nodes with weak references to filter root objects, but other methods have
  // proven difficult to implement
  FilterRoot::Ptr getRootFromTreeNode(const juce::ValueTree &nodeToFind)
  {
    for(auto *z : zeros)
    {
      if(z->node == nodeToFind) return z;
    }
    for(auto *p : poles)
    {
      if(p->node == nodeToFind) return p;
    }
    jassertfalse;
    return nullptr;
  }

  juce::OwnedArray<FilterRoot> zeros;
  juce::OwnedArray<FilterRoot> poles;
  u32 order;

private:

  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override
  {
    if(child.hasType(IDs::Root))
    {
      if(parent.hasType(IDs::Zeros))
      {
        zeros.add(new FilterRoot(child, apvts.undoManager));
      }
      else if(parent.hasType(IDs::Poles))
      {
        poles.add(new FilterRoot(child, apvts.undoManager));
      }
      order += u32(std::abs(s32(child.getProperty(IDs::Order))));
    }
  }

  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index) override
  {
    juce::ignoreUnused(index);
    if(child.hasType(IDs::Root))
    {
      if(auto root = getRootFromTreeNode(child).get())
      {
        if(parent.hasType(IDs::Zeros))
        {
          zeros.removeObject(root);
        }
        else if(parent.hasType(IDs::Poles))
        {
          poles.removeObject(root);
        }
      }
    }
  }

  // TODO(ry): separate trees for filter roots and parameters/automation
  juce::AudioProcessorValueTreeState apvts;
  // juce::ValueTree root;
  // juce::UndoManager um;
};

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
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

  //juce::AudioProcessorValueTreeState state;
  FilterState state;
  juce::UndoManager um;

private:

  juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<SampleType>, juce::dsp::IIR::Coefficients<SampleType>> filter;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
