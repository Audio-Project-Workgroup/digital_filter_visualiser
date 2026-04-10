

#include "ComplexPlaneEditor.h"

ComplexPlaneEditor *ComplexPlaneEditor::RootPoint::editor = nullptr;

// NOTE(ry): ComplexPlaneEditor::RootTooltip implementations

ComplexPlaneEditor::RootTooltip::
RootTooltip()
  : root(nullptr), orderInc("+"), orderDec("-"), orderLabel()
{
  orderInc.onClick = [this](){
    if(auto *r = root.get()) r->order += r->isPole() ? -1 : 1;
  };
  orderDec.onClick = [this](){
    if(auto *r = root.get()) r->order += r->isPole() ? 1 : -1;
  };

  setSize(60, 40);
  setAlwaysOnTop(true);
  setInterceptsMouseClicks(true, true);

  orderLabel.setInterceptsMouseClicks(false, false);

  addAndMakeVisible(orderInc);
  addAndMakeVisible(orderDec);
  addAndMakeVisible(orderLabel);
}

void ComplexPlaneEditor::RootTooltip::
mouseEnter(const juce::MouseEvent&)
{
  DBG("enter tooltip");
  show();
}

void ComplexPlaneEditor::RootTooltip::
mouseExit(const juce::MouseEvent&)
{
  DBG("exit tooltip");
  hide();
}

void ComplexPlaneEditor::RootTooltip::
resized(void)
{
  auto area = getLocalBounds();
  auto halfWidth = area.getWidth() / 2;
  auto halfHeight = area.getHeight() / 2;
  orderLabel.setBounds(area.removeFromLeft(halfWidth));
  orderInc.setBounds(area.removeFromTop(halfHeight));
  orderDec.setBounds(area);
}

void ComplexPlaneEditor::RootTooltip::
paint(juce::Graphics &g)
{
  g.fillAll(juce::Colours::black);
  g.setColour(juce::Colours::white);
  g.drawRect(getLocalBounds(), 1);
}

// NOTE(ry): ComplexPlaneEditor::RootPoint implementations

ComplexPlaneEditor::RootPoint::
RootPoint(bool c, FilterRoot::Ptr r)
  : isConjugate(c), root(r)
{
  if(auto *rootPtr = root.get())
  {
    rootPtr->node.addListener(this);
  }
  setSize(10, 10);
}

ComplexPlaneEditor::RootPoint::
~RootPoint()
{
  if(auto *rootPtr = root.get())
  {
    rootPtr->node.removeListener(this);
  }
}

void ComplexPlaneEditor::RootPoint::
mouseEnter(const juce::MouseEvent &e)
{
  juce::ignoreUnused(e);

  DBG("enter root point");
  editor->activeRoot = root;
  editor->tooltip.setPointAndShow(this);
}

void ComplexPlaneEditor::RootPoint::
mouseExit(const juce::MouseEvent &e)
{
  juce::ignoreUnused(e);

  DBG("exit root point");
  editor->tooltip.hide() ;
  editor->activeRoot = nullptr;
}

void ComplexPlaneEditor::RootPoint::
mouseDown(const juce::MouseEvent &e)
{
  if(e.mods.isLeftButtonDown())
  {
    editor->tooltip.hide();

    jassert(root.get() == editor->activeRoot.get());
    setInterceptsMouseClicks(false, false);
    conjugate->setInterceptsMouseClicks(false, false);

    editor->processor->filterState->um->beginNewTransaction();
    if(auto *rootPtr = root.get())
    {
      valueAtDragStart = rootPtr->value;
      if(isConjugate)
      {
        valueAtDragStart = std::conj(valueAtDragStart);
      }
    }
  }
}

