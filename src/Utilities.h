#pragma once
#include <juce_graphics/juce_graphics.h>

class Utilities
{
public:
    static void setButtonImage(juce::ImageButton* button, juce::Image& image)
    {
        button->setImages(
            false, true, true,
            image, 1.0f, juce::Colours::transparentWhite,
            image, 0.8f, juce::Colours::transparentWhite,
            image, 0.5f, juce::Colours::transparentWhite);
    }
};