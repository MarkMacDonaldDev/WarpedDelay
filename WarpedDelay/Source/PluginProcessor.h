/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class WarpedDelayAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    WarpedDelayAudioProcessor();
    ~WarpedDelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    juce::AudioBuffer<float> m_DelayBuffer;
    int m_nBufferWritePosition = 0;
    bool m_bReverseStateToggle = false;

    //==============================================================================
    void FillDelayBuffer(int nChannel, const int nBufferLength, const int nDelayBufferLength, const float* pBufferData);
    void GetDelayBuffer(juce::AudioBuffer<float>& rBuffer, int nChannel, const int nBufferLength, const int nDelayBufferLength, const float* pDelayBufferData);
    void FeedbackIntoDelayBuffer(int nChannel, const int nBufferLength, const int nDelayBufferLength, const float* pDryBuffer);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarpedDelayAudioProcessor)
};
