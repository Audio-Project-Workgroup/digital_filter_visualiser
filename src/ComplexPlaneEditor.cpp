#include "ComplexPlaneEditor.h"

ComplexPlaneEditor *ComplexPlaneEditor::RootPoint::editor = nullptr;

// NOTE(ry): ComplexPlaneEditor::RootTooltip implementations

ComplexPlaneEditor::RootTooltip::
RootTooltip(ComplexPlaneEditor *e)
  : root(nullptr), destroy("X"), orderInc("+"), orderDec("-"), orderLabel(), editor(e)
{
  setSize(90, 40);
  setAlwaysOnTop(true);
  setInterceptsMouseClicks(true, true);

  destroy.onClick = [this](){
    editor->processor->filterState->um->beginNewTransaction();
    editor->processor->filterState->remove(root);
  };
  orderInc.onClick = [this](){
    editor->processor->filterState->um->beginNewTransaction();
    if(auto *r = root.get()) r->order += r->isPole() ? -1 : 1;
  };
  orderDec.onClick = [this](){
    editor->processor->filterState->um->beginNewTransaction();
    if(auto *r = root.get()) r->order += r->isPole() ? 1 : -1;
  };

  destroy.addMouseListener(this, false);
  orderInc.addMouseListener(this, false);
  orderDec.addMouseListener(this, false);

  orderLabel.setInterceptsMouseClicks(false, false);

  addAndMakeVisible(destroy);
  addAndMakeVisible(orderInc);
  addAndMakeVisible(orderDec);
  addAndMakeVisible(orderLabel);
}

void ComplexPlaneEditor::RootTooltip::
mouseEnter(const juce::MouseEvent &e)
{
  if(e.eventComponent == this) DBG("enter tooltip");
  else DBG("enter tooltip button");
  show();
}

void ComplexPlaneEditor::RootTooltip::
mouseExit(const juce::MouseEvent &e)
{
  if(e.eventComponent == this) DBG("exit tooltip");
  else DBG("exit tooltip button");
  hide();
}

void ComplexPlaneEditor::RootTooltip::
resized(void)
{
  PROFILE_FUNCTION();

  auto area = getLocalBounds();
  auto thirdWidth = area.getWidth() / 3;
  auto halfHeight = area.getHeight() / 2;
  orderLabel.setBounds(area.removeFromLeft(thirdWidth));

  auto incDecArea = area.removeFromLeft(thirdWidth);
  orderInc.setBounds(incDecArea.removeFromTop(halfHeight));
  orderDec.setBounds(incDecArea);

  destroy.setBounds(area);
}

void ComplexPlaneEditor::RootTooltip::
paint(juce::Graphics &g)
{
  PROFILE_FUNCTION();

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
    isDragging = true;
    startTimerHz(timerFreq);

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
    stopTimer();
    isDragging = false;

    // NOTE(ry): snap to axis
    if(editor->isPointHoveringOverAxis())
    {
      root->value = c128(root->value.re, 0.0);
      editor->setPointHoveringOverAxis(false);
    }

    // NOTE(ry): merge roots
    jassert(editor->activeRoot.get());
    auto mergedRoot = editor->processor->filterState->mergeRoots(editor->activeRoot, editor->targetRoot);
    // NOTE(ry): the merging operation may potentially destroy the object the
    // implicit `this` pointer points to, so we have to be careful to check that
    // the object was not destroyed before we access any of its member
    // variables. any data we need related to the object after it's been
    // destroyed should be saved to this function's stack frame (before the
    // merge) or should reside in a global section (eg static members)
    if(auto *mergedRootPtr = mergedRoot.get())
    {
      if(mergedRootPtr == editor->activeRoot.get())
      {
	// NOTE(ry): we were not destroyed. restore dragging state and show tooltip
	setInterceptsMouseClicks(true, true);
	conjugate->setInterceptsMouseClicks(true, true);
	editor->tooltip.setPointAndShow(this);
      }
      // NOTE(ry): we were destroyed. the other root's mouseEnter callback will
      // fire and set the active root and show the tooltip
    }
    else
    {
      // NOTE(ry): both roots we destroyed
      editor->activeRoot = nullptr;
    }
    editor->targetRoot = nullptr;
  }
}

void ComplexPlaneEditor::RootPoint::
moveToLocalSpace(juce::Point<float> localPositionScreen)
{
  auto const editorPosition = editor->getLocalPoint(this, localPositionScreen).toDouble();
  moveToEditorSpace(editorPosition);
}

