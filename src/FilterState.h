#pragma once

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using b32 = s32;

using r32 = float;
using r64 = double;

using c64 = juce::dsp::Complex<float>;
using c128 = juce::dsp::Complex<double>;

namespace IDs
{
  const juce::Identifier FilterState("FILTER_STATE");
  const juce::Identifier Root("Root");
  const juce::Identifier Zeros("Zeros");
  const juce::Identifier Poles("Poles");
  const juce::Identifier Gain("Gain");
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

  bool wasOnAxis;

private:
  JUCE_DECLARE_WEAK_REFERENCEABLE(FilterRoot);
};

struct FilterState : private juce::ValueTree::Listener
{
  FilterState(juce::AudioProcessor &p, juce::UndoManager *um);

  FilterRoot::Ptr add(s32 newOrder);
  void remove(FilterRoot::Ptr rootRef);

  void addListener(juce::ValueTree::Listener *listener);
  void removeListener(juce::ValueTree::Listener *listener);

  FilterRoot::Ptr getRootFromTreeNode(const juce::ValueTree &nodeToFind);

  juce::OwnedArray<FilterRoot> zeros;
  juce::OwnedArray<FilterRoot> poles;
  juce::CachedValue<double> gain;
  u32 finiteZerosOrder; // NOTE(ry): the sum of the orders of all zeros in the finite plane. Causality requires this be at most the total order.
  u32 totalOrder; // NOTE(ry): the total order of the filter. Causality requires this to equal the sum of the negative orders of all poles.

private:

  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index) override;
  void valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property) override;

  void incrementFilterOrder(int delta, bool isPole);

  // TODO(ry): separate trees for filter roots and parameters/automation
  juce::AudioProcessorValueTreeState apvts;
  // juce::ValueTree root;
  // juce::UndoManager um;
};
