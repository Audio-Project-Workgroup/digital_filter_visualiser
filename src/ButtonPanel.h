#pragma once
#include "PluginProcessor.h"

class ButtonPanel final
  :public juce::Component
  ,private juce::ValueTree::Listener
{
public:
    ButtonPanel(AudioPluginAudioProcessor& p);
    ~ButtonPanel();
    void resized() override;

private:
    void valueTreePropertyChanged(juce::ValueTree &node, juce::Identifier const &property) override;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;
    juce::TextButton exportButton;
    juce::PopupMenu exportPopupMenu;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton addRootButton;
    juce::TextButton delRootButton;
    juce::Slider gainSlider;

    void chooseFileAndSave(std::shared_ptr<juce::XmlElement> xml);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonPanel)
};
