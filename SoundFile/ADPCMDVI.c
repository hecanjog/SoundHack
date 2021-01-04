/*
 *	SoundHack�
 *	Copyright �1993 Tom Erbe - Mills College
 *
 *	ADPCMDVI.c - ADPCM encoder and decoder based on Intel DVI algorithm
 *	Translated by Phil Burk for 3DO on 5/25/93.
 *	Ported to SoundHack by Tom Erbe on 5/30/93.
 */

#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
#include "Macros.h"
#include "SoundFile.h"
#include "ADPCMDVI.h"
#include "ADPCM.h"

extern	long	gNumberBlocks;
extern	float	gScale, gScaleL, gScaleR, gScaleDivisor;

/*
 *	ADDVIEncode() - takes two shorts and some values from the previous call of this
 *	function, and returns two nibbles packed in a char. If stereo, the left is the
 *	first nibble the right the second.
 */
 
char
ADDVIEncode(short shortOne, short shortTwo, long channels, Boolean init)
{
	long			delta;
	unsigned char	encodedSample, outputByte;
	static long		lastEstimateL, stepSizeL, stepIndexL,
					lastEstimateR, stepSizeR, stepIndexR;
			
	if(init)	/* set initial values */
	{
		lastEstimateL = lastEstimateR = 0L;
		stepSizeL = stepSizeR = 7L;
		stepIndexL = stepIndexR = 0L;
	}
	outputByte = 0;
	
/* First sample or left sample to be packed in first nibble */
/* calculate delta */
	delta = shortOne - lastEstimateL;
	CLIP(delta, -32768L, 32767L);

/* encode delta relative to the current stepsize */
	encodedSample = EncodeDelta(stepSizeL, delta);

/* pack first nibble */
	outputByte = 0x00F0 & (encodedSample<<4);

/* decode ADPCM code value to reproduce delta and generate an estimated InputSample */
	lastEstimateL += DecodeDelta(stepSizeL, encodedSample);
	CLIP(lastEstimateL, -32768L, 32767L);

/* adapt stepsize */
	stepIndexL += gIndexDeltas[encodedSample];
	CLIP(stepIndexL, 0, 88);
	stepSizeL = gStepSizes[stepIndexL];
    
    if(channels == 2L)
    {
/* calculate delta for second sample */
		delta = shortTwo - lastEstimateR;
		CLIP(delta, -32768L, 32767L);

/* encode delta relative to the current stepsize */
		encodedSample = EncodeDelta(stepSizeR, delta);

/* pack second nibble */
		outputByte |= 0x000F & encodedSample;

/* decode ADPCM code value to reproduce delta and generate an estimated InputSample */
		lastEstimateR += DecodeDelta(stepSizeR, encodedSample);
		CLIP(lastEstimateR, -32768L, 32767L);

/* adapt stepsize */
		stepIndexR += gIndexDeltas[encodedSample];
		CLIP(stepIndexR, 0, 88);
		stepSizeR = gStepSizes[stepIndexR];
	}
	else
	{
/* calculate delta for second sample */
		delta = shortTwo - lastEstimateL;
		CLIP(delta, -32768L, 32767L);

/* encode delta relative to the current stepsize */
		encodedSample = EncodeDelta(stepSizeL, delta);

/* pack second nibble */
		outputByte |= 0x000F & encodedSample;

/* decode ADPCM code value to reproduce delta and generate an estimated InputSample */
		lastEstimateL += DecodeDelta(stepSizeL, encodedSample);
		CLIP(lastEstimateL, -32768L, 32767L);

/* adapt stepsize */
		stepIndexL += gIndexDeltas[encodedSample];
		CLIP(stepIndexL, 0, 88);
		stepSizeL = gStepSizes[stepIndexL];
	}
	return(outputByte);
}

/*
 * This function will decode two samples from the two nibbles in charDVI. If two channel
 * it will return left then right (usual interleaving). If one channel, two succesive
 * samples are returned and only the left static values are used
 */
