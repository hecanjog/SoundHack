/* 
 *	SoundHack- soon to be many things to many people
 *	Copyright �1994 Tom Erbe - California Institute of the Arts
 *
 *	Convolve.c - soundfile convolution and its own spectral display routines.
 */
 
#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>

#include "SoundFile.h"
#include "OpenSoundFile.h"
#include "CloseSoundFile.h"
//#include <AppleEvents.h>
#include "Binaural.h"
#include "Convolve.h"
#include "DrawFunction.h"
#include "Menu.h"
#include "Dialog.h"
#include "SoundHack.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "Misc.h"
#include "RingModulate.h"
#include "FFT.h"
#include "Windows.h"
#include "Normalization.h"
#include "ShowSpect.h"
#include "ShowSono.h"
#include "CarbonGlue.h"


/* Globals */

extern MenuHandle	gAppleMenu, gFileMenu, gEditMenu, gProcessMenu, gControlMenu,
					gWindowMenu, gSigDispMenu, gSonoDispMenu, gSpectDispMenu;
DialogPtr	gFIRDialog;
extern SoundInfoPtr	inSIPtr, filtSIPtr, outSIPtr, frontSIPtr;
extern short			gProcessEnabled, gProcessDisabled, gStopProcess;
extern long			gNumberBlocks;
extern float		Pi, twoPi;
extern Boolean		gNormalize;
extern float		*gFunction, *displaySpectrum;
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
 
short				gProcNChans;
float 				gScale, gScaleL, gScaleR, gScaleDivisor;

ConvolveInfo		gCI;

float 				*impulseLeft, *inputLeft, *outputLeft, *overlapLeft, *impulseRight,
					*inputRight, *outputRight, *overlapRight, *Window;
void
HandleFIRDialog(void)
{
	short	itemType;
	Rect	itemRect, dialogRect;
	Handle	itemHandle;
	WindInfo	*theWindInfo;
	

	gFIRDialog = GetNewDialog(FIR_DLOG,NIL_POINTER,(WindowRef)MOVE_TO_FRONT);
	
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)FIR_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gFIRDialog), (long)theWindInfo);
#else
	SetWRefCon(gFIRDialog, (long)theWindInfo);
#endif

	gProcessDisabled = UTIL_PROCESS;
	// Rename Display MENUs
	SetMenuItemText(gSigDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSpectDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSonoDispMenu, DISP_INPUT_ITEM, "\pInput");
	SetMenuItemText(gSigDispMenu, DISP_AUX_ITEM, "\pImpulse Response");
	SetMenuItemText(gSpectDispMenu, DISP_AUX_ITEM, "\pImpulse Response");
	SetMenuItemText(gSonoDispMenu, DISP_AUX_ITEM, "\pImpulse Response");
	SetMenuItemText(gSigDispMenu, DISP_OUTPUT_ITEM, "\pOutput");
	SetMenuItemText(gSpectDispMenu, DISP_OUTPUT_ITEM, "\pOutput");
	SetMenuItemText(gSonoDispMenu, DISP_OUTPUT_ITEM, "\pOutput");
	
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
	
	/* Initialize Variables */
	gCI.binaural = FALSE;
	
	if(filtSIPtr != nil)
		filtSIPtr = nil;
	
	/* Initialize dialog */
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gFIRDialog));
#else
	SetPort((GrafPtr)gFIRDialog);
#endif
	
	GetDialogItem(gFIRDialog, F_PROCESS_BUTTON, &itemType, &itemHandle, &itemRect);
	HiliteControl((ControlHandle)itemHandle, 255);
	GetDialogItem(gFIRDialog, F_MEMORY_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, "\p");

	gNormalize = FALSE;
	GetDialogItem(gFIRDialog, F_NORM_BOX, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);

	GetDialogItem(gFIRDialog, F_BRIGHTEN_BOX, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,gCI.brighten);

	GetDialogItem(gFIRDialog, F_MOVING_BOX, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gCI.moving);

	GetDialogItem(gFIRDialog, F_RING_BOX, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gCI.ringMod);
	
	gScaleDivisor = 128.0;
	GetDialogItem(gFIRDialog, F_LOW_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gFIRDialog, F_MED_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gFIRDialog, F_HIGH_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,ON);
	
	GetDialogItem(gFIRDialog, F_WINDOW_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gCI.windowType);
	
	ShowDialogItem(gFIRDialog, F_NORM_BOX);
	ShowDialogItem(gFIRDialog, F_LOW_RADIO);
	ShowDialogItem(gFIRDialog, F_MED_RADIO);
	ShowDialogItem(gFIRDialog, F_HIGH_RADIO);
	if(gCI.ringMod)
		HideDialogItem(gFIRDialog, F_BRIGHTEN_BOX);
	else
		ShowDialogItem(gFIRDialog, F_BRIGHTEN_BOX);
	HideDialogItem(gFIRDialog, F_FILTLENGTH_FIELD);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(GetWindowPort(GetDialogWindow(gFIRDialog)), &dialogRect);
	ShowWindow(GetDialogWindow(gFIRDialog));
	SelectWindow(GetDialogWindow(gFIRDialog));