void ComplexPlaneEditor::RootPoint::
mouseUp(const juce::MouseEvent &e)
{
  if(e.mods.isLeftButtonDown())
  {
    if(auto *targetRoot = editor->targetRoot.get())
    {
      // NOTE(ry): merge the active root into the target root
      auto *activeRoot = editor->activeRoot.get();
      jassert(activeRoot != nullptr);

      // NOTE(ry): it is possible that in the process of merging roots that this
      // object will be destroyed. it is important not to access variables or
      // methods on this after a potentially destructive modification, or else
      // risk use-after-free crashes. if you need data stored in this object
      // after it may have been destroyed, put it on this stack frame or use the
      // static `editor` variable.

      int newOrder = targetRoot->order + activeRoot->order;
      if((targetRoot->order < 0) != (activeRoot->order < 0))
      {
	// NOTE(ry): we are merging a zero with a pole. we keep one and remove
	// the other, depending on the sign of the result. the order of the root
	// we keep will decrease
	auto const resultIsPole = newOrder < 0;
	auto zero = targetRoot->order < 0 ? editor->activeRoot : editor->targetRoot;
	auto pole = targetRoot->order < 0 ? editor->targetRoot : editor->activeRoot;

	// NOTE(ry): we must decrement the zero order before we decrement the
	// pole order so we don't add a slack pole
	if(resultIsPole)
	{
	  editor->processor->filterState->remove(zero);
	  pole->order = newOrder;
	  if(auto *polePtr = pole.get())
	  {
	    if(polePtr == activeRoot)
	    {
	      // NOTE(ry): the pole is this, so we need to show the tooltip
	      editor->tooltip.setRootAndShow(pole);
	    }
	    // NOTE(ry): since the pole is the target, after this is destroyed a
	    // mouseEnter callback will fire, calling setRootAndShow on the
	    // desired root
	  }
	}
	else
	{
	  zero->order = newOrder;
	  editor->processor->filterState->remove(pole);
	  if(auto *zeroPtr = zero.get())
	  {
	    if(zeroPtr == activeRoot)
	    {
	      // NOTE(ry): the zero is this, so we need to show the tooltip
	      editor->tooltip.setRootAndShow(zero);
	    }
	    // NOTE(ry): since the zero is the target, after this is destroyed a
	    // mouseEnter callback will fire, calling setRootAndShow on the
	    // desired root
	  }
	}
      }
      else
      {
	// NOTE(ry): we are merging two roots of the same kind (zero & zero or
	// pole & pole). the order of the root we keep will increase

	// NOTE(ry): we must increase the order of the pole we keep before
	// removing the other one, or remove a zero before we increase the order
	// of the other one.
	if(targetRoot->isPole())
	{
	  targetRoot->order = newOrder;
	  editor->processor->filterState->remove(editor->activeRoot);
	}
	else
	{
	  editor->processor->filterState->remove(editor->activeRoot);
	  targetRoot->order = newOrder;
	}
	// NOTE(ry): we don't need to set the root since after this is destroyed
	// a mouseEnter callback will fire, calling setRootAndShow on the
	// desired root
      }
    }
    else
    {
      editor->tooltip.show();
    }

    // NOTE(ry): it's possible the object on which this member function has been
    // called gets destroyed during its execution, so we only reset its input
    // state (and the state of its conjugate) if it still exists. we can check
    // if it still exists by checking if the root it corresponds to still exists
    if(editor->activeRoot.get())
    {
      setInterceptsMouseClicks(true, true);
      conjugate->setInterceptsMouseClicks(true, true);
    }
    else
    {
      editor->activeRoot = nullptr;
    }

    editor->targetRoot = nullptr;
  }
}

