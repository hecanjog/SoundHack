// SpectFileIO.c - putting all my eggs in one basket
#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "SoundFile.h"
#include "SoundHack.h"
#include "PhaseVocoder.h"
#include "Analysis.h"
#include "SpectFileIO.h"
#include "sdif.h"

extern long			gNumberBlocks;
extern float		Pi, twoPi;
extern SoundInfoPtr	inSIPtr, outSIPtr, outLSIPtr, outRSIPtr;
extern PvocInfo 	gAnaPI;

long	WriteCSAData(SoundInfo *mySI, float polarSpectrum[], float lastPhaseIn[], long halfLengthFFT)
{
	long bandNumber, phaseIndex, ampIndex, writeSize, numSamples;
	float  phaseDifference;
	static float factor, fundamental;
	static long lastPos;
    if (gNumberBlocks == 0)
    {
    	lastPos = mySI->dataStart;
		fundamental = inSIPtr->sRate/(halfLengthFFT<<1);
        factor = inSIPtr->sRate/(gAnaPI.decimation*twoPi);
	}
	writeSize = GetPtrSize((Ptr)polarSpectrum);
	SetFPos(mySI->dFileNum, fsFromStart, lastPos);
	
    for(bandNumber = 0; bandNumber <= halfLengthFFT; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
		phaseIndex = ampIndex + 1;
/*
 * take difference between the current phase value and previous value for each channel
 */
		if (polarSpectrum[ampIndex] == 0.0)
	    	phaseDifference = 0.0;
		else
		{
	    	phaseDifference = polarSpectrum[phaseIndex] - lastPhaseIn[bandNumber];
	    	lastPhaseIn[bandNumber] = polarSpectrum[phaseIndex];

	    	while (phaseDifference > Pi)
				phaseDifference -= twoPi;
	    	while (phaseDifference < -Pi)
				phaseDifference += twoPi;
		}
		polarSpectrum[phaseIndex] = phaseDifference*factor + bandNumber*fundamental;
	}
	FSWrite(mySI->dFileNum, &writeSize, polarSpectrum);
	GetFPos(mySI->dFileNum, &lastPos);
	mySI->numBytes += writeSize;
	numSamples = (writeSize * mySI->spectFrameIncr)/((mySI->spectFrameSize + 2) * sizeof(float));
	return(numSamples);
}
	
long	WriteSHAData(SoundInfo *mySI, float polarSpectrum[], short channel)
{
	long writeSize, numSamples;
	static long lastPos;
	
    if(gNumberBlocks == 0 && channel == LEFT)
    	lastPos = mySI->dataStart;
	writeSize = GetPtrSize((Ptr)polarSpectrum);
	SetFPos(mySI->dFileNum, fsFromStart, lastPos);
	FSWrite(mySI->dFileNum, &writeSize, polarSpectrum);
	GetFPos(mySI->dFileNum, &lastPos);
	mySI->numBytes += writeSize;
	numSamples = (writeSize * mySI->spectFrameIncr)/((mySI->spectFrameSize + 2) * sizeof(float));
	if(numSamples == 0)
		return(-2L);
	else
		return(numSamples);
}

