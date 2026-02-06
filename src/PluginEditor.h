#pragma once

#include "PluginProcessor.h"

//==============================================================================
class ComplexPlaneEditor final : public juce::Component, private juce::ValueTree::Listener
{
public:
  ComplexPlaneEditor(AudioPluginAudioProcessor &p)
    : addRoot("+"), delRoot("-"), undo("undo"), redo("redo"), processor(p)
  {
    processor.state.addListener(this);

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
        auto worldUnitsFromPixels = juce::Point<double>(parent->unitsPerPixelX, -parent->unitsPerPixelY);
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
      auto pixelsFromWorldUnits = juce::Point<double>(parent->pixelsPerUnitX, -parent->pixelsPerUnitY);
      auto newPos = pixelsFromWorldUnits * juce::Point<double>(value.real(), value.imag());
      setCentrePosition(juce::roundToInt(newPos.getX()), juce::roundToInt(newPos.getY()));
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

  void resized() override
  {
    pixelsPerUnitX = getWidth() / worldRange;
    pixelsPerUnitY = getHeight() / worldRange;
    unitsPerPixelX = 1.0 / pixelsPerUnitX;
    unitsPerPixelY = 1.0 / pixelsPerUnitY;
    for(auto *p : points)
    {
      if(auto *root = p->root.get())
      {
        p->updateBounds(root->value);
      }
    }
  }

  void paint(juce::Graphics &g) override
  {
    const juce::Colour backgroundColor = juce::Colour(0x08, 0x0C, 0x1C);
    const juce::Colour axisColor = juce::Colours::navajowhite;
    const juce::Colour lineColor = juce::Colours::snow;
    const juce::Colour circleColor = juce::Colours::goldenrod;

    // TODO(ry): label axes, lines

    auto center = getLocalBounds().getCentre().toFloat();

    // NOTE(ry): draw background
    g.fillAll(backgroundColor);

    // NOTE(ry): draw axes
    g.setColour(axisColor);
    g.drawVerticalLine(center.x, 0.f, r32(getHeight()));
    g.drawHorizontalLine(center.y, 0.f, r32(getWidth()));

    // NOTE(ry): draw unit circle
    g.setColour(circleColor);
    auto radiusX = r32(pixelsPerUnitX);
    auto radiusY = r32(pixelsPerUnitY);
    auto minX = center.x - radiusX;
    auto minY = center.y - radiusY;
    auto widthX = 2.f * radiusX;
    auto widthY = 2.f * radiusY;
    g.drawEllipse(minX, minY, widthX, widthY, 1.f);

    // NOTE(ry): draw lines
    g.setColour(lineColor);
    g.drawVerticalLine(center.x - radiusX, 0.f, r32(getHeight()));
    g.drawVerticalLine(center.x + radiusX, 0.f, r32(getHeight()));
    g.drawHorizontalLine(center.y - radiusY, 0.f, r32(getWidth()));
    g.drawHorizontalLine(center.y + radiusY, 0.f, r32(getWidth()));
  }

private:

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

  const double worldRange = 3.0; // TODO(ry): make this variable with zoom

  double pixelsPerUnitX;
  double pixelsPerUnitY;
  double unitsPerPixelX;
  double unitsPerPixelY;

  //FilterState &state;
  AudioPluginAudioProcessor &processor;
};

//==============================================================================
class CoefficientsComponent final : public juce::Component
{
public:
  CoefficientsComponent();
  ~CoefficientsComponent();

  //void paint(juce::Graphics&) override;
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