void ComplexPlaneEditor::RootPoint::
mouseDrag(const juce::MouseEvent &e)
{
  if(auto *rootPtr = root.get())
  {
    auto worldUnitsFromPixels = juce::Point<double>(editor->unitsPerPixel, -editor->unitsPerPixel);
    auto dragOffsetPixels = e.getOffsetFromDragStart().toDouble();
    auto dragOffsetWorld = worldUnitsFromPixels * dragOffsetPixels;
    auto newRootValue = valueAtDragStart + c128(dragOffsetWorld.getX(), dragOffsetWorld.getY());

    auto const snapThresholdPixels = 18.0; // TODO(ry): tune
    auto snapThresholdWorld = editor->unitsPerPixel * snapThresholdPixels;

    // NOTE(ry): snap to axis
    if(std::abs(newRootValue.imag()) < snapThresholdWorld)
    {
      newRootValue = c128(newRootValue.real(), 0.0);
    }

    if(rootPtr->order < 0)
    {
      // TODO(ry): better stability clamp!
      // NOTE(ry): stability clamp
      if(std::abs(newRootValue) >= 1)
      {
	auto const epsClamp = 1e-3; // TODO(ry): tune
        newRootValue /= std::abs(newRootValue) + epsClamp;
      }
    }

    // NOTE(ry): this is maybe the shittiest way I can imagine finding where another component is
    // TODO(ry): use world pos because mouse can be off axis while point is on (or change tolerance)
    auto parentEvent = e.getEventRelativeTo(editor);
    auto *targetComponent = editor->getComponentAt(parentEvent.x, parentEvent.y);
    if(auto *targetPoint = dynamic_cast<RootPoint*>(targetComponent))
    {
      editor->targetRoot = targetPoint->root;
    }
    else
    {
      editor->targetRoot = nullptr;
    }

    // NOTE(ry): update all properties related to this root
    rootPtr->value = newRootValue;
    // TODO(ry): splitting logic (create new root, subtract new root order from current root order)
  }
}

void ComplexPlaneEditor::RootPoint::
paint(juce::Graphics &g)
{
  if(auto *rootPtr = root.get())
  {
    if(rootPtr->order < 0) g.setColour(juce::Colours::red); // poles are red
    else if(rootPtr->order > 0) g.setColour(juce::Colours::white); // zeros are white

    g.fillEllipse(getLocalBounds().toFloat());
  }
}

void ComplexPlaneEditor::RootPoint::
updateBounds(c128 value)
{
  // NOTE(ry): it's unfathomable to me how anyone could come up with such a
  // simple yet unintuitive api. It'd be so nice if I could give the points
  // different names depending on their coordinate spaces. And since it
  // mutates, at least pass by pointer so I can tell that from looking at
  // the call site (instead of this sour comment).
  auto pixelX = value.real();
  auto pixelY = isConjugate ? -value.imag() : value.imag();
  editor->pixelsFromWorldUnits.transformPoint(pixelX, pixelY);
  setCentrePosition(pixelX, pixelY);
}

void ComplexPlaneEditor::RootPoint::
valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property)
{
  if(property == IDs::ValueRe || property == IDs::ValueIm)
  {
    double valueRe = node.getProperty(IDs::ValueRe);
    double valueIm = node.getProperty(IDs::ValueIm);
    updateBounds(c128(valueRe, valueIm));
    editor->tooltip.setPos(getPosition());
  }
  else if(property == IDs::Order)
  {
    if(editor->tooltip.isVisible())
    {
      int order = node.getProperty(IDs::Order);
      editor->tooltip.setText(juce::String(order));
    }
  }
}

// NOTE(ry): ComplexPlaneEditor implementations

ComplexPlaneEditor::
ComplexPlaneEditor(AudioPluginAudioProcessor *p)
  : processor(p), tooltip(), addRoot("+"), delRoot("-"), undo("undo"), redo("redo")
{
  processor->filterState->addListener(this);

  RootPoint::editor = this;

  pixelsPerUnit = 100.0;
  unitsPerPixel = 1.0 / pixelsPerUnit;
  unitsPerLine = 1;

  // NOTE(ry): gain slider setup
  gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  gainSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
  gainSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);
  gainSlider.setRange(-90.f, 6.f);
  gainSlider.setTextValueSuffix(" dB");
  gainSlider.setValue(juce::Decibels::gainToDecibels(r64(processor->filterState->gain)), juce::dontSendNotification);
  gainSlider.addListener(this);
  addAndMakeVisible(gainSlider);

  // NOTE(ry): debug ui setup
  // TODO(ry): better add/remove interface & logic (add poles, remove particular roots)
  addRoot.onClick = [this]{
    processor->filterState->um->beginNewTransaction();
    processor->filterState->add(1);
  };
  delRoot.onClick = [this]{
    // NOTE(ry): guard against when points array is empty
    if(auto *point = points.getLast())  // TODO(ry): remove a paritcular root
    {
      processor->filterState->remove(point->root);
    }
  };

  undo.onClick = [this]{
    processor->filterState->um->undo();
  };
  redo.onClick = [this]{
    processor->filterState->um->redo();
  };

  addChildComponent(&tooltip);

  addAndMakeVisible(addRoot);
  addAndMakeVisible(delRoot);
  addAndMakeVisible(undo);
  addAndMakeVisible(redo);

  processor->filterState->syncListener(this);
}

