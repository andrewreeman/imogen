/*
    This file defines Imogen's internal audio processor as a whole, when Imogen is built as a plugin target
    Parent file: PluginProcessor.h
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

#ifdef imogen_DEFAULT_NUM_VOICES
  #error imogen_DEFAULT_NUM_VOICES symbol already defined elsewhere!
#endif

#define imogen_DEFAULT_NUM_VOICES 12


ImogenAudioProcessor::ImogenAudioProcessor():
    AudioProcessor(makeBusProperties()),
    tree(*this, nullptr, "IMOGEN_PARAMETERS", createParameters()),
    prevRangeTypeIndex(0)
{
    initializeParameterPointers();
    updateParameterDefaults();
    
    parameterDefaultsAreDirty.store (false);
    
    if (isUsingDoublePrecision())
        initialize (doubleEngine);
    else
        initialize (floatEngine);
    
    latchIsOn.store(false);
    intervalLockIsOn.store(false);
}

ImogenAudioProcessor::~ImogenAudioProcessor()
{ }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SampleType>
inline void ImogenAudioProcessor::initialize (bav::ImogenEngine<SampleType>& activeEngine)
{
    double initSamplerate = getSampleRate();
    if (initSamplerate <= 0) initSamplerate = 44100.0;
    
    int initBlockSize = getBlockSize();
    if (initBlockSize <= 0) initBlockSize = 512;
    
    activeEngine.initialize (initSamplerate, initBlockSize, imogen_DEFAULT_NUM_VOICES);
    
    updateAllParameters (activeEngine);
    
    setLatencySamples (activeEngine.reportLatency());
}

#undef imogen_DEFAULT_NUM_VOICES


void ImogenAudioProcessor::prepareToPlay (const double sampleRate, const int samplesPerBlock)
{
    if (isUsingDoublePrecision())
        prepareToPlayWrapped (sampleRate, samplesPerBlock, doubleEngine, floatEngine);
    else
        prepareToPlayWrapped (sampleRate, samplesPerBlock, floatEngine,  doubleEngine);
}


template <typename SampleType1, typename SampleType2>
inline void ImogenAudioProcessor::prepareToPlayWrapped (const double sampleRate, const int samplesPerBlock,
                                                 bav::ImogenEngine<SampleType1>& activeEngine,
                                                 bav::ImogenEngine<SampleType2>& idleEngine)
{
    if (! idleEngine.hasBeenReleased())
        idleEngine.releaseResources();
    
    if (! activeEngine.hasBeenInitialized())
        activeEngine.initialize (sampleRate, samplesPerBlock, 12);
    else
        activeEngine.prepare (sampleRate, samplesPerBlock);
    
    updateAllParameters (activeEngine);
    
    setLatencySamples (activeEngine.reportLatency());
    
#if ! IMOGEN_ONLY_BUILDING_STANDALONE
    needsSidechain = (host.isLogic() || host.isGarageBand());
#endif
}


void ImogenAudioProcessor::releaseResources()
{
    if (! doubleEngine.hasBeenReleased())
        doubleEngine.releaseResources();
    
    if (! floatEngine.hasBeenReleased())
        floatEngine.releaseResources();
}


void ImogenAudioProcessor::reset()
{
    if (isUsingDoublePrecision())
        doubleEngine.reset();
    else
        floatEngine.reset();
}


void ImogenAudioProcessor::killAllMidi()
{
    if (isUsingDoublePrecision())
        doubleEngine.killAllMidi();
    else
        floatEngine.killAllMidi();
}


void ImogenAudioProcessor::pitchbendFromEditor (const int pitchbend)
{
    if (isUsingDoublePrecision())
        doubleEngine.recieveExternalPitchbend (pitchbend);
    else
        floatEngine.recieveExternalPitchbend (pitchbend);
}


void ImogenAudioProcessor::setMidiLatch (const bool isOn)
{
    latchIsOn.store (isOn);
    
    if (isUsingDoublePrecision())
        doubleEngine.updateMidiLatch (isOn);
    else
        floatEngine.updateMidiLatch (isOn);
}


void ImogenAudioProcessor::setIntervalLock (const bool isOn)
{
    intervalLockIsOn.store (isOn);
    
    if (isUsingDoublePrecision())
        doubleEngine.updateIntervalLock (isOn);
    else
        floatEngine.updateIntervalLock (isOn);
}


/*
 These four functions represent the top-level callbacks made by the host during audio processing. Audio samples may be sent to us as float or double values; both of these functions redirect to the templated processBlockWrapped() function below.
 The buffers sent to this function by the host may be variable in size, so I have coded defensively around several edge cases & possible buggy host behavior and created several layers of checks that each callback passes through before individual chunks of audio are actually rendered.
 In this first layer, we just check that the host has initialzed the processor with the correct processing precision mode...
*/

void ImogenAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (isUsingDoublePrecision()) // this would be a REALLY stupid host, butttt ¯\_(ツ)_/¯
        return;
    
    processBlockWrapped (buffer, midiMessages, floatEngine, isBypassed->get());
}


void ImogenAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    if (! isUsingDoublePrecision())
        return;
    
    processBlockWrapped (buffer, midiMessages, doubleEngine, isBypassed->get());
}


void ImogenAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (isUsingDoublePrecision())
        return;
    
    processBlockWrapped (buffer, midiMessages, floatEngine, true);
    
    isBypassed->setValueNotifyingHost (1.0f);
}


void ImogenAudioProcessor::processBlockBypassed (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    if (! isUsingDoublePrecision())
        return;
    
    processBlockWrapped (buffer, midiMessages, doubleEngine, true);
    
    isBypassed->setValueNotifyingHost (1.0f);
}


// LAYER 2 ---------------------------------------------------------------------------------

template <typename SampleType>
inline void ImogenAudioProcessor::processBlockWrapped (juce::AudioBuffer<SampleType>& buffer,
                                                       juce::MidiBuffer& midiMessages,
                                                       bav::ImogenEngine<SampleType>& engine,
                                                       const bool isBypassedNow)
{
    if (! engine.hasBeenInitialized())
        return;
    
    jassert (! engine.hasBeenReleased());
    
#if ! IMOGEN_ONLY_BUILDING_STANDALONE
    if (needsSidechain && (getBusesLayout().getChannelSet(true, 1) == juce::AudioChannelSet::disabled()))
        return;   // our audio input is disabled! can't do processing
#endif
    
    updateAllParameters (engine); // the host might use a 0-sample long audio buffer to tell us to update our state with new automation values, which is why the check for that is AFTER this call.
    
    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return;
    
    juce::AudioBuffer<SampleType> outBus = AudioProcessor::getBusBuffer (buffer, false, 0);
    
#if IMOGEN_ONLY_BUILDING_STANDALONE
    juce::AudioBuffer<SampleType> inBus = AudioProcessor::getBusBuffer (buffer, true, 0);
#else
    juce::AudioBuffer<SampleType> inBus = AudioProcessor::getBusBuffer (buffer, true, needsSidechain);
#endif
    
    engine.process (inBus, outBus, midiMessages, isBypassedNow);
}


/*===========================================================================================================================
 ============================================================================================================================*/


void ImogenAudioProcessor::returnActivePitches (juce::Array<int>& outputArray) const
{
    if (isUsingDoublePrecision())
        doubleEngine.returnActivePitches (outputArray);
    else
        floatEngine.returnActivePitches (outputArray);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// standard and general-purpose functions -----------------------------------------------------------------------------------------------------------

double ImogenAudioProcessor::getTailLengthSeconds() const
{
    if (adsrToggle->get())
        return double(adsrRelease->get()); // ADSR release time in seconds
    
    return double(quickKillMs->get() * 1000.0f); // "quick kill" time in seconds
}

int ImogenAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs, so this should be at least 1, even if you're not really implementing programs.
}

int ImogenAudioProcessor::getCurrentProgram()
{
    return 1;
}

void ImogenAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String ImogenAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ImogenAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    ignoreUnused (index, newName);
}


inline juce::AudioProcessor::BusesProperties ImogenAudioProcessor::makeBusProperties() const
{
#if ! IMOGEN_ONLY_BUILDING_STANDALONE
    if (host.isLogic() || host.isGarageBand())
        return BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withInput ("Sidechain", juce::AudioChannelSet::mono(),   true)
                                .withOutput("Output",    juce::AudioChannelSet::stereo(), true);
#endif

    return BusesProperties().withInput ("Input",     juce::AudioChannelSet::stereo(), true)
                            .withOutput("Output",    juce::AudioChannelSet::stereo(), true);
}


bool ImogenAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled())
    {
#if IMOGEN_ONLY_BUILDING_STANDALONE
        return false;
#else
        if (needsSidechain)
        {
            if (layouts.getChannelSet (true, 1) == juce::AudioChannelSet::disabled())
                return false;
        }
        else
        {
            return false;
        }
#endif
    }
    
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}


bool ImogenAudioProcessor::canAddBus (bool isInput) const
{
#if ! IMOGEN_ONLY_BUILDING_STANDALONE
    if (needsSidechain)
        return isInput;
#else
    juce::ignoreUnused (isInput);
#endif
    
    return false;
}


#if ! IMOGEN_ONLY_BUILDING_STANDALONE
bool ImogenAudioProcessor::shouldWarnUserToEnableSidechain() const
{
    return needsSidechain && (getBusesLayout().getChannelSet(true, 1) == juce::AudioChannelSet::disabled());
}
#endif


juce::AudioProcessorEditor* ImogenAudioProcessor::createEditor()
{
    return new ImogenAudioProcessorEditor(*this);
}


// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImogenAudioProcessor();
}