// we just use the polarSpectrum as a space to reorganize the FFT data
long	WriteSDIFData(SoundInfo *mySI, float spectrum[], float polarSpectrum[], double seconds, short channel)
{
	SDIF_FrameHeader fh;
	SDIF_STFInfoFloatMatrix im;
	SDIF_MatrixHeader mh;
	
	long bandNumber, realIndex, imagIndex;
	long writeSize, numSamples;
	static long lastPos;
	
    if(gNumberBlocks == 0 && channel == LEFT)
    	lastPos = mySI->dataStart;
	SetFPos(mySI->dFileNum, fsFromStart, lastPos);
	// the entire frame includes 
	// frameheader
	// matrixheader
	// ISTF info matrix
	// matrixheader
	// 1STF data (real, imag for each bin)
	
	// first, fill out the frameheader
	fh.frameType[0] = '1';
	fh.frameType[1] = 'S';
	fh.frameType[2] = 'T';
	fh.frameType[3] = 'F';
	fh.size = (mySI->frameSize * mySI->spectFrameIncr) - 8;
	fh.time = seconds;
	fh.streamID = channel;
	fh.matrixCount = 2;
	writeSize = sizeof(fh);
	FSWrite(mySI->dFileNum, &writeSize, &fh);
	mySI->numBytes += writeSize;
	
	// then, fill out the info matrix
	im.matrixType[0] = 'I';
	im.matrixType[1] = 'S';
	im.matrixType[2] = 'T';
	im.matrixType[3] = 'F';
	im.matrixDataType = SDIF_FLOAT32;
	im.rowCount = 1;
	im.columnCount = 3;
	im.sampleRate = mySI->sRate;
	im.frameSeconds = mySI->spectFrameIncr/mySI->sRate;
	im.fftSize = mySI->spectFrameSize;
	im.zeroPad = 0.0;
	writeSize = sizeof(im);
	FSWrite(mySI->dFileNum, &writeSize, &im);
	mySI->numBytes += writeSize;
	
	// fill out the header to the data matrix
	mh.matrixType[0] = '1';
	mh.matrixType[1] = 'S';
	mh.matrixType[2] = 'T';
	mh.matrixType[3] = 'F';
	mh.matrixDataType = SDIF_FLOAT32;
	mh.rowCount = (mySI->spectFrameSize >> 1) + 1;
	mh.columnCount = 2;
	writeSize = sizeof(mh);
	FSWrite(mySI->dFileNum, &writeSize, &mh);
	mySI->numBytes += writeSize;
	
	// rearrange data for this format
	for (bandNumber = 0; bandNumber <= (mySI->spectFrameSize >> 1); bandNumber++)
    {
		realIndex = bandNumber<<1;
		imagIndex = realIndex + 1;
    	if(bandNumber == 0)
    	{
    		polarSpectrum[realIndex] = spectrum[realIndex];
    		polarSpectrum[imagIndex] = 0.0;
    	}
    	else if(bandNumber == (mySI->spectFrameSize >> 1))
    	{
    		polarSpectrum[realIndex] = spectrum[1];
    		polarSpectrum[imagIndex] = 0.0;
    	}
    	else
    	{
			polarSpectrum[realIndex] = spectrum[realIndex];
			polarSpectrum[imagIndex] = spectrum[imagIndex];
		}
	}
	writeSize = GetPtrSize((Ptr)polarSpectrum);
	FSWrite(mySI->dFileNum, &writeSize, polarSpectrum);
	mySI->numBytes += writeSize;
	GetFPos(mySI->dFileNum, &lastPos);
	numSamples = (writeSize * mySI->spectFrameIncr)/((mySI->spectFrameSize + 2) * sizeof(float));
	if(numSamples == 0)
		return(-2L);
	else
		return(numSamples);
}

long	ReadCSAData(SoundInfo *mySI, float polarSpectrum[], float lastPhaseIn[], long halfLengthFFT)
{
	long bandNumber, phaseIndex, ampIndex, readSize, numSamples;
	float  phaseDifference;
	static float factor, fundamental;
	static long lastPos;
    if (gNumberBlocks == 0)
    {
    	lastPos = mySI->dataStart;
		fundamental = mySI->sRate/(halfLengthFFT<<1);
        factor = (gAnaPI.decimation*twoPi)/mySI->sRate;
	}
	readSize = GetPtrSize((Ptr)polarSpectrum);
	SetFPos(mySI->dFileNum, fsFromStart, lastPos);
	FSRead(mySI->dFileNum, &readSize, polarSpectrum);
	GetFPos(mySI->dFileNum, &lastPos);
	
    for(bandNumber = 0; bandNumber <= halfLengthFFT; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
		phaseIndex = ampIndex + 1;
/*
 * take difference between the current phase value and previous value for each channel
 */
		if (polarSpectrum[ampIndex] == 0.0)
	    	polarSpectrum[phaseIndex] = 0.0;
		else
		{
			phaseDifference = (polarSpectrum[phaseIndex] - bandNumber*fundamental)*factor;
	    	while (phaseDifference > Pi)
				phaseDifference -= twoPi;
	    	while (phaseDifference < -Pi)
				phaseDifference += twoPi;
			
	    	polarSpectrum[phaseIndex] = phaseDifference + lastPhaseIn[bandNumber];
	    	lastPhaseIn[bandNumber] = polarSpectrum[phaseIndex];
		}
	}
	numSamples = (readSize * mySI->spectFrameIncr)/((mySI->spectFrameSize + 2) * sizeof(float));
	if(numSamples == 0)
		return(-2L);
	else
		return(numSamples);
}	

