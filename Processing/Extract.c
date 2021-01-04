/* 
 *	SoundHack - soon to be many things to many people
 *	Copyright �1995 Tom Erbe - California Institute of the Arts
 *
 *	Extract.c - spectral rate separation
 */
 
#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>

//#include <AppleEvents.h>
#include "SoundFile.h"
#include "Dialog.h"
#include "Menu.h"
#include "SoundHack.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "PhaseVocoder.h"
#include "Extract.h"
#include "Analysis.h"
#include "SpectFileIO.h"
#include "Misc.h"
#include "FFT.h"
#include "Windows.h"
#include "ShowSpect.h"
#include "ShowSono.h"
#include "ShowWave.h"
#include "CarbonGlue.h"

/* Globals */

DialogPtr	gExtractDialog;
extern MenuHandle	gAppleMenu, gFileMenu, gEditMenu, gProcessMenu, gControlMenu,
					gBandsMenu, gExtractMenu, gSigDispMenu, gSpectDispMenu, gSonoDispMenu;
extern SoundInfoPtr	inSIPtr, outSteadSIPtr, outTransSIPtr;

extern Boolean		gNormalize;
extern short			gProcessEnabled, gProcessDisabled, gStopProcess, gProcNChans;
extern float 		Pi, twoPi, gScale, gScaleL, gScaleR, gScaleDivisor;
extern long			gNumberBlocks, gInPosition;
extern long			gInPointer, gOutPointer;
extern float 		*analysisWindow, *synthesisWindow, *inputL, *inputR, *spectrum,
					*displaySpectrum, *polarSpectrum, *lastPhaseInL,
					*lastPhaseInR;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;
float			*outputTransL, *outputTransR, *outputSteadL, *outputSteadR, *transSpectrum,
				*steadSpectrum, *spectralFramesL, *spectralFramesR;
ExtractInfo		gEI;
PvocInfo		gExtPI;

void
HandleExtractDialog(void)
{
	WindInfo *theWindInfo;
	gExtractDialog = GetNewDialog(EXTRACT_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)EXTRACT_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gExtractDialog), (long)theWindInfo);
#else
	SetWRefCon(gExtractDialog, (long)theWindInfo);
#endif

	gProcessDisabled = UTIL_PROCESS;
	// Rename Display MENUs
	SetMenuItemText(gSigDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSpectDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSonoDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSigDispMenu, DISP_AUX_ITEM, "\pSteady");
	SetMenuItemText(gSpectDispMenu, DISP_AUX_ITEM, "\pSteady");
	SetMenuItemText(gSonoDispMenu, DISP_AUX_ITEM, "\pSteady");
	SetMenuItemText(gSigDispMenu, DISP_OUTPUT_ITEM, "\pTransient");
	SetMenuItemText(gSpectDispMenu, DISP_OUTPUT_ITEM, "\pTransient");
	SetMenuItemText(gSonoDispMenu, DISP_OUTPUT_ITEM, "\pTransient");
	
	EnableItem(gSigDispMenu, DISP_INPUT_ITEM);
	EnableItem(gSpectDispMenu, DISP_INPUT_ITEM);
	EnableItem(gSonoDispMenu, DISP_INPUT_ITEM);
	EnableItem(gSigDispMenu, DISP_AUX_ITEM);
	EnableItem(gSpectDispMenu, DISP_AUX_ITEM);
	EnableItem(gSonoDispMenu, DISP_AUX_ITEM);
	EnableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
	EnableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
	EnableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	MenuUpdate();
	
	SetExtractDefaults();
	
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gExtractDialog));
	SelectWindow(GetDialogWindow(gExtractDialog));
#else
	ShowWindow(gExtractDialog);
	SelectWindow(gExtractDialog);
#endif
}

