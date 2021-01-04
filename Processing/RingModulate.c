#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
//#include <AppleEvents.h>
#include "SoundFile.h"
#include "Convolve.h"
#include "Dialog.h"
#include "Menu.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "SoundHack.h"
#include "Misc.h"
#include "Normalization.h"
#include "Windows.h"
#include "RingModulate.h"

/* Globals */


extern MenuHandle	gAppleMenu, gFileMenu, gEditMenu, gProcessMenu, gControlMenu;
extern SoundInfoPtr	inSIPtr, filtSIPtr, outSIPtr;

extern short			gProcessEnabled, gProcessDisabled, gStopProcess,gProcNChans;
extern Boolean		gNormalize; 
extern long			gNumberBlocks;
extern float		Pi, twoPi, gScale, gScaleL, gScaleR, gScaleDivisor;

extern ConvolveInfo	gCI;

extern float 		*impulseLeft, *inputLeft, *outputLeft, *overlapLeft,
					*impulseRight, *inputRight, *outputRight, *overlapRight, *Window;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

short
InitRingModProcess(void)
{	
	Str255	errStr, err2Str;
	long 	n, windowLength;
	float	tmpFloat, filtLength, inLength;

/*	Initialize variables and then blocks for convolution, gCI.sizeImpulse will have been
	initialized already by HandleFIRDialog, gCI.sizeConvolution is twice as big minus one
	as the impulse so that the full convolution is done (otherwise Y is just as long as
	X), gCI.sizeFFT (FFT length) is the first power of 2 >= gCI.sizeConvolution for 
	convenient calculation */
	
	inLength = (float)inSIPtr->numBytes/(inSIPtr->nChans * inSIPtr->frameSize);
	filtLength = (float)filtSIPtr->numBytes/(filtSIPtr->nChans * filtSIPtr->frameSize);
	tmpFloat = (filtLength/inLength) * gCI.sizeImpulse;
	gCI.incWin = (long)tmpFloat;
	if(gCI.windowImpulse)
		windowLength = gCI.sizeConvolution;
	else
		windowLength = gCI.sizeImpulse;
/*	Create Soundfile for Output */
	outSIPtr = nil;
	outSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
	if(gPreferences.defaultType == 0)
	{
		if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
		{
			outSIPtr->sfType = AIFF;
			outSIPtr->packMode = SF_FORMAT_16_LINEAR;
		}
		else
		{
			outSIPtr->sfType = inSIPtr->sfType;
			outSIPtr->packMode = inSIPtr->packMode;
		}
	}
	else
	{
		outSIPtr->sfType = gPreferences.defaultType;
		outSIPtr->packMode = gPreferences.defaultFormat;
	}
	NameFile(inSIPtr->sfSpec.name, "\p", errStr);
	StringAppend(errStr, "\p x ", err2Str);
	NameFile(filtSIPtr->sfSpec.name, "\p", errStr);
	StringAppend(err2Str, errStr, outSIPtr->sfSpec.name);
	
	outSIPtr->sRate = inSIPtr->sRate;
	outSIPtr->nChans = gProcNChans;
	outSIPtr->numBytes = 0;
	if(CreateSoundFile(&outSIPtr, SOUND_CUST_DIALOG) == -1)
	{
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		RemovePtr((Ptr)outSIPtr);
		outSIPtr = nil;
		return(-1);
	}
	WriteHeader(outSIPtr);
	UpdateInfoWindow(outSIPtr);

	if(AllocConvMem() == -1)
	{
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		outSIPtr = nil;
		return(-1);
	}

/*	Bring Up Window for Processing Time 	*/
	
	
/*	Calculate chosen smoothing window	*/
	if(gCI.windowImpulse)
		GetWindow(Window, gCI.sizeConvolution, gCI.windowType);
	
/*	Initialize overlap	*/
	for(n = 0; n <(gCI.sizeImpulse - 1); n++)
		overlapLeft[n] = 0.0;
	
	if(gProcNChans == STEREO)
	{
		/*	Initialize B Right	*/
		for(n = 0; n <(gCI.sizeImpulse - 1); n++)
			overlapRight[n] = 0.0;
	}

/*	Read in impulse H, and zero pad to gCI.sizeFFT */

	UpdateProcessWindow("\p", "\p", "\pmodulating sound with impulse", 0.0);
	SetFPos(filtSIPtr->dFileNum, fsFromStart, filtSIPtr->dataStart);
	ReadStereoBlock(filtSIPtr, windowLength, impulseLeft, impulseRight);
	if(gCI.windowImpulse)
		for(n = 0; n < gCI.sizeConvolution; n++)
			impulseLeft[n] = impulseLeft[n] * Window[n];
	if(impulseLeft != impulseRight)
	{
		/*	Read in impulse H, and zero pad to gCI.sizeFFT, transform to frequency domain
			and scale for unity gain	*/
		if(gCI.windowImpulse)
			for(n = 0; n < gCI.sizeConvolution; n++)
				impulseRight[n] = impulseRight[n] * Window[n];
	}
	gScaleDivisor = 1.0;
	SetOutputScale(outSIPtr->packMode);

	SetFPos(inSIPtr->dFileNum, fsFromStart, 0L);
		
	gProcessDisabled = NO_PROCESS;
	gProcessEnabled = RING_PROCESS;
	MenuUpdate();
	return(TRUE);
}