ComplexPlaneEditor::
~ComplexPlaneEditor()
{
  processor->filterState->removeListener(this);
}

void ComplexPlaneEditor::
mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w)
{
  auto delta = w.isReversed ? -w.deltaY : w.deltaY;
  pixelsPerUnit *= (1.0 + delta);
  unitsPerPixel = 1.0 / pixelsPerUnit;

  // NOTE(ry): update grid line resolution
  {
    auto scaled = 100.0 * unitsPerPixel;
    auto exponent = std::floor(std::log10(scaled));
    auto fraction = scaled / std::pow(10.0, exponent);
    auto logFraction = std::log10(fraction);
    auto base = (logFraction < 1.0/3.0) ? 1 : (logFraction < 2.0/3.0) ? 2 : 5;
    unitsPerLine = base * std::pow(10.0, exponent);
  }

  // NOTE(ry): update world center
  {
    // NOTE(ry): get mouse position in old coordinates
    auto oldMouseP = e.position.toDouble();
    auto oldMousePX = oldMouseP.x;
    auto oldMousePY = oldMouseP.y;
    worldUnitsFromPixels.transformPoint(oldMousePX, oldMousePY);

    updateTransforms();

    // NOTE(ry): get mouse position in new coordinates
    auto newMousePX = oldMouseP.x;
    auto newMousePY = oldMouseP.y;
    worldUnitsFromPixels.transformPoint(newMousePX, newMousePY);

    // NOTE(ry): move world center so local mouse position doesn't change during zoom
    auto mouseOffsetX = newMousePX - oldMousePX;
    auto mouseOffsetY = newMousePY - oldMousePY;
    worldCenter -= juce::Point<double>(mouseOffsetX, mouseOffsetY);
  }

  updateTransformsAndChildBounds();
  repaint();
}

void ComplexPlaneEditor::
mouseDown(const juce::MouseEvent&)
{
  worldCenterAtDragStart = worldCenter;
}

void ComplexPlaneEditor::
mouseDrag(const juce::MouseEvent &e)
{
  auto offsetPixels = e.getOffsetFromDragStart().toDouble();
  const auto worldUnitsFromPixelsScale = juce::Point<double>(unitsPerPixel, -unitsPerPixel);
  auto offsetWorld = worldUnitsFromPixelsScale * offsetPixels;
  worldCenter = worldCenterAtDragStart - offsetWorld;

  updateTransformsAndChildBounds();
  repaint();
}

void ComplexPlaneEditor::
resized()
{
  auto area = getLocalBounds();

  auto const gainSliderHeight = 100;
  gainSlider.setBounds(area.removeFromTop(gainSliderHeight));

  auto const buttonHeight = 50;
  auto const buttonWidth = 100;
  auto const buttonHMargin = 20;
  area.removeFromLeft(buttonHMargin);

  auto addDelCol = area.removeFromLeft(buttonWidth);
  addRoot.setBounds(addDelCol.removeFromTop(buttonHeight));
  delRoot.setBounds(addDelCol.removeFromTop(buttonHeight));

  auto undoRedoCol = area.removeFromLeft(buttonWidth);
  undo.setBounds(undoRedoCol.removeFromTop(buttonHeight));
  redo.setBounds(undoRedoCol.removeFromTop(buttonHeight));

  updateTransformsAndChildBounds();
}

