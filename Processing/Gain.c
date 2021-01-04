#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>

//#include <AppleEvents.h>
#include "SoundFile.h"
#include "Dialog.h"
#include "Menu.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "SoundHack.h"
#include "Gain.h"
#include "Misc.h"
#include "CarbonGlue.h"

extern long			gNumberBlocks;
extern MenuHandle	gAppleMenu, gFileMenu, gEditMenu, gProcessMenu, gControlMenu,
					gSigDispMenu, gSpectDispMenu, gSonoDispMenu;
DialogPtr	gGainDialog;
extern SoundInfoPtr	inSIPtr, outSIPtr;

extern short			gProcessEnabled, gProcessDisabled, gStopProcess;
extern float		gScale, gScaleL, gScaleR, gScaleDivisor;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

float				*blockL, *blockR, *block3, *block4;
GainInfo			gGI;

void
HandleGainDialog(void)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	WindInfo	*theWindInfo;

	gGainDialog = GetNewDialog(GAIN_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)GAIN_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gGainDialog), (long)theWindInfo);
#else
	SetWRefCon(gGainDialog, (long)theWindInfo);
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
	
	// Initialize parameters
	gGI.peakCh1 = inSIPtr->peakFL;
	gGI.peakCh2 = inSIPtr->peakFR;
	gGI.factor1 = 1.0/gGI.peakCh1;
	gGI.factor2 = 1.0/gGI.peakCh2;
	gNumberBlocks = 0;
	GetDialogItem(gGainDialog, G_DB_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, ON);
	GetDialogItem(gGainDialog, G_FIXED_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, OFF);
	ShowGainDB();
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gGainDialog));
	SelectWindow(GetDialogWindow(gGainDialog));
#else
	ShowWindow(gGainDialog);
	SelectWindow(gGainDialog);
#endif
}

void
HandleGainDialogEvent(short itemHit)
{
	static short	dBSet = TRUE;
	double	tmpFactor;
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	Str255	factor1Str, factor2Str;
	WindInfo	*myWI;
	switch(itemHit)
	{
		case G_DB_RADIO:
			dBSet = TRUE;
			GetDialogItem(gGainDialog, G_DB_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, ON);
			GetDialogItem(gGainDialog, G_FIXED_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, OFF);
			ShowGainDB();
			break;
		case G_FIXED_RADIO:
			dBSet = FALSE;
			GetDialogItem(gGainDialog, G_DB_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, OFF);
			GetDialogItem(gGainDialog, G_FIXED_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, ON);
			ShowGainFixed();
			break;
		case G_ANALYZE_BUTTON:
			GainAnalyze();
			gGI.factor1 = 1.0/gGI.peakCh1;
			gGI.factor2 = 1.0/gGI.peakCh2;
			gGI.addOffsetCh1 = -gGI.offsetCh1;
			gGI.addOffsetCh2 = -gGI.offsetCh2;
			if(dBSet == TRUE)
				ShowGainDB();
			else
				ShowGainFixed();
			break;
		case G_CANCEL_BUTTON:
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gGainDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gGainDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gGainDialog);
			gGainDialog = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			inSIPtr = nil;
			MenuUpdate();
			break;
		case G_PROCESS_BUTTON:
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gGainDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gGainDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gGainDialog);
			gGainDialog = nil;
			InitGainProcess(gGI.factor1, gGI.factor2);
			break;
		case GAIN_FACT_1_FIELD:
			GetDialogItem(gGainDialog,GAIN_FACT_1_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, factor1Str);
			StringToFix(factor1Str, &tmpFactor);
			if(dBSet == TRUE)
				gGI.factor1 = powf(10.0, (tmpFactor/20.0));
			else
				gGI.factor1 = tmpFactor;
			break;
		case GAIN_FACT_2_FIELD:
			GetDialogItem(gGainDialog,GAIN_FACT_2_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, factor2Str);
			StringToFix(factor2Str, &tmpFactor);
			if(dBSet == TRUE)
				gGI.factor2 = powf(10.0, (tmpFactor/20.0));
			else
				gGI.factor2 = tmpFactor;
			break;
		case ADD_OFFSET_1_FIELD:
			GetDialogItem(gGainDialog,ADD_OFFSET_1_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, factor1Str);
			StringToFix(factor1Str, &tmpFactor);
			gGI.addOffsetCh1 = tmpFactor;
			break;
		case ADD_OFFSET_2_FIELD:
			GetDialogItem(gGainDialog,ADD_OFFSET_2_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, factor2Str);
			StringToFix(factor2Str, &tmpFactor);
			gGI.addOffsetCh2 = tmpFactor;
			break;
	}
}

