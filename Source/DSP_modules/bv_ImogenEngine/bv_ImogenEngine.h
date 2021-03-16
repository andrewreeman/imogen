/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
 ID:                 bv_ImogenEngine
 vendor:             Ben Vining
 version:            0.0.1
 name:               ImogenEngine
 description:        base class that wraps the Harmonizer class into a self-sufficient audio engine
 dependencies:       bv_Harmonizer
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/


#pragma once

#include "bv_Harmonizer/bv_Harmonizer.h"



namespace bav

{
    
using namespace juce;


template<typename SampleType>
class ImogenEngine  :   public bav::dsp::FIFOWrappedEngine<SampleType>
{
    
    using FIFOEngine = bav::dsp::FIFOWrappedEngine<SampleType>;
    
public:
    ImogenEngine();
    
    void killAllMidi();
    
    int reportLatency() const noexcept { return FIFOEngine::getLatency(); }
    
    void updateNumVoices (const int newNumVoices); // updates the # of cuncurrently running instances of the pitch shifting algorithm
    int getCurrentNumVoices() const { return harmonizer.getNumVoices(); }
    
    void returnActivePitches (Array<int>& outputArray) const;
    
    void recieveExternalPitchbend (const int bend);
    
    void updateDryWet     (const int percentWet);
    void updateDryVoxPan  (const int newMidiPan);
    void updateAdsr       (const float attack, const float decay, const float sustain, const float release, const bool isOn);
    void updateStereoWidth(const int newStereoWidth, const int lowestPannedNote);
    void updateMidiVelocitySensitivity(const int newSensitivity);
    void updatePitchbendRange (const int rangeST);
    void updatePedalPitch  (const bool isOn, const int upperThresh, const int interval);
    void updateDescant     (const bool isOn, const int lowerThresh, const int interval);
    void updateConcertPitch(const int newConcertPitchHz);
    void updateNoteStealing(const bool shouldSteal);
    void updateMidiLatch   (const bool isLatched);
    void updateLimiter     (const bool isOn);
    void updateNoiseGate (const float newThreshDB, const bool isOn);
    void updateDeEsser (const float deEssAmount, const float thresh_dB, const bool isOn);
    void updateCompressor (const float threshDB, const float ratio, const bool isOn);
    void updateReverb (const float roomSize, const float damping, const int wetPcnt, const bool isOn);
    void updateInputGain  (const float newInGain);
    void updateOutputGain (const float newOutGain);
    void updateAftertouchGainOnOff (const bool shouldBeOn);
    void updatePitchDetectionHzRange (const int minHz, const int maxHz);
    void updateBypassStates (bool leadIsBypassed, bool harmoniesAreBypassed);
    
    int getModulatorSource() const noexcept { return modulatorInput.load(); }
    void setModulatorSource (const int newSource);
    
    void playChord (const Array<int>& desiredNotes, const float velocity, const bool allowTailOffOfOld);
    
    
private:
    
    // determines how the modulator signal is parsed from the [usually] stereo buffer passed into processBlock
    // 0 - left channel only
    // 1 - right channel only
    // 2 - mix all input channels to mono
    std::atomic<int> modulatorInput;
    
    void renderBlock (const AudioBuffer<SampleType>& input, AudioBuffer<SampleType>& output, MidiBuffer& midiMessages) override;
    
    void initialized (int newInternalBlocksize, double samplerate) override;
    
    void prepareToPlay (double samplerate) override;
    
    void resetTriggered() override;
    
    void latencyChanged (int newInternalBlocksize) override;
    
    void release() override;
    
    Harmonizer<SampleType> harmonizer;
    
    AudioBuffer<SampleType> monoBuffer;  // this buffer is used to store the mono input signal so that input gain can be applied
    AudioBuffer<SampleType> wetBuffer; // this buffer is where the 12 harmony voices' output gets added together
    AudioBuffer<SampleType> dryBuffer; // this buffer is used for panning & delaying the dry signal
    
    juce::dsp::ProcessSpec dspSpec;
    
    bav::dsp::SidechainedNoiseGate<SampleType> gate;
    std::atomic<bool> noiseGateIsOn;
    
    bav::dsp::DeEsser<SampleType> deEsser;
    std::atomic<bool> deEsserIsOn;
    
    juce::dsp::DryWetMixer<SampleType> dryWetMixer;
    
    juce::dsp::IIR::Filter<SampleType> initialHiddenLoCut;
    
    bav::dsp::SidechainedCompressor<SampleType> compressor;
    std::atomic<bool> compressorIsOn;
    
    bav::dsp::Reverb<SampleType> reverb;
    std::atomic<bool> reverbIsOn;
    
    bav::dsp::SidechainedLimiter<SampleType> limiter;
    std::atomic<bool> limiterIsOn;
    std::atomic<float> limiterThresh, limiterRelease;
    
    std::atomic<float> inputGain, outputGain;
    std::atomic<float> prevInputGain, prevOutputGain;
    
    bav::dsp::Panner dryPanner;
    
    std::atomic<bool> leadBypass, harmonyBypass;
    
    CriticalSection lock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImogenEngine)
};


} // namespace