short
RingModBlock(void)
{
	float	length;
	long	readLength, writeLength, numSampsRead, n, thisBlock;
	Str255	errStr;
	static long	sOutPos;
	
	if(gNumberBlocks == 0)
		sOutPos = outSIPtr->dataStart;
	if(gStopProcess == TRUE)
	{
		FinishProcess();
		gNumberBlocks = 0;
		DeAllocConvMem();
		return(-1);
	}
	
	SetFPos(outSIPtr->dFileNum, fsFromStart, sOutPos);
	
	if(gCI.windowImpulse)
		readLength = gCI.sizeConvolution;
	else
		readLength = gCI.sizeImpulse;
		
	SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart 
		+ (inSIPtr->nChans * gCI.sizeImpulse * gNumberBlocks * inSIPtr->frameSize)));
	numSampsRead = ReadStereoBlock(inSIPtr, readLength, inputLeft, inputRight);
	
	if(numSampsRead < readLength)
	{
		for(n = numSampsRead; n < readLength; n++)
			inputLeft[n] = inputRight[n] = 0.0;
		thisBlock = numSampsRead;
	}
	else
		thisBlock = gCI.sizeImpulse;
	
	writeLength = thisBlock;
	length = (thisBlock + (gNumberBlocks * gCI.sizeImpulse))/inSIPtr->sRate;
	HMSTimeString(length, errStr);
	length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
	UpdateProcessWindow(errStr, "\p", "\pmodulating sound with impulse", length);
	
	for(n = 0; n < readLength; n++)
		outputLeft[n] = impulseLeft[n] * inputLeft[n];
	if(gCI.windowImpulse)
		for(n = 0; n < (gCI.sizeImpulse - 1); n++)
		{
			outputRight[n] = outputRight[n] + overlapRight[n];
			overlapRight[n] = outputRight[gCI.sizeImpulse + n];
		}
	if(outputLeft != outputRight)
	{
		for(n = 0; n < readLength; n++)
			outputRight[n] = impulseRight[n] * inputRight[n];
		if(gCI.windowImpulse)
			for(n = 0; n < (gCI.sizeImpulse - 1); n++)
			{
				outputRight[n] = outputRight[n] + overlapRight[n];
				overlapRight[n] = outputRight[gCI.sizeImpulse + n];
			}
	}
	if(inSIPtr->packMode != SF_FORMAT_TEXT)
		SetFPos(outSIPtr->dFileNum, fsFromStart, (outSIPtr->dataStart 
			+ (outSIPtr->nChans * gCI.sizeImpulse * gNumberBlocks * outSIPtr->frameSize)));
	if(outSIPtr->nChans == STEREO)
		WriteStereoBlock(outSIPtr, writeLength, outputLeft, outputRight);
	else
		WriteMonoBlock(outSIPtr, writeLength, outputLeft);
	length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
	HMSTimeString(length, errStr);
	UpdateProcessWindow("\p", errStr, "\p", -1.0);
	gNumberBlocks++;
	GetFPos(outSIPtr->dFileNum, &sOutPos);

	
	if(numSampsRead != readLength)
	{
		if(gCI.windowImpulse)
		{
			if(outSIPtr->nChans == STEREO)
				WriteStereoBlock(outSIPtr, (gCI.sizeImpulse - 1), (outputLeft + gCI.sizeImpulse), (outputRight + numSampsRead));
			else
				WriteMonoBlock(outSIPtr, (gCI.sizeImpulse - 1), (outputLeft + gCI.sizeImpulse));
			length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
			HMSTimeString(length, errStr);
			UpdateProcessWindow("\p", errStr, "\p", -1.0);
		}
		gNumberBlocks = 0;
		DeAllocConvMem();
		FlushVol((StringPtr)NIL_POINTER,outSIPtr->sfSpec.vRefNum);
		if(gNormalize == TRUE && outSIPtr->packMode != SF_FORMAT_TEXT)
			InitNormProcess(outSIPtr);
		else
			FinishProcess();
	}
	else if(gCI.moving)
	{
		SetFPos(filtSIPtr->dFileNum, fsFromStart, (filtSIPtr->dataStart 
			+ (filtSIPtr->nChans * gCI.incWin * gNumberBlocks * filtSIPtr->frameSize)));
		ReadStereoBlock(filtSIPtr, gCI.sizeConvolution, impulseLeft, impulseRight);
		if(gCI.windowImpulse)
			for(n = 0; n < gCI.sizeConvolution; n++)
				impulseLeft[n] = impulseLeft[n] * Window[n];
		if(impulseLeft != impulseRight)
		{
			if(gCI.windowImpulse)
				for(n = 0; n < gCI.sizeConvolution; n++)
					impulseRight[n] = impulseRight[n] * Window[n];
		}
	}
	return(TRUE);
}
