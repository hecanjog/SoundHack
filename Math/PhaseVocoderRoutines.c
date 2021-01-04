//#include <AppleEvents.h>
#include "SoundFile.h"
#include "SoundHack.h"
#include "Hypot.h"
#include "Dialog.h"
#include "PhaseVocoder.h"
#include "Analysis.h"
#include "Misc.h"

extern long			gNumberBlocks;
extern float		Pi, twoPi;

/* Scale analysis and synthesis windows */
short
ScaleWindows(float analysisWindow[], float synthesisWindow[], PvocInfo myPI)
{
	long 	index;
	float 	sum = 0.0, halfWindow, analFactor, synthFactor;

/* when windowSize > points, also apply sin(x)/x windows to 
	ensure that window are 0 at increments of points (the FFT length)
	away from the center of the analysis window and of interpolation
	away from the center of the synthesis window */

    if (myPI.windowSize > myPI.points)
    {
		halfWindow = -((float)myPI.windowSize - 1.0)/2.0;
		for ( index = 0; index < myPI.windowSize; index++, halfWindow += 1.0 )
	    	if ( halfWindow != 0.0 )
	    	{
				analysisWindow[index] = analysisWindow[index] * myPI.points
					* sin(Pi*halfWindow/myPI.points)/(Pi*halfWindow);
				if ( myPI.interpolation )
		    		synthesisWindow[index] = synthesisWindow[index] * myPI.interpolation
		    			* sin(Pi*halfWindow/myPI.interpolation)/(Pi*halfWindow);
	    	}
	}
	
	
/*
 * normalize windows for unity gain across unmodified
 * analysis-synthesis procedure
 */
	for (index = 0; index < myPI.windowSize; index++)
		sum += analysisWindow[index];

	analFactor = 2.0/sum;
	if(myPI.analysisType == CSOUND_ANALYSIS)
		analFactor *= myPI.decimation * .5;     	
	synthFactor = (myPI.windowSize > myPI.points) ? 1.0/analFactor : analFactor;
	
    for (index = 0; index < myPI.windowSize; index++)
    {
		analysisWindow[index] *= analFactor;
		synthesisWindow[index] *= synthFactor;
	}
	
	if (myPI.windowSize <= myPI.points)
	{
		sum = 0.0;
		for (index = 0; index < myPI.windowSize; index += myPI.interpolation)
	   		sum += synthesisWindow[index]*synthesisWindow[index];
	   	sum = 1.0/sum;
		for (index = 0; index < myPI.windowSize; index++)
	   		synthesisWindow[index] *= sum;
    }
    return(TRUE);
}

/*
   shift next decimation samples into righthand end of arrays leftBlock
   and rightBlock of length windowSize, padding with zeros after last
   sample
*/
long
ShiftIn(SoundInfo *mySI, float leftBlock[], float rightBlock[], long windowSize, long decimation, long *validSamples)
{
    long 	index;
    long	numSamples;

    if (gNumberBlocks == 0)
		*validSamples = windowSize;

	/* move remainder from last block into left side of array */
    for (index = 0; index < windowSize - decimation; index++)
    {
		leftBlock[index] = leftBlock[index+decimation];
		if(mySI->nChans == STEREO)
			rightBlock[index] = rightBlock[index+decimation];
	}
    if (*validSamples == windowSize)
    {
    	if(mySI->nChans == STEREO)
			numSamples = ReadStereoBlock(mySI, decimation, 
				(leftBlock + windowSize - decimation),
				(rightBlock + windowSize - decimation));
		else
			numSamples = ReadMonoBlock(mySI, decimation, 
				(leftBlock + windowSize - decimation));
				
		if(numSamples != decimation)
			*validSamples = windowSize - decimation + numSamples;
	}
	else
		numSamples = 0L;

    if (*validSamples < windowSize)		/* pad with zeros after EOF */
    {
		for (index=*validSamples; index < windowSize; index++)
		{
	    	leftBlock[index] = 0.0;
    		if(mySI->nChans == STEREO)
		    	rightBlock[index] = 0.0;
    	}
		*validSamples -= decimation;
		if(*validSamples <= 0)
			return(-2L); /* Signal end of file */
    }
    return(numSamples);
}

/*
 * multiply current input Input by window Window (both of length lengthWindow);
 * using modulus arithmetic, fold and rotate windowed input into output array
 * output of (FFT) length lengthFFT according to current input time currentTime
 */
