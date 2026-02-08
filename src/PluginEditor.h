#pragma once

#include "PluginProcessor.h"

//==============================================================================
class ComplexPlaneEditor final : public juce::Component, private juce::ValueTree::Listener
{
  /** TODO(ry):
   * better root creation interface (ability to create poles, increase/decrease root order)
   * merge and split roots
   * dragging roots off of real axis logic
   * restore a root to its previous position when undoing its deletion
   * visually indicate when a root is being hovered over (highlight, show tooltip)
   * label axes and grid lines
   */

public:
  ComplexPlaneEditor(AudioPluginAudioProcessor &p)
    : addRoot("+"), delRoot("-"), undo("undo"), redo("redo"), processor(p)
  {
    processor.state.addListener(this);

    pixelsPerUnit = 100.0;
    unitsPerPixel = 1.0 / pixelsPerUnit;
    unitsPerLine = 1;

    addRoot.setBounds(100, 100, 100, 50);
    delRoot.setBounds(100, 150, 100, 50);
    undo.setBounds(200, 100, 100, 50);
    redo.setBounds(200, 150, 100, 50);

    // TODO(ry): better add/remove interface & logic
    addRoot.onClick = [this]{
      processor.state.add(1);
    };
    delRoot.onClick = [this]{
      processor.state.remove(points.getLast()->root);
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
  ~ComplexPlaneEditor()
  {
    processor.state.removeListener(this);
  }

  class RootPoint final : public juce::Component, private juce::ValueTree::Listener
  {
  public:
    RootPoint(ComplexPlaneEditor *e, FilterRoot::Ptr r) : root(r), parent(e)
    {
      if(auto *rootPtr = root.get())
      {
        rootPtr->node.addListener(this);
      }
      setSize(10, 10);
    }
    ~RootPoint()
    {
      if(auto *rootPtr = root.get())
      {
        rootPtr->node.removeListener(this);
      }
    }

    void mouseDown(const juce::MouseEvent &e) override
    {
      juce::ignoreUnused(e);
      parent->processor.um.beginNewTransaction();
      if(auto *rootPtr = root.get())
      {
        valueAtDragStart = rootPtr->value;
      }
    }

    void mouseDrag(const juce::MouseEvent &e) override
    {
      // TODO(ry): handle the case of dragging a root off of the real axis
      if(auto *rootPtr = root.get())
      {
        auto worldUnitsFromPixels = juce::Point<double>(parent->unitsPerPixel, -parent->unitsPerPixel);
        auto dragOffsetPixels = e.getOffsetFromDragStart().toDouble();
        auto dragOffsetWorld = worldUnitsFromPixels * dragOffsetPixels;
        auto newRootValue = valueAtDragStart + c128(dragOffsetWorld.getX(), dragOffsetWorld.getY());
        if(rootPtr->order < 0) newRootValue /= std::abs(newRootValue); // TODO(ry): better stability clamp!
        rootPtr->value = newRootValue;
      }
    }

    void paint(juce::Graphics &g) override
    {
      juce::Colour color = juce::Colours::black; // default color is black
      if(auto *rootPtr = root.get())
      {
        if(rootPtr->order < 0) color = juce::Colours::red; // poles are red
        else if(rootPtr->order > 0) color = juce::Colours::white; // zeros are white
      }
      g.setColour(color);
      g.fillEllipse(getLocalBounds().toFloat());
    }

    void updateBounds(c128 value)
    {
      // NOTE(ry): it's unfathomable to me how anyone could come up with such a
      // simple yet unintuitive api. It'd be so nice if I could give the points
      // different names depending on their coordinate spaces. And since it
      // mutates, at least pass by pointer so I can tell that from looking at
      // the call site (instead of this sour comment).
      auto pixelX = value.real();
      auto pixelY = value.imag();
      parent->pixelsFromWorldUnits.transformPoint(pixelX, pixelY);
      setCentrePosition(pixelX, pixelY);
    }

    FilterRoot::Ptr root;

  private:

    void valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property) override
    {
      if(property == IDs::ValueRe || property == IDs::ValueIm)
      {
        double valueRe = node.getProperty(IDs::ValueRe);
        double valueIm = node.getProperty(IDs::ValueIm);
        updateBounds(c128(valueRe, valueIm));
      }
    }

    ComplexPlaneEditor *parent;
    c128 valueAtDragStart;
  };

  void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails &w) override
  {
    // TODO(ry): zoom towards cursor position
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

    updateTransformsAndChildBounds();
    repaint();
  }

