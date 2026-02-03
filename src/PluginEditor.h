#pragma once

#include "PluginProcessor.h"

//==============================================================================
class RootSliderComponent final : public juce::Component, private juce::ValueTree::Listener
{
public:
  RootSliderComponent(AudioPluginAudioProcessor &p);
  ~RootSliderComponent();

  struct Slider : juce::Slider, private juce::ValueTree::Listener
  {
    enum SliderKind
    {
      Mag,
      Arg,
    };

    Slider(FilterState &s, FilterRoot::Ptr r, SliderKind k)
      : juce::Slider(), stateRef(s), root(r), kind(k)
    {
      if(auto *rootPtr = root.get()) { rootPtr->node.addListener(this); }
    }
    ~Slider()
    {
      if(auto *rootPtr = root.get()) { rootPtr->node.removeListener(this); }
      stateRef.remove(root);
    }

  private:

    void valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property) override
    {
      // NOTE(ry): we look up the value in the tree here, instead of using the
      // cached value in the root, because this function may be called before
      // the cached value has updated. Thus the tree is the only reliable
      // "source of truth" for listeners, whereas filter roots are for fast
      // access and easy modification.
      // But like, why not pass the value here instead of the property so I
      // don't have to do a O(N) scan to get the value I care about?
      if(property == IDs::ValueRe || property == IDs::ValueIm)
      {
        c128 c = c128(node.getProperty(IDs::ValueRe), node.getProperty(IDs::ValueIm));
        DBG("slider update from tree: (" << c.real() << ", " << c.imag() << ")");
        switch(kind)
        {
          case Slider::Mag: { setValue(std::abs(c), juce::dontSendNotification); }break;
          case Slider::Arg: { setValue(std::arg(c), juce::dontSendNotification); }break;
          default: { jassertfalse; }break;
        }
      }
    }

    FilterState &stateRef;
    FilterRoot::Ptr root;
    SliderKind kind;
  };

  //void paint(juce::Graphics&) override;
  void resized() override;

private:

  void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override
  {
    // sliders.removeLast();
    // sliders.removeLast();
    delRoot.onClick();
  }

  void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override
  {
    juce::ignoreUnused(parent);
    auto root = processorRef.state.getRootFromTreeNode(child);
    auto *mag = sliders.add(new RootSliderComponent::Slider(processorRef.state, root, Slider::Mag));
    auto *arg = sliders.add(new RootSliderComponent::Slider(processorRef.state, root, Slider::Arg));
    mag->setSliderStyle(juce::Slider::LinearHorizontal);
    arg->setSliderStyle(juce::Slider::LinearHorizontal);
    mag->setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    arg->setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 25);
    mag->setRange(0.0, 1.0);
    arg->setRange(-juce::MathConstants<double>::pi, juce::MathConstants<double>::pi);
    // mag->addListener(this);
    // arg->addListener(this);
    auto dragStart = [this]{
      DBG("slider begin new transaction");
      processorRef.um.beginNewTransaction();
    };
    auto valueChange = [root, mag, arg, this]{
      if(auto *r = root.get())
      {
        auto m = mag->getValue();
        auto a = arg->getValue();
        c128 c = std::polar(m, a);
        DBG("slider update: (" << c.real() << ", " << c.imag() << ")");
        r->value = c; // NOTE(ry): you can update the state by writing "normal" code
      }
    };
    mag->onDragStart = arg->onDragStart = dragStart;
    mag->onValueChange = arg->onValueChange = valueChange;
    addAndMakeVisible(mag);
    addAndMakeVisible(arg);
    resized();
  }

  juce::TextButton addRoot;
  juce::TextButton delRoot;
  juce::TextButton undo;
  juce::TextButton redo;
  juce::OwnedArray<Slider> sliders;
  AudioPluginAudioProcessor &processorRef;
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

  RootSliderComponent rootSliders;
  CoefficientsComponent coefficients;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
