#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
//#include <AppleEvents.h>
#include "SoundFile.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "SoundHack.h"
#include "Menu.h"
#include "Misc.h"
#include "FFT.h"
#include "Windows.h"
#include "DrawFunction.h"
#include "Dialog.h"
#include "PhaseVocoder.h"
#include "Analysis.h"
#include "SpectFileIO.h"
#include "ShowSpect.h"
#include "ShowSono.h"
#include "CarbonGlue.h"

extern float		Pi, twoPi;
extern MenuHandle	gAppleMenu, gFileMenu, gEditMenu, gProcessMenu,
					gControlMenu, gBandsMenu, gScaleMenu, gTypeMenu, 
					gFormatMenu, gWindowMenu, gSigDispMenu, gSpectDispMenu, gSonoDispMenu;
extern SoundInfoPtr	inSIPtr, outSIPtr;

extern DialogPtr	gDrawFunctionDialog;
DialogPtr	gPvocDialog;
extern short			gProcessEnabled, gProcessDisabled, gStopProcess;
extern float		gScale, gScaleL, gScaleR, gScaleDivisor;
extern long			gNumberBlocks;
extern Boolean		gNormalize;

extern float		*gFunction;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

long	gInPointer, gOutPointer, gInPosition = 0;
float 	*analysisWindow, *synthesisWindow, *inputL, *inputR, *spectrum,
	*polarSpectrum, *displaySpectrum, *outputL, *outputR, *lastPhaseInL,
	*lastPhaseInR, *lastPhaseOutL, *lastPhaseOutR, *lastAmpL, *lastAmpR,
	*lastFreqL, *lastFreqR, *indexL, *indexR, *sineTable;
PvocInfo 	gPI;

void
HandlePvocDialog(void)
{
	WindInfo *theWindInfo;
	
	gPvocDialog = GetNewDialog(PVOC_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);

	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)PVOC_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gPvocDialog), (long)theWindInfo);
#else
	SetWRefCon(gPvocDialog, (long)theWindInfo);
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
	EnableItem(gSpectDispMenu, DISP_INPUT_ITEM);
	EnableItem(gSonoDispMenu, DISP_INPUT_ITEM);
	DisableItem(gSigDispMenu, DISP_AUX_ITEM);
	DisableItem(gSpectDispMenu, DISP_AUX_ITEM);
	DisableItem(gSonoDispMenu, DISP_AUX_ITEM);
	EnableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
	EnableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
	EnableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	MenuUpdate();
	
	/* Set up variables */
	SetPvocDefaults();
	
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gPvocDialog));
	SelectWindow(GetDialogWindow(gPvocDialog));
#else
	ShowWindow(gPvocDialog);
	SelectWindow(gPvocDialog);
#endif
}