void
HandleExtractDialogEvent(short itemHit)
{
	double	tmpFloat;
	short	itemType;
	static short	choiceBand = 4, choiceFrames = 3;
	WindInfo	*myWI;
	
	Handle	itemHandle;
	Rect	itemRect;
	Str255	errStr, tmpStr;
	switch(itemHit)
	{
		case EX_BANDS_ITEM:
			GetDialogItem(gExtractDialog, EX_BANDS_ITEM, &itemType, &itemHandle, &itemRect);
			choiceBand = GetControlValue((ControlHandle)itemHandle);
			gExtPI.points = 2<<(13 - choiceBand);
			gExtPI.interpolation = gExtPI.decimation = gExtPI.points>>3;
			tmpFloat = (gEI.frames * gExtPI.decimation)/inSIPtr->sRate;
			FixToString(tmpFloat,tmpStr);
			GetDialogItem(gExtractDialog, EX_RATE_TEXT, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			break;
		case EX_FRAMES_ITEM:
			GetDialogItem(gExtractDialog, EX_FRAMES_ITEM, &itemType, &itemHandle, &itemRect);
			choiceFrames = GetControlValue((ControlHandle)itemHandle);
			gEI.frames = choiceFrames + 3;
			tmpFloat = (gEI.frames * gExtPI.decimation)/inSIPtr->sRate;
			FixToString(tmpFloat,tmpStr);
			GetDialogItem(gExtractDialog, EX_RATE_TEXT, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			break;
		case EX_TRANS_DEV_FIELD:
			GetDialogItem(gExtractDialog,EX_TRANS_DEV_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, errStr);
			StringToFix(errStr, &tmpFloat);
			if(tmpFloat < 0.001)
				tmpFloat = 0.001;
			gEI.transDev = tmpFloat;
			break;
		case EX_STEAD_DEV_FIELD:
			GetDialogItem(gExtractDialog,EX_STEAD_DEV_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, errStr);
			StringToFix(errStr, &tmpFloat);
			if(tmpFloat < 0.001)
				tmpFloat = 0.001;
			gEI.steadDev = tmpFloat;
			break;
		case EX_CANCEL_BUTTON:
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gExtractDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gExtractDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gExtractDialog);
			gExtractDialog = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			inSIPtr = nil;
			MenuUpdate();
			break;
		case EX_PROCESS_BUTTON:
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gExtractDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gExtractDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gExtractDialog);
			gExtractDialog = nil;
			InitExtractProcess();
			break;
	}
}

/*
 *	This is called from HandleExtractDialog() 
 *	Then process impulse H[] looking for spectral average.
 *	Then open outSoundFile, set Extract_PROCESS and let PvocBlock() take care
 *	of the rest
 */
