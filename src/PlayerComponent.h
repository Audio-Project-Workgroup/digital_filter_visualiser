#pragma once
#include <juce_core/system/juce_PlatformDefs.h>
#include <juce_events/juce_events.h>
#include "PluginProcessor.h"

class PlayerComponent 
	: public juce::Component
	, public juce::ChangeListener
{
public:
	PlayerComponent(AudioPluginAudioProcessor& p);
	~PlayerComponent();
	void resized() override;
	void updateButtons(PlayerState playerState);
	void changeListenerCallback(juce::ChangeBroadcaster* source) override;
	
	const int buttonsCount = 4;
	
private:
	const juce::String filePatterns = "*.aif;*.aiff;*.flac;*.ogg;*.wav;*.wma";
	void openFile();
	void playPause();
	void stop();
	void updateLoopStatus();

	juce::ImageButton openButton, playButton, stopButton, loopButton;
	juce::Image openImage, playImage, pauseImage, stopImage, loopImage;

	AudioPluginAudioProcessor& processor;
	std::unique_ptr<juce::FileChooser> chooser;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerComponent)
};
