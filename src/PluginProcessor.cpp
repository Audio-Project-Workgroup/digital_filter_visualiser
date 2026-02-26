#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProcessorChainModifier.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
  : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    state(*this, &um),
    activeState(new FullState<SampleType>),
    pendingState(new FullState<SampleType>)
    {
        um.addChangeListener(this);
    }


AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    um.removeChangeListener(this);

    auto* activePtr = activeState.exchange(nullptr);
    if (activePtr != nullptr)
    {
        while (isActiveStateUsed.load())
            juce::Thread::sleep(1);
        delete activePtr;
    }

    if (pendingState != nullptr)
    {
        while (isPendingStateUsed.load())
            juce::Thread::sleep(1);
        delete pendingState;
        pendingState = nullptr;
    }
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
  return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
  return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
  // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
  return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
  juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
  juce::ignoreUnused (index);
  return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
  juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    std::lock_guard<std::mutex> lock(stateMutex);

    crossFadeBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);

    juce::dsp::ProcessSpec newSpec
    {
        sampleRate, 
        juce::uint32(samplesPerBlock), 
        juce::uint32(1) // one channel per filter!
    };
    spec = newSpec;
    auto numChannels = juce::uint32(getTotalNumOutputChannels());

    isActiveStateUsed.store(true);
    isPendingStateUsed.store(true);

    if (auto activeProc = activeState.load())
        if (pendingState != nullptr)
        {
            activeProc->clear(true);
            pendingState->clear(true);

            for (auto i = 0; i < numChannels; i++)
            {
                auto* activeItem = new ProcessorChain<SampleType>;
                activeItem->prepare(spec);
                activeProc->add(activeItem);
                auto* pendingItem = new ProcessorChain<SampleType>;
                pendingItem->prepare(spec);
                pendingState->add(pendingItem);
            }

            ProcessorChainModifier::RootsToJuceCoeffs(state, activeState.load(), spec);
        }
    isPrepared = true;
    isPendingStateUsed.store(false);
    isActiveStateUsed.store(false);
}

void AudioPluginAudioProcessor::releaseResources()
{
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused (layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
      && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<SampleType>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (isNewStateReady.load())
    {
        auto* activeProc = activeState.load();
        juce::dsp::AudioBlock<SampleType> block(buffer);
        juce::dsp::AudioBlock<SampleType> crossFadeBlock(crossFadeBuffer);

        for (auto ch = 0; ch < totalNumInputChannels; ch++)
        {
            crossFadeBuffer.copyFrom(ch, 0, buffer.getReadPointer(ch), numSamples);

            auto oldBlock = block.getSingleChannelBlock(ch);
            juce::dsp::ProcessContextReplacing<SampleType> oldContext(oldBlock);
            activeProc->getUnchecked(ch)->process(oldContext);

            auto newBlock = crossFadeBlock.getSingleChannelBlock(ch);
            juce::dsp::ProcessContextReplacing<SampleType> newContext(newBlock);
            pendingState->getUnchecked(ch)->process(newContext);

            buffer.applyGainRamp(ch, 0, numSamples, 1.0f, 0.0f);
            buffer.addFromWithRamp(ch, 0, crossFadeBuffer.getReadPointer(ch), numSamples, 0.0f, 1.0f);
        }

        auto* old = activeState.exchange(pendingState);
        pendingState = old;
        isNewStateReady.store(false);
    }
    else
    {
        auto* proc = activeState.load();
        juce::dsp::AudioBlock<SampleType> block(buffer);

        for (auto ch = 0; ch < totalNumInputChannels; ch++)
        {
            auto channelBlock = block.getSingleChannelBlock(ch);
            juce::dsp::ProcessContextReplacing<SampleType> context(channelBlock);
            proc->getUnchecked(ch)->process(context);
        }
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
  return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries to make it easy to save and load complex data.
  juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
  // You should use this method to restore your parameters from this memory block,
  // whose contents will have been created by the getStateInformation() call.
  juce::ignoreUnused (data, sizeInBytes);
}

void AudioPluginAudioProcessor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &um)
    {
        ProcessorChainModifier::Process(*this);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new AudioPluginAudioProcessor();
}
