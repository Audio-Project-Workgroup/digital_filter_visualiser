
EquationViewer::
EquationViewer(AudioPluginAudioProcessor *p)
  :processor(p)
{
  addAndMakeVisible(numeratorText);
  addAndMakeVisible(denominatorText);

  numeratorView.setViewedComponent(&numeratorText, false);
  denominatorView.setViewedComponent(&denominatorText, false);
  numeratorView.setScrollBarThickness(scrollBarHeightPixels);
  denominatorView.setScrollBarThickness(scrollBarHeightPixels);

  addAndMakeVisible(numeratorView);
  addAndMakeVisible(denominatorView);

  processor->filterState->addListener(this);
  processor->filterState->syncListener(this);
}

EquationViewer::
~EquationViewer()
{
  processor->filterState->removeListener(this);
}

void EquationViewer::
resized(void)
{
  auto area = getLocalBounds();
  auto constexpr margin = 5;
  auto numeratorBounds = area.removeFromTop(0.5*getHeight()).reduced(margin);
  auto denominatorBounds = area.removeFromTop(0.5*getHeight()).reduced(margin);
  numeratorView.setBounds(numeratorBounds);
  denominatorView.setBounds(denominatorBounds);

  auto constexpr maxTextWidth = 12000;
  numeratorBounds.removeFromBottom(scrollBarHeightPixels);
  denominatorBounds.removeFromBottom(scrollBarHeightPixels);
  numeratorText.setBounds(numeratorBounds.reduced(margin).withWidth(maxTextWidth));
  denominatorText.setBounds(denominatorBounds.reduced(margin).withWidth(maxTextWidth));
}

void EquationViewer::
paint(juce::Graphics &g)
{
  auto constexpr margin = 5;
  auto constexpr barThicknessPixels = 2;
  g.setColour(juce::Colours::white);
  g.fillRect(margin, 0.5*getHeight(), getWidth() - margin, barThicknessPixels);
}

void EquationViewer::
valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child)
{
  juce::ignoreUnused(parent);
  juce::ignoreUnused(child);

  updateCoeffs();
}

void EquationViewer::
valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int index)
{
  juce::ignoreUnused(parent);
  juce::ignoreUnused(child);
  juce::ignoreUnused(index);

  updateCoeffs();
}

void EquationViewer::
valueTreePropertyChanged(juce::ValueTree &node, juce::Identifier const &property)
{
  if(property == IDs::ValueRe || property == IDs::ValueIm)
  {
    updateCoeffs();
  }
  else if(property == IDs::Order)
  {
    int order = node.getProperty(IDs::Order);
    if(order != 0)
    {
      // NOTE(ry): if order is being set to zero, the node will get removed and
      // update will be handled in the removed callback
      updateCoeffs();
    }
  }
}

void EquationViewer::
updateCoeffs(void)
{
  auto ffCoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(processor->filterState->zeros);
  auto fbCoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(processor->filterState->poles);

  numeratorText.setTextFromCoeffs(ffCoeffs);
  denominatorText.setTextFromCoeffs(fbCoeffs);

  repaint();
}

void EquationViewer::EquationText::
resized(void)
{
  // TODO(ry): adjust size based on text width
}

void EquationViewer::EquationText::
paint(juce::Graphics &g)
{
  auto constexpr fontHeightPixels = 24;
  g.setColour(juce::Colours::white);
  g.setFont(fontHeightPixels);

  auto const bottomLeft = getLocalBounds().getBottomLeft();
  g.drawSingleLineText(text, bottomLeft.x, bottomLeft.y);
}

void EquationViewer::EquationText::
setTextFromCoeffs(std::vector<double> const &coeffs)
{
  auto constexpr numDecimalPlaces = 2;

  text.clear();
  for(size_t idx = 0; idx < coeffs.size(); ++idx)
  {
    auto const coeff = coeffs[idx];
    auto const order = coeffs.size() - 1 - idx;
    if(!juce::approximatelyEqual(coeff, 0.0))
    {
      if(coeff > 0.0 && idx > 0)
      {
	text << "+";
      }
      text << juce::String(coeff, numDecimalPlaces);
      if(order > 0)
      {
	text << "z^" << juce::String(order);
      }
    }
  }
}
