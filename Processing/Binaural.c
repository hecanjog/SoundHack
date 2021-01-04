#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
#include "SoundFile.h"
#include "Convolve.h"
#include "DrawFunction.h"
#include "Menu.h"
#include "Macros.h"
#include "Dialog.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "SoundHack.h"
#include "Binaural.h"
#include "Misc.h"
#include "Windows.h"
#include "FFT.h"
#include "CarbonGlue.h"
//#include <AppleEvents.h>

/* Globals */
#include "ImpulsesC.h"

extern MenuHandle	gAppleMenu, gFileMenu, gEditMenu, gProcessMenu, gControlMenu, 
					gSigDispMenu, gSpectDispMenu, gSonoDispMenu;
extern DialogPtr	gDrawFunctionDialog;
DialogPtr	gBinauralDialog;
extern SoundInfoPtr	inSIPtr, filtSIPtr, outSIPtr;
extern ConvolveInfo	gCI;

extern Boolean		gNormalize;
extern short			gProcessEnabled, gProcessDisabled, gStopProcess, gProcNChans;
extern long			gNumberBlocks;
extern float 		Pi, twoPi, gScale, gScaleL, gScaleR, gScaleDivisor;
extern float		*gFunction;
extern float 		*impulseLeft, *overlapLeft, *impulseRight, *overlapRight;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

// dummy SoundInfo structure, because convolve routines assume existance of filtSIPtr
SoundInfo			binSI;

void
HandleBinauralDialog(void)
{
	short	itemType, i;
	Rect	itemRect;
	Handle	itemHandle;
	Str255	positionStr;
	WindInfo *theWindInfo;

	gBinauralDialog = GetNewDialog(BINAURAL_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)BINAURAL_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gBinauralDialog), (long)theWindInfo);
#else
	SetWRefCon(gBinauralDialog, (long)theWindInfo);
#endif

	/* Allocate memory for control function soundfile i/o*/	

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
	DisableItem(gSigDispMenu, DISP_AUX_ITEM);
	DisableItem(gSpectDispMenu, DISP_AUX_ITEM);
	DisableItem(gSonoDispMenu, DISP_AUX_ITEM);
	EnableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
	EnableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
	EnableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	MenuUpdate();
	
	gNumberBlocks = 0;
	for(i = 0; i < 400; i++)
		gFunction[i] = 0.0;
	BinauralButtonsOff();
	GetDialogItem(gBinauralDialog, B_HEIGHT_CNTL, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gCI.binHeight + 1);
	gCI.binHeight = GetControlValue((ControlHandle)itemHandle) - 1;
	SetBinauralButtons(gCI.binPosition);
	NumToString(gCI.binPosition, positionStr);
	GetDialogItem(gBinauralDialog, B_MOVE_BOX, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gCI.moving);
	if(gCI.moving)
		ShowDialogItem(gBinauralDialog, B_MOVE_BUTTON);
	else
		HideDialogItem(gBinauralDialog, B_MOVE_BUTTON);
		
	GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, positionStr);
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gBinauralDialog));
	SelectWindow(GetDialogWindow(gBinauralDialog));
#else
	ShowWindow(gBinauralDialog);
	SelectWindow(gBinauralDialog);
#endif
}

