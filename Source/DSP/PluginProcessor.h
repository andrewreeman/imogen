#pragma once

#include <JuceHeader.h>

#include "DelayBuffer.h"
#include "FancyMidiBuffer.h"
#include "Panner.h"
#include "Harmonizer.h"
#include "PitchDetector.h"

class ImogenAudioProcessorEditor; // forward declaration...

class ImogenAudioProcessor;



template<typename SampleType>
class ImogenEngine
{
public:
    ImogenEngine(ImogenAudioProcessor& p);
    
    ~ImogenEngine();
    
    void changeBlocksize (const int newBlocksize);
    
    void process (AudioBuffer<SampleType>& inBus, AudioBuffer<SampleType>& output, MidiBuffer& midiMessages,
                  const bool applyFadeIn = false, const bool applyFadeOut = false);
    
    void processBypassed (AudioBuffer<SampleType>& inBus, AudioBuffer<SampleType>& output);
    
    void initialize (const double initSamplerate, const int initSamplesPerBlock, const int initNumVoices);
    
    void prepare (double sampleRate, int samplesPerBlock);
    
    void reset();
    
    void releaseResources();
    
    int reportLatency() const noexcept { return internalBlocksize; }
    
    void updateNumVoices(const int newNumVoices); // updates the # of cuncurrently running instances of the pitch shifting algorithm
    
    void returnActivePitches(Array<int>& outputArray) const { return harmonizer.reportActiveNotes(outputArray); }
    
    void updateSamplerate (const int newSamplerate);
    void updateDryWet     (const float newWetMixProportion);
    void updateDryVoxPan  (const int newMidiPan);
    void updateAdsr       (const float attack, const float decay, const float sustain, const float release, const bool isOn);
    void updateQuickKill  (const int newMs);
    void updateQuickAttack(const int newMs);
    void updateStereoWidth(const int newStereoWidth, const int lowestPannedNote);
    void updateMidiVelocitySensitivity(const int newSensitivity);
    void updatePitchbendSettings(const int rangeUp, const int rangeDown);
    void updatePedalPitch  (const bool isOn, const int upperThresh, const int interval);
    void updateDescant     (const bool isOn, const int lowerThresh, const int interval);
    void updateConcertPitch(const int newConcertPitchHz);
    void updateNoteStealing(const bool shouldSteal);
    void updateMidiLatch   (const bool isLatched);
    void updateIntervalLock(const bool isLocked);
    void updateLimiter     (const float thresh, const float release, const bool isOn);
    void updateInputGain  (const float newInGain);
    void updateOutputGain (const float newOutGain);
    void updateDryGain (const float newDryGain);
    void updateWetGain (const float newWetGain);
    void updateSoftPedalGain (const float newGain);
    void updatePitchDetectionHzRange (const int minHz, const int maxHz);
    void updatePitchDetectionConfidenceThresh (const float newThresh);
    
    void clearBuffers();
    
    bool hasBeenReleased()    const noexcept { return resourcesReleased; }
    bool hasBeenInitialized() const noexcept { return initialized; }
    
    
private:
    
    int internalBlocksize; // the size of the processing blocks, in samples, that the algorithm will be processing at a time. This corresponds to the latency of the pitch detector, and, thus, the minimum possible Hz it can detect.
    
    void processWrapped (AudioBuffer<SampleType>& inBus, AudioBuffer<SampleType>& output,
                         MidiBuffer& midiMessages,
                         const bool applyFadeIn, const bool applyFadeOut);
    
    void processBypassedWrapped (AudioBuffer<SampleType>& inBus, AudioBuffer<SampleType>& output);
    
    
    void renderBlock (const AudioBuffer<SampleType>& input, MidiBuffer& midiMessages);
    
    ImogenAudioProcessor& processor;
    
    Harmonizer<SampleType> harmonizer;
    
    DelayBuffer<SampleType> inputBuffer;
    DelayBuffer<SampleType> outputBuffer;
    