void
ADDVIDecode(unsigned char charDVI, short *shortOne, short *shortTwo, long channels, Boolean init)
{
	long outputSample;
	char encodedSample;
	static long lastEstimateL, stepSizeL, stepIndexL,
				lastEstimateR, stepSizeR, stepIndexR;
			
	if(init)	/* set initial values */
	{
		lastEstimateL = lastEstimateR = 0L;
		stepSizeL = stepSizeR = 7L;
		stepIndexL = stepIndexR = 0L;
	}
			

/* Sample one or left sample from first nibble */

	outputSample = lastEstimateL;
	encodedSample = 0x000F & (charDVI>>4);

/* decode ADPCM code value to reproduce Dn and accumulates an estimated outputSample */
	outputSample += DecodeDelta(stepSizeL, encodedSample);
	CLIP(outputSample, -32768L, 32767L);
	*shortOne = lastEstimateL = outputSample;
			
/* stepsize adaptation for next sample */
	stepIndexL += gIndexDeltas[encodedSample];
	CLIP(stepIndexL, 0, 88);
	stepSizeL = gStepSizes[stepIndexL];

/* Look at second nibble */	
	encodedSample = 0x000F & charDVI;

	if(channels == 2L)
	{
/* decode ADPCM code value to reproduce Dn and accumulates an estimated outputSample */
		outputSample = lastEstimateR;
		outputSample += DecodeDelta(stepSizeR, encodedSample);
		CLIP(outputSample, -32768L, 32767L);
		*shortTwo = lastEstimateR = outputSample;

/* stepsize adaptation */
		stepIndexR += gIndexDeltas[encodedSample];
		CLIP( stepIndexR, 0, 88 );
		stepSizeR = gStepSizes[stepIndexR];
	}
	else
	{
/* decode ADPCM code value to reproduce Dn and accumulates an estimated outputSample */
		outputSample += DecodeDelta(stepSizeL, encodedSample);
		CLIP(outputSample, -32768L, 32767L);
		*shortTwo = lastEstimateL = outputSample;
			
/* stepsize adaptation for next sample */
		stepIndexL += gIndexDeltas[encodedSample];
		CLIP(stepIndexL, 0, 88);
		stepSizeL = gStepSizes[stepIndexL];
	}
}

void BlockADDVIEncode(char *charSams, float *blockL, float *blockR, long numSamples, long channels)
{
	short shortOne, shortTwo;
	long i, j;
	Boolean init;
		
	if(gNumberBlocks == 0)	/* set initial values */
		init = TRUE;
	else
		init = FALSE;
		
	for(i = j = 0; i < numSamples; i += 2, j++)
	{
		if(channels == STEREO)
		{
			shortOne = (short)(blockL[j] * gScaleL);
			shortTwo = (short)(blockR[j] * gScaleR);
		}
		else
		{
			shortOne = (short)(blockL[i] * gScale);
			shortTwo = (short)(blockL[i+1] * gScale);
		}
		*(charSams+j) = ADDVIEncode(shortOne, shortTwo, channels, init);
		init = FALSE;
	}
}

/*
 * Just like ADDVIDecode, but optimized for block decoding.
 * This function will decode two samples from the two nibbles in charSams. If two channel
 * it will return left then right (usual interleaving). If one channel, two succesive
 * samples are returned and only the left static values are used
 */
void
BlockADDVIDecode(unsigned char *charSams, short *shortSams, long frames, long channels, Boolean init)
{
	long outputSample, i, j, samples;
	char encodedSample;
	static long lastEstimateL, stepSizeL, stepIndexL,
				lastEstimateR, stepSizeR, stepIndexR;
			
	if(init)	/* set initial values */
	{
		lastEstimateL = lastEstimateR = 0L;
		stepSizeL = stepSizeR = 7L;
		stepIndexL = stepIndexR = 0L;
		return;
	}
	samples = frames * channels;
			
	for(i = 0, j = 0; j < samples; i++, j += 2)
	{
		// Sample one or left sample from first nibble
		outputSample = lastEstimateL;
		encodedSample = 0x000F & (*(charSams+i)>>4);

		// decode ADPCM code value to reproduce Dn and accumulates an estimated outputSample
		outputSample += DecodeDelta(stepSizeL, encodedSample);
		CLIP(outputSample, -32768L, 32767L);
		*(shortSams+j) = lastEstimateL = outputSample;
			
		// stepsize adaptation for next sample
		stepIndexL += gIndexDeltas[encodedSample];
		CLIP(stepIndexL, 0, 88);
		stepSizeL = gStepSizes[stepIndexL];

		// Look at second nibble
		encodedSample = 0x000F & *(charSams+i);

		if(channels == 2L)
		{
			// decode ADPCM code value to reproduce Dn and accumulates an estimated outputSample
			outputSample = lastEstimateR;
			outputSample += DecodeDelta(stepSizeR, encodedSample);
			CLIP(outputSample, -32768L, 32767L);
			*(shortSams+j+1) = lastEstimateR = outputSample;

			// stepsize adaptation
			stepIndexR += gIndexDeltas[encodedSample];
			CLIP( stepIndexR, 0, 88 );
			stepSizeR = gStepSizes[stepIndexR];
		}
		else
		{
			// decode ADPCM code value to reproduce Dn and accumulates an estimated outputSample
			outputSample += DecodeDelta(stepSizeL, encodedSample);
			CLIP(outputSample, -32768L, 32767L);
			*(shortSams+j+1) = lastEstimateL = outputSample;
			
			// stepsize adaptation for next sample
			stepIndexL += gIndexDeltas[encodedSample];
			CLIP(stepIndexL, 0, 88);
			stepSizeL = gStepSizes[stepIndexL];
		}
	}
}