void ComplexPlaneEditor::
paint(juce::Graphics &g)
{
  auto const backgroundColor = juce::Colour(0x08, 0x0C, 0x1C);
  auto const axisColor = juce::Colours::navajowhite;
  auto const lineColor = juce::Colours::snow;
  auto const circleColor = juce::Colours::goldenrod;
  auto const textColor = juce::Colours::white;

  auto const axisLabelFontHeightPixels = 32.f;
  auto const gridLineLabelFontHeightPixels = 24.f;

  auto const axisThicknessPixels = 3;
  auto const circleThicknessPixels = 3;
  auto const lineThicknessPixels = 2;

  auto const eps = 1e-6;

  auto const textRightOffsetPixels = 50;

  // NOTE(ry): draw background
  g.fillAll(backgroundColor);

  // NOTE(ry): what a shit api this is
  auto localBounds = getLocalBounds().toDouble();
  auto leftWorld = localBounds.getX();
  auto topWorld = localBounds.getY();
  auto rightWorld = localBounds.getRight();
  auto bottomWorld = localBounds.getBottom();
  worldUnitsFromPixels.transformPoint(leftWorld, topWorld);
  worldUnitsFromPixels.transformPoint(rightWorld, bottomWorld);

  {
    juce::Graphics::ScopedSaveState savedGraphicsState(g);
    g.addTransform(pixelsFromWorldUnits);

    // NOTE(ry): draw axes
    g.setColour(axisColor);
    g.drawLine(0, bottomWorld, 0, topWorld, axisThicknessPixels * unitsPerPixel);
    g.drawLine(leftWorld, 0, rightWorld, 0, axisThicknessPixels * unitsPerPixel);

    // NOTE(ry): draw unit circle
    g.setColour(circleColor);
    g.drawEllipse(-1, -1, 2, 2, circleThicknessPixels * unitsPerPixel);

    // NOTE(ry): draw lines
    {
      g.setColour(lineColor);

      for(auto lineX = std::floor(leftWorld / unitsPerLine)*unitsPerLine;
          lineX < rightWorld;
          lineX += unitsPerLine)
      {
        if(-eps >= lineX || lineX >= eps)
        { g.drawLine(lineX, bottomWorld, lineX, topWorld, lineThicknessPixels * unitsPerPixel); }
      }

      for(auto lineY = std::floor(bottomWorld / unitsPerLine)*unitsPerLine;
          lineY < topWorld;
          lineY += unitsPerLine)
      {
        if(-eps >= lineY || lineY >= eps)
        { g.drawLine(leftWorld, lineY, rightWorld, lineY, lineThicknessPixels * unitsPerPixel); }
      }
    }
  }

  // NOTE(ry): draw axis and grid line labels
  {
    g.setColour(textColor);
    g.setFont(juce::Font(juce::FontOptions(axisLabelFontHeightPixels)));

    // NOTE(ry): draw axis labels
    {
      auto reLabelX = rightWorld;
      auto reLabelY = 0.0;
      auto imLabelX = 0.0;
      auto imLabelY = topWorld;
      pixelsFromWorldUnits.transformPoint(reLabelX, reLabelY);
      pixelsFromWorldUnits.transformPoint(imLabelX, imLabelY);

      reLabelX -= textRightOffsetPixels;
      reLabelY = std::clamp(reLabelY, localBounds.getY() + axisLabelFontHeightPixels, localBounds.getBottom());
      imLabelX = std::clamp(imLabelX, localBounds.getX(), localBounds.getRight() - textRightOffsetPixels);
      imLabelY += axisLabelFontHeightPixels;
      g.drawSingleLineText("Re", reLabelX, reLabelY);
      g.drawSingleLineText("Im", imLabelX, imLabelY);
    }

    // NOTE(ry): draw grid line labels
    {
      g.setFont(juce::Font(juce::FontOptions(gridLineLabelFontHeightPixels)));

      for(auto labelX = std::floor(leftWorld / unitsPerLine)*unitsPerLine;
          labelX < rightWorld;
          labelX += unitsPerLine)
      {
        auto drawX = labelX;
        auto drawY = 0.0;
        pixelsFromWorldUnits.transformPoint(drawX, drawY);
        drawY = std::clamp(drawY, localBounds.getY() + gridLineLabelFontHeightPixels, localBounds.getBottom());
        if(eps <= labelX || labelX <= -eps)
        { g.drawSingleLineText(juce::String(labelX), drawX, drawY); }
        else
        { g.drawSingleLineText(juce::String(0), drawX, drawY); }
      }

      for(auto labelY = std::floor(bottomWorld / unitsPerLine)*unitsPerLine;
          labelY < topWorld;
          labelY += unitsPerLine)
      {
        auto drawX = 0.0;
        auto drawY = labelY;
        pixelsFromWorldUnits.transformPoint(drawX, drawY);
        drawX = std::clamp(drawX, localBounds.getX(), localBounds.getRight() - textRightOffsetPixels);

        if(eps <= labelY || labelY <= -eps)
        { g.drawSingleLineText(juce::String(labelY), drawX, drawY); }
        else
        { g.drawSingleLineText(juce::String(0), drawX, drawY); }
      }
    }
  }
}

