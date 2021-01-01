#pragma once

#include <JuceHeader.h>

#include "GlobalDefinitions.h"
#include "Harmonizer.h"
#include "InputAnalysis.h"

class ImogenAudioProcessorEditor; // forward declaration...


//==============================================================================
/**
 */
class ImogenAudioProcessor    : public juce::AudioProcessor

{
    
public:
    
    // standard & general-purpose functions ---------------------------------------------------------------------------------------------------------
    ImogenAudioProcessor();
    ~ImogenAudioProcessor() override;
    
    AudioProcessor::BusesProperties makeBusProperties();
    
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
    
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    const juce::String getName() const override { return JucePlugin_Name; }
    
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
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
    void updateSampleRate(const double newSamplerate);
    void updateAdsr();
    void updateIOgains();
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
    void updatePedalPitch();
    void updateDescant();
    
    // misc utility functions -----------------------------------------------------------------------------------------------------------------------
    Array<int> returnActivePitches() const noexcept { return harmonizer.reportActiveNotes(); }
    
    float reportCurrentInputPitch()  const noexcept { return currentInputPitch; }
    
    void killAllMidi();
    
    void updateNumVoices(const int newNumVoices); // updates the # of cuncurrently running instances of the pitch shifting algorithm
    
    AudioProcessorValueTreeState tree;
    
    
    //==============================================================================
    
private:
    
    void processBlockPrivate(AudioBuffer<float>& buffer, const int inputChannel, const int startSample, const int numSamples);
    void renderChunk(AudioBuffer<float>& buffer, const int inputChannel);
    
    AudioProcessorValueTreeState::ParameterLayout createParameters();
    
    void updateAllParameters();
    
    AudioBuffer<float> wetBuffer; // this buffer is where the 12 harmony voices' output gets added together
    AudioBuffer<float> dryBuffer; // this buffer is used for panning & delaying the dry signal
    
    Harmonizer harmonizer;
    
    EpochFinder epochs;
    Array<int> epochIndices;
    
    PitchTracker pitch;
    
    dsp::ProcessSpec dspSpec;
    dsp::Limiter<float> limiter;
    dsp::DryWetMixer<float> dryWetMixer;
    
    // these variables store *current* states:
    double lastSampleRate;
    bool limiterIsOn;
    float inputGainMultiplier, outputGainMultiplier, currentInputPitch;
    
    int dryvoxpanningmults[2]; // stores gain multiplier values, which when applied to the input signal, achieve the desired dry vox panning
    
    // variables to store previous parameter values, to avoid unnecessary update operations:
    int prevDryPan;
    float prevideb, prevodeb;
    
    // listener variables linked to AudioProcessorValueTreeState parameters:
    AudioParameterInt* 	 dryPan             = nullptr;
    AudioParameterInt* 	 dryWet             = nullptr;
    AudioParameterInt* 	 inputChan          = nullptr;
    AudioParameterFloat* adsrAttack         = nullptr;
    AudioParameterFloat* adsrDecay          = nullptr;
    AudioParameterFloat* adsrSustain        = nullptr;
    AudioParameterFloat* adsrRelease        = nullptr;
    AudioParameterBool*  adsrToggle         = nullptr;
    AudioParameterInt*	 quickKillMs        = nullptr;
    AudioParameterInt* 	 quickAttackMs      = nullptr;
    AudioParameterInt*   stereoWidth        = nullptr;
    AudioParameterInt*   lowestPanned       = nullptr;
    AudioParameterInt*   velocitySens       = nullptr;
    AudioParameterInt*   pitchBendUp        = nullptr;
    AudioParameterInt*   pitchBendDown      = nullptr;
    AudioParameterBool*  pedalPitchIsOn     = nullptr;
    AudioParameterInt*   pedalPitchThresh   = nullptr;
    AudioParameterInt*   pedalPitchInterval = nullptr;
    AudioParameterBool*  descantIsOn        = nullptr;
    AudioParameterInt*	 descantThresh      = nullptr;
    AudioParameterInt*	 descantInterval    = nullptr;
    AudioParameterInt*	 concertPitchHz     = nullptr;
    AudioParameterBool*  voiceStealing      = nullptr;
    AudioParameterBool*  latchIsOn          = nullptr;
    AudioParameterFloat* inputGain          = nullptr;
    AudioParameterFloat* outputGain	        = nullptr;
    AudioParameterBool*	 limiterToggle      = nullptr;
    AudioParameterFloat* limiterThresh      = nullptr;
    AudioParameterInt*   limiterRelease     = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImogenAudioProcessor)
};



