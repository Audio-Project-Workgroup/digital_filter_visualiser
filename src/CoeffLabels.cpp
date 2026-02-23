#include "CoeffLabels.h"


CoefficientLabel::CoefficientLabel()
{
    numerator.setEditable(true);
    denominator.setEditable(true);
    fractionLine.setEditable(false);


    fractionLine.setText("/", juce::dontSendNotification);
    fractionLine.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(numerator);
    addAndMakeVisible(fractionLine);
    addAndMakeVisible(denominator);
}


void CoefficientLabel::resized()
{
    auto area = getLocalBounds();
    int w = area.getWidth() / 3;
    int h = area.getHeight();

    numerator.setBounds(0, 0, w, h);
    fractionLine.setBounds(w, 0, w, h);
    denominator.setBounds(2 * w, 0, w, h);
}