void
WindowFold(float input[], float window[], float output[], long currentTime, long points, long windowSize)
{
	long index;
	
    for (index = 0; index < points; index++)
		output[index] = 0.0;
    while (currentTime < 0)
		currentTime += points;
    currentTime %= points;
    for ( index = 0; index < windowSize; index++ )
    {
		output[currentTime] += input[index]*window[index];
		if ( ++currentTime == points )
	    	currentTime = 0;
    }
}

/*
 * spectrum is a spectrum in RealFFT format, index.e., it contains lengthFFT real values
 * arranged as real followed by imaginary values, except for first two values, which 
 * are real parts of 0 and Nyquist frequencies; convert first changes these into 
 * lengthFFT/2+1 PAIRS of magnitude and phase values to be stored in output array 
 * polarSpectrum.
 */
void 
CartToPolar(float spectrum[], float polarSpectrum[], long halfLengthFFT)
{		
	long	realIndex, imagIndex, ampIndex, phaseIndex;
	float	realPart, imagPart;
	long	bandNumber;
	
/*
 * unravel RealFFT-format spectrum: note that halfLengthFFT+1 pairs of values are produced
 */
    for (bandNumber = 0; bandNumber <= halfLengthFFT; bandNumber++)
    {
		realIndex = ampIndex = bandNumber<<1;
		imagIndex = phaseIndex = realIndex + 1;
    	if(bandNumber == 0)
    	{
    		realPart = spectrum[realIndex];
    		imagPart = 0.0;
    	}
    	else if(bandNumber == halfLengthFFT)
    	{
    		realPart = spectrum[1];
    		imagPart = 0.0;
    	}
    	else
    	{
			realPart = spectrum[realIndex];
			imagPart = spectrum[imagIndex];
		}
/*
 * compute magnitude & phase value from real and imaginary parts
 */

		polarSpectrum[ampIndex] = hypot(realPart, imagPart);
		if(polarSpectrum[ampIndex] == 0.0)
	    	polarSpectrum[phaseIndex] = 0.0;
	    else
	    	polarSpectrum[phaseIndex] = -atan2f(imagPart, realPart);
    }
}

/*
 * PolarToCart turns halfLengthFFT+1 PAIRS of amplitude and phase values in
 * polarSpectrum into halfLengthFFT PAIR of complex spectrum data (in RealFFT format)
 * in output array spectrum.
 */
void
PolarToCart(float polarSpectrum[], float spectrum[], long halfLengthFFT)
{
	float	realValue, imagValue;
	long	bandNumber, realIndex, imagIndex, ampIndex, phaseIndex;
 
/*
 * convert to cartesian coordinates, complex pairs
 */
    for (bandNumber = 0; bandNumber <= halfLengthFFT; bandNumber++) 
    {
		realIndex = ampIndex = bandNumber<<1;
		imagIndex = phaseIndex = realIndex + 1;
		if(polarSpectrum[ampIndex] == 0.0)
		{
			realValue = 0.0;
			imagValue = 0.0;
		}
		else if(bandNumber == 0 || bandNumber == halfLengthFFT)
    	{
			realValue = polarSpectrum[ampIndex] * cosf(polarSpectrum[phaseIndex]);
    		imagValue = 0.0;
    	}
    	else
		{
			realValue = polarSpectrum[ampIndex] * cosf(polarSpectrum[phaseIndex]);
			imagValue = -polarSpectrum[ampIndex] * sinf(polarSpectrum[phaseIndex]);
		}
		
 
		if(bandNumber == halfLengthFFT)
			realIndex = 1;
		spectrum[realIndex] = realValue;
		if(bandNumber != halfLengthFFT && bandNumber != 0)
			spectrum[imagIndex] = imagValue;
	}
}


/*
 * oscillator bank resynthesizer for phase vocoder analyzer
 * uses sum of gPI.halfPoints+1 cosinusoidal table lookup oscillators to compute
 * interpolation samples of output from gPI.halfPoints+1 amplitude and phase value-pairs
 * in polarSpectrum; frequencies are scaled by gPI.scaleFactor
 */