  void mouseDown(const juce::MouseEvent&) override
  {
    worldCenterAtDragStart = worldCenter;
  }

  void mouseDrag(const juce::MouseEvent &e) override
  {
    auto offsetPixels = e.getOffsetFromDragStart().toDouble();
    const auto worldUnitsFromPixelsScale = juce::Point<double>(unitsPerPixel, unitsPerPixel);
    auto offsetWorld = worldUnitsFromPixelsScale * offsetPixels;
    worldCenter = worldCenterAtDragStart + offsetWorld;

    updateTransformsAndChildBounds();
    repaint();
  }

  void resized() override
  {
    updateTransformsAndChildBounds();
  }

  void paint(juce::Graphics &g) override
  {
    const juce::Colour backgroundColor = juce::Colour(0x08, 0x0C, 0x1C);
    const juce::Colour axisColor = juce::Colours::navajowhite;
    const juce::Colour lineColor = juce::Colours::snow;
    const juce::Colour circleColor = juce::Colours::goldenrod;

    const auto axisThicknessPixels = 3;
    const auto circleThicknessPixels = 3;
    const auto lineThicknessPixels = 2;

    // TODO(ry): axis & line labels

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
      auto at = juce::Point<double>(unitsPerLine, unitsPerLine);
      while(at.x < rightWorld)
      {
        g.drawLine(at.x, bottomWorld, at.x, topWorld, lineThicknessPixels * unitsPerPixel);
        at.x += unitsPerLine;
      }
      while(at.y < topWorld)
      {
        g.drawLine(leftWorld, at.y, rightWorld, at.y, lineThicknessPixels * unitsPerPixel);
        at.y += unitsPerLine;
      }
      at = juce::Point<double>(-unitsPerLine, -unitsPerLine);
      while(at.x > leftWorld)
      {
        g.drawLine(at.x, bottomWorld, at.x, topWorld, lineThicknessPixels * unitsPerPixel);
        at.x -= unitsPerLine;
      }
      while(at.y > bottomWorld)
      {
        g.drawLine(leftWorld, at.y, rightWorld, at.y, lineThicknessPixels * unitsPerPixel);
        at.y -= unitsPerLine;
      }
    }
  }

  juce::AffineTransform pixelsFromWorldUnits;
  juce::AffineTransform worldUnitsFromPixels;

private:

  void updateTransformsAndChildBounds(void)
  {
    // NOTE(ry): compute transforms between screen space and world space
    auto localBounds = getLocalBounds().toDouble();
    auto regionCenter = localBounds.getCentre();
    pixelsFromWorldUnits = juce::AffineTransform()
      .translated(worldCenter.x, -worldCenter.y)
      .scaled(pixelsPerUnit, -pixelsPerUnit)
      .translated(regionCenter.x, regionCenter.y);
    worldUnitsFromPixels = pixelsFromWorldUnits.inverted();

    // NOTE(ry): update child bounds (using new transforms)
    for(auto *p : points)
    {
      if(auto *root = p->root.get())
      {
        p->updateBounds(root->value);
      }
    }
  }

  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override
  {
    juce::ignoreUnused(parent);
    auto root = processor.state.getRootFromTreeNode(child);
    auto *point = points.add(new ComplexPlaneEditor::RootPoint(this, root));
    addAndMakeVisible(point);
    resized();
  }

  void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override
  {
    points.removeLast();
    repaint();
  }

  juce::TextButton addRoot;
  juce::TextButton delRoot;
  juce::TextButton undo;
  juce::TextButton redo;
  juce::OwnedArray<RootPoint> points;

  double pixelsPerUnit;
  double unitsPerPixel;
  double unitsPerLine;
  juce::Point<double> worldCenter;
  juce::Point<double> worldCenterAtDragStart;

  //FilterState &state;
  AudioPluginAudioProcessor &processor;
};

//==============================================================================
class CoefficientsComponent final : public juce::Component
{
public:
  CoefficientsComponent();
  ~CoefficientsComponent();

  void paint(juce::Graphics& g) override
  {
    g.fillAll(juce::Colours::white);
  }
private:
};

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
  explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
  ~AudioPluginAudioProcessorEditor() override;

  //==============================================================================
  void paint (juce::Graphics&) override;
  void resized() override;

private:
  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  AudioPluginAudioProcessor& processorRef;

  ComplexPlaneEditor complexPlaneEditor;
  CoefficientsComponent coefficients;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