short
InitExtractProcess(void)
{
	gExtPI.halfPoints = gExtPI.points>>1;
	gExtPI.windowSize = gExtPI.points;
	gExtPI.decimation = gExtPI.interpolation = gExtPI.points >> 3;
	
	outSteadSIPtr = nil;
	outSteadSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
	if(gPreferences.defaultType == 0)
	{
		if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
		{
			outSteadSIPtr->sfType = AIFF;
			outSteadSIPtr->packMode = SF_FORMAT_16_LINEAR;
		}
		else
		{
			outSteadSIPtr->sfType = inSIPtr->sfType;
			outSteadSIPtr->packMode = inSIPtr->packMode;
		}
	}
	else
	{
		outSteadSIPtr->sfType = gPreferences.defaultType;
		outSteadSIPtr->packMode = gPreferences.defaultFormat;
	}
	NameFile(inSIPtr->sfSpec.name, "\pSTDY", outSteadSIPtr->sfSpec.name);

	outSteadSIPtr->sRate = inSIPtr->sRate;
	outSteadSIPtr->nChans = inSIPtr->nChans;
	outSteadSIPtr->numBytes = 0;
	if(CreateSoundFile(&outSteadSIPtr, SOUND_CUST_DIALOG) == -1)
	{
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		RemovePtr((Ptr)outSteadSIPtr);
		outSteadSIPtr = nil;
		outTransSIPtr = nil;
		MenuUpdate();
		return(-1);
	}
	WriteHeader(outSteadSIPtr);
	UpdateInfoWindow(outSteadSIPtr);

	outTransSIPtr = nil;
	outTransSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
	NameFile(inSIPtr->sfSpec.name, "\pTRNS", outTransSIPtr->sfSpec.name);

	outTransSIPtr->packMode = outSteadSIPtr->packMode;
	outTransSIPtr->frameSize = outSteadSIPtr->frameSize;
	outTransSIPtr->sfType = outSteadSIPtr->sfType;
	outTransSIPtr->sRate = inSIPtr->sRate;
	outTransSIPtr->nChans = inSIPtr->nChans;
	outTransSIPtr->numBytes = 0;
	if(CreateSoundFile(&outTransSIPtr, SOUND_CUST_DIALOG) == -1)
	{
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		RemovePtr((Ptr)outSteadSIPtr);
		RemovePtr((Ptr)outTransSIPtr);
		outTransSIPtr = nil;
		outSteadSIPtr = nil;
		return(-1);
	}

	WriteHeader(outTransSIPtr);
	UpdateInfoWindow(outTransSIPtr);

	SetOutputScale(outTransSIPtr->packMode);
    
/*
 * allocate memory
 */
	analysisWindow = synthesisWindow = inputL = spectrum = polarSpectrum = nil;
	displaySpectrum = spectralFramesL = outputTransL = transSpectrum = outputSteadL = nil;
	steadSpectrum = inputR = spectralFramesR = outputTransR = outputSteadR = nil;

	analysisWindow = (float *)NewPtr(gExtPI.windowSize*sizeof(float));
	synthesisWindow = (float *)NewPtr(gExtPI.windowSize*sizeof(float));
	inputL = (float *)NewPtr(gExtPI.windowSize*sizeof(float));
	spectrum = (float *)NewPtr(gExtPI.points*sizeof(float));
	polarSpectrum = (float *)NewPtr((gExtPI.points+2)*sizeof(float));		/* analysis channels */
	displaySpectrum = (float *)NewPtr((gExtPI.halfPoints+1)*sizeof(float));	/* For display of amplitude */
	spectralFramesL = (float *)NewPtr((gEI.frames)*(gExtPI.points+2)*sizeof(float));
	outputTransL = (float *)NewPtr(gExtPI.windowSize*sizeof(float));
	transSpectrum = (float *)NewPtr((gExtPI.points+2)*sizeof(float));		/* analysis channels */
	outputSteadL = (float *)NewPtr(gExtPI.windowSize*sizeof(float));
	steadSpectrum = (float *)NewPtr((gExtPI.points+2)*sizeof(float));		/* analysis channels */
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
	{
		lastPhaseInL = (float *)NewPtr((gExtPI.halfPoints+1) * sizeof(float));
		ZeroFloatTable(lastPhaseInL, (gExtPI.halfPoints+1));
		if(inSIPtr->nChans == STEREO)
		{
			lastPhaseInR = (float *)NewPtr((gExtPI.halfPoints+1) * sizeof(float));
			ZeroFloatTable(lastPhaseInR, (gExtPI.halfPoints+1));
		}
	}
	if(inSIPtr->nChans == STEREO)
	{
		inputR = (float *)NewPtr(gExtPI.windowSize*sizeof(float));
		spectralFramesR = (float *)NewPtr((gEI.frames)*(gExtPI.points+2)*sizeof(float));
		outputTransR = (float *)NewPtr(gExtPI.windowSize*sizeof(float));
		outputSteadR = (float *)NewPtr(gExtPI.windowSize*sizeof(float));
	}
	else
	{
		inputR = inputL;
		spectralFramesR = spectralFramesL;
		outputTransR = outputTransL;
		outputSteadR = outputSteadL;
	}
	
	if(spectralFramesR == 0)
	{
		DrawErrorMessage("\pNot enough memory for processing");
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		DeAllocExtractMem();
		outTransSIPtr = nil;
		outSteadSIPtr = nil;
		return(-1);
	}
	ZeroFloatTable(spectrum, gExtPI.points);
	ZeroFloatTable(polarSpectrum, (gExtPI.points+2));
	ZeroFloatTable(inputL, gExtPI.windowSize);
	ZeroFloatTable(spectralFramesL, gEI.frames * (gExtPI.points+2));
	ZeroFloatTable(outputTransL, gExtPI.windowSize);
	ZeroFloatTable(outputSteadL, gExtPI.windowSize);
	if(inSIPtr->nChans == STEREO)
	{
		ZeroFloatTable(inputR, gExtPI.windowSize);
		ZeroFloatTable(spectralFramesR, gEI.frames * (gExtPI.points+2));
		ZeroFloatTable(outputTransR, gExtPI.windowSize);
		ZeroFloatTable(outputSteadR, gExtPI.windowSize);
	}

/*
 * create windows
 */
 	HammingWindow(analysisWindow, gExtPI.windowSize);
 	HammingWindow(synthesisWindow, gExtPI.windowSize);
/*
 *	Scale hamming windows for balanced pvoc processing
 */
 
    ScaleWindows(analysisWindow, synthesisWindow, gExtPI);
    
	
/*
 * initialize input and output time values (in samples)
 */
    gInPointer = -gExtPI.windowSize;
	gOutPointer = (gInPointer*gExtPI.interpolation)/gExtPI.decimation;
	gInPosition = 0;
	
	gNumberBlocks = 0;	
	gProcessDisabled = NO_PROCESS;
	gProcessEnabled = EXTRACT_PROCESS;
	MenuUpdate();
	UpdateProcessWindow("\p", "\p", "\pspectral extraction processing", 0.0);
	return(TRUE);
}