long	ReadSHAData(SoundInfo *mySI, float polarSpectrum[], short channel)
{
	long readSize, numSamples;
	static long lastPos;
	
    if(gNumberBlocks == 0 && channel == LEFT)
    	lastPos = mySI->dataStart;
	readSize = GetPtrSize((Ptr)polarSpectrum);
	SetFPos(mySI->dFileNum, fsFromStart, lastPos);
	FSRead(mySI->dFileNum, &readSize, polarSpectrum);
	GetFPos(mySI->dFileNum, &lastPos);
	numSamples = (readSize * mySI->spectFrameIncr)/((mySI->spectFrameSize + 2) * sizeof(float));
	if(numSamples == 0)
		return(-2L);
	else
		return(numSamples);
}

// i am trusting that the streams always remain in the same order...
long	ReadSDIFData(SoundInfo *mySI, float spectrum[], float polarSpectrum[], double *seconds, short channel)
{
	long readSize, numSamples;
	static long lastPos;
	SDIF_FrameHeader fh;
	SDIF_MatrixHeader mh;
	double *doubleBlock;
	struct
	{
		double	sampleRate;
		double	frameSeconds;
		double	fftSize;
	}	infoDouble;
	struct
	{
		float	sampleRate;
		float	frameSeconds;
		float	fftSize;
		float	zeroPad;
	}	infoFloat;
	
	long bandNumber, realIndex, imagIndex;
	Boolean done = FALSE;
	
    if(gNumberBlocks == 0 && channel == LEFT)
    	lastPos = mySI->dataStart;
	
	while(done == FALSE)
	{
		SetFPos(mySI->dFileNum, fsFromStart, lastPos);
		readSize = sizeof(fh);
		FSRead(mySI->dFileNum, &readSize, &fh);
		if(readSize != sizeof(fh))
		{
			numSamples = 0;
			done = TRUE;
			break;
		}
		if(fh.frameType[0] == '1' && fh.frameType[1] == 'S' && fh.frameType[2] == 'T' 
			&& fh.frameType[3] == 'F' && fh.streamID == channel)
		{
			readSize = sizeof(mh);
			FSRead(mySI->dFileNum, &readSize, &mh);
			if(mh.matrixDataType == SDIF_FLOAT32)
			{
				readSize = sizeof(infoFloat);
				FSRead(mySI->dFileNum, &readSize, &infoFloat);
				*seconds = infoFloat.frameSeconds;
			}
			else
			{
				readSize = sizeof(infoDouble);
				FSRead(mySI->dFileNum, &readSize, &infoDouble);
				*seconds = infoDouble.frameSeconds;
			}
			readSize = sizeof(mh);
			FSRead(mySI->dFileNum, &readSize, &mh);
			if(mh.matrixDataType == SDIF_FLOAT32)
			{
				readSize = mh.rowCount * mh.columnCount * sizeof(float);
				FSRead(mySI->dFileNum, &readSize, polarSpectrum);
				for (bandNumber = 0; bandNumber <= (mySI->spectFrameSize >> 1); bandNumber++)
			    {
					realIndex = bandNumber<<1;
					imagIndex = realIndex + 1;
			    	if(bandNumber == 0)
			    		spectrum[realIndex] = polarSpectrum[realIndex];
			    	else if(bandNumber == (mySI->spectFrameSize >> 1))
			    		spectrum[1] = polarSpectrum[realIndex];
			    	else
			    	{
						spectrum[realIndex] = polarSpectrum[realIndex];
						spectrum[imagIndex] = polarSpectrum[imagIndex];
					}
				}
				done = TRUE;
			}
			else
			{
				readSize = mh.rowCount * mh.columnCount * sizeof(double);
				doubleBlock = (double *)NewPtr(readSize);
				FSRead(mySI->dFileNum, &readSize, doubleBlock);
				for (bandNumber = 0; bandNumber <= (mySI->spectFrameSize >> 1); bandNumber++)
			    {
					realIndex = bandNumber<<1;
					imagIndex = realIndex + 1;
			    	if(bandNumber == 0)
			    		spectrum[realIndex] = doubleBlock[realIndex];
			    	else if(bandNumber == (mySI->spectFrameSize >> 1))
			    		spectrum[1] = doubleBlock[realIndex];
			    	else
			    	{
						spectrum[realIndex] = doubleBlock[realIndex];
						spectrum[imagIndex] = doubleBlock[imagIndex];
					}
				}
				DisposePtr((Ptr)doubleBlock);
				done = TRUE;
			}
			GetFPos(mySI->dFileNum, &lastPos);
		}
		else
		{
			lastPos += fh.size + 8;
		}
	}
	GetFPos(mySI->dFileNum, &lastPos);
	numSamples = (readSize * mySI->spectFrameIncr)/((mySI->spectFrameSize + 2) * sizeof(float));
	if(numSamples == 0)
		return(-2L);
	else
		return(numSamples);
}

