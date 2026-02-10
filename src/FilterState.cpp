FilterState::
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

FilterRoot::Ptr FilterState::
add(s32 newOrder)
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
    }
    else if(parent.hasType(IDs::Poles))
    {
      poles.add(new FilterRoot(child, apvts.undoManager));
    }
      order += u32(std::abs(s32(child.getProperty(IDs::Order))));
  }
}

void FilterState::
valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index)
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