/*
 * main loop--perform phase vocoder analysis-resynthesis
 */
short
ExtractBlock(void)
{
    short	curMark;
	long	numSampsRead;
	float 	length;
	Str255	errStr;
	static long validSamples = -1L;
	static long	sOutSPos, sOutTPos;
	double seconds;
	

	if(gStopProcess == TRUE)
	{
		DeAllocExtractMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	if(gNumberBlocks == 0)
	{
		sOutTPos = outTransSIPtr->dataStart;
		sOutSPos = outSteadSIPtr->dataStart;
	}

	SetFPos(outTransSIPtr->dFileNum, fsFromStart, sOutTPos);
	SetFPos(outSteadSIPtr->dFileNum, fsFromStart, sOutSPos);
	

/*
 * increment times
 */
	gInPointer += gExtPI.decimation;
	gOutPointer += gExtPI.interpolation;
	
/*
 * analysis: input gExtPI.decimation samples; window, fold and rotate input
 * samples into FFT buffer; take FFT; and convert to
 * amplitude-frequency (phase vocoder) form
 */
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT || inSIPtr->sfType == SDIFF)
	{
		if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPFRQ)
			numSampsRead = ReadCSAData(inSIPtr, polarSpectrum, lastPhaseInL, gExtPI.halfPoints);
		else if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPPHS)
			numSampsRead = ReadSHAData(inSIPtr, polarSpectrum, LEFT);
		else if(inSIPtr->packMode == SF_FORMAT_SPECT_COMPLEX)
		{
			numSampsRead = ReadSDIFData(inSIPtr, spectrum, polarSpectrum, &seconds, LEFT);
			CartToPolar(spectrum, polarSpectrum, gExtPI.halfPoints);
		}
	}
	else
	{
		SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart + gInPosition));
		numSampsRead = ShiftIn(inSIPtr, inputL, inputR, gExtPI.windowSize, gExtPI.decimation, &validSamples);
		WindowFold(inputL, analysisWindow, spectrum, gInPointer, gExtPI.points, gExtPI.windowSize);
		RealFFT(spectrum, gExtPI.halfPoints, TIME2FREQ);
		CartToPolar(spectrum, polarSpectrum, gExtPI.halfPoints);
	}
	if(numSampsRead > 0)
	{
		length = (numSampsRead + (gInPosition/(inSIPtr->nChans*inSIPtr->frameSize)))/inSIPtr->sRate;
		HMSTimeString(length, errStr);
		length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
		UpdateProcessWindow(errStr, "\p", "\pspectral extraction processing", length);
	}

	GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(polarSpectrum, displaySpectrum, gExtPI.halfPoints, inSIPtr, LEFT, 1.0, "\pInput Channel 1");
	GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(polarSpectrum, displaySpectrum, gExtPI.halfPoints, inSIPtr, LEFT, 1.0, "\pInput Channel 1");
		
	SpectralExtract(polarSpectrum,  steadSpectrum, transSpectrum, spectralFramesL);
	
	GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(transSpectrum, displaySpectrum, gExtPI.halfPoints, outTransSIPtr, LEFT, 1.0, "\pTransient Output Channel 1");
	GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(transSpectrum, displaySpectrum, gExtPI.halfPoints, outTransSIPtr, LEFT, 1.0, "\pTransient Output Channel 1");
	PolarToCart(transSpectrum, spectrum, gExtPI.halfPoints);
	RealFFT(spectrum, gExtPI.halfPoints, FREQ2TIME);
	OverlapAdd(spectrum, synthesisWindow, outputTransL, gOutPointer, gExtPI.points, gExtPI.windowSize);
		
	GetItemMark(gSpectDispMenu, DISP_AUX_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(steadSpectrum, displaySpectrum, gExtPI.halfPoints, outSteadSIPtr, LEFT, 1.0, "\pSteady Output Channel 1");
	GetItemMark(gSonoDispMenu, DISP_AUX_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(steadSpectrum, displaySpectrum, gExtPI.halfPoints, outSteadSIPtr, LEFT, 1.0, "\pSteady Output Channel 1");
	PolarToCart(steadSpectrum, spectrum, gExtPI.halfPoints);
	RealFFT(spectrum, gExtPI.halfPoints, FREQ2TIME);
	OverlapAdd(spectrum, synthesisWindow, outputSteadL, gOutPointer, gExtPI.points, gExtPI.windowSize);
    if(inSIPtr->nChans == STEREO)
    {
		if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT || inSIPtr->sfType == SDIFF)
		{
			if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPFRQ)
				numSampsRead = ReadCSAData(inSIPtr, polarSpectrum, lastPhaseInL, gExtPI.halfPoints);
			else if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPPHS)
				numSampsRead = ReadSHAData(inSIPtr, polarSpectrum, RIGHT);
			else if(inSIPtr->packMode == SF_FORMAT_SPECT_COMPLEX)
			{
				numSampsRead = ReadSDIFData(inSIPtr, spectrum, polarSpectrum, &seconds, RIGHT);
				CartToPolar(spectrum, polarSpectrum, gExtPI.halfPoints);
			}
		}
		else
		{
			WindowFold(inputR, analysisWindow, spectrum, gInPointer, gExtPI.points, gExtPI.windowSize);
			RealFFT(spectrum, gExtPI.halfPoints, TIME2FREQ);
			CartToPolar(spectrum, polarSpectrum, gExtPI.halfPoints);
		}
		GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(polarSpectrum, displaySpectrum, gExtPI.halfPoints, inSIPtr, RIGHT, 1.0, "\pInput Channel 2");
		GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(polarSpectrum, displaySpectrum, gExtPI.halfPoints, inSIPtr, RIGHT, 1.0, "\pInput Channel 2");

		SpectralExtract(polarSpectrum, steadSpectrum, transSpectrum, spectralFramesR);
		GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(transSpectrum, displaySpectrum, gExtPI.halfPoints, outTransSIPtr, RIGHT, 1.0, "\pTransient Output Channel 2");
		GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(transSpectrum, displaySpectrum, gExtPI.halfPoints, outTransSIPtr, RIGHT, 1.0, "\pTransient Output Channel 2");
		PolarToCart(transSpectrum, spectrum, gExtPI.halfPoints);
		RealFFT(spectrum, gExtPI.halfPoints, FREQ2TIME);
		OverlapAdd(spectrum, synthesisWindow, outputTransR, gOutPointer, gExtPI.points, gExtPI.windowSize);

		GetItemMark(gSpectDispMenu, DISP_AUX_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(steadSpectrum, displaySpectrum, gExtPI.halfPoints, outSteadSIPtr, RIGHT, 1.0, "\pSteady Output Channel 2");
		GetItemMark(gSonoDispMenu, DISP_AUX_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(steadSpectrum, displaySpectrum, gExtPI.halfPoints, outSteadSIPtr, RIGHT, 1.0, "\pSteady Output Channel 2");
		PolarToCart(steadSpectrum, spectrum, gExtPI.halfPoints);
		RealFFT(spectrum, gExtPI.halfPoints, FREQ2TIME);
		OverlapAdd(spectrum, synthesisWindow, outputSteadR, gOutPointer, gExtPI.points, gExtPI.windowSize);
	}
/* Data Output and Header Update */
	ShiftOut(outTransSIPtr, outputTransL, outputTransR, gOutPointer+gExtPI.interpolation, gExtPI.interpolation, gExtPI.windowSize);
	ShiftOut(outSteadSIPtr, outputSteadL, outputSteadR, gOutPointer+gExtPI.interpolation, gExtPI.interpolation, gExtPI.windowSize);
	
	length = (outSteadSIPtr->numBytes/(outSteadSIPtr->frameSize*outSteadSIPtr->nChans*outSteadSIPtr->sRate));
	HMSTimeString(length, errStr);
	UpdateProcessWindow("\p", errStr, "\p", -1.0);
	if(numSampsRead == -2L)
	{
		DeAllocExtractMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	gNumberBlocks++;
	GetFPos(outTransSIPtr->dFileNum, &sOutTPos);
	GetFPos(outSteadSIPtr->dFileNum, &sOutSPos);
 	gInPosition += inSIPtr->nChans * gExtPI.decimation * inSIPtr->frameSize;
 	return(TRUE);
}

void
SpectralExtract(float polarSpectrum[], float steadSpectrum[], float transSpectrum[], float spectralFrames[])
{
	long bandNumber, ampIndex, phaseIndex, i, j, k;
	static long currentFrameNumber;
	float frameDivisor, freqDeviation, cyclesBand, cyclesFrame, instFreq[12], freqTemp;
	
	if((gNumberBlocks + 1) < gEI.frames)
		frameDivisor = 1.0/(float)(gNumberBlocks + 1);
	else
		frameDivisor = 1.0/(float)(gEI.frames);

	cyclesBand = inSIPtr->sRate/gExtPI.points;
	cyclesFrame = inSIPtr->sRate/(gExtPI.decimation*twoPi);
	ZeroFloatTable(steadSpectrum, (gExtPI.points+2));

	if(gNumberBlocks == 0)
		currentFrameNumber = 0;
	
    for(bandNumber = 0; bandNumber <= gExtPI.halfPoints; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
		phaseIndex = ampIndex + 1;
		if(polarSpectrum[ampIndex] == 0.0)
			polarSpectrum[phaseIndex] = 0.0;
			
    	spectralFrames[(currentFrameNumber * (gExtPI.points+2)) + ampIndex] = polarSpectrum[ampIndex];
    	spectralFrames[(currentFrameNumber * (gExtPI.points+2)) + phaseIndex] = polarSpectrum[phaseIndex];

		// Start with the current frame + 1 (the oldest frame) and compute the phase difference
		// between it (i) and the next frame (k), put the result in instFreq[j], then compute the
		// frequency deviation. 
    	for(i = currentFrameNumber+1, j = 0, k = currentFrameNumber+2 ; j < (gEI.frames - 1); i++, j++, k++)
    	{
    		// i and k wrap around the top to zero.
    		if(k >= gEI.frames)
    			k -= gEI.frames;
    		if(i >= gEI.frames)
    			i -= gEI.frames;
			instFreq[j] = spectralFrames[(i * (gExtPI.points+2)) + phaseIndex] - spectralFrames[(k * (gExtPI.points+2)) + phaseIndex];
			// Unwrap phase differences
			while (instFreq[j] > Pi)
				instFreq[j] -= twoPi ;
			while (instFreq[j] < -Pi)
				instFreq[j] += twoPi ;
			instFreq[j] = instFreq[j]*cyclesFrame + bandNumber*cyclesBand;
			if(instFreq[j] < 0.0)
				instFreq[j] = -instFreq[j];
		}
		freqDeviation = 0.0;
		for(j = 0; j < (gEI.frames - 2); j++)
		{
			freqTemp = fabs(instFreq[j] - instFreq[j+1]);
			freqDeviation += freqTemp * frameDivisor;
		}
		transSpectrum[phaseIndex] = polarSpectrum[phaseIndex];
    	steadSpectrum[phaseIndex] = polarSpectrum[phaseIndex];

    	if(freqDeviation>(2.0 * gEI.transDev))
    		transSpectrum[ampIndex] =  polarSpectrum[ampIndex];
    	else if(freqDeviation>gEI.transDev)
    		transSpectrum[ampIndex] =  (freqDeviation-gEI.transDev)/gEI.transDev * polarSpectrum[ampIndex];
    	else
    	    transSpectrum[ampIndex] = 0.0;

    	if(freqDeviation>gEI.steadDev)
    		steadSpectrum[ampIndex] =  0.0;
    	else
    	    steadSpectrum[ampIndex] = (gEI.steadDev - freqDeviation)/gEI.steadDev * polarSpectrum[ampIndex];
    }
    currentFrameNumber++;
    if(currentFrameNumber == gEI.frames)
    	currentFrameNumber = 0;
}

void
SetExtractDefaults(void)
{
	short	itemType;
	float	tmpFloat;
	double	tmpDouble;
	long	tmpLong;
	Rect	itemRect;
	Handle	itemHandle;
	Str255	tmpStr;
	
	gNormalize = FALSE;
	gScaleDivisor = 1.0;
	
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gExtractDialog));
#else
	SetPort((GrafPtr)gExtractDialog);
