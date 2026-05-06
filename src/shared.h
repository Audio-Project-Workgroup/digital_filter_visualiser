#include "FilterState.h"

class AudioPluginAudioProcessor;
class AudioPluginAudioProcessorEditor;
template<typename T> class ProcessorChain;
template<typename T> class ValueChangeBroadcaster;
enum class PlayerState;

namespace juce
{
  class AudioProcessor;
  class AudioProcessorEditor;
  class ChangeBroadcaster;
  class File;
  template<typename T, typename S> class OwnedArray;
};

template<typename SampleType>
using FullState = juce::OwnedArray<ProcessorChain<SampleType>>;

// NOTE(ry): implemented by processor
extern FilterState* filterStateFromProcessor(AudioPluginAudioProcessor *p);
extern double sampleRateFromProcessor(AudioPluginAudioProcessor *p);
extern std::atomic<juce::OwnedArray<ProcessorChain<float>>*>& activeStateFromProcessor(AudioPluginAudioProcessor *p);
extern ValueChangeBroadcaster<PlayerState>& playerStateFromProcessor(AudioPluginAudioProcessor *p);

extern void setProcessorTransportSourceFromFile(AudioPluginAudioProcessor *p, juce::File const &file);
extern void startProcessorTransportSource(AudioPluginAudioProcessor *p);
extern void stopProcessorTransportSource(AudioPluginAudioProcessor *p, bool resetPosition);

extern void setProcessorReaderSourceLooping(AudioPluginAudioProcessor *p, bool loop);

extern bool isProcessorStandalone(AudioPluginAudioProcessor *p);

extern juce::AudioProcessor* downcastProcessor(AudioPluginAudioProcessor *p);
extern juce::ChangeBroadcaster* downcastProcessorBroadcaster(AudioPluginAudioProcessor *p);

// NOTE(ry): implemented by editor
extern AudioPluginAudioProcessorEditor* makeEditor(AudioPluginAudioProcessor *p);
extern juce::AudioProcessorEditor* downcastEditor(AudioPluginAudioProcessorEditor *e);

enum class PlayerState
{
	Empty,
	Stopped,
	Playing,
	Paused
};
