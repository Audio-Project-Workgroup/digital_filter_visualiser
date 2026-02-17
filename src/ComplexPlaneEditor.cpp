// NOTE(ry): ComplexPlaneEditor::RootPoint implementations

ComplexPlaneEditor::RootPoint::
RootPoint(ComplexPlaneEditor *e, bool c, FilterRoot::Ptr r)
  : isConjugate(c), root(r), parent(e)
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
mouseDown(const juce::MouseEvent &e)
{
  juce::ignoreUnused(e);
  parent->processor.um.beginNewTransaction();
  if(auto *rootPtr = root.get())
  {
    valueAtDragStart = rootPtr->value;
    if(isConjugate)
    {
      valueAtDragStart = std::conj(valueAtDragStart);
    }
  }
}

void ComplexPlaneEditor::RootPoint::
mouseDrag(const juce::MouseEvent &e)
{
  if(auto *rootPtr = root.get())
  {
    auto worldUnitsFromPixels = juce::Point<double>(parent->unitsPerPixel, -parent->unitsPerPixel);
    auto dragOffsetPixels = e.getOffsetFromDragStart().toDouble();
    auto dragOffsetWorld = worldUnitsFromPixels * dragOffsetPixels;
    auto newRootValue = valueAtDragStart + c128(dragOffsetWorld.getX(), dragOffsetWorld.getY());

    auto const snapThresholdPixels = 18.0; // TODO(ry): tune
    auto snapThresholdWorld = parent->unitsPerPixel * snapThresholdPixels;

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
        newRootValue /= std::abs(newRootValue);
      }
    }

    // NOTE(ry): update all properties related to this root
    rootPtr->value = newRootValue; // TODO(ry): snap to real axis (ui confirmation)
    // TODO(ry): merging logic (need to know where other roots are)
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
  parent->pixelsFromWorldUnits.transformPoint(pixelX, pixelY);
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
  }
}

// NOTE(ry): ComplexPlaneEditor implementations

ComplexPlaneEditor::
ComplexPlaneEditor(AudioPluginAudioProcessor &p)
  : processor(p), addRoot("+"), delRoot("-"), undo("undo"), redo("redo")
{
  processor.state.addListener(this);

  pixelsPerUnit = 100.0;
  unitsPerPixel = 1.0 / pixelsPerUnit;
  unitsPerLine = 1;

  addRoot.setBounds(100, 100, 100, 50);
  delRoot.setBounds(100, 150, 100, 50);
  undo.setBounds(200, 100, 100, 50);
  redo.setBounds(200, 150, 100, 50);

  // TODO(ry): better add/remove interface & logic (add poles, remove particular roots)
  addRoot.onClick = [this]{
    processor.state.add(1);
  };
  delRoot.onClick = [this]{
    // NOTE(ry): guard against when points array is empty
    if(auto *point = points.getLast())  // TODO(ry): remove a paritcular root
    {
      processor.state.remove(point->root);
    }
  };

  undo.onClick = [this]{
    processor.um.undo();
  };
  redo.onClick = [this]{
    processor.um.redo();
  };

  addAndMakeVisible(addRoot);
  addAndMakeVisible(delRoot);
  addAndMakeVisible(undo);
  addAndMakeVisible(redo);
}

ComplexPlaneEditor::
~ComplexPlaneEditor()
{
  processor.state.removeListener(this);
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
valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child)
{
  juce::ignoreUnused(parent);

  auto root = processor.state.getRootFromTreeNode(child);
  auto *point = points.add(new RootPoint(this, false, root));
  auto *conjugate = points.add(new RootPoint(this, true, root));
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

  repaint();
}