void
HandleBinauralDialogEvent(short itemHit)
{
	short	itemType, i;
	Rect	itemRect;
	Handle	itemHandle;
	long	inLength;
	Str255	positionStr;
	WindInfo	*myWI;
	
	inLength = inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize);
	
	switch(itemHit)
	{
		case B_0_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 0;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_0_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_30_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 30;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_30_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_60_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 60;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_60_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_90_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 90;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_90_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_120_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 120;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_120_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_150_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 150;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_150_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_180_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 180;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_180_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_210_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 210;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_210_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_240_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 240;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_240_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_270_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 270;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_270_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_300_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 300;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_300_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_330_RADIO:
			BinauralButtonsOff();
			gCI.binPosition = 330;
			NumToString(gCI.binPosition, positionStr);
			GetDialogItem(gBinauralDialog, B_330_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, positionStr);
			break;
		case B_DEGREE_FIELD:
			BinauralButtonsOff();
			GetDialogItem(gBinauralDialog, B_DEGREE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, positionStr);
			StringToNum(positionStr, &gCI.binPosition);
			while(gCI.binPosition >= 360)
				gCI.binPosition -= 360;
			NumToString(gCI.binPosition, positionStr);
			SetDialogItemText(itemHandle, positionStr);
			SetBinauralButtons(gCI.binPosition);
			break;
		case B_HEIGHT_CNTL:
			GetDialogItem(gBinauralDialog, B_HEIGHT_CNTL, &itemType, &itemHandle, &itemRect);
			gCI.binHeight = GetControlValue((ControlHandle)itemHandle) - 1;
			break;
		case B_MOVE_BOX:
			GetDialogItem(gBinauralDialog, B_MOVE_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
			{
				SetControlValue((ControlHandle)itemHandle,OFF);
				gCI.moving = FALSE;
				HideDialogItem(gBinauralDialog, B_MOVE_BUTTON);
				ShowDialogItem(gBinauralDialog, B_DEGREE_FIELD);
			} else
			{
				SetControlValue((ControlHandle)itemHandle,ON);
				gCI.moving = TRUE;
				ShowDialogItem(gBinauralDialog, B_MOVE_BUTTON);
				HideDialogItem(gBinauralDialog, B_DEGREE_FIELD);
				for(i = 0; i < 400; i++)
					gFunction[i] = 0.0;
			}
			break;
		case B_MOVE_BUTTON:
			HandleDrawFunctionDialog(360.0, 0.0, 1.0, "\pDegrees:", inLength, TRUE);
			break;
		case B_CANCEL_BUTTON:
			inSIPtr = nil;
			gProcessDisabled = NO_PROCESS;
			gProcessEnabled = NO_PROCESS;
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
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gBinauralDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gBinauralDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gBinauralDialog);
			gBinauralDialog = nil;
			MenuUpdate();
			break;
		case B_PROCESS_BUTTON:
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
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gBinauralDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gBinauralDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gBinauralDialog);
			gBinauralDialog = nil;
			InitBinauralProcess(gCI.binPosition);
			break;
	}
}