void ComplexPlaneEditor::RootPoint::
moveToEditorSpace(juce::Point<double> editorPositionScreen)
{
  auto editorPositionWorldX = editorPositionScreen.toDouble().getX();
  auto editorPositionWorldY = editorPositionScreen.toDouble().getY();
  editor->worldUnitsFromPixels.transformPoint(editorPositionWorldX, editorPositionWorldY);
  auto newRootValue = c128(editorPositionWorldX, editorPositionWorldY);

  moveToWorldSpace(newRootValue);
}

void ComplexPlaneEditor::RootPoint::
moveToWorldSpace(c128 newRootValue)
{
  // NOTE(ry): set hovering over axis
  auto const snapThresholdWorld = editor->unitsPerPixel * ComplexPlaneEditor::snapThresholdPixels;
  auto const isHoveringOverAxis = std::abs(newRootValue.imag()) < snapThresholdWorld;
  editor->setPointHoveringOverAxis(isHoveringOverAxis);

  // NOTE(ry): clamp pole magnitude so filter remains stable
  if(root->order < 0)
  {
    // TODO(ry): better stability clamp!
    if(std::abs(newRootValue) >= FilterState::maxPoleMagnitude)
    {
      newRootValue /= std::abs(newRootValue);
      newRootValue *= FilterState::maxPoleMagnitude;
    }
  }

  // NOTE:(ry): transform new value to screen space to detect if we are on top
  // of another point, taking axis snapping into account
  auto editorPositionScreenX = newRootValue.real();
  auto editorPositionScreenY = isHoveringOverAxis ? 0.0 : newRootValue.imag();
  editor->pixelsFromWorldUnits.transformPoint(editorPositionScreenX, editorPositionScreenY);

  // NOTE(ry): this is maybe the shittiest way I can imagine finding where another component is
  auto *targetComponent = editor->getComponentAt(editorPositionScreenX, editorPositionScreenY);
  if(auto *targetPoint = dynamic_cast<RootPoint*>(targetComponent))
  {
    editor->targetRoot = targetPoint->root;
  }
  else
  {
    editor->targetRoot = nullptr;
  }

  root->value = newRootValue;
}

void ComplexPlaneEditor::RootPoint::
mouseDrag(const juce::MouseEvent &e)
{
  if(e.mods.isLeftButtonDown())
  {
    if(root.get())
    {
      auto const localPosition = e.mouseDownPosition + e.getOffsetFromDragStart().toFloat();
      moveToLocalSpace(localPosition);
    }
  }
}

void ComplexPlaneEditor::RootPoint::
timerCallback(void)
{
  if(isDragging)
  {
    auto const m = juce::ModifierKeys::getCurrentModifiersRealtime();
    auto const isRightButtonDown = m.isRightButtonDown();

    // NOTE(ry): right click while dragging to split higher-order root
    if(isRightButtonDown && !wasRightButtonDown)
    {
      DBG("right click while dragging");
      if(auto *rootPtr = root.get())
      {
	// NOTE(ry): when dragging a higher-order root, deselect roots, decreasing
	// the active root order and moving roots to the start of the drag
	if(std::abs(rootPtr->order) > 1)
	{
	  if(rootPtr->isPole())
	  {
	    editor->processor->filterState->add(-1, valueAtDragStart);
	    rootPtr->order += 1;
	  }
	  else
	  {
	    rootPtr->order += -1;
	    editor->processor->filterState->add(1, valueAtDragStart);
	  }
	}
      }
    }

    wasRightButtonDown = isRightButtonDown;
  }
}

void ComplexPlaneEditor::RootPoint::
paint(juce::Graphics &g)
{
  PROFILE_FUNCTION();

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
    // NOTE(ry): a root's order can change when it is not the active root (eg
    // when the order of a pole at zero changes to maintain the causality
    // invariant), so we have to check if this root is the one associated with
    // the tooltip when updating the tooltip text
    if(editor->tooltip.isVisible() && editor->tooltip.root == root)
    {
      int order = node.getProperty(IDs::Order);
      editor->tooltip.setText(juce::String(order));
    }
  }
}

// NOTE(ry): ComplexPlaneEditor implementations

