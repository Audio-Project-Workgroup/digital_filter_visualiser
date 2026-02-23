#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// https://docs.juce.com/master/classjuce_1_1Label.html
// https://forum.juce.com/t/label-editable-partly/32878

class CoefficientLabel : public juce::Component

{

public:
    CoefficientLabel();

    void resized() override;

private:
    juce::Label numerator, denominator, fractionLine;
};