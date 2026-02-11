/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FFTMetalizerAudioProcessor::FFTMetalizerAudioProcessor()
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
    apvts.addParameterListener("Amount", this);
    apvts.addParameterListener("Low Frequency Cutoff", this);
}

FFTMetalizerAudioProcessor::~FFTMetalizerAudioProcessor()
{
}

//==============================================================================
const juce::String FFTMetalizerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FFTMetalizerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FFTMetalizerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FFTMetalizerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FFTMetalizerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FFTMetalizerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FFTMetalizerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FFTMetalizerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FFTMetalizerAudioProcessor::getProgramName (int index)
{
    return {};
}

void FFTMetalizerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

juce::AudioProcessorParameter* FFTMetalizerAudioProcessor::getBypassParameter() const
{
  return apvts.getParameter("Bypass");
}

juce::AudioProcessorParameter* FFTMetalizerAudioProcessor::getAmountParameter() const
{
  return apvts.getParameter("Amount");
}


//==============================================================================
void FFTMetalizerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setLatencySamples(fft[0].getLatencyInSamples());

    SampleRate = sampleRate;

    fft[0].reset();
    fft[1].reset();
}

void FFTMetalizerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool FFTMetalizerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void FFTMetalizerAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
  DBG(parameterID);
  DBG(newValue);
  if (parameterID == "Amount") {
      fft[0].setScrambleAmount((int)newValue);
      fft[1].setScrambleAmount((int)newValue);
  } else if (parameterID == "Low Frequency Cutoff") {
      fft[0].setLowFreqCutoff((int)newValue, SampleRate);
      fft[1].setLowFreqCutoff((int)newValue, SampleRate);
  }
}

void FFTMetalizerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
  juce::ScopedNoDenormals noDenormals;
  auto numInputChannels = getTotalNumInputChannels();
  auto numOutputChannels = getTotalNumOutputChannels();
  auto numSamples = buffer.getNumSamples();

  for (auto i = numInputChannels; i < numOutputChannels; ++i) {
    buffer.clear(i, 0, numSamples);
  }

  bool bypassed = apvts.getRawParameterValue("Bypass")->load();

  float* channelL = buffer.getWritePointer(0);
  float* channelR = buffer.getWritePointer(1);

  // Processing on a sample-by-sample basis:
  for (int sample = 0; sample < numSamples; ++sample) {
    float sampleL = channelL[sample];
    float sampleR = channelR[sample];

    sampleL = fft[0].processSample(sampleL, bypassed);
    sampleR = fft[1].processSample(sampleR, bypassed);

    channelL[sample] = sampleL;
    channelR[sample] = sampleR;
  }

  /*
  // Processing the entire block at once:
  for (int channel = 0; channel < numInputChannels; ++channel) {
      auto* channelData = buffer.getWritePointer(channel);
      fft[channel].processBlock(channelData, numSamples, bypassed);
  }
  */
}
//==============================================================================
bool FFTMetalizerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}


juce::AudioProcessorEditor* FFTMetalizerAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FFTMetalizerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void FFTMetalizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout FFTMetalizerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("Bypass", 1),
        "Bypass",
        false));
    layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("Amount", 2),
      "Amount",
      0, 10, 5));
    layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("Low Frequency Cutoff", 3),
      "Low Frequency Cutoff",
      0, 20000, 200));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FFTMetalizerAudioProcessor();
}