void
ShowGainDB(void)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	float	dbValue;
	Str255	tmpStr;

	dbValue = 20.0 * log10(gGI.peakCh1);
	FixToString(dbValue, tmpStr);
	GetDialogItem(gGainDialog, PEAK_VAL_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	NumToString(gGI.peakPosCh1, tmpStr); 
	GetDialogItem(gGainDialog, PEAK_POS_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	dbValue = 20.0 * log10(gGI.rmsCh1);
	FixToString(dbValue, tmpStr);
	GetDialogItem(gGainDialog, RMS_VAL_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	dbValue = 20.0 * log10(gGI.factor1);
	FixToString(dbValue, tmpStr);
	GetDialogItem(gGainDialog, GAIN_FACT_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	dbValue = 20.0 * log10(gGI.peakCh2);
	FixToString(dbValue, tmpStr);
	GetDialogItem(gGainDialog, PEAK_VAL_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	NumToString(gGI.peakPosCh2, tmpStr);
	GetDialogItem(gGainDialog, PEAK_POS_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	dbValue = 20.0 * log10(gGI.rmsCh2);
	FixToString(dbValue, tmpStr);
	GetDialogItem(gGainDialog, RMS_VAL_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	dbValue = 20.0 * log10(gGI.factor2);
	FixToString(dbValue, tmpStr);
	GetDialogItem(gGainDialog, GAIN_FACT_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	FixToString(gGI.offsetCh1, tmpStr);
	GetDialogItem(gGainDialog, OFFSET_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	FixToString(gGI.offsetCh2, tmpStr);
	GetDialogItem(gGainDialog, OFFSET_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	FixToString(gGI.addOffsetCh1, tmpStr);
	GetDialogItem(gGainDialog, ADD_OFFSET_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	FixToString(gGI.addOffsetCh2, tmpStr);
	GetDialogItem(gGainDialog, ADD_OFFSET_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
}

void
ShowGainFixed(void)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	Str255	factor1Str, factor2Str, peak1Str, peak2Str, peakPos1Str, peakPos2Str,
			rms1Str, rms2Str,offset1Str, offset2Str, addOffset1Str, addOffset2Str;

	FixToString(gGI.factor1,factor1Str); 
	FixToString(gGI.factor2,factor2Str); 
	FixToString(gGI.offsetCh1,offset1Str); 
	FixToString(gGI.offsetCh2,offset2Str); 
	FixToString(gGI.addOffsetCh1,addOffset1Str); 
	FixToString(gGI.addOffsetCh2,addOffset2Str); 
	FixToString(gGI.peakCh1,peak1Str);
	FixToString(gGI.peakCh2,peak2Str);
	NumToString(gGI.peakPosCh1,peakPos1Str); 
	NumToString(gGI.peakPosCh2,peakPos2Str);
	FixToString(gGI.rmsCh1,rms1Str);
	FixToString(gGI.rmsCh2,rms2Str);

	GetDialogItem(gGainDialog, OFFSET_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, offset1Str);
	GetDialogItem(gGainDialog, OFFSET_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, offset2Str);
	GetDialogItem(gGainDialog, ADD_OFFSET_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, addOffset1Str);
	GetDialogItem(gGainDialog, ADD_OFFSET_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, addOffset2Str);
	GetDialogItem(gGainDialog, PEAK_VAL_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, peak1Str);
	GetDialogItem(gGainDialog, PEAK_POS_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, peakPos1Str);
	GetDialogItem(gGainDialog, RMS_VAL_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, rms1Str);
	GetDialogItem(gGainDialog, GAIN_FACT_1_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, factor1Str);
	GetDialogItem(gGainDialog, PEAK_VAL_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, peak2Str);
	GetDialogItem(gGainDialog, PEAK_POS_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, peakPos2Str);
	GetDialogItem(gGainDialog, RMS_VAL_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, rms2Str);
	GetDialogItem(gGainDialog, GAIN_FACT_2_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, factor2Str);
}

short
InitGainProcess(double gainL, double gainR)
{	
	long	blockSize;
	
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
	NameFile(inSIPtr->sfSpec.name, "\pGAIN", outSIPtr->sfSpec.name);
	outSIPtr->sRate = inSIPtr->sRate;
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
	if(outSIPtr->packMode == SF_FORMAT_8_LINEAR)
	{
		gScale = gScaleL = 128.0 * gainL;
		gScaleR = 128.0 * gainR;
	}
	else if(outSIPtr->packMode == SF_FORMAT_16_LINEAR)
	{
		gScale = gScaleL = 32768.0 * gainL;
		gScaleR = 32768.0 * gainR;
	}
	else if(outSIPtr->packMode == SF_FORMAT_24_LINEAR || outSIPtr->packMode == SF_FORMAT_24_COMP 
		|| outSIPtr->packMode == SF_FORMAT_32_LINEAR || outSIPtr->packMode == SF_FORMAT_32_COMP
		|| outSIPtr->packMode == SF_FORMAT_24_SWAP || outSIPtr->packMode == SF_FORMAT_32_SWAP)
	{
		gScale = gScaleL = 2147483648.0 * gainL;
		gScaleR = 2147483648.0 * gainR;
	}
	WriteHeader(outSIPtr);
	UpdateInfoWindow(outSIPtr);
	
	blockSize = 16384L;
	blockL = blockR = nil;

	blockL = (float *)NewPtr(blockSize * sizeof(float));
	blockR = (float *)NewPtr(blockSize * sizeof(float));
	
	UpdateProcessWindow("\p", "\p", "\pchanging gain", 0.0);
	
	SetFPos(inSIPtr->dFileNum, fsFromStart, inSIPtr->dataStart);
	
	gProcessEnabled = GAIN_PROCESS;
	gProcessDisabled = NO_PROCESS;
	MenuUpdate();
	return(TRUE);
}

short
GainBlock(void)
{
	long	readSize, writeSize, blockSize, i;
	Str255	errStr;
	float	length;
	
	if(gStopProcess == TRUE)
	{
		RemovePtr((Ptr)blockL);
		RemovePtr((Ptr)blockR);
		blockL = blockR = nil;
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	blockSize = 16384L;
	SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart 
		+ (blockSize * inSIPtr->nChans * inSIPtr->frameSize * gNumberBlocks)));
	if(outSIPtr->packMode != SF_FORMAT_TEXT)
		SetFPos(outSIPtr->dFileNum, fsFromStart, (outSIPtr->dataStart 
			+ (blockSize * outSIPtr->nChans * outSIPtr->frameSize * gNumberBlocks)));
	if(inSIPtr->nChans == 1)
	{
		readSize = ReadMonoBlock(inSIPtr, blockSize, blockL);
		length = (readSize + (gNumberBlocks * blockSize))/inSIPtr->sRate;
		HMSTimeString(length, errStr);
		length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
		UpdateProcessWindow(errStr, "\p", "\pchanging gain", length);
		
		if(gGI.addOffsetCh1 != 0.0)
			for(i=0; i < readSize; i++)
				blockL[i] += gGI.addOffsetCh1;
				
		writeSize = WriteMonoBlock(outSIPtr, readSize, blockL);
	} else
	{
		readSize = ReadStereoBlock(inSIPtr, blockSize, blockL, blockR);
		length = (readSize + (gNumberBlocks * blockSize))/inSIPtr->sRate;
		HMSTimeString(length, errStr);
		length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
		UpdateProcessWindow(errStr, "\p", "\pchanging gain", length);
		
		if(gGI.addOffsetCh1 != 0.0)
			for(i=0; i < readSize; i++)
				blockL[i] += gGI.addOffsetCh1;
				
		
		if(gGI.addOffsetCh2 != 0.0)
			for(i=0; i < readSize; i++)
				blockR[i] += gGI.addOffsetCh2;
				
		writeSize = WriteStereoBlock(outSIPtr, readSize, blockL, blockR);
	}
	length = (float)(outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
	HMSTimeString(length, errStr);
	UpdateProcessWindow("\p", errStr, "\p", -1.0);
	gNumberBlocks++;
	if(writeSize != blockSize)
	{
		RemovePtr((Ptr)blockL);
		RemovePtr((Ptr)blockR);
		blockL = blockR = nil;
		FinishProcess();
		gNumberBlocks = 0;
	}
	return(TRUE);
}

void
GainAnalyze(void)
{
	long	numSamples, readSize, n, numFrames, frameLength = 0;
	float	rmsTotL = 0.0, rmsTotR = 0.0, length = 0.0, totalL = 0.0, totalR = 0.0;
	Str255	errStr;
	
	gProcessEnabled = FUDGE_PROCESS;
	gGI.peakCh1 = 0.0;
	gGI.peakCh2 = 0.0;
	numSamples = 8192L;
	readSize = numSamples;
	
	blockL = blockR = nil;

	blockL = (float *)NewPtr(numSamples * sizeof(float));
	if(inSIPtr->nChans == STEREO)
		blockR = (float *)NewPtr(numSamples * sizeof(float));
	
/*	Bring Up Window for Processing Time 	*/	
	UpdateProcessWindow("\p", "\p", "\plooking for peak", 0.0);

	for(gNumberBlocks = 0; readSize == numSamples; gNumberBlocks++)
	{
		switch(inSIPtr->nChans)
		{
			case MONO:
				SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart 
					+ (numSamples * inSIPtr->frameSize * gNumberBlocks)));
				readSize = ReadMonoBlock(inSIPtr, numSamples, blockL);
				for(n = 0; n < readSize; n++)
				{
					if(gGI.peakCh1 < *(blockL + n))
					{
						gGI.peakCh1 = *(blockL + n);
						gGI.peakPosCh1 = (gNumberBlocks*numSamples) + n;
					}else if(gGI.peakCh1 < -(*(blockL + n)))
					{
						gGI.peakCh1 = -(*(blockL + n));
						gGI.peakPosCh1 = (gNumberBlocks*numSamples) + n;
					}
					rmsTotL += (*(blockL + n) * *(blockL + n));
					totalL += *(blockL + n);
				}
				break;
			case STEREO:
				SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart 
					+ (numSamples * inSIPtr->frameSize * inSIPtr->nChans
					* gNumberBlocks)));
				readSize = ReadStereoBlock(inSIPtr, numSamples, blockL, blockR);
				for(n = 0; n < readSize; n++)
				{
					if(gGI.peakCh1 < *(blockL + n))
					{
						gGI.peakCh1 = *(blockL + n);
						gGI.peakPosCh1 = (gNumberBlocks*numSamples) + n;
					}else if(gGI.peakCh1 < -(*(blockL + n)))
					{
						gGI.peakCh1 = -(*(blockL + n));
						gGI.peakPosCh1 = (gNumberBlocks*numSamples) + n;
					}
					if(gGI.peakCh2 < *(blockR + n))
					{
						gGI.peakCh2 = *(blockR + n);
						gGI.peakPosCh2 = (gNumberBlocks*numSamples) + n;
					}else if(gGI.peakCh2 < -(*(blockR + n)))
					{
						gGI.peakCh2 = -(*(blockR + n));
						gGI.peakPosCh2 = (gNumberBlocks*numSamples) + n;
					}
					rmsTotL = rmsTotL + (*(blockL + n) * *(blockL + n));
					rmsTotR = rmsTotR + (*(blockR + n) * *(blockR + n));
					totalL += *(blockL + n);
					totalR += *(blockR + n);
				}
				break;
		}
		frameLength = frameLength + readSize;
		length = frameLength/inSIPtr->sRate;
		HMSTimeString(length, errStr);
		length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
		UpdateProcessWindow(errStr, "\p", "\p", length);
	}
	numFrames = inSIPtr->numBytes/(inSIPtr->nChans*inSIPtr->frameSize);
	gGI.rmsCh1 = powf((rmsTotL/numFrames),0.5);
	gGI.offsetCh1 = totalL/numFrames;
	if(inSIPtr->nChans == STEREO)
	{
		gGI.rmsCh2 = powf((rmsTotR/numFrames),0.5);
		gGI.offsetCh2 = totalR/numFrames;
	}
	gNumberBlocks = 0;
	RemovePtr((Ptr)blockL);
	if(inSIPtr->nChans == STEREO)
		RemovePtr((Ptr)blockR);
	UpdateProcessWindow("\p00:00:00.000", "\p00:00:00.000", "\pidle", 0.0);
	inSIPtr->peakFL = gGI.peakCh1;
	inSIPtr->peakFR = gGI.peakCh2;
	if(inSIPtr->nChans == MONO)
		inSIPtr->peak = gGI.peakCh1;
	else if(gGI.peakCh1 > gGI.peakCh2)
		inSIPtr->peak = gGI.peakCh1;
	else
		inSIPtr->peak = gGI.peakCh2;
	WriteHeader(inSIPtr);
	gProcessEnabled = NO_PROCESS;
}