    AudioBuffer<SampleType> inBuffer;  // this buffer is used to store the mono input signal so that input gain can be applied
    AudioBuffer<SampleType> wetBuffer; // this buffer is where the 12 harmony voices' output gets added together
    AudioBuffer<SampleType> dryBuffer; // this buffer is used for panning & delaying the dry signal
    
    dsp::ProcessSpec dspSpec;
    dsp::Limiter <SampleType> limiter;
    dsp::DryWetMixer<SampleType> dryWetMixer;
    bool limiterIsOn;
    
    bool resourcesReleased;
    bool initialized;
    
    float dryGain, prevDryGain;
    float wetGain, prevWetGain;
    
    float inputGain, prevInputGain;
    float outputGain, prevOutputGain;
    
    MidiBuffer midiChoppingBuffer;
    
    FancyMidiBuffer midiInputCollection;
    FancyMidiBuffer midiOutputCollection;
    FancyMidiBuffer chunkMidiBuffer;
    
    
    void copyRangeOfMidiBuffer (const MidiBuffer& readingBuffer, MidiBuffer& destBuffer,
                                const int startSampleOfInput,
                                const int startSampleOfOutput,
                                const int numSamples);
    

    void pushUpLeftoverSamples (AudioBuffer<SampleType>& targetBuffer,
                                const int numSamplesUsed,
                                const int numSamplesLeft);
    
    Panner dryPanner;
    
    PitchDetector<SampleType> pitchDetector;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImogenEngine)
};




//==============================================================================
/**
 */
class ImogenAudioProcessor    : public juce::AudioProcessor

{
    
public:
    
    // standard & general-purpose functions ---------------------------------------------------------------------------------------------------------
    ImogenAudioProcessor();
    ~ImogenAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    
    void releaseResources() override;
    
    void reset() override;
    
    
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    
    void processBlock (AudioBuffer<double>& buffer, MidiBuffer& midiMessages) override;
    