ComplexPlaneEditor::
ComplexPlaneEditor(AudioPluginAudioProcessor *p)
  :processor(p)
  ,tooltip(this)
{
  processor->filterState->addListener(this);

  RootPoint::editor = this;

  pixelsPerUnit = 100.0;
  unitsPerPixel = 1.0 / pixelsPerUnit;
  unitsPerLine = 1;

  addChildComponent(&tooltip);

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
mouseDown(const juce::MouseEvent &e)
{
  if(e.mods.isLeftButtonDown())
  {
    // NOTE(ry): dragging the world position
    worldCenterAtDragStart = worldCenter;
  }
  else if(e.mods.isRightButtonDown())
  {
    // NOTE(ry): creating a root point.
    // root is a pole if no mods are down, and a zero if control is down.
    s32 newOrder = e.mods.isCtrlDown() ? 1 : -1;
    auto const mousePixels = e.position.toDouble();
    auto mouseWorldX = mousePixels.x;
    auto mouseWorldY = mousePixels.y;
    worldUnitsFromPixels.transformPoint(mouseWorldX, mouseWorldY);

    // NOTE(ry): snap to axis
    auto const snapThresholdWorld = unitsPerPixel * ComplexPlaneEditor::snapThresholdPixels;
    if(std::abs(mouseWorldY) < snapThresholdWorld)
    {
      mouseWorldY = 0.0;
    }

    auto const newVal = c128(mouseWorldX, mouseWorldY);
    processor->filterState->um->beginNewTransaction();
    processor->filterState->add(newOrder, newVal);
  }
}

void ComplexPlaneEditor::
mouseDrag(const juce::MouseEvent &e)
{
  if(e.mods.isLeftButtonDown())
  {
    auto offsetPixels = e.getOffsetFromDragStart().toDouble();
    const auto worldUnitsFromPixelsScale = juce::Point<double>(unitsPerPixel, -unitsPerPixel);
    auto offsetWorld = worldUnitsFromPixelsScale * offsetPixels;
    worldCenter = worldCenterAtDragStart - offsetWorld;

    updateTransformsAndChildBounds();
    repaint();
  }
}

void ComplexPlaneEditor::
resized()
{
  PROFILE_FUNCTION();

  updateTransformsAndChildBounds();
}

void ComplexPlaneEditor::
paint(juce::Graphics &g)
{
  PROFILE_FUNCTION();

  auto const backgroundColor = juce::Colour(0x08, 0x0C, 0x1C);
  auto const axisColor = juce::Colours::navajowhite;
  auto const axisHighlightColor = juce::Colours::cadetblue;
  auto const lineColor = juce::Colours::snow;
  auto const circleColor = juce::Colours::goldenrod;
  auto const textColor = juce::Colours::white;

  auto constexpr axisLabelFontHeightPixels = 32.0;
  auto constexpr gridLineLabelFontHeightPixels = 24.0;

  auto const axisThicknessPixels = 3;
  auto const circleThicknessPixels = 3;
  auto const lineThicknessPixels = 2;

  auto const eps = 1e-6;

  auto const textRightOffsetPixels = 50.0;

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
    PROFILE_SCOPE("draw lines");

    juce::Graphics::ScopedSaveState savedGraphicsState(g);
    g.addTransform(pixelsFromWorldUnits);

    // NOTE(ry): draw axes
    {
      g.setColour(axisColor);
      auto const axisThicknessWorld = axisThicknessPixels * unitsPerPixel;

      g.fillRect(juce::Rectangle<float>()
		 .withSize(axisThicknessWorld, topWorld - bottomWorld)
		 .withCentre(juce::Point<float>(0.f, 0.5f*(topWorld + bottomWorld))));

      if(pointHoveringOverAxis) g.setColour(axisHighlightColor);
      g.fillRect(juce::Rectangle<float>()
		 .withSize(rightWorld - leftWorld, axisThicknessWorld)
		 .withCentre(juce::Point<float>(0.5f*(rightWorld + leftWorld), 0.f)));
    }

    // NOTE(ry): draw unit circle
    g.setColour(circleColor);
    g.drawEllipse(-1, -1, 2, 2, circleThicknessPixels * unitsPerPixel);

    // NOTE(ry): draw lines
    {
      g.setColour(lineColor);
      auto const lineThicknessWorld = lineThicknessPixels * unitsPerPixel;

      for(auto lineX = std::floor(leftWorld/unitsPerLine)*unitsPerLine;
          lineX < rightWorld;
          lineX += unitsPerLine)
      {
	PROFILE_SCOPE("draw x lines");
        if(-eps >= lineX || lineX >= eps)
        {
	  g.fillRect(juce::Rectangle<float>()
		     .withSize(lineThicknessWorld, topWorld - bottomWorld)
		     .withCentre(juce::Point<float>(lineX, 0.5f*(topWorld + bottomWorld))));
	}
      }

      for(auto lineY = std::floor(bottomWorld/unitsPerLine)*unitsPerLine;
          lineY < topWorld;
          lineY += unitsPerLine)
      {
        PROFILE_SCOPE("draw y lines");
        if(-eps >= lineY || lineY >= eps)
        {
	  g.fillRect(juce::Rectangle<float>()
		     .withSize(rightWorld - leftWorld, lineThicknessWorld)
		     .withCentre(juce::Point<float>(0.5f*(rightWorld + leftWorld), lineY)));
	}
      }
    }
  }

  // NOTE(ry): draw axis and grid line labels
  {
    PROFILE_SCOPE("draw labels");

    g.setColour(textColor);
    g.setFont(juce::Font(juce::FontOptions(axisLabelFontHeightPixels)));

    auto constexpr textAxisSpacingX = 8.0;
    auto constexpr textAxisSpacingY = 5.0 + gridLineLabelFontHeightPixels;

    // NOTE(ry): draw axis labels
    {
      PROFILE_SCOPE("draw axis labels");

      auto reLabelX = rightWorld;
      auto reLabelY = 0.0;
      auto imLabelX = 0.0;
      auto imLabelY = topWorld;
      pixelsFromWorldUnits.transformPoint(reLabelX, reLabelY);
      pixelsFromWorldUnits.transformPoint(imLabelX, imLabelY);

      reLabelX -= textRightOffsetPixels;
      reLabelY = std::clamp(reLabelY - (textAxisSpacingY - gridLineLabelFontHeightPixels),
			    localBounds.getY() + std::min(axisLabelFontHeightPixels,
							  localBounds.getHeight()),
			    localBounds.getBottom());
      imLabelX = std::clamp(imLabelX - 1.1*axisLabelFontHeightPixels,
			    localBounds.getX(),
			    localBounds.getRight() - std::min(textRightOffsetPixels,
							      localBounds.getWidth()));
      imLabelY += axisLabelFontHeightPixels;
      g.drawSingleLineText("Re", reLabelX, reLabelY);
      g.drawSingleLineText("Im", imLabelX, imLabelY);
    }

    // NOTE(ry): draw grid line labels
    {
      PROFILE_SCOPE("draw grid line labels");

      g.setFont(juce::Font(juce::FontOptions(gridLineLabelFontHeightPixels)));

      for(auto labelX = std::floor(leftWorld/unitsPerLine)*unitsPerLine;
          labelX < rightWorld;
          labelX += unitsPerLine)
      {
        PROFILE_SCOPE("draw x labels");

        auto drawX = labelX;
        auto drawY = 0.0;
        pixelsFromWorldUnits.transformPoint(drawX, drawY);
        drawY = std::clamp(drawY + textAxisSpacingY,
			   localBounds.getY() + std::min(gridLineLabelFontHeightPixels,
							 localBounds.getHeight()),
			   localBounds.getBottom());
        if(eps <= labelX || labelX <= -eps)
        { g.drawSingleLineText(juce::String(labelX), drawX + textAxisSpacingX, drawY); }
        else
        { g.drawSingleLineText(juce::String(0), drawX + textAxisSpacingX, drawY); }
      }

      for(auto labelY = std::floor(bottomWorld/unitsPerLine)*unitsPerLine;
          labelY < topWorld;
          labelY += unitsPerLine)
      {
	PROFILE_SCOPE("draw y labels");

        auto drawX = 0.0;
        auto drawY = labelY;
        pixelsFromWorldUnits.transformPoint(drawX, drawY);
        drawX = std::clamp(drawX + textAxisSpacingX,
			   localBounds.getX(),
			   localBounds.getRight() - std::min(textRightOffsetPixels,
							     localBounds.getWidth()));

        if(eps <= labelY || labelY <= -eps)
        { g.drawSingleLineText(juce::String(labelY), drawX, drawY + textAxisSpacingY); }
        else
        { g.drawSingleLineText(juce::String(0), drawX, drawY + textAxisSpacingY); }
      }
    }
  }
}

void ComplexPlaneEditor::
setPointHoveringOverAxis(bool hovering)
{
  auto const oldHovering = pointHoveringOverAxis;
  pointHoveringOverAxis = hovering;
  if(oldHovering != hovering) repaint();
}

bool ComplexPlaneEditor::
isPointHoveringOverAxis(void)
{
  return(pointHoveringOverAxis);
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
