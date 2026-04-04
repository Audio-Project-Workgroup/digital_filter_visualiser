#include "PluginEditor.h"

// NOTE(ry): Don't delete this. I know what I'm doing
#include "ComplexPlaneEditor.cpp"
#include "StateSerializer.h"

//==============================================================================
CoefficientsComponent::CoefficientsComponent()
{}

CoefficientsComponent::~CoefficientsComponent()
{}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
  : AudioProcessorEditor (&p), processorRef (p), complexPlaneEditor(&p), phaseFrequencyResponseViewer(&p), exportButton("Export...")
{
  juce::ignoreUnused (processorRef);

  exportPopupMenu.addItem("Filter coefficients", [this]
    {
      chooseFileAndSave(StateSerializer::exportCoefficients(this->processorRef.filterState.get()));
    });
  exportPopupMenu.addItem("Processor chain parameters", [this]
    {
      chooseFileAndSave(StateSerializer::exportProcessorChainParameters(this->processorRef.activeState.load()));
    });
  exportButton.onClick = [this] { exportPopupMenu.showMenuAsync({}); };

  addAndMakeVisible(complexPlaneEditor);
  addAndMakeVisible(coefficients);
  addAndMakeVisible(phaseFrequencyResponseViewer);
  addAndMakeVisible(exportButton);

  setSize(640, 480);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
  juce::ignoreUnused(g);
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

  // g.setColour (juce::Colours::white);
  // g.setFont (15.0f);
  // g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
  exportButton.setBounds(0, 0, 100, 30);
  coefficients.setBounds(0, 0, getWidth() / 2, getHeight() / 2);
  phaseFrequencyResponseViewer.setBounds(0, getHeight() / 2, getWidth() / 2, getHeight() / 2);
  complexPlaneEditor.setBounds(getWidth() / 2, 0, getWidth() / 2, getHeight());
}

void AudioPluginAudioProcessorEditor::chooseFileAndSave(std::shared_ptr<juce::XmlElement> xml)
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
