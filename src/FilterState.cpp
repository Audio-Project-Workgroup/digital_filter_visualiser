#include "FilterState.h"

FilterState::
FilterState(juce::ValueTree treeToUse, juce::UndoManager *umToUse)
  : treeRoot(treeToUse),
    um(umToUse)
{
  jassert(treeRoot.isValid()); // the passed value tree root node must be valid

  totalOrder = 0;
  finiteZerosOrder = 0;

  auto zerosNode = treeRoot.getOrCreateChildWithName(IDs::Zeros, nullptr);
  auto polesNode = treeRoot.getOrCreateChildWithName(IDs::Poles, nullptr);

  treeRoot.setProperty(IDs::Gain, 0.0, nullptr);
  gain.referTo(treeRoot, IDs::Gain, um);

  treeRoot.addListener(this);
  zerosNode.addListener(this);
  polesNode.addListener(this);
}

FilterRoot::Ptr FilterState::
add(s32 newOrder)
{
  jassert(newOrder != 0); // order must be nonzero

  juce::ValueTree newNode(IDs::Root);
  newNode.setProperty(IDs::Order, newOrder, nullptr);
  newNode.setProperty(IDs::ValueRe, newOrder > 0 ? 1.0 : 0.0, nullptr);
  newNode.setProperty(IDs::ValueIm, 0.0, nullptr);

  // NOTE(ry): This function can get called during an undo/redo operation. New
  // undoable actions can't be created while the undo manager is performing
  // undo/redo, so we can't pass a non-null undo manager in that case.
  auto *currentUm = um->isPerformingUndoRedo() ? nullptr : um;
  if(newOrder > 0)
  {
    treeRoot.getChildWithName(IDs::Zeros).appendChild(newNode, currentUm);
  }
  else
  {
    treeRoot.getChildWithName(IDs::Poles).appendChild(newNode, currentUm);
  }

  FilterRoot::Ptr result = getRootFromTreeNode(newNode);
  //result.get()->wasOnAxis = true;
  return(result);
}

void FilterState::
remove(FilterRoot::Ptr rootRef)
{
  // NOTE: if the root has already been deleted, we don't need to do anything
  if(auto root = rootRef.get())
  {
    auto node = root->node;
    auto parent = node.getParent();
    parent.removeChild(node, um);
  }
}

void FilterState::
addListener(juce::ValueTree::Listener *listener)
{
  treeRoot.addListener(listener);
}

void FilterState::
removeListener(juce::ValueTree::Listener *listener)
{
  treeRoot.removeListener(listener);
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
    s32 order = std::abs(s32(child.getProperty(IDs::Order)));
    s32 orderIncrement = wasOnAxis ? order : 2*order;

    if(parent.hasType(IDs::Zeros))
    {
      auto *zero = zeros.add(new FilterRoot(child, um));
      zero->wasOnAxis = wasOnAxis;
      incrementFilterOrder(orderIncrement, false);
    }
    else if(parent.hasType(IDs::Poles))
    {
      auto *pole = poles.add(new FilterRoot(child, um));
      pole->wasOnAxis = wasOnAxis;
      incrementFilterOrder(orderIncrement, true);
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
    if(auto *rootPtr = root.get())
    {
      r64 valueIm = rootPtr->value.im;
      s32 order = rootPtr->order;
      bool isPole = parent.hasType(IDs::Poles);

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
    else if(property == IDs::Order)
    {
      // TODO(ry): maintain causality through update to origin pole
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

  // NOTE(ry): make sure the order invariant is satisfied
  if(finiteZerosOrder > totalOrder)
  {
    // NOTE(ry): add a pole to make up for the difference in order
    // TODO(ry): automatically merge newly created roots so we don't have
    // multiple slack poles lying on top of each other
    u32 slack = finiteZerosOrder - totalOrder;
    DBG("creating slack pole of order " << int(slack));
    add(-s32(slack));
    // NOTE(ry): `add()` itself calls `incrementFilterOrder()`, but we don't end
    // up in an infinite loop since this branch will not be taken in the second
    // call (the invariant will have been satisfied)
  }
  jassert(totalOrder >= finiteZerosOrder);
}
