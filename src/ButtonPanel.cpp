#include "ButtonPanel.h"
#include "StateSerializer.h"

ButtonPanel::ButtonPanel(AudioPluginAudioProcessor& p) :
    processorRef(p),
	exportButton("Export...")
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
    addAndMakeVisible(exportButton);
}

void ButtonPanel::resized()
{
	exportButton.setBounds(0, 0, 100, getHeight());
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
