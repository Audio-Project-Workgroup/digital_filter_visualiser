#include "FilterState.h"

#include "profile.cpp"

// NOTE(ry): FilterRoot implementations

FilterRoot::CachedOrder& FilterRoot::CachedOrder::
operator+=(const int &delta)
{
  int current = this->get();
  *this = current + delta;

  return(*this);
}

// NOTE(ry): FilterState implementations

juce::ListenerList<juce::ValueTree::Listener> FilterState::listeners;

FilterState::
FilterState(juce::ValueTree treeToUse, juce::UndoManager *umToUse)
  : treeRoot(treeToUse),
    um(umToUse)
{
  jassert(treeRoot.isValid()); // the passed value tree root node must be valid

  auto zerosNode = treeRoot.getOrCreateChildWithName(IDs::Zeros, nullptr);
  auto polesNode = treeRoot.getOrCreateChildWithName(IDs::Poles, nullptr);

  totalOrder = 0;
  finiteZerosOrder = 0;

  syncListener(this);

  gain.referTo(treeRoot, IDs::Gain, um);
  if(!treeRoot.hasProperty(IDs::Gain))
  {
    treeRoot.setProperty(IDs::Gain, 1.0, nullptr);
  }

  treeRoot.addListener(this);
  zerosNode.addListener(this);
  polesNode.addListener(this);
}

FilterRoot::Ptr FilterState::
add(s32 newOrder)
{
  jassert(newOrder != 0); // order must be nonzero

  auto const isZero = newOrder > 0;
  auto const defRe = isZero ? 1.0 : 0.0;
  auto const defIm = 0.0;
  auto const defValue = c128(defRe, defIm);

  auto result = add(newOrder, defValue);
  return(result);
}

FilterRoot::Ptr FilterState::
add(s32 newOrder, c128 newValue)
{
  jassert(newOrder != 0); // order must be nonzero

  // TODO(ry): I hate having to do a linear scan here, especially since we scan
  // through *again* to get the root pointer from the value tree node (we
  // shouldn't have to do that either). We should really make a data structure
  // for storing roots that supports spatial queries

  // NOTE(ry): if there is already a root at the desired location, we don't add
  // a new one, and instead increment the existing root's order
  auto const isZero = newOrder > 0;
  if(isZero)
  {
    for(auto *z : zeros)
    {
      if(z->value == newValue)
      {
	z->order += newOrder;
	return(z);
      }
    }
  }
  else
  {
    for(auto *p : poles)
    {
      if(p->value == newValue)
      {
	p->order += newOrder;
	return(p);
      }
    }
  }

  // NOTE(ry): setting initial values is not undoable
  juce::ValueTree newNode(IDs::Root);
  newNode.setProperty(IDs::Order, newOrder, nullptr);
  newNode.setProperty(IDs::ValueRe, newValue.real(), nullptr);
  newNode.setProperty(IDs::ValueIm, newValue.imag(), nullptr);

  if(isZero)
  {
    treeRoot.getChildWithName(IDs::Zeros).appendChild(newNode, getCurrentUndoManager());
  }
  else
  {
    treeRoot.getChildWithName(IDs::Poles).appendChild(newNode, getCurrentUndoManager());
  }

  FilterRoot::Ptr result = getRootFromTreeNode(newNode);
  return(result);
}

void FilterState::
remove(FilterRoot::Ptr rootRef)
{
  // NOTE: if the root has already been deleted, we don't need to do anything
  if(auto root = rootRef.get())
  {
    auto node = root->node;
    // auto parent = node.getParent();
    // parent.removeChild(node, getCurrentUndoManager());

    // TODO(ry): this is a hacky fix to make removals that trigger slack pole
    // additions work with undo. the way the ValueTree/UndoManager system works
    // doesn't handle the case where a child addition happens inside a child
    // removal, which is what currently happens when we remove a pole with
    // nonzero order (and that removal violates causality). setting the order to
    // 0 defers the slack pole addition until after the removal action has
    // completed, so everything behaves nicely. we should consider better ways
    // of maintaining invariants after multiple state updates, such as using
    // AsyncUpdater or a better data structure.
    node.setProperty(IDs::Order, 0, getCurrentUndoManager());
  }
}

void FilterState::
addListener(juce::ValueTree::Listener *listener)
{
  treeRoot.addListener(listener);
  listeners.add(listener);
}

void FilterState::
removeListener(juce::ValueTree::Listener *listener)
{
  treeRoot.removeListener(listener);
  listeners.remove(listener);
}

// TODO(ry): I'd love to not have to do a linear scan to associate value tree
// nodes with weak references to filter root objects, but other methods have
// proven difficult to implement
FilterRoot::Ptr FilterState::
getRootFromTreeNode(const juce::ValueTree &nodeToFind)
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

void FilterState::
valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child)
{
  if(child.hasType(IDs::Root))
  {
    r64 valueIm = child.getProperty(IDs::ValueIm);
    bool wasOnAxis = juce::exactlyEqual(valueIm, 0.0);
    s32 order = child.getProperty(IDs::Order);
    s32 orderIncrement = wasOnAxis ? order : 2*order;

    if(parent.hasType(IDs::Zeros))
    {
      auto *zero = zeros.add(new FilterRoot(child, um));
      zero->wasOnAxis = wasOnAxis;
      zero->lastKnownOrder = order;
      incrementFilterOrder(std::abs(orderIncrement), false);
    }
    else if(parent.hasType(IDs::Poles))
    {
      auto *pole = poles.add(new FilterRoot(child, um));
      pole->wasOnAxis = wasOnAxis;
      pole->lastKnownOrder = order;
      incrementFilterOrder(std::abs(orderIncrement), true);
    }
  }

  DBG("filter order: " << int(totalOrder));
  DBG("filter zeros order: " << int(finiteZerosOrder));
}