#else
	dialogRect = gFIRDialog->portRect;
	ShowWindow(gFIRDialog);
	SelectWindow(gFIRDialog);
#endif
	
}

void
HandleFIRDialogEvent(short itemHit)
{
	static double	sfLengthMax, sfLength, sfLengthNew;
	long	memory;
	short	itemType;
	
	Handle	itemHandle;
	Rect	itemRect;
	Str255	sfLengthString, memoryString;
	WindInfo	*myWI;
	Boolean	okToOK;
	
	if(filtSIPtr == nil)
		okToOK = FALSE;
	else
		okToOK = TRUE;
	switch(itemHit)
	{
		case F_FILTOPEN_BUTTON:
			if(OpenSoundFile(nilFSSpec, FALSE) != -1)
			{
				if(frontSIPtr == inSIPtr)
				{
					DrawErrorMessage("\pYou can't use the input as the impulse.");
					break;
				}
				filtSIPtr = frontSIPtr;
				sfLengthMax = filtSIPtr->numBytes/(filtSIPtr->nChans*filtSIPtr->sRate*filtSIPtr->frameSize);
				sfLength = sfLengthMax;
				FixToString(sfLength, sfLengthString);
				ShowDialogItem(gFIRDialog, F_FILTLENGTH_FIELD);
				GetDialogItem(gFIRDialog, F_FILTLENGTH_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, sfLengthString);
					
				gCI.sizeImpulse = (long)(sfLength * filtSIPtr->sRate);
				gCI.sizeConvolution = 2 * gCI.sizeImpulse - 1;
				for(gCI.sizeFFT = 1; gCI.sizeFFT < gCI.sizeConvolution;)
					gCI.sizeFFT <<= 1;
				gCI.halfSizeFFT = gCI.sizeFFT >> 1;
				if(inSIPtr->nChans == STEREO || filtSIPtr->nChans == STEREO)
					gProcNChans = STEREO;
				else
					gProcNChans = MONO;
				memory = (200000L + (3L * gProcNChans * (sizeof(float) * (gCI.sizeFFT 
					+ gCI.sizeImpulse))))/1024L;
				NumToString(memory,memoryString);
				GetDialogItem(gFIRDialog, F_MEMORY_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, memoryString);
					
				GetDialogItem(gFIRDialog, F_PROCESS_BUTTON, &itemType, &itemHandle, &itemRect);
				HiliteControl((ControlHandle)itemHandle, 0);
				okToOK = TRUE;
			}
			break;
		case F_FILTLENGTH_FIELD:
			GetDialogItem(gFIRDialog,F_FILTLENGTH_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle,sfLengthString);
			StringToFix(sfLengthString, &sfLengthNew);
			if(sfLengthNew > sfLengthMax)
			{
				FixToString(sfLength, sfLengthString);
				GetDialogItem(gFIRDialog,F_FILTLENGTH_FIELD, &itemType, &itemHandle, &itemRect);				
				SetDialogItemText(itemHandle,sfLengthString);
			} 
			else
				sfLength = sfLengthNew;
			gCI.sizeImpulse = (long)(sfLength * filtSIPtr->sRate);
			gCI.sizeConvolution = 2 * gCI.sizeImpulse - 1;
			for(gCI.sizeFFT = 1; gCI.sizeFFT < gCI.sizeConvolution;)
				gCI.sizeFFT <<= 1;
			gCI.halfSizeFFT = gCI.sizeFFT >> 1;
			memory = (200000L + (3L * gProcNChans * (sizeof(float) * (gCI.sizeFFT 
				+ gCI.sizeImpulse))))/1024L;
			NumToString(memory,memoryString);
			GetDialogItem(gFIRDialog, F_MEMORY_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, memoryString);
			break;
		case F_LOW_RADIO:
			GetDialogItem(gFIRDialog, F_LOW_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gFIRDialog, F_MED_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gFIRDialog, F_HIGH_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			gScaleDivisor = 1.0;
			break;
		case F_MED_RADIO:
			GetDialogItem(gFIRDialog, F_LOW_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gFIRDialog, F_MED_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gFIRDialog, F_HIGH_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			gScaleDivisor = 16.0;
			break;
		case F_HIGH_RADIO:
			GetDialogItem(gFIRDialog, F_LOW_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gFIRDialog, F_MED_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gFIRDialog, F_HIGH_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			gScaleDivisor = 128.0;
			break;
		case F_NORM_BOX:
			GetDialogItem(gFIRDialog, F_NORM_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
			{
				SetControlValue((ControlHandle)itemHandle,OFF);
				gNormalize = FALSE;
			}
			else
			{
				SetControlValue((ControlHandle)itemHandle,ON);
				gNormalize = TRUE;
			}
			break;
		case F_BRIGHTEN_BOX:
			GetDialogItem(gFIRDialog, F_BRIGHTEN_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
			{
				SetControlValue((ControlHandle)itemHandle,OFF);
				gCI.brighten = FALSE;
			}
			else
			{
				SetControlValue((ControlHandle)itemHandle,ON);
				gCI.brighten = TRUE;
			}
			break;
		case F_RING_BOX:
			GetDialogItem(gFIRDialog, F_RING_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
			{
				SetControlValue((ControlHandle)itemHandle,OFF);
				gCI.ringMod = FALSE;
				ShowDialogItem(gFIRDialog, F_LOW_RADIO);
				ShowDialogItem(gFIRDialog, F_MED_RADIO);
				ShowDialogItem(gFIRDialog, F_HIGH_RADIO);
				ShowDialogItem(gFIRDialog, F_BRIGHTEN_BOX);
			}
			else
			{
				SetControlValue((ControlHandle)itemHandle,ON);
				gCI.ringMod = TRUE;
				HideDialogItem(gFIRDialog, F_LOW_RADIO);
				HideDialogItem(gFIRDialog, F_MED_RADIO);
				HideDialogItem(gFIRDialog, F_HIGH_RADIO);
				HideDialogItem(gFIRDialog, F_BRIGHTEN_BOX);
			}
			break;
		case F_WINDOW_MENU:
			GetDialogItem(gFIRDialog, F_WINDOW_MENU, &itemType, &itemHandle, &itemRect);
			gCI.windowType = GetControlValue((ControlHandle)itemHandle);
			if(gCI.windowType != RECTANGLE)
				gCI.windowImpulse = TRUE;
			else
				gCI.windowImpulse = FALSE;
			break;
		case F_MOVING_BOX:
			GetDialogItem(gFIRDialog, F_MOVING_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
			{
				SetControlValue((ControlHandle)itemHandle,OFF);
				gCI.moving = FALSE;
			} else
			{
				SetControlValue((ControlHandle)itemHandle,ON);
				gCI.moving = TRUE;
			}
			break;
		case F_CANCEL_BUTTON:
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gFIRDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gFIRDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gFIRDialog);
			gFIRDialog = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			inSIPtr = nil;
			MenuUpdate();
			break;
		case F_PROCESS_BUTTON:
			if(okToOK)
			{
#if TARGET_API_MAC_CARBON == 1
				myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gFIRDialog));
#else
				myWI = (WindInfoPtr)GetWRefCon(gFIRDialog);
#endif
				RemovePtr((Ptr)myWI);
				DisposeDialog(gFIRDialog);
				gFIRDialog = nil;
				if(gCI.ringMod)
					InitRingModProcess();
				else
					InitFIRProcess();
			}
			break;
	}
}
/*
 *	This is called from HandleFIRDialog() 
 *	Then process impulse.
 *	Then open outSoundFile, set FIR_PROCESS and let ConvolveBlock() take care
 *	of the rest
 */
short
InitFIRProcess(void)
{	
	long 	n;
	Str255	errStr, err2Str;
	float	tmpFloat, filtLength, inLength;

/*	Initialize variables and then blocks for convolution, gCI.sizeImpulse will have been
	initialized already by HandleFIRDialog, gCI.sizeConvolution is twice as big minus one
	as the impulse so that the full convolution is done (otherwise output is just as long as
	input), gCI.sizeFFT (FFT length) is the first power of 2 >= gCI.sizeConvolution for 
	convenient calculation */
	
	gNumberBlocks = 0;
	inLength = (float)inSIPtr->numBytes/(inSIPtr->nChans * inSIPtr->frameSize);
	filtLength = (float)filtSIPtr->numBytes/(filtSIPtr->nChans * filtSIPtr->frameSize);
	tmpFloat = (filtLength/inLength) * gCI.sizeImpulse;
	gCI.incWin = (long)tmpFloat;

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
	StringAppend(inSIPtr->sfSpec.name, "\p * ", errStr);
	StringAppend(errStr, filtSIPtr->sfSpec.name, err2Str);
	NameFile(err2Str, "\p", outSIPtr->sfSpec.name);

	outSIPtr->sRate = inSIPtr->sRate;
	outSIPtr->nChans = gProcNChans;
	outSIPtr->numBytes = 0;
	
	if(CreateSoundFile(&outSIPtr, SOUND_CUST_DIALOG) == -1)
	{
		gProcessDisabled = NO_PROCESS;
		gProcessEnabled = NO_PROCESS;
		DisposePtr((Ptr)outSIPtr);
		outSIPtr = nil;
		MenuUpdate();
		return(-1);
	}
	WriteHeader(outSIPtr);
	UpdateInfoWindow(outSIPtr);
	HandleUpdateEvent();

	if(AllocConvMem() == -1)
	{
		gProcessDisabled = NO_PROCESS;
		gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		outSIPtr = nil;
		return(-1);
	}

	UpdateProcessWindow("\p", "\p", "\psetting up impulse window", 0.0);
	HandleUpdateEvent();
	if(gCI.windowImpulse)
		GetWindow(Window, gCI.sizeImpulse, gCI.windowType);

/*	Initialize overlap	*/
	for(n = 0; n <(gCI.sizeImpulse - 1); n++)
		overlapLeft[n] = 0.0;
	
	if(gProcNChans == STEREO)
	{
		/*	Initialize overlap Right	*/
		for(n = 0; n <(gCI.sizeImpulse - 1); n++)
			overlapRight[n] = 0.0;
	}
	SetOutputScale(outSIPtr->packMode);
	SetFPos(inSIPtr->dFileNum, fsFromStart, inSIPtr->dataStart);
	gProcessEnabled = FIR_PROCESS;
	gProcessDisabled = NO_PROCESS;
	MenuUpdate();
	return(TRUE);
}

short
ConvolveBlock(void)
{
	short	curMark;
	float	length;
	long	i, j, pointer;
	float	realTemp, imagTemp, scale;
	double	position;
	long	readLength, writeLength, numSampsRead, n;
	Str255	errStr, tmpStr;
	static long	sOutPos;
	
	if(gProcessEnabled == NO_PROCESS && gStopProcess == FALSE)
		return(-1);

	if(gStopProcess == TRUE)
	{
		DeAllocConvMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	
	if(gNumberBlocks == 0)
		sOutPos = outSIPtr->dataStart;
	SetFPos(outSIPtr->dFileNum, fsFromStart, sOutPos);
	readLength = gCI.sizeImpulse;
	scale = (2.0)/gCI.sizeImpulse;
	
	if(gCI.moving == TRUE  && gCI.binaural == TRUE)
	{
 		pointer = inSIPtr->nChans * gCI.sizeImpulse * gNumberBlocks * inSIPtr->frameSize;
 		if(pointer < 0)
 			pointer = 0;
		position = InterpFunctionValue(pointer, TRUE);
		
		FixToString(position, errStr);	
		StringAppend("\pangle = ", errStr, tmpStr);
		UpdateProcessWindow("\p", "\p", tmpStr, -1.0);
		
		NewImpulse(position);
	}
	
	if((gCI.moving || gNumberBlocks == 0) && (gCI.binaural == FALSE))
	{
		UpdateProcessWindow("\p", "\p", "\pfiltering sound with impulse", -1.0);
		SetFPos(filtSIPtr->dFileNum, fsFromStart, (filtSIPtr->dataStart 
			+ (filtSIPtr->nChans * gCI.incWin * gNumberBlocks * filtSIPtr->frameSize)));
		numSampsRead = ReadStereoBlock(filtSIPtr, gCI.sizeImpulse, impulseLeft, impulseRight);
		for(n = numSampsRead; n<gCI.sizeFFT; n++)
			impulseLeft[n] = 0.0;
		if(gCI.windowImpulse)
			for(n = 0; n < gCI.sizeImpulse; n++)
				impulseLeft[n] = impulseLeft[n] * Window[n];
		RealFFT(impulseLeft, gCI.halfSizeFFT,TIME2FREQ);
		if(gCI.brighten)
			BrightenFFT(impulseLeft, gCI.halfSizeFFT);
		
		// Check MENU to see if display is checked (curMark != noMark)
		GetItemMark(gSpectDispMenu, DISP_AUX_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(impulseLeft, displaySpectrum, gCI.halfSizeFFT, filtSIPtr, LEFT, scale, "\pImpulse Channel 1: ");
		GetItemMark(gSonoDispMenu, DISP_AUX_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(impulseLeft, displaySpectrum, gCI.halfSizeFFT, filtSIPtr, LEFT, scale, "\pImpulse Channel 1: ");
		for(n = 0; n < gCI.sizeFFT; n += 2)
		{
			impulseLeft[n] *= scale;
			impulseLeft[n+1] *= scale;
		}
		if(impulseLeft != impulseRight)
		{
			for(n = numSampsRead; n<gCI.sizeFFT; n++)
				impulseRight[n] = 0.0;
			if(gCI.windowImpulse)
				for(n = 0; n < gCI.sizeImpulse; n++)
					impulseRight[n] = impulseRight[n] * Window[n];
			RealFFT(impulseRight, gCI.halfSizeFFT,TIME2FREQ);
			if(gCI.brighten)
				BrightenFFT(impulseRight, gCI.halfSizeFFT);
			// Check MENU to see if display is checked (curMark != noMark)
			GetItemMark(gSpectDispMenu, DISP_AUX_ITEM, &curMark);
			if(curMark != noMark)
				DisplaySpectrum(impulseRight, displaySpectrum, gCI.halfSizeFFT, filtSIPtr, RIGHT, scale, "\pImpulse Channel 2: ");
			GetItemMark(gSonoDispMenu, DISP_AUX_ITEM, &curMark);
			if(curMark != noMark)
				DisplaySonogram(impulseRight, displaySpectrum, gCI.halfSizeFFT, filtSIPtr, RIGHT, scale, "\pImpulse Channel 2: ");
			for(n = 0; n < gCI.sizeFFT; n += 2)
			{
				impulseRight[n] *= scale;
				impulseRight[n+1] *= scale;
			}
		}
	}

	SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart 
		+ (inSIPtr->nChans * gCI.sizeImpulse * gNumberBlocks * inSIPtr->frameSize)));
	numSampsRead = ReadStereoBlock(inSIPtr, readLength, inputLeft, inputRight);
	
	writeLength = numSampsRead;
	length = (numSampsRead + (gNumberBlocks * gCI.sizeImpulse))/inSIPtr->sRate;
	HMSTimeString(length, errStr);
	length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
	UpdateProcessWindow(errStr, "\p", "\p", length);
	for(n = numSampsRead; n < gCI.sizeFFT; n++)
		inputLeft[n] = 0.0;
	RealFFT(inputLeft, gCI.halfSizeFFT,TIME2FREQ);
	// Check MENU to see if display is checked (curMark != noMark)
	GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(inputLeft, displaySpectrum, gCI.halfSizeFFT, inSIPtr, LEFT, scale, "\pInput Channel 1: ");
	GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(inputLeft, displaySpectrum, gCI.halfSizeFFT, inSIPtr, LEFT, scale, "\pInput Channel 1: ");
	outputLeft[0] = impulseLeft[0] * inputLeft[0];
	outputLeft[1] = impulseLeft[1] * inputLeft[1];
	for(i = 2, j = 3; i < gCI.sizeFFT; i += 2, j += 2)
	{
		realTemp = impulseLeft[i] * inputLeft[i] - impulseLeft[j] * inputLeft[j];
		imagTemp = impulseLeft[i] * inputLeft[j] + impulseLeft[j] * inputLeft[i];
		outputLeft[i] = realTemp;
		outputLeft[j] = imagTemp;
	}
	// Check MENU to see if display is checked (curMark != noMark)
	GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(outputLeft, displaySpectrum, gCI.halfSizeFFT, outSIPtr, LEFT, scale*gScaleDivisor, "\pOutput Channel 1: ");
	GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(outputLeft, displaySpectrum, gCI.halfSizeFFT, outSIPtr, LEFT, scale*gScaleDivisor, "\pOutput Channel 1: ");
	RealFFT(outputLeft, gCI.halfSizeFFT,FREQ2TIME);
	for(n = 0; n < (gCI.sizeImpulse - 1); n++)
	{
		outputLeft[n] = outputLeft[n] + overlapLeft[n];
		overlapLeft[n] = outputLeft[gCI.sizeImpulse + n];
	}
	if(outputLeft != outputRight)
	{
		for(n = numSampsRead; n < gCI.sizeFFT; n++)
			inputRight[n] = 0.0;
		RealFFT(inputRight, gCI.halfSizeFFT,TIME2FREQ);
		if(inSIPtr->nChans == 2)
		{
			// Check MENU to see if display is checked (curMark != noMark)
			GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
			if(curMark != noMark)
				DisplaySpectrum(inputRight, displaySpectrum, gCI.halfSizeFFT, inSIPtr, RIGHT, scale, "\pInput Channel 2: ");
			GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
			if(curMark != noMark)
				DisplaySonogram(inputRight, displaySpectrum, gCI.halfSizeFFT, inSIPtr, RIGHT, scale, "\pInput Channel 2: ");
		}
		outputRight[0] = impulseRight[0] * inputRight[0];
		outputRight[1] = impulseRight[1] * inputRight[1];
		for(i = 2, j = 3; i < gCI.sizeFFT; i += 2, j += 2)
		{
			realTemp = impulseRight[i] * inputRight[i] - impulseRight[j] * inputRight[j];
			imagTemp = impulseRight[i] * inputRight[j] + impulseRight[j] * inputRight[i];
			outputRight[i] = realTemp;
			outputRight[j] = imagTemp;
		}
		// Check MENU to see if display is checked (curMark != noMark)
		GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(inputRight, displaySpectrum, gCI.halfSizeFFT, outSIPtr, RIGHT, scale*gScaleDivisor, "\pOutput Channel 2: ");
		GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(inputRight, displaySpectrum, gCI.halfSizeFFT, outSIPtr, RIGHT, scale*gScaleDivisor, "\pOutput Channel 2: ");
		RealFFT(outputRight, gCI.halfSizeFFT,FREQ2TIME);
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
		if(outSIPtr->nChans == STEREO)
			WriteStereoBlock(outSIPtr, (gCI.sizeImpulse - 1), (outputLeft + numSampsRead), (outputRight + numSampsRead));
		else
			WriteMonoBlock(outSIPtr, (gCI.sizeImpulse - 1), (outputLeft + numSampsRead));
		length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
		HMSTimeString(length, errStr);
		UpdateProcessWindow("\p", errStr, "\p", -1.0);
		gNumberBlocks = 0;
		DeAllocConvMem();
		FlushVol((StringPtr)NIL_POINTER,outSIPtr->sfSpec.vRefNum);
		if(gNormalize == TRUE && outSIPtr->packMode != SF_FORMAT_TEXT)
			InitNormProcess(outSIPtr);
		else
			FinishProcess();
	}
	return(TRUE);
}


short
AllocConvMem(void)
{
	long 	sizeImpulse, sizeOverlap, sizeInput, sizeWindow;
	OSErr	error = 0;
		
/*
 * Allocate memory for impulse, input and output (input and output are two pointers
 * to the same data), for gCI.sizeFFT*sizeof(float), overlap holds the overlapped
 * segments, and is one sample smaller than impulse and soundBlock for
 * gCI.sizeImpulse*sizeof(short)
 */
	
	sizeImpulse = (long)((gCI.sizeFFT) * sizeof(float));
	sizeInput = (long)((gCI.sizeFFT) * sizeof(float));
	sizeOverlap = (long)((gCI.sizeFFT+2) * sizeof(float));
	if(gCI.ringMod)
	{
		sizeWindow = (long)(gCI.sizeConvolution * sizeof(float));
	}
	else
	{
		sizeWindow = (long)(gCI.sizeImpulse * sizeof(float));
	}
	Window = displaySpectrum = impulseLeft = inputLeft = outputLeft = overlapLeft = nil;
	impulseRight = inputRight = outputRight = overlapRight  = nil;
	
	if(gCI.windowImpulse)
	{
		Window = (float *)NewPtr(sizeWindow);
		error += MemError();
	}
	displaySpectrum = (float *)NewPtr((gCI.halfSizeFFT+1)*sizeof(float));
	error += MemError();
	impulseLeft = (float *)NewPtr(sizeImpulse);
	error += MemError();
	inputLeft = outputLeft = (float *)NewPtr(sizeInput);
	error += MemError();
	overlapLeft = (float *)NewPtr(sizeOverlap);
	error += MemError();
	
	if(filtSIPtr->nChans == STEREO)
	{
		impulseRight = (float *)NewPtr(sizeImpulse);
		error += MemError();
	}
	else
	{
		impulseRight = impulseLeft;
		error += MemError();
	}
	if(gProcNChans == STEREO)
	{
		inputRight = outputRight = (float *)NewPtr(sizeInput);
		error += MemError();
		overlapRight = (float *)NewPtr(sizeOverlap);
		error += MemError();
	} else
	{
		outputRight = inputRight = inputLeft;
		error += MemError();
		overlapRight = overlapLeft;
	}
	
	if((overlapLeft && overlapRight && impulseLeft && impulseRight && inputLeft && inputRight) == 0)
	{
		DrawErrorMessage("\pNot enough memory for processing");
		DeAllocConvMem();
		return(-1);
	}
	if(error != 0)
	{
		DrawErrorMessage("\pMemory allocation error");
		DeAllocConvMem();
		return(-1);
	}
	return(TRUE);
}

void
DeAllocConvMem(void)
{
	OSErr	error = 0;

	if(gCI.windowImpulse)
	{
		RemovePtr((Ptr)Window);
		error += MemError();
	}
		
	RemovePtr((Ptr)displaySpectrum);
	error += MemError();
	RemovePtr((Ptr)impulseLeft);
	error += MemError();
	RemovePtr((Ptr)inputLeft);
	error += MemError();
	RemovePtr((Ptr)overlapLeft);
	error += MemError();
	if(filtSIPtr->nChans == STEREO)
	{
		RemovePtr((Ptr)impulseRight);
		error += MemError();
	}
	if(gProcNChans == STEREO)
	{
		RemovePtr((Ptr)inputRight);
		error += MemError();
		RemovePtr((Ptr)overlapRight);
		error += MemError();
	}
		
	Window = displaySpectrum = impulseLeft = inputLeft = outputLeft = overlapLeft = nil;
	impulseRight = inputRight = outputRight = overlapRight = nil;
}

void
BrightenFFT(float cartSpectrum[], long halfLengthFFT)
{		
	long	realIndex, imagIndex;
	float	twoOverHalfFFT;
	long	bandNumber;
	
	twoOverHalfFFT = 64.0/halfLengthFFT;
/*
 * unravel RealFFT-format spectrum: note that halfLengthFFT+1 pairs of values are produced
 * & give it a 6dB per octave high pass at the sample rate
 */
    for (bandNumber = 0; bandNumber <= halfLengthFFT; bandNumber++)
    {
		realIndex = bandNumber<<1;
		imagIndex = realIndex + 1;
    	if(bandNumber == 0)
    		cartSpectrum[realIndex] = cartSpectrum[realIndex] * (float)bandNumber * twoOverHalfFFT;
    	else if(bandNumber == halfLengthFFT)
    		cartSpectrum[1] = cartSpectrum[1] * (float)bandNumber * twoOverHalfFFT;
    	else
    	{
			cartSpectrum[realIndex] = cartSpectrum[realIndex] * (float)bandNumber * twoOverHalfFFT;
			cartSpectrum[imagIndex] = cartSpectrum[imagIndex] * (float)bandNumber * twoOverHalfFFT;
		}
    }
}