void
AddSynth(float polarSpectrum[], float output[], float lastAmp[], float lastFreq[], float lastPhase[], float sineIndex[], float sineTable[], PvocInfo myPI)
{
	long	ampIndex, freqIndex, sample, bandNumber, numberPartials;
	float	phaseDifference, cyclesFrame, cyclesBand, oneOvrInterp;
	
/*
 * initialize variables
 */

	oneOvrInterp = 1.0/myPI.interpolation;
	cyclesBand = myPI.scaleFactor*8192L/myPI.points;
	cyclesFrame = myPI.scaleFactor*8192L/(myPI.decimation*twoPi);
	if(myPI.scaleFactor > 1.0)
		numberPartials = (long)(myPI.halfPoints/myPI.scaleFactor);
	else
		numberPartials = myPI.halfPoints;
		
/*
 * convert phase representation into instantaneous frequency- this makes polarSpectrum
 * useless for future operations as it does an in-place conversion. Then
 * for each channel, compute interpolation samples using linear
 * interpolation on the amplitude and frequency
 */
    for ( bandNumber = 0; bandNumber < numberPartials; bandNumber++ )
    {
		register float amplitude, ampIncrement, frequency, freqIncrement, address;
		
		ampIndex = bandNumber<<1;
		freqIndex = ampIndex + 1;
		
		/* Start where we left off, keep phase */
 		address = sineIndex[bandNumber];
 		
		if(polarSpectrum[ampIndex] == 0.0)
		{
			polarSpectrum[freqIndex] = bandNumber*cyclesBand; /*Just set for next interpolation */
		}
		else
		{
			phaseDifference = polarSpectrum[freqIndex] - lastPhase[bandNumber];
			lastPhase[bandNumber] = polarSpectrum[freqIndex];
			
			/* Unwrap phase differences */
			while (phaseDifference > Pi)
				phaseDifference -= twoPi ;
			while (phaseDifference < -Pi)
				phaseDifference += twoPi ;

			/* Convert to instantaneos frequency */
			polarSpectrum[freqIndex] = phaseDifference*cyclesFrame + bandNumber*cyclesBand;
			
			/* Start with last amplitude */
 			amplitude = lastAmp[bandNumber];
 			
 			/*Increment per sample to get to new frequency */
			ampIncrement = (polarSpectrum[ampIndex] -  amplitude)*oneOvrInterp;
			
			/*Start with last frequency */
 			frequency = lastFreq[bandNumber];
 			
 			/*Increment per sample to get to new frequency */
			freqIncrement = (polarSpectrum[freqIndex] - frequency)*oneOvrInterp;
			
			/* Fill the output with one sine component */
			for ( sample = 0; sample < myPI.interpolation; sample++ )
			{
				output[sample] += amplitude*sineTable[(long)address];
				address += frequency;
				while (address >= 8192L) /* Unwrap phase */
					address -= 8192L;
				while (address < 0)
					address += 8192L;
				amplitude += ampIncrement;
				frequency += freqIncrement;
			} 
		}
		
/*
 * save current values for next iteration
 */
		lastFreq[bandNumber] = polarSpectrum[freqIndex];
		lastAmp[bandNumber] = polarSpectrum[ampIndex];
		sineIndex[bandNumber] = address;
    }
}


/*
 * Input are folded samples of length points; output and
 * synthesisWindow are of length lengthWindow--overlap-add windowed,
 * unrotated, unfolded input data into output
 */
void
OverlapAdd(float input[], float synthesisWindow[], float output[], long currentTime, long points, long windowSize)
{
	long index;
	
    while ( currentTime < 0 )
		currentTime += points;
    currentTime %= points;
    for ( index = 0; index < windowSize; index++ )
    {
		output[index] += input[currentTime] * synthesisWindow[index];
		if ( ++currentTime == points )
	  	  currentTime = 0;
    }
}


/*
   if output time currentTime >= 0, output first interpolation samples in arrays leftBlock 
   and rightBlock of length windowSize, then shift leftBlock and rightBlock left by interpolation samples,
   padding with zeros after last sample
*/
long
ShiftOut(SoundInfo *mySI, float leftBlock[], float rightBlock[], long currentTime, long interpolation, long windowSize)
{
	long index;
	long numSamples;
	
    if ( currentTime >= 0 )
    {
    	if(mySI->nChans == STEREO)
			numSamples = WriteStereoBlock(mySI, interpolation, leftBlock, rightBlock);
		else
			numSamples = WriteMonoBlock(mySI, interpolation, leftBlock);
	}

    for ( index = 0; index < windowSize - interpolation; index++ )
    {
		leftBlock[index] = leftBlock[index+interpolation];
    	if(mySI->nChans == STEREO)
			rightBlock[index] = rightBlock[index+interpolation];
	}
    for ( index = windowSize - interpolation; index < windowSize; index++ )
    {
		leftBlock[index] = 0.0;
		if(mySI->nChans == STEREO)
			rightBlock[index] = 0.0;
	}
	return(numSamples);
}

