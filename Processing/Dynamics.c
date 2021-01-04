/* 
 *	SoundHack - soon to be many things to many people
 *	Copyright �1994 Tom Erbe - California Institute of the Arts
 *
 *	Dynamics.c - spectral dynamics processing
 *	10/18/94 - added new scheme for attack decay smoothing.
 *	10/22/94 - ironed out some bugs in decay smoothing.
 */
 
#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>

//#include <AppleEvents.h>
#include "SoundFile.h"
#include "OpenSoundFile.h"
#include "Dialog.h"
#include "Menu.h"
#include "SoundHack.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "PhaseVocoder.h"
#include "Dynamics.h"
#include "Analysis.h"
#include "SpectFileIO.h"
#include "Misc.h"
#include "FFT.h"
#include "Windows.h"
#include "ShowSpect.h"
#include "ShowWave.h"
#include "ShowSono.h"
#include "CarbonGlue.h"

/* Globals */

DialogPtr	gDynamicsDialog;
extern MenuHandle	gAppleMenu, gFileMenu, gEditMenu, gProcessMenu, gControlMenu,
					gBandsMenu, gDynamicsMenu, gSigDispMenu, gSpectDispMenu, gSonoDispMenu;
extern SoundInfoPtr	inSIPtr, filtSIPtr, outSIPtr, frontSIPtr;

extern Boolean		gNormalize;
extern short		gProcessEnabled, gProcessDisabled, gStopProcess, gProcNChans;
extern float 		Pi, twoPi, gScale, gScaleL, gScaleR, gScaleDivisor;
extern long			gNumberBlocks, gInPosition;
extern long			gInPointer, gOutPointer;
extern float 		*analysisWindow, *synthesisWindow, *inputL, *inputR, *spectrum,
					*displaySpectrum, *polarSpectrum, *outputL, *outputR, *lastPhaseInL,
					*lastPhaseInR, *spectralFramesL, *spectralFramesR;
extern FSSpec		nilFSSpec;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

DynamicsInfo		gDI;
PvocInfo			gDynPI;

float	*trigRefL, *trigRefR, *curFacL, *curFacR;

void
HandleDynamicsDialog(void)
{
	WindInfo *theWindInfo;
	
	gDynamicsDialog = GetNewDialog(DYNAMICS_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)DYNAMICS_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gDynamicsDialog), (long)theWindInfo);
#else
	SetWRefCon(gDynamicsDialog, (long)theWindInfo);
#endif

	gProcessDisabled = LEMON_PROCESS;
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
	EnableItem(gSpectDispMenu, DISP_INPUT_ITEM);
	EnableItem(gSonoDispMenu, DISP_INPUT_ITEM);
	DisableItem(gSigDispMenu, DISP_AUX_ITEM);
	DisableItem(gSpectDispMenu, DISP_AUX_ITEM);
	DisableItem(gSonoDispMenu, DISP_AUX_ITEM);
	EnableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
	EnableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
	EnableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	MenuUpdate();
	
	if(filtSIPtr != nil)
		filtSIPtr = nil;

	SetDynamicsDefaults();
	
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gDynamicsDialog));
	SelectWindow(GetDialogWindow(gDynamicsDialog));
#else
	ShowWindow(gDynamicsDialog);
	SelectWindow(gDynamicsDialog);
#endif
}

