/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "DSP/Saturation.hpp"

//==============================================================================
DisperseAudioProcessor::DisperseAudioProcessor()
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
  addParameter (mix = new juce::AudioParameterFloat (MIX_PARAM_ID, "Mix", 0.0, 1.0, 0.30));
  addParameter (time = new juce::AudioParameterFloat (TIME_PARAM_ID, "Delay Time (ms)", 50.0, MAX_DELAY_TIME - MAX_TIME_SPREAD, 600.0));
  addParameter (feedback = new juce::AudioParameterFloat (FEEDBACK_PARAM_ID, "Feedback (%)", 0.0, 1.0, 0.5));
  addParameter (spread = new juce::AudioParameterFloat (SPREAD_PARAM_ID, "Stereo Spread", 0.0, 1.0, 0.2));
  addParameter (dispersion = new juce::AudioParameterFloat (DISPERSION_PARAM_ID, "Dispersion", 0.0, 1.0, 0.2));
  addParameter (numVoices = new juce::AudioParameterInt (NUM_VOICES_PARAM_ID, "Number of Delay Voices", 0, 8, 4));
  addParameter (seed = new juce::AudioParameterInt (SEED_PARAM_ID, "Random Seed", 1, 100000, 1234));
  
  this->mix->addListener(this);
  this->time->addListener(this);
  this->feedback->addListener(this);
  this->spread->addListener(this);
  this->dispersion->addListener(this);
  this->numVoices->addListener(this);
  this->seed->addListener(this);
}

DisperseAudioProcessor::~DisperseAudioProcessor()
{
}

//==============================================================================
const juce::String DisperseAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DisperseAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DisperseAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DisperseAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DisperseAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DisperseAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DisperseAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DisperseAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DisperseAudioProcessor::getProgramName (int index)
{
    return {};
}

void DisperseAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DisperseAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
  
  this->sampleRate = sampleRate;
  this->inputBuffer = juce::AudioBuffer<float>(2, samplesPerBlock);
    
  std::cout << "Running tests!" << std::endl;
  juce::UnitTestRunner testRunner;
  testRunner.runAllTests();
  
  this->effect.initialize(sampleRate, 1234);
  
  auto parameters = this->getParameters();
  for (int i = 0; i < parameters.size(); i ++) {
    auto parameter = parameters[i];
    this->parameterValueChanged(parameter->getParameterIndex(), parameter->getValue());
  }
}

void DisperseAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DisperseAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
#endif

void DisperseAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
  
    this->inputBuffer.makeCopyOf(buffer, true);
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    stereofloat input;
    stereofloat output;
    for (int i = 0; i < buffer.getNumSamples(); i++) {
      input.L = buffer.getSample(0, i);
      input.R = buffer.getSample(1, i);
      
      output = this->effect.process(input);
            
      buffer.setSample(0, i, output.L);
      buffer.setSample(1, i, output.R);
    }

}

//==============================================================================
bool DisperseAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DisperseAudioProcessor::createEditor()
{
    return new DisperseAudioProcessorEditor (*this);
}

//==============================================================================
void DisperseAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DisperseAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DisperseAudioProcessor();
}


void DisperseAudioProcessor::parameterValueChanged (int parameterIndex, float val) {

  if (parameterIndex == this->time->getParameterIndex()) {
    this->effect.setTimeMs(this->time->get());

  } else if (parameterIndex == this->mix->getParameterIndex()) {
    this->effect.setMix(this->mix->get());

  } else if (parameterIndex == this->feedback->getParameterIndex()) {
    this->effect.setFeedback(this->feedback->get());

  } else if (parameterIndex == this->spread->getParameterIndex()) {
    this->effect.setSpread(this->spread->get());

  } else if (parameterIndex == this->dispersion->getParameterIndex()) {
    this->effect.setDispersion(this->dispersion->get());

  } else if (this->numVoices->getParameterIndex()) {
    this->effect.setVoiceArrangement(std::vector<int>(this->numVoices->get(), 1));

  } else if (this->seed->getParameterIndex()) {
    this->effect.setRandomSeed(this->seed->get());

  }
}

void DisperseAudioProcessor::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)  {
  
}
