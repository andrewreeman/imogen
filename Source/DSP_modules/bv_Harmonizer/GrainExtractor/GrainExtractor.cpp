/*
    Part of module: bv_Harmonizer
    Direct parent file: GrainExtractor.h
    Classes: GrainExtractor
 */


#include "bv_Harmonizer/GrainExtractor/GrainExtractor.h"


#undef bvhge_NUM_PEAKS_TO_TEST
#undef bvhge_DEFAULT_FINAL_HANDFUL_SIZE
#undef bvhge_MIN_DELTA_RANGE



namespace bav

{
    
    
template<typename SampleType>
GrainExtractor<SampleType>::GrainExtractor()
{ }


template<typename SampleType>
GrainExtractor<SampleType>::~GrainExtractor()
{ }
    
    
template<typename SampleType>
void GrainExtractor<SampleType>::releaseResources()
{
    peakIndices.clear();
    peakCandidates.clear();
    peakSearchingOrder.clear();
    candidateDeltas.clear();
    finalHandful.clear();
    finalHandfulDeltas.clear();
}
    
    
template<typename SampleType>
void GrainExtractor<SampleType>::prepare (const int maxBlocksize)
{
    // maxBlocksize = max period of input audio
    
    jassert (maxBlocksize > 0);
    
    peakIndices.ensureStorageAllocated (maxBlocksize);
    
#define bvhge_NUM_PEAKS_TO_TEST 10
    peakCandidates.ensureStorageAllocated (bvhge_NUM_PEAKS_TO_TEST);
    peakCandidates.clearQuick();
    peakSearchingOrder.ensureStorageAllocated(maxBlocksize);
    peakSearchingOrder.clearQuick();
    candidateDeltas.ensureStorageAllocated (bvhge_NUM_PEAKS_TO_TEST);
    candidateDeltas.clearQuick();
    finalHandful.ensureStorageAllocated (bvhge_NUM_PEAKS_TO_TEST);
    finalHandful.clearQuick();
    finalHandfulDeltas.ensureStorageAllocated (bvhge_NUM_PEAKS_TO_TEST);
    finalHandfulDeltas.clearQuick();
}


template<typename SampleType>
void GrainExtractor<SampleType>::getGrainOnsetIndices (Array<int>& targetArray,
                                                       const AudioBuffer<SampleType>& inputAudio,
                                                       const int period)
{
    targetArray.clearQuick();
    
    const int totalNumSamples = inputAudio.getNumSamples();
    const SampleType* reading = inputAudio.getReadPointer(0);
    
    // identify  peak indices for each pitch period & places them in the peakIndices array
    
    findPsolaPeaks (peakIndices, reading, totalNumSamples, period);
    
    jassert (! peakIndices.isEmpty());
    
    const int grainLength = period * 2;
    const int numSamples = inputAudio.getNumSamples();
    const int halfPeriod = roundToInt (period / 2.0f);
    
    // create array of grain start indices, such that grains are 2 pitch periods long, CENTERED on points of synchronicity previously identified
    
    for (int i = 0; i < peakIndices.size(); ++i)
    {
        const int peakIndex = peakIndices.getUnchecked(i);
        
        int grainStart = peakIndex - period; // offset the peak index by the period so that the peak index will be in the center of the grain (if grain is 2 periods long)
        
        if (grainStart < 0)
        {
            if (i < peakIndices.size() - 2 || targetArray.size() > 1)
                continue;
            
            while (grainStart < 0)
                grainStart += halfPeriod;
        }
        
        if (grainStart + grainLength > numSamples)
        {
            if (i < peakIndices.size() - 2 || targetArray.size() > 1)
                continue;
            
            const int quarterPeriod = roundToInt (halfPeriod * 0.5f);
            
            while (grainStart + grainLength > numSamples)
                grainStart -= quarterPeriod;
            
            if (grainStart < 0)
                grainStart = 0;
        }
        
        targetArray.add (grainStart);
    }
    
    jassert (! targetArray.isEmpty());
}
    
    
template<typename SampleType>
void GrainExtractor<SampleType>::findPsolaPeaks (Array<int>& targetArray,
                                                 const SampleType* reading,
                                                 const int totalNumSamples,
                                                 const int period)
{
    targetArray.clearQuick();
    
    const int grainSize = 2 * period;
    const int halfPeriod = roundToInt (ceil (period * 0.5f));
    
    int analysisIndex = 0; // marks the center of the analysis windows (which are 1 period long) -- but start @ 0
    
    do {
        const int frameStart = std::max (0, analysisIndex - halfPeriod);
        
        targetArray.add (findNextPeak (frameStart, std::min (totalNumSamples, frameStart + period),
                                       analysisIndex, reading, targetArray, period, grainSize));
        
        // analysisIndex marks the middle of our next analysis window, so it's where our next predicted peak should be:
        if (targetArray.size() == 1)
            analysisIndex = targetArray.getUnchecked(0) + period;
        else
            analysisIndex = targetArray.getUnchecked(targetArray.size() - 2) + grainSize;
    }
    while ((analysisIndex - halfPeriod) < totalNumSamples);
}
    

template<typename SampleType>
int GrainExtractor<SampleType>::findNextPeak (const int frameStart, const int frameEnd,
                                              int predictedPeak, const SampleType* reading,
                                              const Array<int>& targetArray,
                                              const int period, const int grainSize)
{
    predictedPeak = jlimit (frameStart, frameEnd, predictedPeak);
    
    peakCandidates.clearQuick();
    
    if (frameStart == frameEnd) // possible edge case?
    {
        peakCandidates.add (frameStart);
    }
    else
    {
        peakSearchingOrder.clearQuick();
        sortSampleIndicesForPeakSearching (peakSearchingOrder, frameStart, frameEnd, predictedPeak);
        
        for (int i = 0; i < bvhge_NUM_PEAKS_TO_TEST; ++i)
        {
            getPeakCandidateInRange (peakCandidates, reading, frameStart, frameEnd, predictedPeak, peakSearchingOrder);
            
            if (peakCandidates.size() == bvhge_NUM_PEAKS_TO_TEST)
                break;
        }
    }
#undef bvhge_NUM_PEAKS_TO_TEST
    
    jassert (! peakCandidates.isEmpty());
    
    switch (peakCandidates.size())
    {
        case 1:
            return peakCandidates.getUnchecked(0);
            
        case 2:
            return choosePeakWithGreatestPower (peakCandidates, reading);
            
        default:
            if (targetArray.size() <= 1)
                return choosePeakWithGreatestPower (peakCandidates, reading);
            
            return chooseIdealPeakCandidate (peakCandidates, reading,
                                             targetArray.getLast() + period,
                                             targetArray.getUnchecked(targetArray.size() - 2) + grainSize);
    }
}
    

template<typename SampleType>
void GrainExtractor<SampleType>::getPeakCandidateInRange (Array<int>& candidates, const SampleType* input,
                                                          const int startSample, const int endSample, const int predictedPeak,
                                                          const Array<int>& searchingOrder)
{
    jassert (! searchingOrder.isEmpty());
    
    int starting = -1;
    
    for (int poss : searchingOrder)
    {
        if (! candidates.contains (poss))
        {
            starting = poss;
            break;
        }
    }
    
    if (starting == -1)
        return;
    
    struct weighting
    {
        static inline SampleType weight (int index, int predicted, int numSamples)
        {
            return static_cast<SampleType>( 1.0 - ( ( (abs(index - predicted)) / numSamples ) * 0.5 ) );
        }
    };
    
    const int numSamples = endSample - startSample;
    
    jassert (starting >= startSample && starting <= endSample);
    
    SampleType localMin = input[starting] * weighting::weight (starting, predictedPeak, numSamples);
    SampleType localMax = localMin;
    int indexOfLocalMin = starting;
    int indexOfLocalMax = starting;
    
    for (int i = 0; i < numSamples; ++i)
    {
        const int index = searchingOrder.getUnchecked(i);
        
        if (index == starting || candidates.contains (index))
            continue;
        
        const SampleType currentSample = input[index] * weighting::weight (index, predictedPeak, numSamples);
        
        if (currentSample < localMin)
        {
            localMin = currentSample;
            indexOfLocalMin = index;
        }
        
        if (currentSample > localMax)
        {
            localMax = currentSample;
            indexOfLocalMax = index;
        }
    }
    
    constexpr SampleType zero = SampleType(0.0);
    
    if (indexOfLocalMax == indexOfLocalMin)
    {
        candidates.add (indexOfLocalMax);
    }
    else if (localMax < zero)
    {
        candidates.add (indexOfLocalMin);
    }
    else if (localMin > zero)
    {
        candidates.add (indexOfLocalMax);
    }
    else
    {
        candidates.add (std::min (indexOfLocalMax, indexOfLocalMin));
        candidates.add (std::max (indexOfLocalMax, indexOfLocalMin));
    }
}

    

template<typename SampleType>
int GrainExtractor<SampleType>::chooseIdealPeakCandidate (const Array<int>& candidates, const SampleType* reading,
                                                          const int deltaTarget1, const int deltaTarget2)
{
    candidateDeltas.clearQuick();
    finalHandful.clearQuick();
    finalHandfulDeltas.clearQuick();
    
    // 1. calculate delta values for each peak candidate
    // delta represents how far off this peak candidate is from the expected peak location - in a way it's a measure of the jitter that picking a peak candidate as this frame's peak would introduce to the overall alignment of the stream of grains based on the previous grains
    
    for (int candidate : candidates)
    {
        // deltaTarget1 = this peak's expected location based on the last peak found (ie, the neighboring OVERLAPPING output OLA grain)
        // deltatarget2 = this peak's expected location based on the second to last peak found (the neighboring CONSECUTIVE OLA grain)
        const int   delta1 = abs (candidate - deltaTarget1);
        const float delta2 = abs (candidate - deltaTarget2) * 1.5f; // weight this delta, this value is of more consequence
        
        candidateDeltas.add ((delta1 + delta2) * 0.5f); // average the two delta values
    }
    
    // 2. whittle our remaining candidates down to the final candidates with the minimum delta values
    
#define bvhge_DEFAULT_FINAL_HANDFUL_SIZE 5
    const int finalHandfulSize = std::min (bvhge_DEFAULT_FINAL_HANDFUL_SIZE, candidateDeltas.size());
#undef bvhge_DEFAULT_FINAL_HANDFUL_SIZE
    
    for (int i = 0; i < finalHandfulSize; ++i)
    {
        float* lowestElement = std::min_element (candidateDeltas.begin(), candidateDeltas.end());
        const int indexOfLowestDelta = static_cast<int> (std::distance (candidateDeltas.begin(), lowestElement));
        
        finalHandfulDeltas.add (*lowestElement);
        finalHandful.add (candidates.getUnchecked (indexOfLowestDelta));
        
        candidateDeltas.set (indexOfLowestDelta, 1000.0f); // make sure this value won't be chosen again, w/o deleting it from the candidateDeltas array
    }
    
    // 3. choose the strongest overall peak from these final candidates, with peaks weighted by their delta values
    
    const float deltaRange = FloatVectorOperations::findMinAndMax (finalHandfulDeltas.getRawDataPointer(), finalHandfulSize)
    .getLength();
    
#define bvhge_MIN_DELTA_RANGE 2.0f
    if (deltaRange < bvhge_MIN_DELTA_RANGE)
        return finalHandful.getUnchecked(0);
#undef bvhge_MIN_DELTA_RANGE
    
    struct weighting
    {
        static inline float weight (float delta, float deltaRange)
        {
            return 1.0f - ((delta / deltaRange) * 0.75f);
        }
    };
    
    int chosenPeak = finalHandful.getUnchecked (0);
    SampleType strongestPeak = abs(reading[chosenPeak]) * weighting::weight(finalHandfulDeltas.getUnchecked(0), deltaRange);
    
    for (int i = 1; i < finalHandfulSize; ++i)
    {
        const int candidate = finalHandful.getUnchecked(i);
        
        if (candidate == chosenPeak)
            continue;
        
        SampleType testingPeak = abs(reading[candidate]) * weighting::weight(finalHandfulDeltas.getUnchecked(i), deltaRange);
        
        if (testingPeak > strongestPeak)
        {
            strongestPeak = testingPeak;
            chosenPeak = candidate;
        }
    }
    
    return chosenPeak;
}


template<typename SampleType>
int GrainExtractor<SampleType>::choosePeakWithGreatestPower (const Array<int>& candidates, const SampleType* reading)
{
    int strongestPeakIndex = candidates.getUnchecked (0);
    SampleType strongestPeak = abs(reading[strongestPeakIndex]);
    
    for (int candidate : candidates)
    {
        const SampleType current = abs(reading[candidate]);
        
        if (current > strongestPeak)
        {
            strongestPeak = current;
            strongestPeakIndex = candidate;
        }
    }
    
    return strongestPeakIndex;
}


template<typename SampleType>
void GrainExtractor<SampleType>::sortSampleIndicesForPeakSearching (Array<int>& output, // array to write the sorted sample indices to
                                                                    const int startSample, const int endSample,
                                                                    const int predictedPeak)
{
    jassert (predictedPeak >= startSample && predictedPeak <= endSample);
    
    output.clearQuick();
    
    output.set (0, predictedPeak);
    
    int p = 1, m = -1;
    
    for (int n = 1;
         n < (endSample - startSample);
         ++n)
    {
        const int pos = predictedPeak + p;
        const int neg = predictedPeak + m;
        
        if (n % 2 == 0) // n is even
        {
            if (neg >= startSample)
            {
                output.set (n, neg);
                --m;
            }
            else
            {
                jassert (pos <= endSample);
                output.set (n, pos);
                ++p;
            }
        }
        else
        {
            if (pos <= endSample)
            {
                output.set (n, pos);
                ++p;
            }
            else
            {
                jassert (neg >= startSample);
                output.set (n, neg);
                --m;
            }
        }
    }
}



template class GrainExtractor<float>;
template class GrainExtractor<double>;


} // namespace