void ComplexPlaneEditor::
updateTransforms(void)
{
  // NOTE(ry): compute transforms between screen space and world space
  auto localBounds = getLocalBounds().toDouble();
  auto regionCenter = localBounds.getCentre();
  pixelsFromWorldUnits = juce::AffineTransform()
    .translated(-worldCenter.x, -worldCenter.y)
    .scaled(pixelsPerUnit, -pixelsPerUnit)
    .translated(regionCenter.x, regionCenter.y);
  worldUnitsFromPixels = pixelsFromWorldUnits.inverted();
}

void ComplexPlaneEditor::
updateTransformsAndChildBounds(void)
{
  updateTransforms();

  // NOTE(ry): update child bounds (using new transforms)
  for(auto *p : points)
  {
    if(auto *root = p->root.get())
    {
      p->updateBounds(root->value);
    }
  }
}

void ComplexPlaneEditor::
sliderValueChanged(juce::Slider *slider)
{
  if(slider == &gainSlider)
  {
    auto const dB = slider->getValue();
    auto const amp = juce::Decibels::decibelsToGain(dB);
    processor->filterState->gain = amp;
  }
}

void ComplexPlaneEditor::
valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child)
{
  juce::ignoreUnused(parent);

  auto root = processor->filterState->getRootFromTreeNode(child);
  auto *point = points.add(new RootPoint(false, root));
  auto *conjugate = points.add(new RootPoint(true, root));
  point->conjugate = conjugate;
  conjugate->conjugate = point;

  addAndMakeVisible(point);
  addAndMakeVisible(conjugate);

  resized();
}

void ComplexPlaneEditor::
valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index)
{
  juce::ignoreUnused(parent);
  juce::ignoreUnused(index);

  for(int i = points.size(); --i >= 0;)
  {
    if(auto *root = points[i]->root.get())
    {
      if(root->node == child)
      {
        // NOTE(ry): removing a point from the array because its root matches
        // the node being removed
        points.remove(i);
      }
    }
    else
    {
      // NOTE(ry): removing a point from the array because its root was
      // destroyed (it matched the node being removed in the filter state's
      // callback)
      points.remove(i);
    }
  }

  if(!tooltip.root.get())
  {
    tooltip.keepaliveCounter = 0;
    tooltip.pendingEventCount = 0;
    tooltip.stopTimer();
    tooltip.setVisible(false);
  }

  repaint();
}

void ComplexPlaneEditor::
valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property)
{
  if(property == IDs::Gain)
  {
    auto const amp = r64(node.getProperty(IDs::Gain));
    auto const dB = juce::Decibels::gainToDecibels(amp);
    gainSlider.setValue(dB, juce::dontSendNotification);
  }
}
