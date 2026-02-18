class ComplexPlaneEditor final
  : public juce::Component,
    private juce::Slider::Listener,
    private juce::ValueTree::Listener
{
  /** TODO(ry):
   * better root creation interface (ability to create poles, increase/decrease root order)
   * merge and split roots
   * visually indicate snaping roots to real axis
   * visually indicate when a root is being hovered over (highlight, show tooltip)
   * restore a root to its previous position when undoing its deletion
   * improve axis label positioning relative to grid lines
   */

public:
  ComplexPlaneEditor(AudioPluginAudioProcessor &p);
  ~ComplexPlaneEditor();

  class RootPoint final : public juce::Component, private juce::ValueTree::Listener
  {
  public:

    RootPoint(ComplexPlaneEditor *e, bool c, FilterRoot::Ptr r);
    ~RootPoint();

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;

    void paint(juce::Graphics &g) override;

    void updateBounds(c128 value);

    bool isConjugate;
    FilterRoot::Ptr root;

  private:

    void valueTreePropertyChanged(juce::ValueTree &node, const juce::Identifier &property) override;

    ComplexPlaneEditor *parent;
    c128 valueAtDragStart;
  };

  void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w) override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  void resized() override;

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

  //FilterState &state;
  AudioPluginAudioProcessor &processor;

  juce::Point<double> worldCenter; // NOTE(ry): the center of the drawable region in world coordinates
  juce::Point<double> worldCenterAtDragStart;

  juce::OwnedArray<RootPoint> points;

  juce::Slider gainSlider;

  // NOTE(ry): debug ui
  juce::TextButton addRoot;
  juce::TextButton delRoot;
  juce::TextButton undo;
  juce::TextButton redo;
};