void
HandleDynamicsDialogEvent(short itemHit)
{
	double	tmpFloat;
	long	tmpLong;
	short	itemType;
	static short	choiceBand = 4;
	
	Rect	itemRect;
	Handle	itemHandle;
	Str255	errStr;
	WindInfo	*myWI;
	
		switch(itemHit)
		{
			case D_TYPE_MENU:
				GetDialogItem(gDynamicsDialog, D_TYPE_MENU, &itemType, &itemHandle, &itemRect);
				gDI.type = GetControlValue((ControlHandle)itemHandle);
				GetDialogItem(gDynamicsDialog, D_GAIN_TEXT, &itemType, &itemHandle, &itemRect);
				if(gDI.type == GATE)
				{
					SetDialogItemText(itemHandle, "\pGain/Reduction (dB):");
					GetDialogItem(gDynamicsDialog, D_GAIN_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\p-40.000000");
					gDI.factor = 0.01;
				}
				else if(gDI.type == EXPAND)
				{
					SetDialogItemText(itemHandle, "\pExpand Ratio (1:N):");
					GetDialogItem(gDynamicsDialog, D_GAIN_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\p1.000000");
					gDI.factor = 1.0;
				}
				else if(gDI.type == COMPRESS)
				{
					SetDialogItemText(itemHandle, "\pCompress Ratio (N:1):");
					GetDialogItem(gDynamicsDialog, D_GAIN_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\p1.000000");
					gDI.factor = 1.0;
				}
				break;
			case D_BANDS_MENU:
				GetDialogItem(gDynamicsDialog, D_BANDS_MENU, &itemType, &itemHandle, &itemRect);
				choiceBand = GetControlValue((ControlHandle)itemHandle);
				gDynPI.points = 2<<(13 - choiceBand);
				gDynPI.halfPoints = gDynPI.points>>1;
				gDynPI.decimation = gDynPI.interpolation = gDynPI.points>>2;
				if(gDI.lowBand > gDynPI.halfPoints)
				{
					gDI.lowBand = 0;
					NumToString(gDI.lowBand,errStr);
					GetDialogItem(gDynamicsDialog, D_LOW_BAND_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
				}
				tmpFloat = (gDI.lowBand * inSIPtr->sRate)/gDynPI.points;
				FixToString(tmpFloat, errStr);
				GetDialogItem(gDynamicsDialog, D_LOW_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, errStr);
				if(gDI.highBand > gDynPI.halfPoints)
				{
					gDI.highBand = gDynPI.halfPoints;
					NumToString(gDI.highBand,errStr);
					GetDialogItem(gDynamicsDialog, D_HIGH_BAND_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
				}
				tmpFloat = (gDI.highBand * inSIPtr->sRate)/gDynPI.points;
				FixToString(tmpFloat, errStr);
				GetDialogItem(gDynamicsDialog, D_HIGH_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, errStr);
				if(gDynPI.decimation/inSIPtr->sRate > gDI.time)
					gDI.time = gDynPI.decimation/inSIPtr->sRate;
				HMSTimeString(gDI.time, errStr);
				GetDialogItem(gDynamicsDialog, D_TIME_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, errStr);
				break;
			case D_LOW_BAND_FIELD:
				GetDialogItem(gDynamicsDialog,D_LOW_BAND_FIELD, &itemType, &itemHandle, &itemRect);
				GetDialogItemText(itemHandle, errStr);
				StringToNum(errStr, &tmpLong);
				if(tmpLong >= gDI.highBand && tmpLong < gDynPI.halfPoints)
				{
					gDI.lowBand = tmpLong;
					tmpFloat = (gDI.lowBand * inSIPtr->sRate)/gDynPI.points;
					FixToString(tmpFloat, errStr);
					GetDialogItem(gDynamicsDialog, D_LOW_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
					gDI.highBand = tmpLong + 1;
					tmpFloat = (gDI.highBand * inSIPtr->sRate)/gDynPI.points;
					FixToString(tmpFloat, errStr);
					GetDialogItem(gDynamicsDialog, D_HIGH_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
					NumToString(gDI.highBand, errStr);
					GetDialogItem(gDynamicsDialog, D_HIGH_BAND_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
				}				
				else if(tmpLong < gDI.highBand && tmpLong >= 0L)
				{
					gDI.lowBand = tmpLong;
					tmpFloat = (gDI.lowBand * inSIPtr->sRate)/gDynPI.points;
					FixToString(tmpFloat, errStr);
					GetDialogItem(gDynamicsDialog, D_LOW_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
				}
				break;
			case D_HIGH_BAND_FIELD:
				GetDialogItem(gDynamicsDialog,D_HIGH_BAND_FIELD, &itemType, &itemHandle, &itemRect);
				GetDialogItemText(itemHandle, errStr);
				StringToNum(errStr, &tmpLong);
				if(tmpLong <= gDI.lowBand && tmpLong > 0)
				{
					gDI.highBand = tmpLong;
					tmpFloat = (gDI.highBand * inSIPtr->sRate)/gDynPI.points;
					FixToString(tmpFloat, errStr);
					GetDialogItem(gDynamicsDialog, D_HIGH_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
					gDI.lowBand = tmpLong - 1;
					tmpFloat = (gDI.lowBand * inSIPtr->sRate)/gDynPI.points;
					FixToString(tmpFloat, errStr);
					GetDialogItem(gDynamicsDialog, D_LOW_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
					NumToString(gDI.lowBand, errStr);
					GetDialogItem(gDynamicsDialog, D_LOW_BAND_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
				}
				if(tmpLong <= gDynPI.halfPoints && tmpLong >= gDI.lowBand)
				{
					gDI.highBand = tmpLong;
					tmpFloat = (gDI.highBand * inSIPtr->sRate)/gDynPI.points;
					FixToString(tmpFloat, errStr);
					GetDialogItem(gDynamicsDialog, D_HIGH_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, errStr);
				}
				break;
			case D_GAIN_FIELD:
				GetDialogItem(gDynamicsDialog,D_GAIN_FIELD, &itemType, &itemHandle, &itemRect);
				GetDialogItemText(itemHandle, errStr);
				StringToFix(errStr, &tmpFloat);
				if(gDI.type == GATE)
					gDI.factor = powf(10.0, (tmpFloat/20.0));
				else if(gDI.type == EXPAND)
				 {
				 	if(tmpFloat < 0.0)
						gDI.factor = -tmpFloat;
					else 
						gDI.factor = tmpFloat;
				}
				else if(gDI.type == COMPRESS)
				{
				 	if(tmpFloat < 0.0)
						gDI.factor = -tmpFloat;
					else 
						gDI.factor = tmpFloat;
					gDI.factor = 1.0/gDI.factor;
				}
				break;
			case D_BELOW_RADIO:
				GetDialogItem(gDynamicsDialog, D_BELOW_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,ON);
				GetDialogItem(gDynamicsDialog, D_ABOVE_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				gDI.below = TRUE;
				break;
			case D_ABOVE_RADIO:
				GetDialogItem(gDynamicsDialog, D_BELOW_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,OFF);
				GetDialogItem(gDynamicsDialog, D_ABOVE_RADIO, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,ON);
				gDI.below = FALSE;
				break;
			case D_THRESH_FIELD:
				GetDialogItem(gDynamicsDialog,D_THRESH_FIELD, &itemType, &itemHandle, &itemRect);
				GetDialogItemText(itemHandle, errStr);
				StringToFix(errStr, &tmpFloat);
				gDI.threshold = powf(10.0, (tmpFloat/20.0));
				break;
			case D_TIME_FIELD:
				GetDialogItem(gDynamicsDialog,D_TIME_FIELD, &itemType, &itemHandle, &itemRect);
				GetDialogItemText(itemHandle, errStr);
				StringToFix(errStr, &tmpFloat);
				if(tmpFloat < gDynPI.decimation/inSIPtr->sRate)
					gDI.time = gDynPI.decimation/inSIPtr->sRate;
				else
					gDI.time = tmpFloat;
				break;
			case D_AVERAGE_BOX:
				if(gDI.average == FALSE)
				{
					GetDialogItem(gDynamicsDialog, D_AVERAGE_BOX, &itemType, &itemHandle, &itemRect);
					SetControlValue((ControlHandle)itemHandle,ON);
					HideDialogItem(gDynamicsDialog, D_TIME_FIELD);
					HideDialogItem(gDynamicsDialog, D_TIME_TEXT);
					gDI.average = TRUE;
				}
				else
				{
					GetDialogItem(gDynamicsDialog, D_AVERAGE_BOX, &itemType, &itemHandle, &itemRect);
					SetControlValue((ControlHandle)itemHandle,OFF);
					ShowDialogItem(gDynamicsDialog, D_TIME_FIELD);
					ShowDialogItem(gDynamicsDialog, D_TIME_TEXT);
					gDI.average = FALSE;
				}
				break;
			case D_BAND_GROUP_BOX:
				if(gDI.group == FALSE)
				{
					GetDialogItem(gDynamicsDialog, D_BAND_GROUP_BOX, &itemType, &itemHandle, &itemRect);
					SetControlValue((ControlHandle)itemHandle,ON);
					gDI.group = TRUE;
				}
				else
				{
					GetDialogItem(gDynamicsDialog, D_BAND_GROUP_BOX, &itemType, &itemHandle, &itemRect);
					SetControlValue((ControlHandle)itemHandle,OFF);
					gDI.group = FALSE;
				}
				break;
			case D_RELATIVE_BOX:
				GetDialogItem(gDynamicsDialog, D_RELATIVE_BOX, &itemType, &itemHandle, &itemRect);
				if(gDI.relative)
				{
					SetControlValue((ControlHandle)itemHandle,OFF);
					ShowDialogItem(gDynamicsDialog, D_FILE_BOX);
					gDI.relative = FALSE;
				}
				else
				{
					SetControlValue((ControlHandle)itemHandle,ON);
					HideDialogItem(gDynamicsDialog, D_FILE_BOX);
					gDI.relative = TRUE;
				}
				break;
			case D_FILE_BOX:
				GetDialogItem(gDynamicsDialog, D_FILE_BOX, &itemType, &itemHandle, &itemRect);
				if(gDI.useFile)
				{
					SetControlValue((ControlHandle)itemHandle,OFF);
					GetDialogItem(gDynamicsDialog, D_FILE_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\p");
					GetDialogItem(gDynamicsDialog, D_THRESH_TEXT, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\pThreshold Level (dB):");
					GetDialogItem(gDynamicsDialog, D_THRESH_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\p-40.000000");
					ShowDialogItem(gDynamicsDialog, D_RELATIVE_BOX);
					HideDialogItem(gDynamicsDialog, D_FILE_TEXT);
					HideDialogItem(gDynamicsDialog, D_FILE_FIELD);
					HideDialogItem(gDynamicsDialog, D_FILE_BUTTON);
					gDI.threshold = 0.01;
					gDI.useFile = FALSE;
					GetDialogItem(gDynamicsDialog, D_PROCESS_BUTTON, &itemType, &itemHandle, &itemRect);
					HiliteControl((ControlHandle)itemHandle, 0);
				}
				else
				{
					SetControlValue((ControlHandle)itemHandle,ON);
					GetDialogItem(gDynamicsDialog, D_FILE_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\p");
					GetDialogItem(gDynamicsDialog, D_THRESH_TEXT, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\pThresh. Above File (dB):");
					GetDialogItem(gDynamicsDialog, D_THRESH_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, "\p0.000000");
					HideDialogItem(gDynamicsDialog, D_RELATIVE_BOX);
					ShowDialogItem(gDynamicsDialog, D_FILE_TEXT);
					ShowDialogItem(gDynamicsDialog, D_FILE_FIELD);
					ShowDialogItem(gDynamicsDialog, D_FILE_BUTTON);
					gDI.threshold = 1.0;
					gDI.useFile = TRUE;
					GetDialogItem(gDynamicsDialog, D_PROCESS_BUTTON, &itemType, &itemHandle, &itemRect);
					HiliteControl((ControlHandle)itemHandle, 255);
				}
				break;
			case D_FILE_BUTTON:
				if(OpenSoundFile(nilFSSpec, FALSE) != -1)
				{
					filtSIPtr = frontSIPtr;
					GetDialogItem(gDynamicsDialog, D_FILE_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle,filtSIPtr->sfSpec.name);
					GetDialogItem(gDynamicsDialog, D_PROCESS_BUTTON, &itemType, &itemHandle, &itemRect);
					HiliteControl((ControlHandle)itemHandle, 0);
				}
				break;
			case D_CANCEL_BUTTON:
#if TARGET_API_MAC_CARBON == 1
				myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gDynamicsDialog));
#else
				myWI = (WindInfoPtr)GetWRefCon(gDynamicsDialog);
#endif
				RemovePtr((Ptr)myWI);
				DisposeDialog(gDynamicsDialog);
				gDynamicsDialog = nil;
				gProcessDisabled = gProcessEnabled = NO_PROCESS;
				inSIPtr = nil;
				MenuUpdate();
				break;
			case D_PROCESS_BUTTON:
				gDI.increment = (gDI.factor - 1.0)/((inSIPtr->sRate * gDI.time)/gDynPI.decimation);
#if TARGET_API_MAC_CARBON == 1
				myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gDynamicsDialog));
#else
				myWI = (WindInfoPtr)GetWRefCon(gDynamicsDialog);
#endif
				RemovePtr((Ptr)myWI);
				DisposeDialog(gDynamicsDialog);
				gDynamicsDialog = nil;
				InitDynamicsProcess();
				break;
		}
}

/*
 *	This is called from HandleDynamicsDialog() 
 *	Then process impulse H[] looking for spectral average.
 *	Then open outSoundFile, set DYNAMICS_PROCESS and let PvocBlock() take care
 *	of the rest
 */
short
InitDynamicsProcess(void)
{
    short		i;
	long	numberRead, bandNum;
	float	oneOver;

	gDynPI.windowSize = gDynPI.points;
	gDynPI.halfPoints = gDynPI.points >> 1;
	if(inSIPtr->nChans == STEREO)
		gProcNChans = STEREO;
	else if(gDI.useFile)
	{
		if(filtSIPtr->nChans == STEREO)
			gProcNChans = STEREO;
	}
	else
		gProcNChans = MONO;
	
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
	if(gDI.type == GATE)
		NameFile(inSIPtr->sfSpec.name, "\pGATE", outSIPtr->sfSpec.name);
	else if(gDI.type == EXPAND)
		NameFile(inSIPtr->sfSpec.name, "\pXPND", outSIPtr->sfSpec.name);
	else if(gDI.type == COMPRESS)
		NameFile(inSIPtr->sfSpec.name, "\pCMPR", outSIPtr->sfSpec.name);

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
	WriteHeader(outSIPtr);
	UpdateInfoWindow(outSIPtr);
	SetOutputScale(outSIPtr->packMode);
    
/*
 * allocate memory
 */
	analysisWindow = synthesisWindow = inputL = outputL = spectrum = nil;
	polarSpectrum = displaySpectrum = trigRefL = curFacL = inputR = outputR = nil;
	trigRefR = curFacR = nil;

	analysisWindow = (float *)NewPtr(gDynPI.windowSize*sizeof(float));
	synthesisWindow = (float *)NewPtr(gDynPI.windowSize*sizeof(float));
	inputL = (float *)NewPtr(gDynPI.windowSize*sizeof(float));
	outputL = (float *)NewPtr(gDynPI.windowSize*sizeof(float));
	
	spectrum = (float *)NewPtr(gDynPI.points*sizeof(float));
	
	polarSpectrum = (float *)NewPtr((gDynPI.points+2)*sizeof(float));		/* analysis channels */
	
	displaySpectrum = (float *)NewPtr((gDynPI.halfPoints+1)*sizeof(float));	/* For display of amplitude */
	if(gDI.average)
	{
		spectralFramesL = (float *)NewPtr((gDynPI.halfPoints+1)*sizeof(float)*4);
		ZeroFloatTable(spectralFramesL, (gDynPI.halfPoints+1)*4);
	}
	trigRefL = (float *)NewPtr((gDynPI.halfPoints+1)*sizeof(float));
	curFacL = (float *)NewPtr((gDynPI.halfPoints+1)*sizeof(float));
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
	{
		lastPhaseInL = (float *)NewPtr((gDynPI.halfPoints+1) * sizeof(float));
		ZeroFloatTable(lastPhaseInL, (gDynPI.halfPoints+1));
		if(inSIPtr->nChans == STEREO)
		{
			lastPhaseInR = (float *)NewPtr((gDynPI.halfPoints+1) * sizeof(float));
			ZeroFloatTable(lastPhaseInR, (gDynPI.halfPoints+1));
		}
	}
	
	if(inSIPtr->nChans == STEREO)
	{
		inputR = (float *)NewPtr(gDynPI.windowSize*sizeof(float));
		outputR = (float *)NewPtr(gDynPI.windowSize*sizeof(float));
		trigRefR = (float *)NewPtr((gDynPI.halfPoints+1)*sizeof(float));
		curFacR = (float *)NewPtr((gDynPI.halfPoints+1)*sizeof(float));
		if(gDI.average)
		{
			spectralFramesR = (float *)NewPtr((gDynPI.halfPoints+1)*sizeof(float)*4);
			ZeroFloatTable(spectralFramesR, (gDynPI.halfPoints+1)*4);
		}
	}
	else
	{
		inputR = inputL;
		outputR = outputL;
		trigRefR = trigRefL;
		curFacR = curFacL;
	}
	
	if(outputL == 0)
	{
		DrawErrorMessage("\pNot enough memory for processing");
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		DeAllocSpecGateMem();
		outSIPtr = nil;
		return(-1);
	}
	for(i = 0; i < gDynPI.halfPoints+1; i++)
		curFacL[i] = 1.0;
	ZeroFloatTable(spectrum, gDynPI.points);
	ZeroFloatTable(polarSpectrum, (gDynPI.points+2));
	ZeroFloatTable(trigRefL, (gDynPI.halfPoints+1));
	ZeroFloatTable(inputL, gDynPI.windowSize);
	ZeroFloatTable(outputL, gDynPI.windowSize);

	if(inSIPtr->nChans == STEREO)
	{
		for(i = 0; i < gDynPI.halfPoints+1; i++)
			curFacR[i] = 1.0;
		ZeroFloatTable(trigRefR, (gDynPI.halfPoints+1));
		ZeroFloatTable(inputR, gDynPI.windowSize);
		ZeroFloatTable(outputR, gDynPI.windowSize);
	}

/*
 * create windows
 */
 	HammingWindow(analysisWindow, gDynPI.windowSize);
 	HammingWindow(synthesisWindow, gDynPI.windowSize);
/*
 *	Scale hamming windows for balanced pvoc processing
 */
 
    ScaleWindows(analysisWindow, synthesisWindow, gDynPI);
	UpdateProcessWindow("\p", "\p", "\panalyzing trigger file", 0.0);

/*
 * analyze trigger file
 */
 	if(gDI.useFile)
 	{
 		numberRead = gDynPI.points;
		SetFPos(filtSIPtr->dFileNum, fsFromStart, filtSIPtr->dataStart);
 		for(i = 0; i < 5 && numberRead == gDynPI.points; i++)
 		{
 			numberRead = ReadStereoBlock(filtSIPtr, gDynPI.points, inputL, inputR);
			for(bandNum = 0; bandNum < numberRead; bandNum++)
			{
				inputL[bandNum] *= analysisWindow[bandNum];
 				if(inSIPtr->nChans == STEREO)
					inputR[bandNum] *= analysisWindow[bandNum];
			}
			for(; bandNum < gDynPI.points; bandNum++)
			{
				inputL[bandNum] = 0.0;
 				if(inSIPtr->nChans == STEREO)
					inputR[bandNum] = 0.0;
			}
 			RealFFT(inputL, gDynPI.halfPoints, TIME2FREQ);
 			CartToPolar(inputL, polarSpectrum, gDynPI.halfPoints);
 			for(bandNum = 0; bandNum <= gDynPI.halfPoints; bandNum++)
 				trigRefL[bandNum] += polarSpectrum[bandNum<<1];
 			if(inSIPtr->nChans == STEREO)
 			{
 				RealFFT(inputR, gDynPI.halfPoints, TIME2FREQ);
 				CartToPolar(inputR, polarSpectrum, gDynPI.halfPoints);
 				for(bandNum = 0; bandNum <= gDynPI.halfPoints; bandNum++)
 					trigRefR[bandNum] += polarSpectrum[bandNum<<1];
 			}
		}
		oneOver = 1.0/i;
		for(bandNum = 0; bandNum <= gDynPI.halfPoints; bandNum++)
		{
			trigRefL[bandNum] = trigRefL[bandNum] * oneOver * gDI.threshold;
 			if(inSIPtr->nChans == STEREO)
				trigRefR[bandNum] = trigRefR[bandNum] * oneOver * gDI.threshold;
		}
	}
/*
 * initialize input and output time values (in samples)
 */
    gInPointer = -gDynPI.windowSize;
	gOutPointer = (gInPointer*gDynPI.interpolation)/gDynPI.decimation;
	gInPosition = 0;
	
	gNumberBlocks = 0;	
	gProcessEnabled = DYNAMICS_PROCESS;
	gProcessDisabled = NO_PROCESS;
	MenuUpdate();
	UpdateProcessWindow("\p", "\p", "\pdynamics processing", 0.0);
	return(TRUE);
}

/*
 * main loop--perform phase vocoder analysis-resynthesis
 */
short
SpecGateBlock(void)
{
    short	curMark;
	long	numSampsRead;
	float 	length;
	Str255	errStr;
	static long validSamples = -1L;
	static long	sOutPos;
	double seconds;
	

	if(gStopProcess == TRUE)
	{
		DeAllocSpecGateMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	if(gNumberBlocks == 0)
		sOutPos = outSIPtr->dataStart;
	SetFPos(outSIPtr->dFileNum, fsFromStart, sOutPos);

/*
 * increment times
 */
	gInPointer += gDynPI.decimation;
	gOutPointer += gDynPI.interpolation;
	
/*
 * analysis: input gDynPI.decimation samples; window, fold and rotate input
 * samples into FFT buffer; take FFT; and convert to
 * amplitude-frequency (phase vocoder) form
 */
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT || inSIPtr->sfType == SDIFF)
	{
		if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPFRQ)
			numSampsRead = ReadCSAData(inSIPtr, polarSpectrum, lastPhaseInL, gDynPI.halfPoints);
		else if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPPHS)
			numSampsRead = ReadSHAData(inSIPtr, polarSpectrum, LEFT);
		else if(inSIPtr->packMode == SF_FORMAT_SPECT_COMPLEX)
		{
			numSampsRead = ReadSDIFData(inSIPtr, spectrum, polarSpectrum, &seconds, LEFT);
			CartToPolar(spectrum, polarSpectrum, gDynPI.halfPoints);
		}
	}
	else
	{
		SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart + gInPosition));
		numSampsRead = ShiftIn(inSIPtr, inputL, inputR, gDynPI.windowSize, gDynPI.decimation, &validSamples);
		WindowFold(inputL, analysisWindow, spectrum, gInPointer, gDynPI.points, gDynPI.windowSize);
		RealFFT(spectrum, gDynPI.halfPoints, TIME2FREQ);
		CartToPolar(spectrum, polarSpectrum, gDynPI.halfPoints);
	}
	if(numSampsRead > 0)
	{
		length = (numSampsRead + (gInPosition/(inSIPtr->nChans*inSIPtr->frameSize)))/inSIPtr->sRate;
		HMSTimeString(length, errStr);
		length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
		UpdateProcessWindow(errStr, "\p", "\pdynamics processing", length);
	}
	GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(polarSpectrum, displaySpectrum, gDynPI.halfPoints, inSIPtr, LEFT, 1.0, "\pInput Channel 1");
	GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(polarSpectrum, displaySpectrum, gDynPI.halfPoints, inSIPtr, LEFT, 1.0, "\pInput Channel 1");
	switch(gDI.type)
	{
		case GATE:
			SpectralGate(polarSpectrum, trigRefL, curFacL, spectralFramesL);
			break;
		case EXPAND:
			SpectralCompand(polarSpectrum, trigRefL, curFacL, spectralFramesL);
			break;
		case COMPRESS:
			SpectralCompand(polarSpectrum, trigRefL, curFacL, spectralFramesL);
			break;
	}
	GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(polarSpectrum, displaySpectrum, gDynPI.halfPoints, outSIPtr, LEFT, 1.0, "\pOutput Channel 1");
	GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(polarSpectrum, displaySpectrum, gDynPI.halfPoints, outSIPtr, LEFT, 1.0, "\pOutput Channel 1");
	PolarToCart(polarSpectrum, spectrum, gDynPI.halfPoints);
	RealFFT(spectrum, gDynPI.halfPoints, FREQ2TIME);
	OverlapAdd(spectrum, synthesisWindow, outputL, gOutPointer, gDynPI.points, gDynPI.windowSize);
    if(inSIPtr->nChans == STEREO)
    {
		if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT || inSIPtr->sfType == SDIFF)
		{
			if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPFRQ)
				numSampsRead = ReadCSAData(inSIPtr, polarSpectrum, lastPhaseInR, gDynPI.halfPoints);
			else if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPPHS)
				numSampsRead = ReadSHAData(inSIPtr, polarSpectrum, RIGHT);
			else if(inSIPtr->packMode == SF_FORMAT_SPECT_COMPLEX)
			{
				numSampsRead = ReadSDIFData(inSIPtr, spectrum, polarSpectrum, &seconds, RIGHT);
				CartToPolar(spectrum, polarSpectrum, gDynPI.halfPoints);
			}
		}
		else
		{
			WindowFold(inputR, analysisWindow, spectrum, gInPointer, gDynPI.points, gDynPI.windowSize);
			RealFFT(spectrum, gDynPI.halfPoints, TIME2FREQ);
			CartToPolar(spectrum, polarSpectrum, gDynPI.halfPoints);
		}
		GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(polarSpectrum, displaySpectrum, gDynPI.halfPoints, inSIPtr, RIGHT, 1.0, "\pInput Channel 2");
		GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(polarSpectrum, displaySpectrum, gDynPI.halfPoints, inSIPtr, RIGHT, 1.0, "\pInput Channel 2");
		switch(gDI.type)
		{
			case GATE:
				SpectralGate(polarSpectrum, trigRefR, curFacR, spectralFramesR);
				break;
			case COMPRESS:
				SpectralCompand(polarSpectrum, trigRefR, curFacR, spectralFramesR);
				break;
			case EXPAND:
				SpectralCompand(polarSpectrum, trigRefR, curFacR, spectralFramesR);
				break;
		}
		GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(polarSpectrum, displaySpectrum, gDynPI.halfPoints, outSIPtr, RIGHT, 1.0, "\pOutput Channel 2");
		GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(polarSpectrum, displaySpectrum, gDynPI.halfPoints, outSIPtr, RIGHT, 1.0, "\pOutput Channel 2");
		PolarToCart(polarSpectrum, spectrum, gDynPI.halfPoints);
		RealFFT(spectrum, gDynPI.halfPoints, FREQ2TIME);
		OverlapAdd(spectrum, synthesisWindow, outputR, gOutPointer, gDynPI.points, gDynPI.windowSize);
	}
/* Data Output and Header Update */
	ShiftOut(outSIPtr, outputL, outputR, gOutPointer+gDynPI.interpolation, gDynPI.interpolation, gDynPI.windowSize);
	length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
	HMSTimeString(length, errStr);
	UpdateProcessWindow("\p", errStr, "\p", -1.0);
	if(numSampsRead == -2L)
	{
		DeAllocSpecGateMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	gNumberBlocks++;
	GetFPos(outSIPtr->dFileNum, &sOutPos);
 	gInPosition += inSIPtr->nChans * gDynPI.decimation * inSIPtr->frameSize;
 	return(TRUE);
}

void
SpectralGate(float polarSpectrum[], float trigRef[], float curFac[], float spectralFrames[])
{
	long lastDivision, numberBands, nextDivision, bandNumber, ampIndex, i;
	Boolean	triggered;
	double	averageFac, nextDivisionFloat, nextOctave, maxAmplitude, trigger, relativeThresh, average, avefac;
	static long curFrameNum;
	
	if(gNumberBlocks == 0)
		curFrameNum = 0;
	if(gDI.relative)
	{
		/* Find maximum amplitude */
		for(bandNumber = 0, maxAmplitude = 0.0; bandNumber <= gDynPI.halfPoints; bandNumber++)
		{
			ampIndex = bandNumber<<1;
			if(polarSpectrum[ampIndex] > maxAmplitude)
				maxAmplitude = polarSpectrum[ampIndex];
		}
		relativeThresh = gDI.threshold * maxAmplitude;
	}
	
    for(bandNumber = gDI.lowBand; bandNumber <= gDI.highBand; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
    	
    	if(gDI.relative == TRUE)		// threshold is relative to peak amplitude this bin.
    		trigger = relativeThresh;
		else if(gDI.useFile == FALSE)	// if not using a file, threshold is from dialog.		
    		trigger = gDI.threshold;
    	else							// or else it is from the file.
    		trigger = trigRef[bandNumber];

		if(gDI.average)
		{
			// gain = g0 + (1-g0) * [avg / (avg + th*th*nref)] ^ sh
    		spectralFrames[(curFrameNum * (gDynPI.halfPoints+1)) + bandNumber] = polarSpectrum[ampIndex];
			
			// simply compute the average of the five frames of sound.
			average = 0.0;
    		for(i = 0; i < 4; i++)
				average += spectralFrames[(i * (gDynPI.halfPoints+1)) + bandNumber];
			
			average *= 0.25;
			
			if(gDI.below)
				avefac = average/(average + trigger);
			else
				avefac = trigger/(average + trigger);
			curFac[bandNumber] = gDI.factor + (1 - gDI.factor) * avefac;
		}
		else
		{
			if(gDI.below)
				if(polarSpectrum[ampIndex] < trigger)
					triggered = TRUE;
				else
					triggered = FALSE;
			else
    			if(polarSpectrum[ampIndex] > trigger)
    				triggered = TRUE;
    			else
			  	  triggered = FALSE;

    		if(triggered == TRUE)
    		{
    			curFac[bandNumber] = curFac[bandNumber] + gDI.increment;
		    	if(gDI.factor <= 1.0)
				{
					if(curFac[bandNumber] < gDI.factor)
						curFac[bandNumber] = gDI.factor;
				}
  				else
  				{
					if(curFac[bandNumber] > gDI.factor)
						curFac[bandNumber] = gDI.factor;
				}
    		}
			// curFac[] is heading to 1.0
    		else
    		{
    			curFac[bandNumber] = curFac[bandNumber] - gDI.increment;
				if(gDI.factor <= 1.0)
				{
					if(curFac[bandNumber] > 1.0)
						curFac[bandNumber] = 1.0;
				}
				else
				{
					if(curFac[bandNumber] < 1.0)
						curFac[bandNumber] = 1.0;
				}
    		}
    	}    		
    }

	if(gDI.group)
	{
    	// some code to average the curFac per third octave band
		lastDivision = gDynPI.halfPoints;
		nextOctave = -0.3333333333333;
		nextDivisionFloat = EExp2(nextOctave) * gDynPI.halfPoints;
		nextDivision = (long)nextDivisionFloat;
		numberBands = 0;
		averageFac = 0.0;
    	for(bandNumber = gDynPI.halfPoints; bandNumber >= 0; bandNumber--)
    	{
    		if(bandNumber <= nextDivision)
    		{
    			averageFac = averageFac/numberBands;
   				for(i = nextDivision+1; i <= lastDivision; i++)
   					curFac[i] = averageFac;
	   			lastDivision = nextDivision;
				nextOctave -= 0.3333333333333;
				nextDivisionFloat = EExp2(nextOctave) * gDynPI.halfPoints;
				nextDivision = (long)nextDivisionFloat;
				numberBands = 0;
				averageFac = 0.0;
			}
    		averageFac += curFac[bandNumber];
    		numberBands++;
		}
    }
    
    
    // apply the gate
    for(bandNumber = gDI.lowBand; bandNumber <= gDI.highBand; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
    	polarSpectrum[ampIndex] *= curFac[bandNumber];
    }
    	
	curFrameNum++;
    if(curFrameNum == 4)
    	curFrameNum = 0;
}

void
SpectralCompand(float polarSpectrum[], float trigRef[], float curFac[], float spectralFrames[])
{
	long bandNumber, ampIndex, i, nextDivision, lastDivision, numberBands;
	Boolean	triggered;
	double tmpAmp, maxAmplitude, trigger, relativeThresh, average, avefac, averageFac, invFac, nextOctave, nextDivisionFloat;
	static long curFrameNum;
	
	if(gNumberBlocks == 0)
		curFrameNum = 0;
	if(gDI.relative)
	{
		/* Find maximum amplitude */
		for(bandNumber = 0, maxAmplitude = 0.0; bandNumber <= gDynPI.halfPoints; bandNumber++)
		{
			ampIndex = bandNumber<<1;
			if(polarSpectrum[ampIndex] > maxAmplitude)
				maxAmplitude = polarSpectrum[ampIndex];
		}
		relativeThresh = gDI.threshold * maxAmplitude;
	}

	
    for(bandNumber = gDI.lowBand; bandNumber <= gDI.highBand; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
    	
    	if(gDI.relative == TRUE)		// threshold is relative to peak amplitude this bin.
    		trigRef[bandNumber] = trigger = relativeThresh;
		else if(gDI.useFile == FALSE)	// if not using a file, threshold is from dialog.		
    		trigRef[bandNumber] = trigger = gDI.threshold;
    	else							// or else it is from the file.
    		trigger = trigRef[bandNumber];

		if(gDI.average)
		{
			// gain = g0 + (1-g0) * [avg / (avg + th*th*nref)] ^ sh
    		spectralFrames[(curFrameNum * (gDynPI.halfPoints+1)) + bandNumber] = polarSpectrum[ampIndex];
			
			// simply compute the average of the five frames of sound.
			average = 0.0;
    		for(i = 0; i < 4; i++)
				average += spectralFrames[(i * (gDynPI.halfPoints+1)) + bandNumber];
			
			average *= 0.25;
			
			if(gDI.below)
				avefac = average/(average + trigger);
			else
				avefac = trigger/(average + trigger);
			if(gDI.factor > 1.0)
			{
				invFac = 1.0/gDI.factor;
				curFac[bandNumber] = invFac + (1 - invFac) * avefac;
				curFac[bandNumber] = 1.0/curFac[bandNumber];
			}
			else
				curFac[bandNumber] = gDI.factor + (1 - gDI.factor) * avefac;
		}
		else
		{
			if(gDI.below)
				if(polarSpectrum[ampIndex] < trigger)
					triggered = TRUE;
				else
					triggered = FALSE;
			else
    			if(polarSpectrum[ampIndex] > trigger)
    				triggered = TRUE;
    			else
			    	triggered = FALSE;

	    	if(triggered == TRUE)
    		{
    			curFac[bandNumber] = curFac[bandNumber] + gDI.increment;
		    	if(gDI.factor <= 1.0)
			    {
					if(curFac[bandNumber] < gDI.factor)
						curFac[bandNumber] = gDI.factor;
				}
				else
				{
					if(curFac[bandNumber] > gDI.factor)
						curFac[bandNumber] = gDI.factor;
				}
    		}
			/* curFac[] is heading to 1.0 */
    		else
			{
    			curFac[bandNumber] = curFac[bandNumber] - gDI.increment;
			    if(gDI.factor <= 1.0)
			    {
					if(curFac[bandNumber] > 1.0)
					curFac[bandNumber] = 1.0;
				}
				else
				{
					if(curFac[bandNumber] < 1.0)
						curFac[bandNumber] = 1.0;
				}
    		}
    	}
    }
    
	if(gDI.group)
	{
    	// some code to average the curFac per third octave band
		lastDivision = gDynPI.halfPoints;
		nextOctave = -0.3333333333333;
		nextDivisionFloat = EExp2(nextOctave) * gDynPI.halfPoints;
		nextDivision = (long)nextDivisionFloat;
		numberBands = 0;
		averageFac = 0.0;
    	for(bandNumber = gDynPI.halfPoints; bandNumber >= 0; bandNumber--)
    	{
    		if(bandNumber <= nextDivision)
    		{
    			averageFac = averageFac/numberBands;
   				for(i = nextDivision+1; i <= lastDivision; i++)
   					curFac[i] = averageFac;
	   			lastDivision = nextDivision;
				nextOctave -= 0.3333333333333;
				nextDivisionFloat = EExp2(nextOctave) * gDynPI.halfPoints;
				nextDivision = (long)nextDivisionFloat;
				numberBands = 0;
				averageFac = 0.0;
			}
    		averageFac += curFac[bandNumber];
    		numberBands++;
		}
    }
    
    // apply the expansion/compression factor
    for(bandNumber = gDI.lowBand; bandNumber <= gDI.highBand; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
    	tmpAmp = polarSpectrum[ampIndex] / trigRef[bandNumber];
    	tmpAmp = powf(tmpAmp, curFac[bandNumber]);
    	polarSpectrum[ampIndex] = tmpAmp * trigRef[bandNumber];
    }
    
	curFrameNum++;
    if(curFrameNum == 4)
    	curFrameNum = 0;
}

void
SetDynamicsDefaults(void)
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
	SetPort(GetDialogPort(gDynamicsDialog));
#else
	SetPort((GrafPtr)gDynamicsDialog);
#endif
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
	{
		gDynPI.points = inSIPtr->spectFrameSize;
		gDynPI.decimation = gDynPI.interpolation = inSIPtr->spectFrameIncr;
		GetDialogItem(gDynamicsDialog, D_BANDS_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);
	}
	else
	{
		gDynPI.decimation = gDynPI.interpolation = gDynPI.points >> 2;
		GetDialogItem(gDynamicsDialog, D_BANDS_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);
	}
	GetDialogItem(gDynamicsDialog, D_TYPE_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gDI.type);
	
	tmpDouble =	LLog2((double)gDynPI.points);
	tmpFloat = tmpDouble;
	tmpLong = (long)tmpFloat;	// 10 for 1024
	GetDialogItem(gDynamicsDialog, D_BANDS_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, (14 - tmpLong));
	
	NumToString(gDI.lowBand,tmpStr);
	GetDialogItem(gDynamicsDialog, D_LOW_BAND_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	tmpFloat = (gDI.lowBand * inSIPtr->sRate)/gDynPI.points;
	FixToString(tmpFloat, tmpStr);
	GetDialogItem(gDynamicsDialog, D_LOW_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	NumToString(gDI.highBand,tmpStr);
	GetDialogItem(gDynamicsDialog, D_HIGH_BAND_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	tmpFloat = (gDI.highBand * inSIPtr->sRate)/gDynPI.points;
	FixToString(tmpFloat, tmpStr);
	GetDialogItem(gDynamicsDialog, D_HIGH_FREQ_TEXT, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	if(gDI.below == TRUE)
	{
		GetDialogItem(gDynamicsDialog, D_BELOW_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		GetDialogItem(gDynamicsDialog, D_ABOVE_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
	}
	else
	{
		GetDialogItem(gDynamicsDialog, D_BELOW_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		GetDialogItem(gDynamicsDialog, D_ABOVE_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
	}

	if(gDI.average == TRUE)
	{
		GetDialogItem(gDynamicsDialog, D_AVERAGE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		HideDialogItem(gDynamicsDialog, D_TIME_FIELD);
		HideDialogItem(gDynamicsDialog, D_TIME_TEXT);
	}
	else
	{
		GetDialogItem(gDynamicsDialog, D_AVERAGE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		ShowDialogItem(gDynamicsDialog, D_TIME_FIELD);
		ShowDialogItem(gDynamicsDialog, D_TIME_TEXT);
	}

	GetDialogItem(gDynamicsDialog, D_BAND_GROUP_BOX, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gDI.group);

	tmpFloat = 20 * log10(gDI.threshold);
	FixToString(tmpFloat, tmpStr);	
	GetDialogItem(gDynamicsDialog, D_THRESH_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	if(gDI.type	== GATE)
	{
		GetDialogItem(gDynamicsDialog, D_GAIN_TEXT, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, "\pGain/Reduction (dB):");
		tmpFloat = 20 * log10(gDI.factor);
		FixToString(tmpFloat, tmpStr);	
		GetDialogItem(gDynamicsDialog, D_GAIN_FIELD, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, tmpStr);
	}
	else if(gDI.type == EXPAND)
	{
		GetDialogItem(gDynamicsDialog, D_GAIN_TEXT, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, "\pExpand Ratio (1:N):");
		FixToString(gDI.factor, tmpStr);	
		GetDialogItem(gDynamicsDialog, D_GAIN_FIELD, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, tmpStr);
	}
	else if(gDI.type == COMPRESS)
	{
		GetDialogItem(gDynamicsDialog, D_GAIN_TEXT, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, "\pCompress Ratio (N:1):");
		tmpFloat = 1.0/gDI.factor;
		FixToString(tmpFloat, tmpStr);	
		GetDialogItem(gDynamicsDialog, D_GAIN_FIELD, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, tmpStr);
	}
		
	gDI.time = gDynPI.decimation/inSIPtr->sRate;
	HMSTimeString(gDI.time, tmpStr);
	GetDialogItem(gDynamicsDialog, D_TIME_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);

	if(gDI.relative == FALSE)
	{
		GetDialogItem(gDynamicsDialog, D_RELATIVE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		ShowDialogItem(gDynamicsDialog, D_FILE_BOX);
	}
	else
	{
		GetDialogItem(gDynamicsDialog, D_RELATIVE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		HideDialogItem(gDynamicsDialog, D_FILE_BOX);
	}
	
	if(gDI.useFile == FALSE)
	{
		GetDialogItem(gDynamicsDialog, D_THRESH_TEXT, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, "\pThreshold Level (dB):");
		GetDialogItem(gDynamicsDialog, D_FILE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		HideDialogItem(gDynamicsDialog, D_FILE_TEXT);
		HideDialogItem(gDynamicsDialog, D_FILE_FIELD);
		HideDialogItem(gDynamicsDialog, D_FILE_BUTTON);
		ShowDialogItem(gDynamicsDialog, D_RELATIVE_BOX);
	}
	else
	{
		GetDialogItem(gDynamicsDialog, D_THRESH_TEXT, &itemType, &itemHandle, &itemRect);
		SetDialogItemText(itemHandle, "\pThresh. Above File (dB):");
		GetDialogItem(gDynamicsDialog, D_FILE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		ShowDialogItem(gDynamicsDialog, D_FILE_TEXT);
		ShowDialogItem(gDynamicsDialog, D_FILE_FIELD);
		ShowDialogItem(gDynamicsDialog, D_FILE_BUTTON);
		HideDialogItem(gDynamicsDialog, D_RELATIVE_BOX);
	}
	GetDialogItem(gDynamicsDialog, D_FILE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, "\p");
}

void
DeAllocSpecGateMem(void)
{
	RemovePtr((Ptr)analysisWindow);
	RemovePtr((Ptr)synthesisWindow);
	RemovePtr((Ptr)inputL);
	RemovePtr((Ptr)spectrum);
	RemovePtr((Ptr)polarSpectrum);
	RemovePtr((Ptr)outputL);
	RemovePtr((Ptr)curFacL);
	RemovePtr((Ptr)trigRefL);
	if(gDI.average)
		RemovePtr((Ptr)spectralFramesL);
	if(inSIPtr->nChans == STEREO)
	{
		RemovePtr((Ptr)inputR);
		RemovePtr((Ptr)outputR);
		RemovePtr((Ptr)curFacR);
		RemovePtr((Ptr)trigRefR);
		if(gDI.average)
			RemovePtr((Ptr)spectralFramesR);
	}

	analysisWindow = synthesisWindow = inputL = outputL = spectrum = nil;
	polarSpectrum = displaySpectrum = trigRefL = curFacL = inputR = outputR = nil;
	trigRefR = curFacR = spectralFramesL = spectralFramesR = nil;

}
