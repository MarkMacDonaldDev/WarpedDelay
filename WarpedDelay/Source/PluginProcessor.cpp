/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

const int g_nSampleRate = 44100;        //samples per second
const int g_nDelayTime = 300;           //in milliseconds
const int g_nSecondsToMilliseconds = 1000;
const float g_fPI = 3.14159;
const float g_fDistortionBlend = 0.1f;
const float g_fStartGainDefault = 0.8f;
const float g_fEndGainDefault = 0.8f;

//==============================================================================
WarpedDelayAudioProcessor::WarpedDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

WarpedDelayAudioProcessor::~WarpedDelayAudioProcessor()
{
}

//==============================================================================
const juce::String WarpedDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WarpedDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WarpedDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WarpedDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WarpedDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WarpedDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int WarpedDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WarpedDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WarpedDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void WarpedDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void WarpedDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int numInputChannels = getTotalNumInputChannels();
    const int delayBufferSize = 2 * (sampleRate * samplesPerBlock);

    m_DelayBuffer.setSize(numInputChannels, delayBufferSize);
}

void WarpedDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WarpedDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
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
#endif

void WarpedDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    const int nBufferLength = buffer.getNumSamples();
    const int nDelayBufferLength = m_DelayBuffer.getNumSamples();

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        const float* pBufferData = buffer.getReadPointer(channel);
        float* pDryBuffer = buffer.getWritePointer(channel);
        const float* pDelayBufferData = m_DelayBuffer.getReadPointer(channel);

        FillDelayBuffer(channel, nBufferLength, nDelayBufferLength, pBufferData);
        GetDelayBuffer(buffer, channel, nBufferLength, nDelayBufferLength, pDelayBufferData);
        FeedbackIntoDelayBuffer(channel, nBufferLength, nDelayBufferLength, pDryBuffer);
    }

    m_nBufferWritePosition += nBufferLength;
    m_nBufferWritePosition %= nDelayBufferLength;
}

//==============================================================================
void WarpedDelayAudioProcessor::FillDelayBuffer(int nChannel, const int nBufferLength, const int nDelayBufferLength, const float* pBufferData)
{
    if (nDelayBufferLength > (nBufferLength + m_nBufferWritePosition))
    {
        m_DelayBuffer.copyFromWithRamp(nChannel, m_nBufferWritePosition, pBufferData, nBufferLength, g_fStartGainDefault, g_fEndGainDefault);
    }
    else
    {
        const int nBufferRemaining = nDelayBufferLength - m_nBufferWritePosition;
        m_DelayBuffer.copyFromWithRamp(nChannel, m_nBufferWritePosition, pBufferData, nBufferRemaining, g_fStartGainDefault, g_fEndGainDefault);
        m_DelayBuffer.copyFromWithRamp(nChannel, 0, pBufferData, nBufferLength - nBufferRemaining, g_fStartGainDefault, g_fEndGainDefault);
    }
}

//==============================================================================
void WarpedDelayAudioProcessor::GetDelayBuffer(juce::AudioBuffer<float>& rBuffer, int nChannel, const int nBufferLength, const int nDelayBufferLength, const float* pDelayBufferData)
{
    const int nReadPosition = static_cast<int>(nDelayBufferLength + m_nBufferWritePosition - (g_nSampleRate * g_nDelayTime / g_nSecondsToMilliseconds)) % nDelayBufferLength;

    if (nDelayBufferLength > nBufferLength + nReadPosition)
    {
        rBuffer.copyFrom(nChannel, 0, pDelayBufferData + nReadPosition, nBufferLength);
    }
    else
    {
        const int bufferRemaining = nDelayBufferLength - nReadPosition;
        rBuffer.copyFrom(nChannel, 0, pDelayBufferData + nReadPosition, bufferRemaining);
        rBuffer.copyFrom(nChannel, bufferRemaining, pDelayBufferData, nBufferLength - bufferRemaining);
    }
}

//==============================================================================
void WarpedDelayAudioProcessor::FeedbackIntoDelayBuffer(int nChannel, const int nBufferLength, const int nDelayBufferLength, const float* pDryBuffer)
{
    if (nDelayBufferLength > (nBufferLength + m_nBufferWritePosition))
    {
        m_DelayBuffer.addFromWithRamp(nChannel, m_nBufferWritePosition, pDryBuffer, nBufferLength, g_fStartGainDefault, g_fEndGainDefault);
    }
    else
    {
        const int remainingBuffer = nDelayBufferLength - m_nBufferWritePosition;
        m_DelayBuffer.addFromWithRamp(nChannel, remainingBuffer, pDryBuffer, remainingBuffer, g_fStartGainDefault, g_fEndGainDefault);
        m_DelayBuffer.addFromWithRamp(nChannel, 0, pDryBuffer, (nBufferLength - remainingBuffer), g_fStartGainDefault, g_fEndGainDefault);
    }
}

//==============================================================================
bool WarpedDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* WarpedDelayAudioProcessor::createEditor()
{
    return new WarpedDelayAudioProcessorEditor (*this);
}

//==============================================================================
void WarpedDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void WarpedDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WarpedDelayAudioProcessor();
}
