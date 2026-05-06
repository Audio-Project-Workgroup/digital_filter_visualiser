#pragma once

class PhaseFrequencyResponseViewer final :
    public juce::Component,
    juce::ChangeListener
{
public:
    PhaseFrequencyResponseViewer(AudioPluginAudioProcessor* _processor);
    ~PhaseFrequencyResponseViewer();
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void calculate(
        bool isLogScale,
        int intWidth,
        std::vector<double>& angles,
        std::vector<double>& amplitudes,
        std::vector<double>& phases);
    inline void calculateCoefficients(
        double angle, 
        FilterRoot* root, 
        double& ampCoeff, 
        double& phaseCoeff);

    const juce::Colour
        backgroundColor = juce::Colour(0x08, 0x0C, 0x1C),
        lineColour = juce::Colours::white,
        gridColour = juce::Colours::darkgrey;
    const juce::PathStrokeType strokeType{ 3.f };
    const int
        padding = 5,
        plotPaddingLeft = 55,
        plotPaddingRight = 45,
        plotPaddingTop = 40,
        plotPaddingBottom = 20;
    const int
        zoomButtonsSize = 20,
        logScaleButtonWidth = 60,
        textWidth = 40,
        textHeight = 10;
    const float
        minAmpDb = 6.f,
        maxAmpDb = 96.f,
        minFreq = 20.f;

    float ampDb;
    double sampleRate;

    AudioPluginAudioProcessor* processor;

    juce::TextButton zoomInButton, zoomOutButton, logScaleButton;
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
