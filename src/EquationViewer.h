class EquationViewer
  :public juce::Component
  ,private juce::ValueTree::Listener
{
public:
  EquationViewer(AudioPluginAudioProcessor *p);
  ~EquationViewer();

  void resized(void) override;
  void paint(juce::Graphics &g) override;

private:
  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index) override;
  void valueTreePropertyChanged(juce::ValueTree &node, juce::Identifier const &property) override;
  void updateCoeffs(void);

  class EquationText : public juce::Component
  {
  public:
    void paint(juce::Graphics &g) override;

  private:
    void setTextFromCoeffs(std::vector<double> const &coeffs);
    friend class EquationViewer;
    juce::String text;
  };

  AudioPluginAudioProcessor *processor;

  EquationText numeratorText, denominatorText;
  juce::Viewport numeratorView, denominatorView;

  static auto constexpr scrollBarHeightPixels = 5;
};