    void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    
    void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages) override;
    
    
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    const juce::String getName() const override { return JucePlugin_Name; }
    
    bool  acceptsMidi()  const override { return true;  }
    bool  producesMidi() const override { return true;  }
    bool  supportsMPE()  const override { return false; }
    bool  isMidiEffect() const override { return false; }
    
    double getTailLengthSeconds() const override;
    
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // functions for custom preset management system ------------------------------------------------------------------------------------------------
    void savePreset  (juce::String presetName);
    void loadPreset  (juce::String presetName);
    void deletePreset(juce::String presetName);
    juce::File getPresetsFolder() const;
    
    // functions to update parameters ---------------------------------------------------------------------------------------------------------------
    void updateAdsr();
    void updateIOgains();
    void updateDryWetGains();
    void updateLimiter();
    void updateStereoWidth();
    void updateQuickKillMs();
    void updateQuickAttackMs();
    void updateDryVoxPan();
    void updateMidiVelocitySensitivity();
    void updateNoteStealing();
    void updatePitchbendSettings();
    void updateDryWet();
    void updateConcertPitch();
    void updateMidiLatch();
    void updateIntervalLock();
    void updatePedalPitch();
    void updateDescant();
    void updatePitchDetectionSettings();
    
    // misc utility functions -----------------------------------------------------------------------------------------------------------------------
    
    void returnActivePitches(Array<int>& outputArray) const;
    
    void killAllMidi();
    
    AudioProcessorValueTreeState tree;
    
    enum ModulatorInputSource { left, right, mixToMono }; // determines how the plugin will take input from the stereo buffer fed to it from the host
    
    void changeModulatorInputSource(ModulatorInputSource newSource) noexcept { modulatorInput = newSource; }
    
    bool canAddBus(bool isInput) const override;
    
    bool shouldWarnUserToEnableSidechain() const;
    
    bool supportsDoublePrecisionProcessing() const override { return true; }
    
    void updateTrackProperties (const TrackProperties& properties) override; // informs the plugin about the properties of the DAW mixer track it's loaded on
    
    void updateNumVoices(const int newNumVoices);
    
    //==============================================================================
    
    ModulatorInputSource getModulatorSource() const noexcept { return modulatorInput; }
    
    template<typename SampleType>
    void updateAllParameters(ImogenEngine<SampleType>& activeEngine);
    
    // listener variables linked to AudioProcessorValueTreeState parameters:
    AudioParameterInt*   dryPan             = nullptr;
    AudioParameterInt*   dryWet             = nullptr;
    AudioParameterInt*   inputChan          = nullptr;
    AudioParameterFloat* adsrAttack         = nullptr;
    AudioParameterFloat* adsrDecay          = nullptr;
    AudioParameterFloat* adsrSustain        = nullptr;
    AudioParameterFloat* adsrRelease        = nullptr;
    AudioParameterBool*  adsrToggle         = nullptr;
    AudioParameterInt*   quickKillMs        = nullptr;
    AudioParameterInt*   quickAttackMs      = nullptr;
    AudioParameterInt*   stereoWidth        = nullptr;
    AudioParameterInt*   lowestPanned       = nullptr;
    AudioParameterInt*   velocitySens       = nullptr;
    AudioParameterInt*   pitchBendUp        = nullptr;
    AudioParameterInt*   pitchBendDown      = nullptr;
    AudioParameterBool*  pedalPitchIsOn     = nullptr;
    AudioParameterInt*   pedalPitchThresh   = nullptr;
    AudioParameterInt*   pedalPitchInterval = nullptr;
    AudioParameterBool*  descantIsOn        = nullptr;
    AudioParameterInt*   descantThresh      = nullptr;
    AudioParameterInt*   descantInterval    = nullptr;
    AudioParameterInt*   concertPitchHz     = nullptr;
    AudioParameterBool*  voiceStealing      = nullptr;
    AudioParameterBool*  latchIsOn          = nullptr;
    AudioParameterBool*  intervalLockIsOn   = nullptr;
    AudioParameterFloat* inputGain          = nullptr;
    AudioParameterFloat* outputGain         = nullptr;
    AudioParameterBool*  limiterToggle      = nullptr;
    AudioParameterFloat* limiterThresh      = nullptr;
    AudioParameterInt*   limiterRelease     = nullptr;
    AudioParameterFloat* dryGain            = nullptr;
    AudioParameterFloat* wetGain            = nullptr;
    AudioParameterFloat* softPedalGain      = nullptr;
    AudioParameterInt*   minDetectedHz      = nullptr;
    AudioParameterInt*   maxDetectedHz      = nullptr;
    AudioParameterFloat* confidenceThresh   = nullptr;
    
    
private:
    
    template <typename SampleType>
    void initialize (ImogenEngine<SampleType>& activeEngine);
    
    template <typename SampleType>
    void processBlockWrapped (AudioBuffer<SampleType>& buffer, MidiBuffer& midiMessages, ImogenEngine<SampleType>& engine);
    
    template <typename SampleType>
    void processBlockBypassedWrapped (AudioBuffer<SampleType>& buffer, MidiBuffer& midiMessages, ImogenEngine<SampleType>& engine);
    
    template <typename SampleType1, typename SampleType2>
    void prepareToPlayWrapped (const double sampleRate, const int samplesPerBlock,
                               ImogenEngine<SampleType1>& activeEngine,
                               ImogenEngine<SampleType2>& idleEngine);
    
    
    
    ImogenEngine<float>  floatEngine;
    ImogenEngine<double> doubleEngine;
    
    ModulatorInputSource modulatorInput; // determines how the modulator signal is parsed from the [usually] stereo buffer passed into processBlock
    
    int latencySamples;
    
    PluginHostType host;
    
    AudioProcessorValueTreeState::ParameterLayout createParameters() const;
    
    AudioProcessor::BusesProperties makeBusProperties() const;
    
    bool wasBypassedLastCallback; // used to activate a fade out instead of an instant kill when the bypass is activated
    
    template <typename SampleType>
    void updatePitchDetectionWrapped (ImogenEngine<SampleType>& activeEngine);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImogenAudioProcessor)
};
