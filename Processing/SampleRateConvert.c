/* 
 *	SoundHack - soon to be many things to many people
 *	Copyright �1994 Tom Erbe - California Institute of the Arts
 *
 *	SampleRateConvert.c - varispeed and sample rate conversion
 *	9/30/94 - fixed pop problem, the process used to start at the first byte,
 *	now at the sound offset.(SetFPos(inSIPtr->dFileNum, fsFromStart, inSIPtr->dataStart);)
 *	10/22/94 - insured that one extra block be written at the end of each soundfile.
 */
 
#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
//#include <AppleEvents.h>
#include "SoundFile.h"
#include "SoundHack.h"
#include "Menu.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "Misc.h"
#include "DrawFunction.h"
#include "Windows.h"
#include "Dialog.h"
#include "SampleRateConvert.h"
#include "CarbonGlue.h"

extern DialogPtr	gDrawFunctionDialog;
DialogPtr	gSRConvDialog;
extern MenuHandle	gFileMenu, gProcessMenu, gControlMenu, gSigDispMenu,
					gSpectDispMenu, gSonoDispMenu;
extern SoundInfoPtr	inSIPtr, outSIPtr;

extern float		*gFunction;
extern float		gScaleDivisor, gScaleL, gScaleR, gScale, Pi;
extern short			gProcessEnabled, gProcessDisabled, gStopProcess;
extern long			gNumberBlocks;
extern float		*inputL, *outputL, *inputR, *outputR;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

float	*sincBuffer;
Ptr		sincBufferPtr;
SRCInfo	gSRCI;

void
HandleSRConvDialog(void)
{
	short		i;
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	Str255	errStr;
	WindInfo	*theWindInfo;
	
	gSRConvDialog = GetNewDialog(SRATE_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)SRATE_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gSRConvDialog), (long)theWindInfo);
#else
	SetWRefCon(gSRConvDialog, (long)theWindInfo);
