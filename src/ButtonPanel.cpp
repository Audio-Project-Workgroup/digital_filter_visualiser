#include "ButtonPanel.h"
#include "StateSerializer.h"

ButtonPanel::ButtonPanel(AudioPluginAudioProcessor& p)
  :processorRef(p)
  ,exportButton("Export...")
  ,undoButton("undo")
  ,redoButton("redo")
  ,addRootButton("+")
  ,delRootButton("-")
{
    exportPopupMenu.addItem("Filter coefficients", [this]
        {
            chooseFileAndSave(StateSerializer::exportCoefficients(this->processorRef.filterState.get()));
        });
    exportPopupMenu.addItem("Processor chain parameters", [this]
        {
            chooseFileAndSave(StateSerializer::exportProcessorChainParameters(this->processorRef.activeState.load()));
        });
    exportButton.onClick = [this] { exportPopupMenu.showMenuAsync({}); };

    undoButton.onClick = [this]{
      processorRef.filterState->um->undo();
      jassert(processorRef.filterState->totalOrder >= processorRef.filterState->finiteZerosOrder);
    };
    redoButton.onClick = [this]{
      processorRef.filterState->um->redo();
      jassert(processorRef.filterState->totalOrder >= processorRef.filterState->finiteZerosOrder);
    };
    addRootButton.onClick = [this]{
      processorRef.filterState->um->beginNewTransaction();
      processorRef.filterState->add(1);
    };

    // NOTE(ry): gain slider setup
    gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    gainSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);
    gainSlider.setRange(-90.f, 6.f);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.onValueChange = [this]{
      auto const dB = gainSlider.getValue();
      auto const amp = juce::Decibels::decibelsToGain(dB);
      processorRef.filterState->gain = amp;
    };

    addAndMakeVisible(exportButton);
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);
    addAndMakeVisible(addRootButton);
    addAndMakeVisible(delRootButton);
    addAndMakeVisible(gainSlider);

    processorRef.filterState->addListener(this);
    processorRef.filterState->syncListener(this);
}

ButtonPanel::~ButtonPanel()
{
    processorRef.filterState->removeListener(this);
}

void ButtonPanel::resized()
{
  auto area = getLocalBounds();
  auto constexpr padding = 5;
  auto constexpr buttonWidth_PercentOfParent = 0.1;
  auto const buttonWidth = getWidth() * buttonWidth_PercentOfParent;
  exportButton.setBounds(area.removeFromLeft(buttonWidth).reduced(padding));
  undoButton.setBounds(area.removeFromLeft(buttonWidth).reduced(padding));
  redoButton.setBounds(area.removeFromLeft(buttonWidth).reduced(padding));
  addRootButton.setBounds(area.removeFromLeft(buttonWidth).reduced(padding));
  delRootButton.setBounds(area.removeFromLeft(buttonWidth).reduced(padding));
  gainSlider.setBounds(area.reduced(padding));
  //exportButton.setBounds(0, 0, 100, getHeight());
}

void ButtonPanel::chooseFileAndSave(std::shared_ptr<juce::XmlElement> xml)
{
    chooser = std::make_unique<juce::FileChooser>(
        "Save to file...", juce::File{}, "*.xml");
    const auto chooserFlags =
        juce::FileBrowserComponent::FileChooserFlags::saveMode |
        juce::FileBrowserComponent::FileChooserFlags::warnAboutOverwriting;
    std::function<void(const juce::FileChooser& fc)> function =
        [this, xml](const juce::FileChooser& fc)
        {
            try
            {
                juce::File file = fc.getResult();
                if (file != juce::File{})
                {
                    auto str = xml.get()->toString().toStdString();
                    xml->writeTo(file, {});
                }
            }
            catch (const std::exception& e)
            {
                juce::NativeMessageBox::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon, "Error", e.what());
            }
        };
    chooser->launchAsync(chooserFlags, function);
}

void ButtonPanel::valueTreePropertyChanged(juce::ValueTree &node, juce::Identifier const &property)
{
  if(property == IDs::Gain)
  {
    auto const amp = r64(node.getProperty(IDs::Gain));
    auto const dB = juce::Decibels::gainToDecibels(amp);
    gainSlider.setValue(dB, juce::dontSendNotification);
  }
}
