/*
  ==============================================================================

    HarmonyVoice.h
    Created: 2 Nov 2020 7:35:03am
    Author:  Ben Vining

 
 	This class defines one instance of the harmony algorithm.
 
  ==============================================================================
*/

#pragma once

#include "shifter.h"


class HarmonyVoice {
	
	// STiLL NEED TO DEAL WITH:
	// pitch wheel / pitch bend
	
public:
		
	bool voiceIsOn;
	
	HarmonyVoice(const int thisVoiceNumber): voiceIsOn(false), pitchBendRangeUp(2), pitchBendRangeDown(2), thisVoiceNumber(thisVoiceNumber), prevPan(-1), panningMultR(0.5), panningMultL(0.5)
	{
		shiftedBuffer.setSize(1, 512);
		harmonyBuffer.setSize(2, 512);
	};
	
	
	void startNote (const int midiPitch, const int velocity, const int midiPan, const int lastPitchBend)
	{
		lastNoteRecieved = midiPitch;
		desiredFrequency = mtof(returnMidiFloat(lastPitchBend));
		amplitudeMultiplier = calcVelocityMultiplier(velocity);
		voiceIsOn = true;
		adsrEnv.noteOn();
	};
	
	
	void stopNote () {
		adsrEnv.noteOff();
		// voiceIsOn is set to FALSE in the renderNextBlock function so that the bool changes only after the ADSR has actually reached 0.
	};
	
	
	void changeNote (const int midiPitch, const int velocity, const int midiPan, const int lastPitchBend) {  // run this function to change the assigned midi pitch without retriggering the ADSR
		lastNoteRecieved = midiPitch;
		desiredFrequency = mtof(returnMidiFloat(lastPitchBend));
		amplitudeMultiplier = calcVelocityMultiplier(velocity);
		voiceIsOn = true;
	};
	
	
	void updateDSPsettings(const double newSampleRate, const int newBlockSize) {
		adsrEnv.setSampleRate(newSampleRate);
		pitchShifter.updateDSPsettings(newSampleRate, newBlockSize);  // passes settings thru to shifter instance 
	};
	
	
	void adsrSettingsListener(float* adsrAttackTime, float* adsrDecayTime, float* adsrSustainRatio, float* adsrReleaseTime, float* midiVelocitySensListener) {
		// attack/decay/release time in SECONDS; sustain ratio 0.0 - 1.0
		adsrParams.attack = *adsrAttackTime;
		adsrParams.decay = *adsrDecayTime;
		adsrParams.sustain = *adsrSustainRatio;
		adsrParams.release = *adsrReleaseTime;
		adsrEnv.setParameters(adsrParams);
		
		midiVelocitySensitivity = (float)(*midiVelocitySensListener / 100);
	};
	
	
	void pitchBendSettingsListener(float* rangeUp, float* rangeDown) {
		pitchBendRangeUp = *rangeUp;
		pitchBendRangeDown = *rangeDown;
	};
	
	
	void checkBufferSizes(const int newNumSamples) {
		if(shiftedBuffer.getNumSamples() != newNumSamples) {
			shiftedBuffer.setSize(1, newNumSamples);
		}
		if(harmonyBuffer.getNumSamples() != newNumSamples) {
			harmonyBuffer.setSize(2, newNumSamples);
		}
		pitchShifter.checkBuffer(newNumSamples);
	};
	
	void clearBuffers() {
		shiftedBuffer.clear();
		harmonyBuffer.clear();
	};
	
	
	void renderNextBlock (AudioBuffer <float>& inputBuffer, const float* readPointer, const int numSamples, const int inputChannel, const double modInputFreq, const int analysisShift, const int analysisShiftHalved) {
		// this function needs to write shifted samples to the stereo harmonyBuffer
		
		checkBufferSizes(numSamples);
		
		// this function puts shifted samples into the mono shiftedBuffer
		pitchShifter.doTheShifting(inputBuffer, inputChannel, shiftedBuffer, numSamples, modInputFreq, desiredFrequency, analysisShift, analysisShiftHalved);
		
		// transfer samples into the stereo harmonyBuffer, which is where the processBlock will grab them from
		const float* shiftedReader = shiftedBuffer.getReadPointer(0);
		for(int sample = 0; sample < numSamples; ++sample) {

			if(adsrEnv.isActive() == false) {
				voiceIsOn = false;
			} else {
				const float envelopedShiftedSignal = shiftedReader[sample] * amplitudeMultiplier * adsrEnv.getNextSample();

				for (int channel = 0; channel < 2; ++channel) {  // put into STEREO BUFFER
					harmonyBuffer.getWritePointer(channel)[sample] = envelopedShiftedSignal * panningMultipliers[channel];
				}
			}
		}
		
	};
	
	
	void changePanning(const int newPanVal) {   // this function updates the voice's panning if it is active when the stereo width setting is changed
												// TODO: ramp this value???
		midiPan = newPanVal;
		prevPan = newPanVal;
		calculatePanningChannelMultipliers(newPanVal);
	};
	
	
	void pitchBend(const int pitchBend) {
		if (pitchBend > 64) {
			desiredFrequency = mtof(((pitchBendRangeUp * (pitchBend - 65)) / 62) + lastNoteRecieved);
		} else if (pitchBend < 64) {
			desiredFrequency = mtof((((1 - pitchBendRangeDown) * pitchBend) / 63) + lastNoteRecieved - pitchBendRangeDown);
		} else if (pitchBend == 64) {
			desiredFrequency = mtof(lastNoteRecieved);
		}
	};
	
	
	AudioBuffer<float> harmonyBuffer; // this buffer stores the harmony voice's actual output. STEREO BUFFER [step 2]
	
	
	ADSR adsrEnv;
	ADSR::Parameters adsrParams;
	
	
private:
	
	AudioBuffer<float> shiftedBuffer; // this audio buffer will store the shifted signal. MONO BUFFER [step 1] - only use channel 0
	
	int pitchBendRangeUp;
	int pitchBendRangeDown;
	
	const int thisVoiceNumber;
	
	int midiPan;
	int prevPan;
	int panning;
	float panningMultR;
	float panningMultL;
	float panningMultipliers[2];
	
	float midiVelocitySensitivity;  
	
	double desiredFrequency;
	int lastNoteRecieved;
	
	float amplitudeMultiplier;
	
	Shifter pitchShifter;
	
	void calculatePanningChannelMultipliers(const int midipanning) {
		panningMultR = midipanning / 127.0;
		panningMultL = 1.0 - panningMultR;
		panningMultipliers[0] = panningMultL;
		panningMultipliers[1] = panningMultR;
	};
	
	
	float returnMidiFloat(const int bend) const {
		if (bend > 64) {
			return ((pitchBendRangeUp * (bend - 65)) / 62) + lastNoteRecieved;
		} else if (bend < 64) {
			return (((1 - pitchBendRangeDown) * bend) / 63) + lastNoteRecieved - pitchBendRangeDown;
		} else {
			return lastNoteRecieved;
		}
	};
	
	
	double mtof(const float midiNote) const {  // converts midiPitch to frequency in Hz
		return 440.0 * std::pow(2.0, ((midiNote - 69.0) / 12.0));
	};
	
	
	float calcVelocityMultiplier(const int midiVelocity) const {
		const float initialMutiplier = midiVelocity / 127.0; // what the multiplier would be without any sensitivity calculations...
		return ((1 - initialMutiplier) * (1 - midiVelocitySensitivity) + initialMutiplier);
	};
};