void
SetBinauralButtons(long position)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	long	nearPosition;
	
	nearPosition = 30 * (long)(position/30);
	if(position >= 0 & position < 30)
	{
		GetDialogItem(gBinauralDialog, B_0_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_30_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 30 & position < 60)
	{
		GetDialogItem(gBinauralDialog, B_30_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_60_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 60 & position < 90)
	{
		GetDialogItem(gBinauralDialog, B_60_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_90_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 90 & position < 120)
	{
		GetDialogItem(gBinauralDialog, B_90_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_120_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 120 & position < 150)
	{
		GetDialogItem(gBinauralDialog, B_120_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_150_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 150 & position < 180)
	{
		GetDialogItem(gBinauralDialog, B_150_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_180_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 180 & position < 210)
	{
		GetDialogItem(gBinauralDialog, B_180_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_210_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 210 & position < 240)
	{
		GetDialogItem(gBinauralDialog, B_210_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_240_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 240 & position < 270)
	{
		GetDialogItem(gBinauralDialog, B_240_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_270_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 270 & position < 300)
	{
		GetDialogItem(gBinauralDialog, B_270_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_300_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}else if(position >= 300 & position < 330)
	{
		GetDialogItem(gBinauralDialog, B_300_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_330_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}
	if(position >= 330 & position < 360)
	{
		GetDialogItem(gBinauralDialog, B_330_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		if(nearPosition != position)
		{
			GetDialogItem(gBinauralDialog, B_0_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
		}
	}
}

void
BinauralButtonsOff(void)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	
	GetDialogItem(gBinauralDialog, B_0_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_30_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_60_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_90_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_120_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_150_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_180_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_210_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_240_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_270_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_300_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
	GetDialogItem(gBinauralDialog, B_330_RADIO, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle,OFF);
}

/* This uses functions from Convolve.c since it is very similar */
short
InitBinauralProcess(long position)
{	
	long 	n;
	float	floatPosition;
	Str255	errStr, err2Str;

/*	Initialize variables and then blocks for convolution, gCI.sizeImpulse will have been
	initialized already by HandleFIRDialog, gCI.sizeConvolution is twice as big minus one
	as the impulse so that the full convolution is done (otherwise output is just as long as
	input), gCI.sizeFFT (FFT length) is the first power of 2 >= gCI.sizeConvolution for 
	convenient calculation */
	
	gProcNChans = 2;
	gCI.windowType = RECTANGLE;
	gCI.windowImpulse = FALSE;
	gCI.ringMod = FALSE;
	gCI.brighten = FALSE;
	gCI.binaural = TRUE;
	gCI.sizeImpulse = 128;
	gCI.sizeConvolution = 2 * gCI.sizeImpulse - 1;
	for(gCI.sizeFFT = 1; gCI.sizeFFT < gCI.sizeConvolution;)
		gCI.sizeFFT <<= 1;
	gCI.halfSizeFFT = gCI.sizeFFT >> 1;
	gScaleDivisor = 0.8;
	floatPosition = (float)position;
	
	// the binaural impulse is used by processes that usually handle soundfiles, so we need to
	// create a dummy SoundInfo structure and point filtSIPtr to it.
	StringCopy("\pBinaural Impulse", binSI.sfSpec.name);
	binSI.sRate = 44100.0;
	binSI.numBytes = 128 * 4;
	binSI.packMode = SF_FORMAT_32_FLOAT;
	StringCopy("\pnot compressed", binSI.compName);
	binSI.nChans = STEREO;
	binSI.frameSize = 4;
	binSI.sfType = RAW;
	binSI.dFileNum = -1;
	binSI.rFileNum = -1;
	binSI.sigDisp.windo = (WindowPtr)(-1L);
	binSI.sigDisp.offScreen = NIL_POINTER;
	binSI.spectDisp.windo = (WindowPtr)(-1L);
	binSI.spectDisp.offScreen = NIL_POINTER;
	binSI.sonoDisp.windo = (WindowPtr)(-1L);
	binSI.sonoDisp.offScreen = NIL_POINTER;

	filtSIPtr = &binSI;

	// Create SoundInfo structure for output
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
	if(gCI.moving == TRUE)
		NameFile(inSIPtr->sfSpec.name, "\pBNMV", outSIPtr->sfSpec.name);
	else
	{
		NumToString(position, errStr);
		StringAppend("\pBN", errStr, err2Str);
		NameFile(inSIPtr->sfSpec.name, err2Str, outSIPtr->sfSpec.name);
	}
	outSIPtr->sRate = inSIPtr->sRate;
	outSIPtr->nChans = 2;
	outSIPtr->numBytes = 0;

	if(CreateSoundFile(&outSIPtr, SOUND_CUST_DIALOG) == -1)
	{
		gProcessDisabled = NO_PROCESS;
		gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		DisposePtr((Ptr)outSIPtr);
		outSIPtr = nil;
		return(-1);
	}
	WriteHeader(outSIPtr);
	UpdateInfoWindow(outSIPtr);
	
	if(AllocConvMem() == -1)
	{
		gProcessDisabled = NO_PROCESS;
		gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		outSIPtr = nil;
		return(-1);
	}
	
	
	// Copy or mix impulse memory.
	NewImpulse(floatPosition);
	
/*	Initialize overlap	*/
	for(n = 0; n <(gCI.sizeImpulse - 1); n++)
		overlapLeft[n] = 0.0;
	for(n = 0; n <(gCI.sizeImpulse - 1); n++)
		overlapRight[n] = 0.0;
	SetOutputScale(outSIPtr->packMode);	

	gProcessEnabled = FIR_PROCESS;
	gProcessDisabled = NO_PROCESS;
	MenuUpdate();
	return(TRUE);
}

short
NewImpulse(float position)
{
	static float	oldPosition = -1;
	long	nearPosition, n;
	float ratio, scale;
	Str255 angleStr, messageStr;
	
	while(position >= 360)
		position -= 360;
	
	if(position == oldPosition && gNumberBlocks != 0)
		return(-1);
	oldPosition = position;
			
	scale = (2.0)/gCI.sizeImpulse;

	FixToString(position, angleStr);
	StringAppend("\pangle = ", angleStr, messageStr);
	UpdateProcessWindow("\p", "\p", messageStr, -1.0);
				
	nearPosition = 30 * (short)(position/30.0);
	ratio = (float)(position - nearPosition)/30.0;
	/*	Copy Appropriate impulse to impulseLeft and impulseRight with appropriate delay */
	if(ratio == 0.0)
	{
		if(gCI.binHeight == ELEVATED)
		{
			switch(nearPosition)
			{
				case 0:
					CopyImpulse(a000e, impulseLeft, impulseDelayE[0]);
					CopyImpulse(a360e, impulseRight, impulseDelayE[0]);
					break;
				case 30:
					CopyImpulse(a030e, impulseLeft, impulseDelayE[1]);
					CopyImpulse(a330e, impulseRight, impulseDelayE[11]);
					break;
				case 60:
					CopyImpulse(a060e, impulseLeft, impulseDelayE[2]);
					CopyImpulse(a300e, impulseRight, impulseDelayE[10]);
					break;
				case 90:
					CopyImpulse(a090e, impulseLeft, impulseDelayE[3]);
					CopyImpulse(a270e, impulseRight, impulseDelayE[9]);
					break;
				case 120:
					CopyImpulse(a120e, impulseLeft, impulseDelayE[4]);
					CopyImpulse(a240e, impulseRight, impulseDelayE[8]);
					break;
				case 150:
					CopyImpulse(a150e, impulseLeft, impulseDelayE[5]);
					CopyImpulse(a210e, impulseRight, impulseDelayE[7]);
					break;
				case 180:
					CopyImpulse(a180e, impulseLeft, impulseDelayE[6]);
					CopyImpulse(a181e, impulseRight, impulseDelayE[6]);
					break;
				case 210:
					CopyImpulse(a210e, impulseLeft, impulseDelayE[7]);
					CopyImpulse(a150e, impulseRight, impulseDelayE[5]);
					break;
				case 240:
					CopyImpulse(a240e, impulseLeft, impulseDelayE[8]);
					CopyImpulse(a120e, impulseRight, impulseDelayE[4]);
					break;
				case 270:
					CopyImpulse(a270e, impulseLeft, impulseDelayE[9]);
					CopyImpulse(a090e, impulseRight, impulseDelayE[3]);
					break;
				case 300:
					CopyImpulse(a300e, impulseLeft, impulseDelayE[10]);
					CopyImpulse(a060e, impulseRight, impulseDelayE[2]);
					break;
				case 330:
					CopyImpulse(a330e, impulseLeft, impulseDelayE[11]);
					CopyImpulse(a030e, impulseRight, impulseDelayE[1]);
					break;
				default:
					DrawErrorMessage("\pImpulse didn't copy. Acch!");
					break;
			}
		}
		else if(gCI.binHeight == EAR_LEVEL)
		{
			switch(nearPosition)
			{
				case 0:
					CopyImpulse(a000z, impulseLeft, impulseDelayZ[0]);
					CopyImpulse(a360z, impulseRight, impulseDelayZ[0]);
					break;
				case 30:
					CopyImpulse(a030z, impulseLeft, impulseDelayZ[1]);
					CopyImpulse(a330z, impulseRight, impulseDelayZ[11]);
					break;
				case 60:
					CopyImpulse(a060z, impulseLeft, impulseDelayZ[2]);
					CopyImpulse(a300z, impulseRight, impulseDelayZ[10]);
					break;
				case 90:
					CopyImpulse(a090z, impulseLeft, impulseDelayZ[3]);
					CopyImpulse(a270z, impulseRight, impulseDelayZ[9]);
					break;
				case 120:
					CopyImpulse(a120z, impulseLeft, impulseDelayZ[4]);
					CopyImpulse(a240z, impulseRight, impulseDelayZ[8]);
					break;
				case 150:
					CopyImpulse(a150z, impulseLeft, impulseDelayZ[5]);
					CopyImpulse(a210z, impulseRight, impulseDelayZ[7]);
					break;
				case 180:
					CopyImpulse(a180z, impulseLeft, impulseDelayZ[6]);
					CopyImpulse(a181z, impulseRight, impulseDelayZ[6]);
					break;
				case 210:
					CopyImpulse(a210z, impulseLeft, impulseDelayZ[7]);
					CopyImpulse(a150z, impulseRight, impulseDelayZ[5]);
					break;
				case 240:
					CopyImpulse(a240z, impulseLeft, impulseDelayZ[8]);
					CopyImpulse(a120z, impulseRight, impulseDelayZ[4]);
					break;
				case 270:
					CopyImpulse(a270z, impulseLeft, impulseDelayZ[9]);
					CopyImpulse(a090z, impulseRight, impulseDelayZ[3]);
					break;
				case 300:
					CopyImpulse(a300z, impulseLeft, impulseDelayZ[10]);
					CopyImpulse(a060z, impulseRight, impulseDelayZ[2]);
					break;
				case 330:
					CopyImpulse(a330z, impulseLeft, impulseDelayZ[11]);
					CopyImpulse(a030z, impulseRight, impulseDelayZ[1]);
					break;
				default:
					DrawErrorMessage("\pImpulse didn't copy. Acch!");
					break;
			}
		}
		else if(gCI.binHeight == LOWERED)
		{
			switch(nearPosition)
			{
				case 0:
					CopyImpulse(a000l, impulseLeft, impulseDelayL[0]);
					CopyImpulse(a360l, impulseRight, impulseDelayL[0]);
					break;
				case 30:
					CopyImpulse(a030l, impulseLeft, impulseDelayL[1]);
					CopyImpulse(a330l, impulseRight, impulseDelayL[11]);
					break;
				case 60:
					CopyImpulse(a060l, impulseLeft, impulseDelayL[2]);
					CopyImpulse(a300l, impulseRight, impulseDelayL[10]);
					break;
				case 90:
					CopyImpulse(a090l, impulseLeft, impulseDelayL[3]);
					CopyImpulse(a270l, impulseRight, impulseDelayL[9]);
					break;
				case 120:
					CopyImpulse(a120l, impulseLeft, impulseDelayL[4]);
					CopyImpulse(a240l, impulseRight, impulseDelayL[8]);
					break;
				case 150:
					CopyImpulse(a150l, impulseLeft, impulseDelayL[5]);
					CopyImpulse(a210l, impulseRight, impulseDelayL[7]);
					break;
				case 180:
					CopyImpulse(a180l, impulseLeft, impulseDelayL[6]);
					CopyImpulse(a181l, impulseRight, impulseDelayL[6]);
					break;
				case 210:
					CopyImpulse(a210l, impulseLeft, impulseDelayL[7]);
					CopyImpulse(a150l, impulseRight, impulseDelayL[5]);
					break;
				case 240:
					CopyImpulse(a240l, impulseLeft, impulseDelayL[8]);
					CopyImpulse(a120l, impulseRight, impulseDelayL[4]);
					break;
				case 270:
					CopyImpulse(a270l, impulseLeft, impulseDelayL[9]);
					CopyImpulse(a090l, impulseRight, impulseDelayL[3]);
					break;
				case 300:
					CopyImpulse(a300l, impulseLeft, impulseDelayL[10]);
					CopyImpulse(a060l, impulseRight, impulseDelayL[2]);
					break;
				case 330:
					CopyImpulse(a330l, impulseLeft, impulseDelayL[11]);
					CopyImpulse(a030l, impulseRight, impulseDelayL[1]);
					break;
				default:
					DrawErrorMessage("\pImpulse didn't copy. Acch!");
					break;
			}
		}
	}
	else
	{
		if(gCI.binHeight == ELEVATED)
		{
			switch(nearPosition)
			{
				case 0:
					MixImpulse(a000e, a030e, impulseLeft, ratio, impulseDelayE[0], impulseDelayE[1]);
					MixImpulse(a360e, a330e, impulseRight, ratio, impulseDelayE[0], impulseDelayE[11]);
					break;
				case 30:
					MixImpulse(a030e, a060e, impulseLeft, ratio, impulseDelayE[1], impulseDelayE[2]);
					MixImpulse(a330e, a300e, impulseRight, ratio, impulseDelayE[11], impulseDelayE[10]);
					break;
				case 60:
					MixImpulse(a060e, a090e, impulseLeft, ratio, impulseDelayE[2], impulseDelayE[3]);
					MixImpulse(a300e, a270e, impulseRight, ratio, impulseDelayE[10], impulseDelayE[9]);
					break;
				case 90:
					MixImpulse(a090e, a120e, impulseLeft, ratio, impulseDelayE[3], impulseDelayE[4]);
					MixImpulse(a270e, a240e, impulseRight, ratio, impulseDelayE[9], impulseDelayE[8]);
					break;
				case 120:
					MixImpulse(a120e, a150e, impulseLeft, ratio, impulseDelayE[4], impulseDelayE[5]);
					MixImpulse(a240e, a210e, impulseRight, ratio, impulseDelayE[8], impulseDelayE[7]);
					break;
				case 150:
					MixImpulse(a150e, a180e, impulseLeft, ratio, impulseDelayE[5], impulseDelayE[6]);
					MixImpulse(a210e, a181e, impulseRight, ratio, impulseDelayE[7], impulseDelayE[6]);
					break;
				case 180:
					MixImpulse(a180e, a210e, impulseLeft, ratio, impulseDelayE[6], impulseDelayE[7]);
					MixImpulse(a181e, a150e, impulseRight, ratio, impulseDelayE[6], impulseDelayE[5]);
					break;
				case 210:
					MixImpulse(a210e, a240e, impulseLeft, ratio, impulseDelayE[7], impulseDelayE[8]);
					MixImpulse(a150e, a120e, impulseRight, ratio, impulseDelayE[5], impulseDelayE[4]);
					break;
				case 240:
					MixImpulse(a240e, a270e, impulseLeft, ratio, impulseDelayE[8], impulseDelayE[9]);
					MixImpulse(a120e, a090e, impulseRight, ratio, impulseDelayE[4], impulseDelayE[3]);
					break;
				case 270:
					MixImpulse(a270e, a300e, impulseLeft, ratio, impulseDelayE[9], impulseDelayE[10]);
					MixImpulse(a090e, a060e, impulseRight, ratio, impulseDelayE[3], impulseDelayE[2]);
					break;
				case 300:
					MixImpulse(a300e, a330e, impulseLeft, ratio, impulseDelayE[10], impulseDelayE[11]);
					MixImpulse(a060e, a030e, impulseRight, ratio, impulseDelayE[2], impulseDelayE[1]);
					break;
				case 330:
					MixImpulse(a330e, a000e, impulseLeft, ratio, impulseDelayE[11], impulseDelayE[0]);
					MixImpulse(a030e, a360e, impulseRight, ratio, impulseDelayE[1], impulseDelayE[0]);
					break;
				default:
					DrawErrorMessage("\pImpulse didn't mix. Acch!");
					break;
			}
		}
		else if(gCI.binHeight == EAR_LEVEL)
		{
			switch(nearPosition)
			{
				case 0:
					MixImpulse(a000z, a030z, impulseLeft, ratio, impulseDelayZ[0], impulseDelayZ[1]);
					MixImpulse(a360z, a330z, impulseRight, ratio, impulseDelayZ[0], impulseDelayZ[11]);
					break;
				case 30:
					MixImpulse(a030z, a060z, impulseLeft, ratio, impulseDelayZ[1], impulseDelayZ[2]);
					MixImpulse(a330z, a300z, impulseRight, ratio, impulseDelayZ[11], impulseDelayZ[10]);
					break;
				case 60:
					MixImpulse(a060z, a090z, impulseLeft, ratio, impulseDelayZ[2], impulseDelayZ[3]);
					MixImpulse(a300z, a270z, impulseRight, ratio, impulseDelayZ[10], impulseDelayZ[9]);
					break;
				case 90:
					MixImpulse(a090z, a120z, impulseLeft, ratio, impulseDelayZ[3], impulseDelayZ[4]);
					MixImpulse(a270z, a240z, impulseRight, ratio, impulseDelayZ[9], impulseDelayZ[8]);
					break;
				case 120:
					MixImpulse(a120z, a150z, impulseLeft, ratio, impulseDelayZ[4], impulseDelayZ[5]);
					MixImpulse(a240z, a210z, impulseRight, ratio, impulseDelayZ[8], impulseDelayZ[7]);
					break;
				case 150:
					MixImpulse(a150z, a180z, impulseLeft, ratio, impulseDelayZ[5], impulseDelayZ[6]);
					MixImpulse(a210z, a181z, impulseRight, ratio, impulseDelayZ[7], impulseDelayZ[6]);
					break;
				case 180:
					MixImpulse(a180z, a210z, impulseLeft, ratio, impulseDelayZ[6], impulseDelayZ[7]);
					MixImpulse(a181z, a150z, impulseRight, ratio, impulseDelayZ[6], impulseDelayZ[5]);
					break;
				case 210:
					MixImpulse(a210z, a240z, impulseLeft, ratio, impulseDelayZ[7], impulseDelayZ[8]);
					MixImpulse(a150z, a120z, impulseRight, ratio, impulseDelayZ[5], impulseDelayZ[4]);
					break;
				case 240:
					MixImpulse(a240z, a270z, impulseLeft, ratio, impulseDelayZ[8], impulseDelayZ[9]);
					MixImpulse(a120z, a090z, impulseRight, ratio, impulseDelayZ[4], impulseDelayZ[3]);
					break;
				case 270:
					MixImpulse(a270z, a300z, impulseLeft, ratio, impulseDelayZ[9], impulseDelayZ[10]);
					MixImpulse(a090z, a060z, impulseRight, ratio, impulseDelayZ[3], impulseDelayZ[2]);
					break;
				case 300:
					MixImpulse(a300z, a330z, impulseLeft, ratio, impulseDelayZ[10], impulseDelayZ[11]);
					MixImpulse(a060z, a030z, impulseRight, ratio, impulseDelayZ[2], impulseDelayZ[1]);
					break;
				case 330:
					MixImpulse(a330z, a000z, impulseLeft, ratio, impulseDelayZ[11], impulseDelayZ[0]);
					MixImpulse(a030z, a360z, impulseRight, ratio, impulseDelayZ[1], impulseDelayZ[0]);
					break;
				default:
					DrawErrorMessage("\pImpulse didn't mix. Acch!");
					break;
			}
		}
		else if(gCI.binHeight == LOWERED)
		{
			switch(nearPosition)
			{
				case 0:
					MixImpulse(a000l, a030l, impulseLeft, ratio, impulseDelayL[0], impulseDelayL[1]);
					MixImpulse(a360l, a330l, impulseRight, ratio, impulseDelayL[0], impulseDelayL[11]);
					break;
				case 30:
					MixImpulse(a030l, a060l, impulseLeft, ratio, impulseDelayL[1], impulseDelayL[2]);
					MixImpulse(a330l, a300l, impulseRight, ratio, impulseDelayL[11], impulseDelayL[10]);
					break;
				case 60:
					MixImpulse(a060l, a090l, impulseLeft, ratio, impulseDelayL[2], impulseDelayL[3]);
					MixImpulse(a300l, a270l, impulseRight, ratio, impulseDelayL[10], impulseDelayL[9]);
					break;
				case 90:
					MixImpulse(a090l, a120l, impulseLeft, ratio, impulseDelayL[3], impulseDelayL[4]);
					MixImpulse(a270l, a240l, impulseRight, ratio, impulseDelayL[9], impulseDelayL[8]);
					break;
				case 120:
					MixImpulse(a120l, a150l, impulseLeft, ratio, impulseDelayL[4], impulseDelayL[5]);
					MixImpulse(a240l, a210l, impulseRight, ratio, impulseDelayL[8], impulseDelayL[7]);
					break;
				case 150:
					MixImpulse(a150l, a180l, impulseLeft, ratio, impulseDelayL[5], impulseDelayL[6]);
					MixImpulse(a210l, a181l, impulseRight, ratio, impulseDelayL[7], impulseDelayL[6]);
					break;
				case 180:
					MixImpulse(a180l, a210l, impulseLeft, ratio, impulseDelayL[6], impulseDelayL[7]);
					MixImpulse(a181l, a150l, impulseRight, ratio, impulseDelayL[6], impulseDelayL[5]);
					break;
				case 210:
					MixImpulse(a210l, a240l, impulseLeft, ratio, impulseDelayL[7], impulseDelayL[8]);
					MixImpulse(a150l, a120l, impulseRight, ratio, impulseDelayL[5], impulseDelayL[4]);
					break;
				case 240:
					MixImpulse(a240l, a270l, impulseLeft, ratio, impulseDelayL[8], impulseDelayL[9]);
					MixImpulse(a120l, a090l, impulseRight, ratio, impulseDelayL[4], impulseDelayL[3]);
					break;
				case 270:
					MixImpulse(a270l, a300l, impulseLeft, ratio, impulseDelayL[9], impulseDelayL[10]);
					MixImpulse(a090l, a060l, impulseRight, ratio, impulseDelayL[3], impulseDelayL[2]);
					break;
				case 300:
					MixImpulse(a300l, a330l, impulseLeft, ratio, impulseDelayL[10], impulseDelayL[11]);
					MixImpulse(a060l, a030l, impulseRight, ratio, impulseDelayL[2], impulseDelayL[1]);
					break;
				case 330:
					MixImpulse(a330l, a000l, impulseLeft, ratio, impulseDelayL[11], impulseDelayL[0]);
					MixImpulse(a030l, a360l, impulseRight, ratio, impulseDelayL[1], impulseDelayL[0]);
					break;
				default:
					DrawErrorMessage("\pImpulse didn't mix. Acch!");
					break;
			}
		}
	}		
/*	Zero pad impulse H to gCI.sizeFFT, transform to frequency domain and scale 
	for unity gain	*/

	for(n = gCI.sizeImpulse; n<gCI.sizeFFT; n++)
		impulseLeft[n] = 0.0;

	RealFFT(impulseLeft, gCI.halfSizeFFT,TIME2FREQ);
	for(n = 0; n < gCI.sizeFFT; n += 2)
	{
		impulseLeft[n] *= scale * gCI.sizeFFT;
		impulseLeft[n+1] *= scale * gCI.sizeFFT;
	}

	for(n = gCI.sizeImpulse; n<gCI.sizeFFT; n++)
		impulseRight[n] = 0.0;
	RealFFT(impulseRight, gCI.halfSizeFFT,TIME2FREQ);
	for(n = 0; n < gCI.sizeFFT; n += 2)
	{
		impulseRight[n] *= scale * gCI.sizeFFT;
		impulseRight[n+1] *= scale * gCI.sizeFFT;
	}
	return(TRUE);
}

void	
CopyImpulse(float source[], float target[], long delay)
{
	long m, n;
	
	for(n = 0; n < delay; n++)
		target[n] = 0.0;
	for(m = 0; n < 128; n++, m++)
		target[n] = source[m];
}

void	
MixImpulse(float sourceA[], float sourceB[], float target[], float percentB, long delayA, long delayB)
{
	long ia, ib, n;

	if(delayA < delayB)
	{
		for(n = 0; n < delayA; n++)
			target[n] = 0.0;
		for(ia = 0; n < delayB; ia++, n++)
			target[n] = sourceA[ia] * (1.0 - percentB);
		for(ib = 0; n < 256; ia++, ib++, n++)
			target[n] = sourceB[ib] * percentB + sourceA[ia] * (1.0 - percentB);
	}
	else if(delayB < delayA)
	{
		for(n = 0; n < delayB; n++)
			target[n] = 0.0;
		for(ib = 0; n < delayA; ib++, n++)
			target[n] = sourceB[ib] * percentB;
		for(ia = 0; n < 256; ia++, ib++, n++)
			target[n] = sourceB[ib] * percentB + sourceA[ia] * (1.0 - percentB);
	}
	else if(delayB == delayA)
	{
		for(n = 0; n < delayB; n++)
			target[n] = 0.0;
		for(ia = 0, ib = 0; n < 256; ia++, ib++, n++)
			target[n] = sourceB[ib] * percentB + sourceA[ia] * (1.0 - percentB);
	}
}