void
HandlePvocDialogEvent(short itemHit)
{
	short		i;
	short	itemType;
	Rect	itemRect, dialogRect;
	Handle	itemHandle;
	Str255	tmpStr;
	double	tmpFloat, inLength;
	static short	choice = 4, scaleChoice = 1;
	static long	oldWindowChoice;
	static Boolean	altScale = FALSE;
	WindInfo	*myWI;
		
	inLength = inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(GetDialogPort(gPvocDialog), &dialogRect);
#else
	dialogRect = gPvocDialog->portRect;
#endif
	switch(itemHit)
	{
		case P_BANDS_MENU:
			GetDialogItem(gPvocDialog, P_BANDS_MENU, &itemType, &itemHandle, &itemRect);
			choice = GetControlValue((ControlHandle)itemHandle);
			gPI.points = 2<<(13 - choice);
			tmpFloat = inSIPtr->sRate/gPI.points;
			FixToString(tmpFloat, tmpStr);
			GetDialogItem(gPvocDialog, P_FREQ_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			GetDialogItem(gPvocDialog,P_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
			choice = GetControlValue((ControlHandle)itemHandle);
			switch(choice)
			{
				case 1:
					gPI.windowSize = (long)(gPI.points * 4.0);
					break;
				case 2:
					gPI.windowSize = (long)(gPI.points * 2.0);
					break;
				case 3:
					gPI.windowSize = gPI.points;
					break;
				case 4:
					gPI.windowSize = (long)(gPI.points * 0.5);
					break;
			}
			gPI.scaleFactor = FindBestRatio();
			if(gPI.time)
			{
				if(altScale)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
				}
				else
					FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			NumToString(gPI.decimation, tmpStr);
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			NumToString(gPI.interpolation, tmpStr);
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			break;
		case P_WINDOW_MENU:
			GetDialogItem(gPvocDialog,P_WINDOW_MENU, &itemType, &itemHandle, &itemRect);
			gPI.windowType = GetControlValue((ControlHandle)itemHandle);
			break;
		case P_OVERLAP_MENU:
			GetDialogItem(gPvocDialog,P_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
			choice = GetControlValue((ControlHandle)itemHandle);
			switch(choice)
			{
				case 1:
					gPI.windowSize = (long)(gPI.points * 4.0);
					break;
				case 2:
					gPI.windowSize = (long)(gPI.points * 2.0);
					break;
				case 3:
					gPI.windowSize = gPI.points;
					break;
				case 4:
					gPI.windowSize = (long)(gPI.points * 0.5);
					break;
			}
			gPI.scaleFactor = FindBestRatio();
			if(gPI.time)
			{
				if(altScale)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
				}
				else
					FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			NumToString(gPI.decimation, tmpStr);
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			NumToString(gPI.interpolation, tmpStr);
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			break;
		case P_ANALYSIS_RATE_FIELD:
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, tmpStr);
			StringToNum(tmpStr, &gPI.decimation);
			if(gPI.decimation > (gPI.windowSize >> 3))
				gPI.decimation = gPI.windowSize >> 3;
			if(gPI.time)
				gPI.scaleFactor = ((float)gPI.interpolation/gPI.decimation);
			else
				gPI.interpolation = gPI.decimation;
			if(gPI.time)
			{
				if(altScale)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
				}
				else
					FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			NumToString(gPI.decimation, tmpStr);
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			NumToString(gPI.interpolation, tmpStr);
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			break;
		case P_SYNTHESIS_RATE_FIELD:
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, tmpStr);
			StringToNum(tmpStr, &gPI.interpolation);
			if(gPI.interpolation > (gPI.windowSize >> 3))
				gPI.interpolation = gPI.windowSize >> 3;
			if(gPI.time)
				gPI.scaleFactor = ((float)gPI.interpolation/gPI.decimation);
			else
				gPI.decimation = gPI.interpolation;
			if(gPI.time)
			{
				if(altScale)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
				}
				else
					FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			NumToString(gPI.decimation, tmpStr);
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			NumToString(gPI.interpolation, tmpStr);
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			break;
		case P_SCALE_FIELD:
			GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, tmpStr);
			if(altScale)
			{
				StringToFix(tmpStr, &tmpFloat);
				if(gPI.time)
					gPI.scaleFactor = tmpFloat/inLength;
				else
					gPI.scaleFactor = EExp2(tmpFloat/12.0);					
			}
			else
				StringToFix(tmpStr,&gPI.scaleFactor);
			if(gPI.time)
			{
				FindBestRatio();
				if(altScale)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
				}
				else
					FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
				NumToString(gPI.decimation, tmpStr);
				GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
				NumToString(gPI.interpolation, tmpStr);
				GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			break;
		case P_SCALE_MENU:
			GetDialogItem(gPvocDialog, P_SCALE_MENU, &itemType, &itemHandle, &itemRect);
			scaleChoice = GetControlValue((ControlHandle)itemHandle);
			altScale = scaleChoice - 1;
			if(!altScale)
			{
				FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			else
			{
				if(gPI.time)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
					GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, tmpStr);
				}
				else
				{
					tmpFloat = 12.0 * LLog2(gPI.scaleFactor);
					FixToString(tmpFloat, tmpStr);
					GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, tmpStr);
				}
			}
			gPI.scaleFactor = FindBestRatio();
			if(gPI.time)
			{
				if(altScale)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
				}
				else
					FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			NumToString(gPI.decimation, tmpStr);
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			NumToString(gPI.interpolation, tmpStr);
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			break;
		case P_TIME_RADIO:
			GetDialogItem(gPvocDialog, P_TIME_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gPvocDialog, P_PITCH_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			for(i = 0; i < 400; i++)
				gFunction[i] = 1.0;
			if(altScale)
			{
				tmpFloat = gPI.scaleFactor * inLength;
				HMSTimeString(tmpFloat, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			gPI.time = TRUE;
			GetDialogItem(gPvocDialog, P_SCALE_MENU, &itemType, &itemHandle, &itemRect);
			SetMenuItemText(gScaleMenu, 2, "\pLength Desired:");
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			HiliteControl((ControlHandle)itemHandle,0);		
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			HiliteControl((ControlHandle)itemHandle,0);		
			gPI.scaleFactor = FindBestRatio();
			if(gPI.time)
			{
				if(altScale)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
				}
				else
					FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			NumToString(gPI.decimation, tmpStr);
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			NumToString(gPI.interpolation, tmpStr);
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			DrawDialog(gPvocDialog);
			break;
		case P_PITCH_RADIO:
			GetDialogItem(gPvocDialog, P_TIME_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gPvocDialog, P_PITCH_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			for(i = 0; i < 400; i++)
				gFunction[i] = 0.0;
			if(altScale)
			{
				tmpFloat = 12.0 * LLog2(gPI.scaleFactor);
				FixToString(tmpFloat, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			gPI.time = FALSE;
			GetDialogItem(gPvocDialog, P_SCALE_MENU, &itemType, &itemHandle, &itemRect);
			SetMenuItemText(gScaleMenu, 2, "\pSemitone Shift:");
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			HiliteControl((ControlHandle)itemHandle,255);		
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			HiliteControl((ControlHandle)itemHandle,255);		
			gPI.scaleFactor = FindBestRatio();
			if(gPI.time)
			{
				if(altScale)
				{
					tmpFloat = gPI.scaleFactor * inLength;
					FixToString(tmpFloat, tmpStr);
				}
				else
					FixToString(gPI.scaleFactor, tmpStr);
				GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			NumToString(gPI.decimation, tmpStr);
			GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			NumToString(gPI.interpolation, tmpStr);
			GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			DrawDialog(gPvocDialog);
			break;
		case P_DRAW_BOX:
			GetDialogItem(gPvocDialog, P_DRAW_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
			{
				SetControlValue((ControlHandle)itemHandle,OFF);
				ShowDialogItem(gPvocDialog, P_SCALE_FIELD);
				HideDialogItem(gPvocDialog, P_DRAW_BUTTON);
				gPI.useFunction = FALSE;
			} 
			else
			{
				SetControlValue((ControlHandle)itemHandle,ON);
				HideDialogItem(gPvocDialog, P_SCALE_FIELD);
				ShowDialogItem(gPvocDialog, P_DRAW_BUTTON);
				gPI.useFunction = TRUE;
			}
			break;
		case P_DRAW_BUTTON:
			if(gPI.time == FALSE)
				HandleDrawFunctionDialog(60.0, -60.0, 1.0, "\pSemitones:", inLength, FALSE);
			else
				HandleDrawFunctionDialog((float)(gPI.points/8), (float)(8.0/gPI.points), 0.125, "\pTime Scale:", inLength, FALSE);
#if TARGET_API_MAC_CARBON == 1
			SetPort(GetDialogPort(gPvocDialog));
#else
			SetPort((GrafPtr)gPvocDialog);
#endif
			ClipRect(&dialogRect);
			break;
		case P_GATE_BOX:
			GetDialogItem(gPvocDialog, P_GATE_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
			{
				SetControlValue((ControlHandle)itemHandle,OFF);
				HideDialogItem(gPvocDialog, P_MINAMP_TEXT);
				HideDialogItem(gPvocDialog, P_RATIO_TEXT);
				HideDialogItem(gPvocDialog, P_RATIO_FIELD);
				HideDialogItem(gPvocDialog, P_MINAMP_FIELD);
				gPI.minAmplitude = 0.0;					
				gPI.maskRatio = 0.0;
			} 
			else
			{
				SetControlValue((ControlHandle)itemHandle,ON);
				ShowDialogItem(gPvocDialog, P_MINAMP_TEXT);
				ShowDialogItem(gPvocDialog, P_RATIO_TEXT);
				ShowDialogItem(gPvocDialog, P_RATIO_FIELD);
				ShowDialogItem(gPvocDialog, P_MINAMP_FIELD);
				gPI.minAmplitude = 0.00001;					
				gPI.maskRatio = 0.001;
				NumToString((long)(20.0 * log10(gPI.minAmplitude)), tmpStr);
				GetDialogItem(gPvocDialog, P_MINAMP_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
				NumToString((long)(20.0 * log10(gPI.maskRatio)), tmpStr);
				GetDialogItem(gPvocDialog, P_RATIO_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, tmpStr);
			}
			break;
		case P_MINAMP_FIELD:
			GetDialogItem(gPvocDialog, P_MINAMP_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, tmpStr);
			StringToFix(tmpStr, &tmpFloat);
			gPI.minAmplitude = powf(10.0, tmpFloat/20.0);					
			break;
		case P_RATIO_FIELD:
			GetDialogItem(gPvocDialog, P_RATIO_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, tmpStr);
			StringToFix(tmpStr, &tmpFloat);
			gPI.maskRatio = powf(10.0, tmpFloat/20.0);
			break;
		case P_CANCEL_BUTTON:
			inSIPtr = nil;
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
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gPvocDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gPvocDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gPvocDialog);
			gPvocDialog = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			break;
		case P_PROCESS_BUTTON:
			if(gPI.time)
			{
				gPI.scaleFactor = ((float)gPI.interpolation/gPI.decimation);
				if(gPI.scaleFactor <= 0.0)
				{
					DrawErrorMessage("\pNegative or zero scale factors not allowed");
					gPI.scaleFactor = 1.0;
					altScale = FALSE;
					GetDialogItem(gPvocDialog, P_SCALE_MENU, &itemType, &itemHandle, &itemRect);
					SetControlValue((ControlHandle)itemHandle, 1);
					FixToString(gPI.scaleFactor, tmpStr);
					GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
					SetDialogItemText(itemHandle, tmpStr);
					break;
				}
			}
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
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gPvocDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gPvocDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gPvocDialog);
			gPvocDialog = nil;
			InitPvocProcess();
			break;
		}
}

void
SetPvocDefaults(void)
{
	short	itemType, i;
	Rect	itemRect;
	Handle	itemHandle;
	float	frequency, tmpFloat;
	double	tmpDouble;
	long	tmpLong;
	Str255	tmpStr;

	gNormalize = FALSE;
	gScaleDivisor = 1.0;
	
	gPI.phaseLocking = false;
	
	for(i = 0; i < 400; i++)
		gFunction[i] = 1.0;

#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gPvocDialog));
#else
	SetPort((GrafPtr)gPvocDialog);
#endif
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
	{
		gPI.points = inSIPtr->spectFrameSize;
		gPI.decimation = gPI.interpolation = inSIPtr->spectFrameIncr;
		if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPFRQ)
			gPI.windowSize = gPI.decimation * 4;
		else
			gPI.windowSize = gPI.decimation * 8;
		GetDialogItem(gPvocDialog, P_BANDS_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);
		GetDialogItem(gPvocDialog, P_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);		
	}
	else
	{
		gPI.decimation = gPI.interpolation = gPI.windowSize >> 2;
		GetDialogItem(gPvocDialog, P_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);		
		GetDialogItem(gPvocDialog, P_BANDS_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);
	}
	
	if(gPI.useFunction == FALSE)
	{
		ShowDialogItem(gPvocDialog, P_SCALE_FIELD);
		HideDialogItem(gPvocDialog, P_DRAW_BUTTON);
		GetDialogItem(gPvocDialog, P_DRAW_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
	}
	else
	{
		HideDialogItem(gPvocDialog, P_SCALE_FIELD);
		ShowDialogItem(gPvocDialog, P_DRAW_BUTTON);
		GetDialogItem(gPvocDialog, P_DRAW_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
	}

	GetDialogItem(gPvocDialog, P_SCALE_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, 1);
	if(gPI.time == TRUE)
	{
		GetDialogItem(gPvocDialog, P_SCALE_MENU, &itemType, &itemHandle, &itemRect);
		SetMenuItemText(gScaleMenu, 2, "\pLength Desired:");
		GetDialogItem(gPvocDialog, P_TIME_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		GetDialogItem(gPvocDialog, P_PITCH_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);		
		GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);		
	}
	else
	{
		GetDialogItem(gPvocDialog, P_SCALE_MENU, &itemType, &itemHandle, &itemRect);
		SetMenuItemText(gScaleMenu, 2, "\pSemitone Shift:");
		GetDialogItem(gPvocDialog, P_TIME_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		GetDialogItem(gPvocDialog, P_PITCH_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);		
		GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);		
	}
	gPI.scaleFactor = FindBestRatio();
	FixToString(gPI.scaleFactor, tmpStr);
	GetDialogItem(gPvocDialog, P_SCALE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	NumToString(gPI.decimation, tmpStr);
	GetDialogItem(gPvocDialog, P_ANALYSIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	NumToString(gPI.interpolation, tmpStr);
	GetDialogItem(gPvocDialog, P_SYNTHESIS_RATE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	GetDialogItem(gPvocDialog, P_WINDOW_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gPI.windowType);
	
	tmpDouble =	LLog2((double)gPI.points);
	tmpFloat = tmpDouble;
	tmpLong = (long)tmpFloat;	// 10 for 1024
	GetDialogItem(gPvocDialog, P_BANDS_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, (14 - tmpLong));
	frequency = inSIPtr->sRate/gPI.points;
	FixToString(frequency, tmpStr);
	GetDialogItem(gPvocDialog, P_FREQ_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	
	tmpLong = (gPI.windowSize * 2)/gPI.points;
	GetDialogItem(gPvocDialog,P_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
	switch(tmpLong)
	{
		case 1:
			SetControlValue((ControlHandle)itemHandle, 4);
			break;
		case 2:
			SetControlValue((ControlHandle)itemHandle, 3);
			break;
		case 4:
			SetControlValue((ControlHandle)itemHandle, 2);
			break;
		case 8:
			SetControlValue((ControlHandle)itemHandle, 1);
			break;
	}

	if(gPI.minAmplitude == 0.0)
	{
		GetDialogItem(gPvocDialog, P_GATE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,OFF);
		HideDialogItem(gPvocDialog, P_MINAMP_TEXT);
		HideDialogItem(gPvocDialog, P_RATIO_TEXT);
		HideDialogItem(gPvocDialog, P_RATIO_FIELD);
		HideDialogItem(gPvocDialog, P_MINAMP_FIELD);
	}
	else
	{
		GetDialogItem(gPvocDialog, P_GATE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle,ON);
		ShowDialogItem(gPvocDialog, P_MINAMP_TEXT);
		ShowDialogItem(gPvocDialog, P_RATIO_TEXT);
		ShowDialogItem(gPvocDialog, P_RATIO_FIELD);
		ShowDialogItem(gPvocDialog, P_MINAMP_FIELD);
	}
	NumToString((long)(20.0 * log10(gPI.minAmplitude)), tmpStr);
	GetDialogItem(gPvocDialog, P_MINAMP_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);
	NumToString((long)(20.0 * log10(gPI.maskRatio)), tmpStr);
	GetDialogItem(gPvocDialog, P_RATIO_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, tmpStr);

}

short
InitPvocProcess(void)
{
    short		n;
	long	blockSize;
	float	realScale;
	Str255	tmpStr1, tmpStr2, tmpStr;
	
	/* windowSize/8 seems to be the largest value possible for time stretching
		without distortion */
		
	gPI.halfPoints = gPI.points>>1;
	
    if(gPI.time)
		gPI.scaleFactor = FindBestRatio();
 	
 	if(gPI.useFunction)
 	{
		gPI.scaleFactor = gFunction[0];
		if(gPI.time)
			gPI.scaleFactor = FindBestRatio();
		else
		{
			realScale = gPI.scaleFactor;
			gPI.scaleFactor = EExp2(realScale/12.0);
		}
	}
		
	if(gPI.time)
		realScale = (float)gPI.interpolation/gPI.decimation;
	else
		realScale = gPI.scaleFactor;

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
	if(gPI.time && gPI.useFunction)
		NameFile(inSIPtr->sfSpec.name, "\pTVAR", outSIPtr->sfSpec.name);
	else if(!gPI.time && gPI.useFunction)
		NameFile(inSIPtr->sfSpec.name, "\pPVAR", outSIPtr->sfSpec.name);
	else if(gPI.time && !gPI.useFunction)
	{
		FixToString3(gPI.scaleFactor, tmpStr2);
		StringAppend("\pT", tmpStr2, tmpStr1);
		NameFile(inSIPtr->sfSpec.name, tmpStr1, outSIPtr->sfSpec.name);
	}
	else if(!gPI.time && !gPI.useFunction)
	{
		FixToString3(gPI.scaleFactor, tmpStr2);
		StringAppend("\pP", tmpStr2, tmpStr1);
		NameFile(inSIPtr->sfSpec.name, tmpStr1, outSIPtr->sfSpec.name);
	}
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
    if(gPI.interpolation <= gPI.decimation)
    	blockSize = gPI.decimation;
    else
    	blockSize = gPI.interpolation;

/*
 * allocate memory
 */
	analysisWindow = synthesisWindow = inputL = nil;
	spectrum = polarSpectrum = displaySpectrum = outputL = lastPhaseInL = nil;
	lastPhaseOutL = lastAmpL = lastFreqL = indexL = sineTable = nil;
 	inputR = outputR = lastPhaseInR = lastPhaseOutR = lastAmpR = nil;
 	lastFreqR = indexR = nil;

	analysisWindow = (float *)NewPtr(gPI.windowSize*sizeof(float));
	synthesisWindow = (float *)NewPtr(gPI.windowSize*sizeof(float));
	inputL = (float *)NewPtr(gPI.windowSize*sizeof(float));
	spectrum = (float *)NewPtr(gPI.points*sizeof(float));
	polarSpectrum = (float *)NewPtr((gPI.points+2)*sizeof(float));		/* analysis polarSpectrum */
	displaySpectrum = (float *)NewPtr((gPI.halfPoints+1)*sizeof(float));	/* For display of amplitude */
	outputL = (float *)NewPtr(gPI.windowSize*sizeof(float));
	lastPhaseInL = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
	lastPhaseOutL = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
	if(gPI.time == 0)
	{
		lastAmpL = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
		lastFreqL = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
		indexL = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
		sineTable = (float *)NewPtr(8192 * sizeof(float));
		if(sineTable == 0)
		{
			DrawErrorMessage("\pNot enough memory for processing");
			DeAllocPvocMem();
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			outSIPtr = nil;
			return(-1);
		}
	}
	
	if(inSIPtr->nChans == STEREO)
	{
		inputR = (float *)NewPtr(gPI.windowSize*sizeof(float));	/* inputR buffer */
		outputR = (float *)NewPtr(gPI.windowSize*sizeof(float));	/* outputR buffer */
		lastPhaseInR = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
		lastPhaseOutR = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
		if(lastPhaseOutR == 0)
		{
			DrawErrorMessage("\pNot enough memory for processing");
			DeAllocPvocMem();
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			outSIPtr = nil;
			return(-1);
		}
		if(gPI.time == 0)
		{
			lastAmpR = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
			lastFreqR = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
			indexR = (float *)NewPtr((gPI.halfPoints+1) * sizeof(float));
			if(indexR == 0)
			{
				DrawErrorMessage("\pNot enough memory for processing");
				DeAllocPvocMem();
				gProcessDisabled = gProcessEnabled = NO_PROCESS;
				MenuUpdate();
				outSIPtr = nil;
				return(-1);
			}
		}
	}
	
	if(outputL == 0 || lastPhaseOutL == 0)
	{
		DrawErrorMessage("\pNot enough memory for processing");
		DeAllocPvocMem();
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		outSIPtr = nil;
		return(-1);
	}
	ZeroFloatTable(lastPhaseInL, gPI.halfPoints+1);
	ZeroFloatTable(lastPhaseOutL, gPI.halfPoints+1);
	ZeroFloatTable(spectrum, gPI.points);
	ZeroFloatTable(polarSpectrum, (gPI.points+2));
	ZeroFloatTable(inputL, gPI.windowSize);
	ZeroFloatTable(outputL, gPI.windowSize);
	if(gPI.time == 0)
	{
		for ( n = 0; n < 8192L; n++ )
	    	sineTable[n] = 0.5 * cos(n*twoPi/8192L);
		ZeroFloatTable(lastAmpL, gPI.halfPoints+1);
		ZeroFloatTable(lastFreqL, gPI.halfPoints+1);
		ZeroFloatTable(indexL, gPI.halfPoints+1);
	}
	if(inSIPtr->nChans == STEREO)
	{
		ZeroFloatTable(lastPhaseInR, gPI.halfPoints+1);
		ZeroFloatTable(lastPhaseOutR, gPI.halfPoints+1);
		ZeroFloatTable(inputR, gPI.windowSize);
		ZeroFloatTable(outputR, gPI.windowSize);
		if(gPI.time == 0)
		{
			ZeroFloatTable(lastAmpR, gPI.halfPoints+1);
			ZeroFloatTable(lastFreqR, gPI.halfPoints+1);
			ZeroFloatTable(indexR, gPI.halfPoints+1);
		}
	}
	FixToString(gPI.scaleFactor, tmpStr);	
	if(gPI.time)
	{
		StringAppend("\pchanging length by ", tmpStr, tmpStr1);
		UpdateProcessWindow("\p", "\p", tmpStr1, 0.0);
	}
	else
	{
		StringAppend("\pchanging pitch by ", tmpStr, tmpStr1);
		UpdateProcessWindow("\p", "\p", tmpStr1, 0.0);
	}
/*
 * create and scale windows
 */
 	GetWindow(analysisWindow, gPI.windowSize, gPI.windowType);
 	GetWindow(synthesisWindow, gPI.windowSize, gPI.windowType);
    ScaleWindows(analysisWindow, synthesisWindow, gPI);
/*
 * initialize input and output time values (in samples)
 */
    gInPointer = -gPI.windowSize;
	gOutPointer = (gInPointer*gPI.interpolation)/gPI.decimation;
	gInPosition	= 0;
	
	gNumberBlocks = 0;	
	gProcessEnabled = PVOC_PROCESS;
	gProcessDisabled = NO_PROCESS;
	MenuUpdate();
	return(TRUE);
}

/*
 * main loop--perform phase vocoder analysis-resynthesis
 */
short
PvocBlock(void)
{
    short	curMark;
	long	numSampsRead, pointer;
	float 	length, realScale, displayScale;
	Str255	errStr, tmpStr;
	static long validSamples = -1L;
	static long	sOutPos;
	double seconds;
	

	FixToString(gPI.scaleFactor, tmpStr);	
	if(gPI.time)
	{
		StringAppend("\pchanging length by ", tmpStr, errStr);
		UpdateProcessWindow("\p", "\p", errStr, -1.0);
	}
	else
	{
		StringAppend("\pchanging pitch by ", tmpStr, errStr);
		UpdateProcessWindow("\p", "\p", errStr, -1.0);
	}
	if(gStopProcess == TRUE)
	{
		DeAllocPvocMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	if(gNumberBlocks == 0)
		sOutPos = outSIPtr->dataStart;
	SetFPos(outSIPtr->dFileNum, fsFromStart, sOutPos);
	displayScale = 1.0; 
/*
 * increment times
 */
	gInPointer += gPI.decimation;
	gOutPointer += gPI.interpolation;
	
/*
 * analysis: input gPI.decimation samples; window, fold and rotate input
 * samples into FFT buffer; take FFT; and convert to
 * amplitude-frequency (phase vocoder) form
 */
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
	{
		if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPFRQ)
			numSampsRead = ReadCSAData(inSIPtr, polarSpectrum, lastPhaseInL, gPI.halfPoints);
		else if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPPHS)
			numSampsRead = ReadSHAData(inSIPtr, polarSpectrum, LEFT);
		else if(inSIPtr->packMode == SF_FORMAT_SPECT_COMPLEX)
		{
			numSampsRead = ReadSDIFData(inSIPtr, spectrum, polarSpectrum, &seconds, LEFT);
			CartToPolar(spectrum, polarSpectrum, gPI.halfPoints);
		}
	}
	else
	{
		SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart + gInPosition));
		numSampsRead = ShiftIn(inSIPtr, inputL, inputR, gPI.windowSize, gPI.decimation, &validSamples);
		WindowFold(inputL, analysisWindow, spectrum, gInPointer, gPI.points, gPI.windowSize);
		RealFFT(spectrum, gPI.halfPoints, TIME2FREQ);
		CartToPolar(spectrum, polarSpectrum, gPI.halfPoints);
	}
	if(numSampsRead > 0)
	{
		length = (numSampsRead + (gInPosition/(inSIPtr->nChans*inSIPtr->frameSize)))/inSIPtr->sRate;
		HMSTimeString(length, errStr);
		length = length / (inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize));
		UpdateProcessWindow(errStr, "\p", "\p", length);
	}
	GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(polarSpectrum, displaySpectrum, gPI.halfPoints, inSIPtr, LEFT, displayScale, "\pInput Channel 1");
	GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(polarSpectrum, displaySpectrum, gPI.halfPoints, inSIPtr, LEFT, displayScale, "\pInput Channel 1");
	/* output */
	if (gPI.time)
	{
		/* overlap-add resynthesis */
		SimpleSpectralGate(polarSpectrum);
		GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(polarSpectrum, displaySpectrum, gPI.halfPoints, outSIPtr, LEFT, 1.0, "\pOutput Channel 1");
		GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(polarSpectrum, displaySpectrum, gPI.halfPoints, outSIPtr, LEFT, 1.0, "\pOutput Channel 1");
		PhaseInterpolate(polarSpectrum, lastPhaseInL, lastPhaseOutL);
	    PolarToCart(polarSpectrum, spectrum, gPI.halfPoints);
	    RealFFT(spectrum, gPI.halfPoints, FREQ2TIME);
	    OverlapAdd(spectrum, synthesisWindow, outputL, gOutPointer, gPI.points, gPI.windowSize);
    }
    else
    {
		/* additive resynthesis */
		SimpleSpectralGate(polarSpectrum);
		GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(polarSpectrum, displaySpectrum, gPI.halfPoints, outSIPtr, LEFT, 1.0, "\pOutput Channel 1");
		GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(polarSpectrum, displaySpectrum, gPI.halfPoints, outSIPtr, LEFT, 1.0, "\pOutput Channel 1");
		AddSynth(polarSpectrum, outputL, lastAmpL, lastFreqL, lastPhaseInL, indexL, sineTable, gPI);
	}
    if(inSIPtr->nChans == STEREO)
    {
		if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
		{
			if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPFRQ)
				numSampsRead = ReadCSAData(inSIPtr, polarSpectrum, lastPhaseInR, gPI.halfPoints);
			else if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPPHS)
				numSampsRead = ReadSHAData(inSIPtr, polarSpectrum, RIGHT);
			else if(inSIPtr->packMode == SF_FORMAT_SPECT_COMPLEX)
			{
				numSampsRead = ReadSDIFData(inSIPtr, spectrum, polarSpectrum, &seconds, RIGHT);
				CartToPolar(spectrum, polarSpectrum, gPI.halfPoints);
			}
		}
		else
		{
			WindowFold(inputR, analysisWindow, spectrum, gInPointer, gPI.points, gPI.windowSize);
			RealFFT(spectrum, gPI.halfPoints, TIME2FREQ);
			CartToPolar(spectrum, polarSpectrum, gPI.halfPoints);
		}
		GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(polarSpectrum, displaySpectrum, gPI.halfPoints, inSIPtr, RIGHT, displayScale, "\pInput Channel 2");
		GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(polarSpectrum, displaySpectrum, gPI.halfPoints, inSIPtr, RIGHT, displayScale, "\pInput Channel 2");
		/* output */
		if(gPI.time)
		{
			SimpleSpectralGate(polarSpectrum);
			GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
			if(curMark != noMark)
				DisplaySpectrum(polarSpectrum, displaySpectrum, gPI.halfPoints, outSIPtr, RIGHT, 1.0, "\pOutput Channel 2");
			GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
			if(curMark != noMark)
				DisplaySonogram(polarSpectrum, displaySpectrum, gPI.halfPoints, outSIPtr, RIGHT, 1.0, "\pOutput Channel 2");
			PhaseInterpolate(polarSpectrum, lastPhaseInR, lastPhaseOutR);
		    PolarToCart(polarSpectrum, spectrum, gPI.halfPoints);
		    RealFFT(spectrum, gPI.halfPoints, FREQ2TIME);
		    OverlapAdd(spectrum, synthesisWindow, outputR, gOutPointer, gPI.points, gPI.windowSize);
	    }else
	    {
	    	SimpleSpectralGate(polarSpectrum);
			GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
			if(curMark != noMark)
				DisplaySpectrum(polarSpectrum, displaySpectrum, gPI.halfPoints, outSIPtr, RIGHT, 1.0, "\pOutput Channel 2");
			GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
			if(curMark != noMark)
				DisplaySonogram(polarSpectrum, displaySpectrum, gPI.halfPoints, outSIPtr, RIGHT, 1.0, "\pOutput Channel 2");
			AddSynth(polarSpectrum, outputR, lastAmpR, lastFreqR, lastPhaseInR, indexR, sineTable, gPI);
		}
	}
/* Data Output and Header Update */
	if(gPI.time)
	{
		ShiftOut(outSIPtr, outputL, outputR, gOutPointer+gPI.interpolation, gPI.interpolation, gPI.windowSize);
		length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
		HMSTimeString(length, errStr);
		UpdateProcessWindow("\p", errStr, "\p", -1.0);
	}else
	{
		ShiftOut(outSIPtr, outputL, outputR, gOutPointer+gPI.windowSize-gPI.interpolation, gPI.interpolation, gPI.windowSize);
		length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
		HMSTimeString(length, errStr);
		UpdateProcessWindow("\p", errStr, "\p", -1.0);
	}
	if(numSampsRead == -2L)
	{
		DeAllocPvocMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	gNumberBlocks++;
	GetFPos(outSIPtr->dFileNum, &sOutPos);

 	gInPosition += inSIPtr->nChans * gPI.decimation * inSIPtr->frameSize;
 	if(gPI.useFunction)
 	{
 		pointer = inSIPtr->nChans * gInPointer * inSIPtr->frameSize;
 		if(pointer < 0)
 			pointer = 0;
		gPI.scaleFactor = InterpFunctionValue(pointer, FALSE);
		if(gPI.time)
		{
			gPI.scaleFactor = FindBestRatio();
			realScale = (float)gPI.interpolation/gPI.decimation;
			FixToString(realScale, errStr);	
		}
		else
		{
			realScale = gPI.scaleFactor;
			gPI.scaleFactor = EExp2(realScale/12.0);
			FixToString(gPI.scaleFactor, errStr);	
		}	

		if(gPI.time)
		{
			StringAppend("\pchanging length by ", errStr, tmpStr);
			UpdateProcessWindow("\p", "\p", tmpStr, -1.0);
		}
		else
		{
			StringAppend("\pchanging pitch by ", errStr, tmpStr);
			UpdateProcessWindow("\p", "\p", tmpStr, -1.0);
		}
	}
	return(TRUE);
}


float FindBestRatio(void)
{
	float	testScale, percentError = 2.0, newScaleFactor;
	long	maxInterpolate, maxDecimate;
	
	
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT)
	{
		if(gPI.time)
			gPI.interpolation = (long)(gPI.decimation * gPI.scaleFactor);
		else
		{
			gPI.interpolation = gPI.decimation;
			gPI.scaleFactor = 1.0;
		}
		return(gPI.scaleFactor);
	}
	
	maxInterpolate = maxDecimate = gPI.windowSize/8;
	if(!gPI.time)
	{
		gPI.decimation = gPI.interpolation = maxInterpolate;
//		gPI.scaleFactor = 1.0;
		return(gPI.scaleFactor);
	}
	if(gPI.scaleFactor>1.0)
		for(gPI.interpolation = maxInterpolate; percentError > 1.01; gPI.interpolation--)
		{
			gPI.decimation = (long)((float)gPI.interpolation/gPI.scaleFactor);
			testScale = ((float)gPI.interpolation/gPI.decimation);
			if(testScale > gPI.scaleFactor)
				percentError = testScale/gPI.scaleFactor;
			else
				percentError = gPI.scaleFactor/testScale;
			if(percentError<1.004)
			{
				break;
			}
			if(gPI.interpolation == 1)
			{
				gPI.interpolation = maxInterpolate;
				gPI.decimation = (long)((float)gPI.interpolation/gPI.scaleFactor);
				newScaleFactor = ((float)gPI.interpolation/gPI.decimation);
				percentError = 1.0;
			}
		}
	else
		for(gPI.decimation = maxDecimate; percentError > 1.01; gPI.decimation--)
		{
			gPI.interpolation = (long)(gPI.decimation * gPI.scaleFactor);
			testScale = ((float)gPI.interpolation/gPI.decimation);
			if(testScale > gPI.scaleFactor)
				percentError = testScale/gPI.scaleFactor;
			else
				percentError = gPI.scaleFactor/testScale;
			if(percentError<1.004)
			{
				break;
			}
			if(gPI.decimation == 1)
			{
				gPI.decimation = maxDecimate;
				gPI.interpolation = (long)(gPI.decimation * gPI.scaleFactor);
				newScaleFactor = ((float)gPI.interpolation/gPI.decimation);
				percentError = 1.0;
			}
		}
	newScaleFactor = ((float)gPI.interpolation/gPI.decimation);
	return(newScaleFactor);

}

void
PhaseInterpolate(float polarSpectrum[], float lastPhaseIn[], float lastPhaseOut[])
{
	long bandNumber, phaseIndex, ampIndex, phaseLockBand, phaseLockPhase, phaseLockAmp;
	float phaseDifference;
	float maxAmplitude;
	float amplitude, phase;
	
	float phasePerBand;
	phasePerBand = ((float)gPI.decimation * twoPi)/(float)gPI.points;
    for (bandNumber = 0; bandNumber <= gPI.halfPoints; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
		phaseIndex = ampIndex + 1;
/*
 * take difference between the current phase value and previous value for each channel
 */
		if (polarSpectrum[ampIndex] == 0.0)
		{
	    	phaseDifference = 0.0;
			polarSpectrum[phaseIndex] = lastPhaseOut[bandNumber];
		}
		else
		{
			if(gPI.phaseLocking)
			{
				maxAmplitude = 0.0;
				// set low band info
				if(bandNumber > 1)
				{
					maxAmplitude = polarSpectrum[ampIndex - 2];
					phaseDifference = (polarSpectrum[phaseIndex - 2] - lastPhaseIn[bandNumber - 1]) - phasePerBand;
				}
				if(polarSpectrum[ampIndex] > maxAmplitude)
				{
					maxAmplitude = polarSpectrum[ampIndex];
					phaseDifference = polarSpectrum[phaseIndex] - lastPhaseIn[bandNumber];
				}
				if(bandNumber != gPI.halfPoints)
				{
					if(polarSpectrum[ampIndex + 2] > maxAmplitude)
					{
						phaseDifference = (polarSpectrum[phaseIndex + 2] - lastPhaseIn[bandNumber + 1]) + phasePerBand;
					}
				}
			}
			else
			{
	    		phaseDifference = polarSpectrum[phaseIndex] - lastPhaseIn[bandNumber];
	    	}
	    	
	    	lastPhaseIn[bandNumber] = polarSpectrum[phaseIndex];
	    	
/*
 * unwrap phase differences
 */

//	    	while (phaseDifference > Pi)
//				phaseDifference -= twoPi;
//	    	while (phaseDifference < -Pi)
//				phaseDifference += twoPi;
			phaseDifference *= gPI.scaleFactor;
/*
 * create new phase from interpolate/decimate ratio
 */
			polarSpectrum[phaseIndex] = lastPhaseOut[bandNumber] + phaseDifference;
	    	while (polarSpectrum[phaseIndex] > Pi)
				polarSpectrum[phaseIndex] -= twoPi;
	    	while (polarSpectrum[phaseIndex] < -Pi)
				polarSpectrum[phaseIndex] += twoPi;
			phase = lastPhaseOut[bandNumber] = polarSpectrum[phaseIndex];
			amplitude = polarSpectrum[ampIndex];
		}
    }
}

void
SimpleSpectralGate(float polarSpectrum[])
{
	long bandNumber, ampIndex;
	float maxAmplitude, maskAmplitude;
	
	maxAmplitude = 0.0;
		
/* Find maximum amplitude */
	for(bandNumber = 0; bandNumber <= gPI.halfPoints; bandNumber++)
	{
		ampIndex = bandNumber<<1;
		if(polarSpectrum[ampIndex] > maxAmplitude)
			maxAmplitude = polarSpectrum[ampIndex];
	}
	maskAmplitude = gPI.maskRatio * maxAmplitude;
    for(bandNumber = 0; bandNumber <= gPI.halfPoints; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
    	/* Set for Ducking */
    	if(polarSpectrum[ampIndex] < maskAmplitude)
    		polarSpectrum[ampIndex] = 0.0;
    	else if(polarSpectrum[ampIndex] < gPI.minAmplitude)
    		polarSpectrum[ampIndex] = 0.0;
    }
}

void
DeAllocPvocMem(void)
{
	RemovePtr((Ptr)analysisWindow);
	RemovePtr((Ptr)synthesisWindow);
	RemovePtr((Ptr)inputL);
	RemovePtr((Ptr)spectrum);
	RemovePtr((Ptr)polarSpectrum);
	RemovePtr((Ptr)displaySpectrum);
	RemovePtr((Ptr)outputL);
	RemovePtr((Ptr)lastPhaseInL);
	RemovePtr((Ptr)lastPhaseOutL);
	if(gPI.time == 0)
	{
		RemovePtr((Ptr)lastAmpL);
		RemovePtr((Ptr)lastFreqL);
		RemovePtr((Ptr)indexL);
		RemovePtr((Ptr)sineTable);
	}
	
	if(inSIPtr->nChans == STEREO)
	{
		RemovePtr((Ptr)inputR);
		RemovePtr((Ptr)outputR);
		RemovePtr((Ptr)lastPhaseInR);
		RemovePtr((Ptr)lastPhaseOutR);
		if(gPI.time == 0)
		{
			RemovePtr((Ptr)lastAmpR);
			RemovePtr((Ptr)lastFreqR);
			RemovePtr((Ptr)indexR);
		}
	}
	analysisWindow = synthesisWindow = inputL = nil;
	spectrum = polarSpectrum = displaySpectrum = outputL = lastPhaseInL = nil;
	lastPhaseOutL = lastAmpL = lastFreqL = indexL = sineTable = nil;
 	inputR = outputR = lastPhaseInR = lastPhaseOutR = lastAmpR = nil;
 	lastFreqR = indexR = nil;
}
