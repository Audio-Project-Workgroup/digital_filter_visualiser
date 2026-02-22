#include "FilterState.h"

FilterState::
FilterState(juce::AudioProcessor &p, juce::UndoManager *um)
  : apvts(p, um)
{
  if(!apvts.state.isValid())
  {
    apvts.state = juce::ValueTree(IDs::FilterState);
  }

  totalOrder = 0;
  finiteZerosOrder = 0;

  auto zerosNode = apvts.state.getOrCreateChildWithName(IDs::Zeros, nullptr);
  auto polesNode = apvts.state.getOrCreateChildWithName(IDs::Poles, nullptr);

  apvts.state.setProperty(IDs::Gain, 0.0, nullptr);
  gain.referTo(apvts.state, IDs::Gain, um);

  apvts.state.addListener(this);
  zerosNode.addListener(this);
  polesNode.addListener(this);
}

FilterRoot::Ptr FilterState::
add(s32 newOrder)
{
  jassert(newOrder != 0);

  juce::ValueTree newNode(IDs::Root);
  newNode.setProperty(IDs::Order, newOrder, apvts.undoManager);
  newNode.setProperty(IDs::ValueRe, newOrder > 0 ? 1.0 : 0.0, apvts.undoManager);
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
  result.get()->wasOnAxis = true;
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
    parent.removeChild(node, apvts.undoManager);
  }
}

void FilterState::
addListener(juce::ValueTree::Listener *listener)
{
  apvts.state.addListener(listener);
}

void FilterState::
removeListener(juce::ValueTree::Listener *listener)
{
  apvts.state.removeListener(listener);
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
    if(parent.hasType(IDs::Zeros))
    {
      zeros.add(new FilterRoot(child, apvts.undoManager));
      incrementFilterOrder(std::abs(s32(child.getProperty(IDs::Order))), false);
    }
    else if(parent.hasType(IDs::Poles))
    {
      poles.add(new FilterRoot(child, apvts.undoManager));
      incrementFilterOrder(std::abs(s32(child.getProperty(IDs::Order))), true);
    }
  }

  DBG("filter order: " << int(totalOrder));
}

void FilterState::
valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index)
{
  juce::ignoreUnused(index);
  if(child.hasType(IDs::Root))
  {
    if(auto root = getRootFromTreeNode(child).get())
    {
      bool isPole = false;
      if(parent.hasType(IDs::Zeros))
      {
        zeros.removeObject(root);
      }
      else if(parent.hasType(IDs::Poles))
      {
        poles.removeObject(root);
        isPole = true;
      }

      if(juce::exactlyEqual(r64(child.getProperty(IDs::ValueIm)), 0.0))
      {
        incrementFilterOrder(-std::abs(s32(child.getProperty(IDs::Order))), isPole);
      }
      else
      {
        incrementFilterOrder(-2*std::abs(s32(child.getProperty(IDs::Order))), isPole);
      }
    }
  }

  DBG("filter order: " << int(totalOrder));
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
    add(-s32(slack));
    // NOTE(ry): `add()` itself calls `incrementFilterOrder()`, but we don't end
    // up in an infinite loop since this branch will not be taken in the second
    // call (the invariant will have been satisfied)
  }
  jassert(totalOrder >= finiteZerosOrder);
}
