#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "FilterState.h"
#include "PluginProcessor.h"

class ComplexPlaneEditor final :
    public juce::Component,
    private juce::Slider::Listener,
    private juce::ValueTree::Listener
{
  /** TODO(ry):
   * better root creation interface (ability to create poles, increase/decrease root order)
   * split roots
   * visually indicate snaping roots to real axis
   * visually indicate when a root is being hovered over (highlight, show tooltip)
   * restore a root to its previous position when undoing its deletion
   * improve axis label positioning relative to grid lines
   */

public:
  ComplexPlaneEditor(AudioPluginAudioProcessor *p);
  ~ComplexPlaneEditor();

  class RootPoint final
    : public juce::Component,
      private juce::ValueTree::Listener,
      private juce::Timer
  {
  public:
    RootPoint(bool c, FilterRoot::Ptr r);
    ~RootPoint();

    void mouseEnter(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;

    void paint(juce::Graphics &g) override;

    void updateBounds(c128 value);

    bool isConjugate;
    FilterRoot::Ptr root;
    RootPoint *conjugate;

  private:
    void valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property) override;
    void timerCallback(void) override;

    static s32 constexpr timerFreq = 50;

    friend ComplexPlaneEditor;
    static ComplexPlaneEditor *editor;
    c128 valueAtDragStart;

    bool isDragging;
    bool wasRightButtonDown;
  };

  class RootTooltip final : public juce::Component, private juce::Timer
  {
  public:
    explicit RootTooltip(ComplexPlaneEditor *e);

    enum VisibilityFlags : u32
    {
      hint = (1 << 0),
      require = (1 << 1),
    };

    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

    void resized(void) override;

    void paint(juce::Graphics &g) override;

    inline void setText(juce::String const &text)
    {
      orderLabel.setText(text, juce::dontSendNotification);
    }

    inline void setPos(juce::Point<int> point)
    {
      setTopRightPosition(point.getX() + (this->getWidth()  + 10),
			  point.getY() - (this->getHeight() - 10));
    }

    inline void setRoot(FilterRoot::Ptr r)
    {
      root = r;
      if(auto *rootPtr = root.get())
      {
	setText(juce::String(rootPtr->order));
      }
    }

    inline void show()
    {
      keepaliveCounter += 1;
      setVisible(true);
    }

    inline void setRootAndShow(FilterRoot::Ptr r)
    {
      setRoot(r);
      show();
    }

    inline void setPointAndShow(RootPoint *point)
    {
      setPos(juce::Point<int>(point->getX(), point->getY()));
      setRootAndShow(point->root);
    }

    inline void hide(void)
    {
      if(keepaliveCounter)
      {
	if(!isTimerRunning())
	{
	  startTimer(keepaliveDelayMs);
	}
	++pendingEventCount;
      }
    }

  private:
    void timerCallback(void) override
    {
      DBG("tooltip timer callback: keepalive = " << keepaliveCounter);
      keepaliveCounter -= 1;
      if(!(--pendingEventCount))
      {
	stopTimer();
      }
      if(keepaliveCounter <= 0)
      {
	setVisible(false);
      }
    }

    FilterRoot::Ptr root;

    juce::TextButton destroy;
    juce::TextButton orderInc;
    juce::TextButton orderDec;
    juce::Label orderLabel;

    s32 keepaliveCounter = 0;
    s32 pendingEventCount = 0;
    static s32 const keepaliveDelayMs = 60;

    friend ComplexPlaneEditor;
    ComplexPlaneEditor *editor;
  };

  void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w) override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  void resized(void) override;

  void paint(juce::Graphics &g) override;

  juce::AffineTransform pixelsFromWorldUnits;
  juce::AffineTransform worldUnitsFromPixels;

private:
  void updateTransforms(void);
  void updateTransformsAndChildBounds(void);

  void sliderValueChanged(juce::Slider *slider) override;

  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index) override;
  void valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property) override;

  double pixelsPerUnit;
  double unitsPerPixel;
  double unitsPerLine;

  AudioPluginAudioProcessor *processor;

  juce::Point<double> worldCenter; // NOTE(ry): the center of the drawable region in world coordinates
  juce::Point<double> worldCenterAtDragStart;

  juce::OwnedArray<RootPoint> points;
  FilterRoot::Ptr activeRoot; // NOTE(ry): the root the mouse is hovering over or being dragged
  FilterRoot::Ptr targetRoot; // NOTE(ry): the root the active root is hovering over

  RootTooltip tooltip;

  juce::Slider gainSlider;

  // NOTE(ry): debug ui
  juce::TextButton addRoot;
  juce::TextButton delRoot;
  juce::TextButton undo;
  juce::TextButton redo;
};