#endif

	gProcessDisabled = UTIL_PROCESS;
	// Rename Display MENUs
	SetMenuItemText(gSigDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSpectDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSonoDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSigDispMenu, DISP_AUX_ITEM, "\p ");
	SetMenuItemText(gSpectDispMenu, DISP_AUX_ITEM, "\p ");
	SetMenuItemText(gSonoDispMenu, DISP_AUX_ITEM, "\p ");
	SetMenuItemText(gSigDispMenu, DISP_OUTPUT_ITEM, "\pOutput");
	SetMenuItemText(gSpectDispMenu, DISP_OUTPUT_ITEM, "\pOutput");
	SetMenuItemText(gSonoDispMenu, DISP_OUTPUT_ITEM, "\pOutput");
	
	EnableItem(gSigDispMenu, DISP_INPUT_ITEM);
	DisableItem(gSpectDispMenu, DISP_INPUT_ITEM);
	DisableItem(gSonoDispMenu, DISP_INPUT_ITEM);
	DisableItem(gSigDispMenu, DISP_AUX_ITEM);
	DisableItem(gSpectDispMenu, DISP_AUX_ITEM);
	DisableItem(gSonoDispMenu, DISP_AUX_ITEM);
	EnableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
	DisableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
	DisableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	MenuUpdate();
	
	gScaleDivisor = 32768.0;
	
	for(i = 0; i < 400; i++)
		gFunction[i] = 1.0;
	
	GetDialogItem(gSRConvDialog, V_BEST_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gSRConvDialog, V_MEDIUM_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gSRConvDialog, V_FAST_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	switch(gSRCI.quality)
	{
		case 1:
			GetDialogItem(gSRConvDialog, V_FAST_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			break;
		case 4:
			GetDialogItem(gSRConvDialog, V_MEDIUM_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			break;
		case 16:
			GetDialogItem(gSRConvDialog, V_BEST_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			break;
	}	
	FixToString(gSRCI.newSRate, errStr);
	GetDialogItem(gSRConvDialog, V_RATE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, errStr);
	
	if(gSRCI.pitchScale == TRUE)
	{
		GetDialogItem(gSRConvDialog, V_PITCH_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		GetDialogItem(gSRConvDialog, V_SCALE_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
	}
	else
	{
		GetDialogItem(gSRConvDialog, V_PITCH_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		GetDialogItem(gSRConvDialog, V_SCALE_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
	}

	if(gSRCI.variSpeed == FALSE)
	{
		GetDialogItem(gSRConvDialog, V_VARI_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		ShowDialogItem(gSRConvDialog, V_RATE_FIELD);
		HideDialogItem(gSRConvDialog, V_VARI_BUTTON);
		HideDialogItem(gSRConvDialog, V_PITCH_RADIO);
		HideDialogItem(gSRConvDialog, V_SCALE_RADIO);
	}
	else
	{
		GetDialogItem(gSRConvDialog, V_VARI_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		HideDialogItem(gSRConvDialog, V_RATE_FIELD);
		ShowDialogItem(gSRConvDialog, V_VARI_BUTTON);
		ShowDialogItem(gSRConvDialog, V_PITCH_RADIO);
		ShowDialogItem(gSRConvDialog, V_SCALE_RADIO);
		gSRCI.newSRate = inSIPtr->sRate;
	}
	
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gSRConvDialog));
	SelectWindow(GetDialogWindow(gSRConvDialog));
#else
	ShowWindow(gSRConvDialog);
	SelectWindow(gSRConvDialog);
#endif
}

void
HandleSRConvDialogEvent(short itemHit)
{
	short	itemType, i;
	Rect	itemRect, dialogRect;
	Handle	itemHandle;
	Str255	errStr;
	double	inLength;
	WindInfo	*myWI;
	
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(GetDialogPort(gSRConvDialog), &dialogRect);
#else
	dialogRect = gSRConvDialog->portRect;
#endif
	inLength = inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize);
		switch(itemHit)
		{
			case V_BEST_RADIO:
				GetDialogItem(gSRConvDialog, V_BEST_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,ON);
				GetDialogItem(gSRConvDialog, V_MEDIUM_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				GetDialogItem(gSRConvDialog, V_FAST_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				gSRCI.quality = 16;
				break;
			case V_MEDIUM_RADIO:
				GetDialogItem(gSRConvDialog, V_BEST_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				GetDialogItem(gSRConvDialog, V_MEDIUM_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,ON);
				GetDialogItem(gSRConvDialog, V_FAST_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				gSRCI.quality = 4;
				break;
			case V_FAST_RADIO:
				GetDialogItem(gSRConvDialog, V_BEST_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				GetDialogItem(gSRConvDialog, V_MEDIUM_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				GetDialogItem(gSRConvDialog, V_FAST_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,ON);
				gSRCI.quality = 1;
				break;
			case V_PITCH_RADIO:
				GetDialogItem(gSRConvDialog, V_PITCH_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,ON);
				GetDialogItem(gSRConvDialog, V_SCALE_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				for(i = 0; i < 400; i++)
					gFunction[i] = 0.0;
				gSRCI.pitchScale = TRUE;
				break;
			case V_SCALE_RADIO:
				GetDialogItem(gSRConvDialog, V_PITCH_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				GetDialogItem(gSRConvDialog, V_SCALE_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,ON);
				for(i = 0; i < 400; i++)
					gFunction[i] = 1.0;
				gSRCI.pitchScale = FALSE;
				break;
			case V_RATE_FIELD:
				GetDialogItem(gSRConvDialog,V_RATE_FIELD, &itemType, &itemHandle, &itemRect);
				GetDialogItemText(itemHandle, errStr);
				StringToFix(errStr, &gSRCI.newSRate);
				break;
			case V_VARI_BOX:
				GetDialogItem(gSRConvDialog, V_VARI_BOX, &itemType, &itemHandle, &itemRect);
				if(GetControlValue((ControlHandle)itemHandle) == TRUE)
				{
					SetControlValue((ControlHandle)itemHandle,OFF);
					ShowDialogItem(gSRConvDialog, V_RATE_FIELD);
					HideDialogItem(gSRConvDialog, V_PITCH_RADIO);
					HideDialogItem(gSRConvDialog, V_SCALE_RADIO);
					HideDialogItem(gSRConvDialog, V_VARI_BUTTON);
					FixToString(gSRCI.newSRate, errStr);
					GetDialogItem(gSRConvDialog,V_RATE_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
					gSRCI.variSpeed = FALSE;
				} else
				{
					SetControlValue((ControlHandle)itemHandle,ON);
					HideDialogItem(gSRConvDialog, V_RATE_FIELD);
					ShowDialogItem(gSRConvDialog, V_VARI_BUTTON);
					ShowDialogItem(gSRConvDialog, V_PITCH_RADIO);
					ShowDialogItem(gSRConvDialog, V_SCALE_RADIO);
					gSRCI.newSRate = inSIPtr->sRate;
					gSRCI.variSpeed = TRUE;
				}
				break;
			case V_VARI_BUTTON:
				if(gSRCI.pitchScale)
					HandleDrawFunctionDialog(60.0, -60.0, 1.0, "\pSemitones:", inLength, FALSE);
				else
					HandleDrawFunctionDialog(32.0, 0.03125, 1.0, "\pScaling:", inLength, FALSE);
#if TARGET_API_MAC_CARBON == 1
				SetPort(GetDialogPort(gSRConvDialog));
#else
				SetPort((GrafPtr)gSRConvDialog);
#endif
				ClipRect(&dialogRect);
				break;
			case V_CANCEL_BUTTON:
				if(gProcessEnabled == DRAW_PROCESS)
				{
#if TARGET_API_MAC_CARBON == 1
					HideWindow(GetDialogWindow(gDrawFunctionDialog));
#else
					HideWindow(gDrawFunctionDialog);
#endif
					gProcessEnabled = NO_PROCESS;
				}
				gProcessDisabled = gProcessEnabled = NO_PROCESS;
#if TARGET_API_MAC_CARBON == 1
				myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gSRConvDialog));
#else
				myWI = (WindInfoPtr)GetWRefCon(gSRConvDialog);
#endif	
				RemovePtr((Ptr)myWI);
				DisposeDialog(gSRConvDialog);
				gSRConvDialog = nil;
				inSIPtr = nil;
				MenuUpdate();
				break;
			case V_PROCESS_BUTTON:
				if(gProcessEnabled == DRAW_PROCESS)
				{
#if TARGET_API_MAC_CARBON == 1
					HideWindow(GetDialogWindow(gDrawFunctionDialog));
#else
					HideWindow(gDrawFunctionDialog);
#endif
					gProcessEnabled = NO_PROCESS;
				}
#if TARGET_API_MAC_CARBON == 1
				myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gSRConvDialog));
#else
				myWI = (WindInfoPtr)GetWRefCon(gSRConvDialog);
#endif	
				RemovePtr((Ptr)myWI);
				DisposeDialog(gSRConvDialog);
				gSRConvDialog = nil;
				InitSRConvProcess();
				break;
		}
}

short
InitSRConvProcess(void)
{
	long	windowSize, sincPeriod, halfWindowSize, i;
	float	sum, length;
	long	tmpLong, numSampsRead;
	Str255	errStr, err2Str;
	

/* setup output file */
	outSIPtr = nil;
	outSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
	if(gPreferences.defaultType == 0)
	{
		outSIPtr->sfType = inSIPtr->sfType;
		outSIPtr->packMode = inSIPtr->packMode;
	}
	else
	{
		outSIPtr->sfType = gPreferences.defaultType;
		outSIPtr->packMode = gPreferences.defaultFormat;
	}
	if(gSRCI.variSpeed)
		NameFile(inSIPtr->sfSpec.name, "\pVRSP", outSIPtr->sfSpec.name);
	else
	{
		tmpLong = (long)(gSRCI.newSRate);
		NumToString(tmpLong, errStr);
		StringAppend("\pSR", errStr, err2Str);
		NameFile(inSIPtr->sfSpec.name, err2Str, outSIPtr->sfSpec.name);
	}
	outSIPtr->sRate = gSRCI.newSRate;
	outSIPtr->nChans = inSIPtr->nChans;
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
	gScaleDivisor = 0.95;
	SetOutputScale(outSIPtr->packMode);
	
	FixToString(gSRCI.newSRate, errStr);	
	StringAppend("\psample rate: ", errStr, err2Str);

	UpdateProcessWindow("\p", "\p", err2Str, 0.0);

	gNumberBlocks = 0;
		
/* 
 * initialize variables and calculate gSRCI.increment: if decimating, then 
 * sincBuffer is impulse response of low-pass filter with cutoff frequency at
 * half of outSIPtr->sRate; if interpolating, then sincBuffer is impulse 
 * response of lowpass filter with cutoff frequency at half of 
 * inSIPtr->sRate.
 */

	gSRCI.internalSRFactor = 120;
	gSRCI.increment = ((float) (gSRCI.internalSRFactor * inSIPtr->sRate) / outSIPtr->sRate);
	if (gSRCI.increment > gSRCI.internalSRFactor)
		sincPeriod = gSRCI.increment;
	else
		sincPeriod = 120;
	windowSize = gSRCI.quality * sincPeriod * 10 + 1;
	gSRCI.halfSampWinSize = (windowSize/2 - gSRCI.internalSRFactor) / gSRCI.internalSRFactor;
	gSRCI.maxCurSamp = BUFFERSIZE;
	gSRCI.curSampInBuf = HALFBUFFERSIZE;
	gSRCI.index = 0.0;
	gSRCI.offset = 0;
	gSRCI.numberSamplesIn = 0;

	if (gSRCI.variSpeed)
	{
		if(gSRCI.pitchScale)
		{
			gSRCI.increment = EExp2(gFunction[0]/12.0) * gSRCI.internalSRFactor;
			gSRCI.newSRate = EExp2(gFunction[0]/12.0) * inSIPtr->sRate;
			gSRCI.meanScale = 0;
			for(i=0;i<400;i++)
				gSRCI.meanScale += EExp2(gFunction[i]/12.0);
			gSRCI.meanScale *= 0.0025;
		}
		else
		{
			gSRCI.increment = gFunction[0] * gSRCI.internalSRFactor;
			gSRCI.newSRate = gFunction[0] * inSIPtr->sRate;
			gSRCI.meanScale = 0;
			for(i=0;i<400;i++)
				gSRCI.meanScale += gFunction[i];
			gSRCI.meanScale *= 0.0025;
		}
	}
	
/* Allocate Memory and clear buffers */
	inputL = outputL = inputR = outputR = sincBuffer = nil;

	inputL = (float *)NewPtr(BUFFERSIZE * sizeof(float));
	outputL = (float *)NewPtr(BUFFERSIZE * sizeof(float));
	
	ZeroFloatTable(inputL, BUFFERSIZE);
	ZeroFloatTable(outputL, BUFFERSIZE);
	
	if(inSIPtr->nChans == STEREO)
	{
		inputR = (float *)NewPtr(BUFFERSIZE * sizeof(float));
		ZeroFloatTable(inputR, BUFFERSIZE);
		outputR = (float *)NewPtr(BUFFERSIZE * sizeof(float));
		ZeroFloatTable(outputR, BUFFERSIZE);
	}
	else
	{
		inputR = inputL;
		outputR = outputL;
	}
	sincBuffer = (float *)NewPtr((windowSize+1) * sizeof(float));
	sincBufferPtr = (Ptr)sincBuffer;
	ZeroFloatTable(sincBuffer, windowSize+1);
	

/* make sincBuffer: the sincBuffer is the product of a kaiser and a sin(x)/x */

	KaiserWindow(sincBuffer, (long)windowSize);

	sincBuffer += (halfWindowSize = (windowSize-1)/2);		/* move pointer to center */

	for (i = 1; i <= halfWindowSize; i++) 
		*(sincBuffer + i) = *(sincBuffer - i) *= sincPeriod * sin((double) Pi*i/sincPeriod) / (Pi*i);
 
/* scale sincBuffer for unity gain */
	sum = *sincBuffer; /* center point */
	for (i = gSRCI.internalSRFactor-1; i <= halfWindowSize; i += gSRCI.internalSRFactor)
		sum += *(sincBuffer - i) + *(sincBuffer + i);	/* the rest of the points */

	sum = 1.0/sum;	

	*sincBuffer *= sum;
	for (i = 1; i <= halfWindowSize; i++)
		*(sincBuffer + i) = *(sincBuffer - i) *= sum;
	*(sincBuffer + halfWindowSize + 1) = 0.0;
      
	SetFPos(inSIPtr->dFileNum, fsFromStart, inSIPtr->dataStart);
	numSampsRead = ReadStereoBlock(inSIPtr, HALFBUFFERSIZE, inputL+HALFBUFFERSIZE, inputR+HALFBUFFERSIZE);
	gSRCI.numberSamplesIn = numSampsRead;
	length = (float)gSRCI.numberSamplesIn/inSIPtr->sRate;
	HMSTimeString(length, errStr);
	length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
	UpdateProcessWindow(errStr, "\p", "\p", length);

	gProcessEnabled = SRATE_PROCESS;
	gProcessDisabled = NO_PROCESS;
	MenuUpdate();
	return(TRUE);
}

short
SRConvBlock(void)
{
	float	indexModOne, interpWindow, length;
	long	outputIndex, intIndex, i, sincIndex, inputIndex, readSize;
	static Boolean endOfInFile;
	static long numSampsRead, numBytesIn, currentByte, inCounter, lastInSamp;
	Str255  errStr, err2Str;
	static long	sOutPos, sInPos;
	

	if(gNumberBlocks == 0L)
	{
		endOfInFile = FALSE;
		lastInSamp = 3000;
		sOutPos = outSIPtr->dataStart;
		sInPos = inSIPtr->dataStart + (HALFBUFFERSIZE * inSIPtr->nChans * inSIPtr->frameSize);
	}
	if(gStopProcess == TRUE)
	{
		DeAllocSRConvMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	if(endOfInFile == TRUE)
		gStopProcess = TRUE;

	SetFPos(outSIPtr->dFileNum, fsFromStart, sOutPos);
	
	
	for(outputIndex = 0; outputIndex<BUFFERSIZE; outputIndex++)
	{
		outputL[outputIndex] = 0.0;
		if(inSIPtr->nChans == STEREO)
			outputR[outputIndex] = 0.0;
		intIndex = (short)gSRCI.index;
		indexModOne = gSRCI.index - (float)intIndex;
		sincIndex = (-(gSRCI.halfSampWinSize + 1) * gSRCI.internalSRFactor) - intIndex;
		inputIndex = gSRCI.curSampInBuf - gSRCI.halfSampWinSize;
		if (inputIndex < 0)
			inputIndex += BUFFERSIZE;
		for (i = -gSRCI.halfSampWinSize; i <= gSRCI.halfSampWinSize+1; i++)
		{
			sincIndex += gSRCI.internalSRFactor;
			if (++inputIndex >= BUFFERSIZE)
				inputIndex = 0;
			interpWindow = *(sincBuffer+sincIndex) 
				+ indexModOne * (*(sincBuffer+sincIndex+1) - *(sincBuffer+sincIndex));
			outputL[outputIndex] += interpWindow * inputL[inputIndex];
			if(inSIPtr->nChans == STEREO)
				outputR[outputIndex] += interpWindow * inputR[inputIndex];
		}
		if(endOfInFile && lastInSamp <= gSRCI.curSampInBuf && (gSRCI.curSampInBuf - lastInSamp) < 4)
			break;
		
		/* move sincBuffer */
		gSRCI.index += gSRCI.increment;
		
		while (gSRCI.index >= (float)gSRCI.internalSRFactor)
		{
			gSRCI.index -= (float)gSRCI.internalSRFactor;
			gSRCI.curSampInBuf++;
			inCounter++;
			currentByte = (long)(inSIPtr->nChans * inCounter * inSIPtr->frameSize);
			// check to see if we need more samples
			if (gSRCI.curSampInBuf + gSRCI.halfSampWinSize + 1 >= gSRCI.maxCurSamp)
			{
				inCounter = 0;
				currentByte = 0;
				gSRCI.maxCurSamp += HALFBUFFERSIZE;
				if (gSRCI.offset >= BUFFERSIZE)
					gSRCI.offset = 0;
				
				if(gSRCI.offset > HALFBUFFERSIZE)
					readSize = BUFFERSIZE - gSRCI.offset;
				else
					readSize = HALFBUFFERSIZE;
				SetFPos(inSIPtr->dFileNum, fsFromStart, sInPos);
				if(endOfInFile)
					numSampsRead = 0L;
				else
					numSampsRead = ReadStereoBlock(inSIPtr, readSize, inputL+gSRCI.offset, inputR+gSRCI.offset);
				gSRCI.offset += numSampsRead;
				GetFPos(inSIPtr->dFileNum, &sInPos);
				
				gSRCI.numberSamplesIn += numSampsRead;
				
				numBytesIn = (long)(inSIPtr->nChans * gSRCI.numberSamplesIn * inSIPtr->frameSize);

				length = (float)gSRCI.numberSamplesIn/inSIPtr->sRate;
				HMSTimeString(length, errStr);
				length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
				UpdateProcessWindow(errStr, "\p", "\p", length);
				
				if(numSampsRead != readSize)
				{
					if(endOfInFile == FALSE)
					{
						lastInSamp = gSRCI.offset;
						if(lastInSamp >= BUFFERSIZE)
							lastInSamp -= BUFFERSIZE;
					}
					endOfInFile = TRUE;
				}

				if (numSampsRead < HALFBUFFERSIZE)
					for (; numSampsRead < HALFBUFFERSIZE; numSampsRead++)
					{
						*(inputL+gSRCI.offset) = 0.;
						if(inSIPtr->nChans == STEREO)
							*(inputR+gSRCI.offset) = 0.;
						gSRCI.offset++;
					}
			}
			if (gSRCI.curSampInBuf >= BUFFERSIZE)
			{
				gSRCI.curSampInBuf = 0;
				gSRCI.maxCurSamp = HALFBUFFERSIZE;
			}
		}
		
// time-varying - recompute gSRCI.increment- interpolate on gFunction

		if(gSRCI.variSpeed)
		{
			if(gSRCI.pitchScale)
				gSRCI.increment = EExp2(InterpFunctionValue((outSIPtr->numBytes+(outputIndex*outSIPtr->frameSize*outSIPtr->nChans)) * gSRCI.meanScale, FALSE)/12.0) * gSRCI.internalSRFactor;
			else
				gSRCI.increment = InterpFunctionValue((outSIPtr->numBytes+(outputIndex*outSIPtr->frameSize*outSIPtr->nChans)) * gSRCI.meanScale, FALSE) * gSRCI.internalSRFactor;
		}
	}
	
	if(outputIndex != BUFFERSIZE)
		outputIndex -= 2;			// 1 past the output
	if(outputIndex > 0)
	{
		if(outSIPtr->nChans == STEREO)
			WriteStereoBlock(outSIPtr, outputIndex, outputL, outputR);
		else
			WriteMonoBlock(outSIPtr, outputIndex, outputL);
	}
		
	length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
	HMSTimeString(length, errStr);
	UpdateProcessWindow("\p", errStr, "\p", -1.0);
	gNumberBlocks++;
	GetFPos(outSIPtr->dFileNum, &sOutPos);
	
	if(gSRCI.variSpeed)
	{
		if(gSRCI.pitchScale)
			gSRCI.newSRate = EExp2(InterpFunctionValue((outSIPtr->numBytes+(outputIndex*outSIPtr->frameSize*outSIPtr->nChans)) * gSRCI.meanScale, FALSE)/12.0) * inSIPtr->sRate;
		else
			gSRCI.newSRate = InterpFunctionValue((outSIPtr->numBytes+(outputIndex*outSIPtr->frameSize*outSIPtr->nChans)) * gSRCI.meanScale, FALSE) * inSIPtr->sRate;
	}
	FixToString(gSRCI.newSRate,errStr);
	StringAppend("\psample rate: ", errStr, err2Str);
	UpdateProcessWindow("\p", "\p", err2Str, -1.0);
	return(TRUE);
}

void		
DeAllocSRConvMem(void)
{
	RemovePtr((Ptr)inputL);
	RemovePtr((Ptr)outputL);
	RemovePtr((Ptr)sincBufferPtr);
	if(inSIPtr->nChans == STEREO)
	{
		RemovePtr((Ptr)inputR);
		RemovePtr((Ptr)outputR);
	}
	inputL = outputL = inputR = outputR = sincBuffer = nil;
}