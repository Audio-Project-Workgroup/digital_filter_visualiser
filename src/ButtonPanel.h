#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class ButtonPanel final :
    public juce::Component
{
public:
    ButtonPanel(AudioPluginAudioProcessor& p);
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;
    juce::TextButton exportButton;
    juce::PopupMenu exportPopupMenu;
    std::unique_ptr<juce::FileChooser> chooser;

    void chooseFileAndSave(std::shared_ptr<juce::XmlElement> xml);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonPanel)
};
