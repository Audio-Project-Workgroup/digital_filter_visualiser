#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#define DISABLE_PROFILING
#include "context.h"
#include "types.h"
#include "profile.h"

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
    CachedComplex& operator=(const c128 &other)
    {
      re = other.real();
      im = other.imag();
      return *this;
    }

    bool operator==(const c128 &other)
    {
      // NOTE(ry): it really sucks that juce code doesn't comply with their own
      // warnings on gcc. Otherwise this would be a one-liner.
      r64 const real = re;
      r64 const imag = im;
      auto const result =
	juce::approximatelyEqual(real, other.real()) &&
	juce::approximatelyEqual(imag, other.imag());
      return(result);
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

    juce::CachedValue<double> re;
    juce::CachedValue<double> im;
  };

  struct CachedOrder : juce::CachedValue<int>
  {
    CachedOrder& operator=(const int &order)
    {
      juce::CachedValue<int>::operator=(order);
      return(*this);
    }

    CachedOrder& operator+=(const int &delta);
  };

  FilterRoot(juce::ValueTree v, juce::UndoManager *um) : node(v)
  {
    value.re.referTo(node, IDs::ValueRe, um);
    value.im.referTo(node, IDs::ValueIm, um);
    order.referTo(node, IDs::Order, um);
  }

  /** returns true if the root is on the real axis */
  bool isReal(void)
  {
    return(juce::exactlyEqual(value.im.get(), 0.0));
  }
  /** returns true if the root is on the real axis */
  bool isReal(void) const
  {
    return(juce::exactlyEqual(value.im.get(), 0.0));
  }

  /** returns true if the root is at the origin */
  bool isAtZero(void)
  {
    return(juce::exactlyEqual(std::abs(value.get()), 0.0));
  }
  /** returns true if the root is at the origin */
  bool isAtZero(void) const
  {
    return(juce::exactlyEqual(std::abs(value.get()), 0.0));
  }

  /** check if a root is a zero */
  bool isZero(void)
  {
    return(order.get() > 0);
  }
  /** check if a root is a zero */
  bool isZero(void) const
  {
    return(order.get() > 0);
  }

  /** check if a root is a pole */
  bool isPole(void)
  {
    return(order.get() < 0);
  }
  /** check if a root is a pole */
  bool isPole(void) const
  {
    return(order.get() < 0);
  }

  /** each root manages its own node in the state tree
   */
  juce::ValueTree node;

  /** the root's position in the complex plane
   */
  CachedComplex value;

  /** the root's order. contributes x2 when the value is not real. positive for
   * zeros, negative for poles.
   */
  CachedOrder order;

  /** for getting the order increment in a property change callback
   */
  int lastKnownOrder;

  /** for detecting when a root needs to be split into a complex conjugate pair
   * or a pair needs to be merged back onto the real axis.
   */
  bool wasOnAxis;

private:
  JUCE_DECLARE_WEAK_REFERENCEABLE(FilterRoot);
};

struct FilterState : private juce::ValueTree::Listener
{
  FilterState(juce::ValueTree treeToUse, juce::UndoManager *umToUse);

  /** Add a new root with given order.
   * Positive order adds a zero, negative order adds a pole.
   * Creates value tree node and adds it as a child to the appropriate node.
   * Filter order is updated automatically.
   */
  FilterRoot::Ptr add(s32 newOrder);

  /** Remove given root.
   * This is the only function that should destroy `FilterRoot`s.
   * Filter order is updated automatically.
   */
  void remove(FilterRoot::Ptr rootRef);

  /** Let listeners add or remove themselves to the underlying value tree
   */
  void addListener(juce::ValueTree::Listener *listener);
  void removeListener(juce::ValueTree::Listener *listener);

  // TODO(ry): make this constant-time using acceleration structure
  /** Helper function for mapping value tree nodes to filter root pointers
   */
  FilterRoot::Ptr getRootFromTreeNode(const juce::ValueTree &nodeToFind);

  /** Helper function for incrementing filter order.
   * Adds poles to satisfy causality invariant.
   */
  void incrementFilterOrder(int delta, bool isPole);

  juce::OwnedArray<FilterRoot> zeros;
  juce::OwnedArray<FilterRoot> poles;
  juce::CachedValue<double> gain;

  /** the sum of the orders of all zeros in the finite plane. Causality
   *requires this be at most the total order.
   */
  u32 finiteZerosOrder;

  /** the total order of the filter. Causality requires this to equal
   * the sum of the negative orders of all poles.
   */
  u32 totalOrder;

  // TODO(ry): separate trees for filter roots and parameters/automation
  juce::ValueTree treeRoot;
  juce::UndoManager *um;

private:

  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index) override;
  void valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property) override;

  /** this function should be called to safely handle the case when we need to
   * modify the state in response to other state modification. It is necessary
   * to pass the undo manager when we want the modification to be undoable,
   * unless we are reacting to a modification triggered by an udno/redo, in
   * which we do not want the action to be undoable and this returns nullptr.
   */
  juce::UndoManager* getCurrentUndoManager(void);
};