#endif
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
	{
		gExtPI.points = inSIPtr->spectFrameSize;
		gExtPI.decimation = gExtPI.interpolation = inSIPtr->spectFrameIncr;
		GetDialogItem(gExtractDialog, EX_BANDS_ITEM, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);
	}
	else
	{
		gExtPI.decimation = gExtPI.interpolation = gExtPI.points >> 3;
		GetDialogItem(gExtractDialog, EX_BANDS_ITEM, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);
	}
	
	tmpDouble =	LLog2((double)gExtPI.points);
	tmpFloat = tmpDouble;
	tmpLong = (long)tmpFloat;	// 10 for 1024
	GetDialogItem(gExtractDialog, EX_BANDS_ITEM, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, (14 - tmpLong));
	
	GetDialogItem(gExtractDialog, EX_FRAMES_ITEM, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, (gEI.frames - 3));
	
	tmpFloat = (gEI.frames * gExtPI.decimation)/inSIPtr->sRate;
	FixToString(tmpFloat,tmpStr);
	GetDialogItem(gExtractDialog, EX_RATE_TEXT, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	FixToString(gEI.transDev, tmpStr);
	GetDialogItem(gExtractDialog, EX_TRANS_DEV_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	FixToString(gEI.steadDev, tmpStr);
	GetDialogItem(gExtractDialog, EX_STEAD_DEV_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
}

void
DeAllocExtractMem(void)
{
	RemovePtr((Ptr)analysisWindow);
	RemovePtr((Ptr)synthesisWindow);
	RemovePtr((Ptr)inputL);
	RemovePtr((Ptr)spectrum);
	RemovePtr((Ptr)polarSpectrum);
	RemovePtr((Ptr)displaySpectrum);
	RemovePtr((Ptr)spectralFramesL);
	RemovePtr((Ptr)transSpectrum);
	RemovePtr((Ptr)steadSpectrum);
	RemovePtr((Ptr)outputSteadL);
	RemovePtr((Ptr)outputTransL);
	if(inSIPtr->nChans == STEREO)
	{
		RemovePtr((Ptr)inputR);
		RemovePtr((Ptr)outputTransR);
		RemovePtr((Ptr)spectralFramesR);
		RemovePtr((Ptr)outputSteadR);
	}
	analysisWindow = synthesisWindow = inputL = spectrum = polarSpectrum = nil;
	displaySpectrum = spectralFramesL = outputTransL = transSpectrum = outputSteadL = nil;
	steadSpectrum = inputR = spectralFramesR = outputTransR = outputSteadR = nil;
}