void FilterState::
valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index)
{
  juce::ignoreUnused(index);

  if(child.hasType(IDs::Root))
  {
    auto root = getRootFromTreeNode(child);

    r64 valueIm = child.getProperty(IDs::ValueIm);
    s32 order = child.getProperty(IDs::Order);
    auto const isPole = parent.hasType(IDs::Poles);

    if(root.get())
    {
      // NOTE(ry): apparently removeObject can just fail silently?! like at
      // least log something, or return an indicator that the operation failed
      // that I can check so I know what's actually happening. some seriously
      // sloppy shit here. this api ain't worth a cent. anyone who buys a juce
      // commercial license seriously needs a psychological evaluation
      if(isPole)
      {
        poles.removeObject(root);
      }
      else
      {
        zeros.removeObject(root);
      }

      if(juce::exactlyEqual(valueIm, 0.0))
      {
        incrementFilterOrder(-std::abs(order), isPole);
      }
      else
      {
        incrementFilterOrder(-2*std::abs(order), isPole);
      }
    }
  }

  DBG("filter order: " << int(totalOrder));
  DBG("filter zeros order: " << int(finiteZerosOrder));
}

void FilterState::
valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property)
{
  if(property == IDs::ValueRe || property == IDs::ValueIm)
  {
    if(auto *root = getRootFromTreeNode(node).get())
    {
      double valueIm = node.getProperty(IDs::ValueIm);
      auto isOnAxis = juce::exactlyEqual(valueIm, 0.0);
      if(isOnAxis != root->wasOnAxis)
      {
        // NOTE(ry): update filter order if a conjugate root was created or destroyed
        int rootOrder = node.getProperty(IDs::Order);
        bool isPole = rootOrder < 0;
        if(isOnAxis)
        {
          incrementFilterOrder(-std::abs(rootOrder), isPole);
        }
        else
        {
          incrementFilterOrder(std::abs(rootOrder), isPole);
        }

	DBG("filter order: " << int(totalOrder));
	DBG("filter zeros order: " << int(finiteZerosOrder));
      }
      root->wasOnAxis = isOnAxis;
    }
  }
  else if(property == IDs::Order)
  {
    if(auto *root = getRootFromTreeNode(node).get())
    {
      int const newOrder = node.getProperty(IDs::Order);
      if(newOrder == 0)
      {
	// NOTE(ry): we remove the root if it has zero order.
	auto parent = node.getParent();
	parent.removeChild(node, getCurrentUndoManager());

	// NOTE(ry): we also have to decrement the filter order here since
	// removing the root will not do that (the order is now zero!)
	auto const delta = -root->lastKnownOrder;
	auto const decrement = root->isReal() ? delta : 2*delta;
	auto const wasPole = root->lastKnownOrder < 0;
	incrementFilterOrder(wasPole ? -decrement : decrement, wasPole);
      }
      else
      {
	jassert((newOrder < 0) == (root->lastKnownOrder < 0)); // we can't have the root order change sign

	auto const isPole = newOrder < 0;
	auto const delta = newOrder - root->lastKnownOrder;
	auto const increment = root->isReal() ? delta : 2*delta;
	root->lastKnownOrder = newOrder;
	incrementFilterOrder(isPole ? -increment : increment, isPole);

	DBG("filter order: " << int(totalOrder));
	DBG("filter zeros order: " << int(finiteZerosOrder));
      }
    }
  }
}

void FilterState::
incrementFilterOrder(int delta, bool isPole)
{
  // NOTE(ry): increment the appropriate order variable
  if(isPole)
  {
    s32 result = s32(totalOrder) + delta;
    jassert(result >= 0);
    totalOrder = u32(result);
  }
  else
  {
    s32 result = s32(finiteZerosOrder) + delta;
    jassert(result >= 0);
    finiteZerosOrder = u32(result);
  }

  // NOTE(ry): if we're in the middle of an undo/redo, we shouldn't try to
  // maintain the causality invariant through additional state
  // modifications. the transaction should contain changes that when performed
  // result in a valid state
  if(!um->isPerformingUndoRedo())
  {
    // NOTE(ry): maintain the causality invariant.
    if(finiteZerosOrder > totalOrder)
    {
      // NOTE(ry): add a pole to make up for the difference in order
      u32 slack = finiteZerosOrder - totalOrder;
      DBG("creating slack pole of order " << int(slack));
      // NOTE(ry): `add()` itself calls `incrementFilterOrder()`, but we don't end
      // up in an infinite loop since this branch will not be taken in the second
      // call (the invariant will have been satisfied)
      add(-s32(slack));
    }
    jassert(totalOrder >= finiteZerosOrder);
  }
}

void FilterState::
syncListener(juce::ValueTree::Listener *listener)
{
  auto polesNode = treeRoot.getChildWithName(IDs::Poles);
  for(auto pole : polesNode)
  {
    listener->valueTreeChildAdded(polesNode, pole);
  }

  auto zerosNode = treeRoot.getChildWithName(IDs::Zeros);
  for(auto zero : zerosNode)
  {
    listener->valueTreeChildAdded(zerosNode, zero);
  }

  listener->valueTreePropertyChanged(treeRoot, IDs::Gain);
}

juce::UndoManager* FilterState::
getCurrentUndoManager(void)
{
  auto *result = um->isPerformingUndoRedo() ? nullptr : um;
  return(result);
}
