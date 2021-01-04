#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
//#include <AppleEvents.h>
#include <QuickTime/QuickTimeComponents.h>
#include "SoundFile.h"
#include "SoundHack.h"
#include "Menu.h"
#include "Misc.h"
#include "FFT.h"
#include "Windows.h"
#include "Dialog.h"
#include "PhaseVocoder.h"
#include "Analysis.h"
#include "ReadHeader.h"
#include "SpectralHeader.h"
#include "CreateSoundFile.h"
#include "CloseSoundFile.h"
#include "WriteHeader.h"
#include "ShowSpect.h"
#include "ShowSono.h"
#include "CarbonGlue.h"
#include "SpectFileIO.h"

#define QTRPI 0.785398163397448
#define FOUROVERPI 1.273239544735163
#define ONEOVERTHREEPI 0.106103295394597
#define ONEOVERPI 0.318309886183791
#define TWOOVERTHREEPI 0.212206590789194
#define THREEPIOVERTWO 4.71238898038469

extern float		Pi, twoPi;
extern MenuHandle	gSigDispMenu, gSpectDispMenu, gSonoDispMenu;
extern SoundInfoPtr	inSIPtr, outSIPtr, outLSIPtr, outRSIPtr;

DialogPtr	gAnalDialog, gSoundPICTDialog;
extern short		gProcessEnabled, gProcessDisabled, gStopProcess;
extern float		gScale, gScaleL, gScaleR, gScaleDivisor;
extern long			gNumberBlocks;
extern short		gAppFileNum;

extern long	gInPointer, gInPosition, gOutPointer;
extern float 	*analysisWindow, *synthesisWindow, *inputL, *inputR, *spectrum,
		 *outputL, *outputR, *polarSpectrum, *displaySpectrum, *lastPhaseInL, *lastPhaseInR;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

PictCoderInfo	gPCI;
Handle 			gCompressedImage;

SoundDisp	gAnalPICTDispL, gAnalPICTDispR, gSynthPICTDisp;

PvocInfo 	gAnaPI;

long		gSpectPICTWidth;

void	SynthAnalDialogInit(void)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	float	frequency;
	double	tmpDouble;
	float	tmpFloat;
	long	tmpLong;
	Str255	errStr;
	WindInfo *theWindInfo;

	gAnalDialog = GetNewDialog(ANAL_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)ANAL_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gAnalDialog), (long)theWindInfo);
#else
	SetWRefCon(gAnalDialog, (long)theWindInfo);
#endif

	gProcessDisabled = UTIL_PROCESS;
	// Rename Display MENUs
	SetMenuItemText(gSigDispMenu, DISP_AUX_ITEM, "\p ");
	SetMenuItemText(gSpectDispMenu, DISP_AUX_ITEM, "\p ");
	SetMenuItemText(gSonoDispMenu, DISP_AUX_ITEM, "\p ");
	DisableItem(gSigDispMenu, DISP_AUX_ITEM);
	DisableItem(gSpectDispMenu, DISP_AUX_ITEM);
	DisableItem(gSonoDispMenu, DISP_AUX_ITEM);
	
	
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == SDIFF)
	{
		SetMenuItemText(gSigDispMenu, DISP_INPUT_ITEM, "\p ");
		SetMenuItemText(gSpectDispMenu, DISP_INPUT_ITEM, "\p ");
		SetMenuItemText(gSonoDispMenu, DISP_INPUT_ITEM, "\p ");
	
		SetMenuItemText(gSigDispMenu, DISP_OUTPUT_ITEM, "\pOutput ");
		SetMenuItemText(gSpectDispMenu, DISP_OUTPUT_ITEM, "\pOutput ");
		SetMenuItemText(gSonoDispMenu, DISP_OUTPUT_ITEM, "\pOutput ");

		DisableItem(gSigDispMenu, DISP_INPUT_ITEM);
		DisableItem(gSpectDispMenu, DISP_INPUT_ITEM);
		DisableItem(gSonoDispMenu, DISP_INPUT_ITEM);
		EnableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
		EnableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
		EnableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	}
	else
	{
		SetMenuItemText(gSigDispMenu, DISP_INPUT_ITEM, "\pInput");
		SetMenuItemText(gSpectDispMenu, DISP_INPUT_ITEM, "\pInput");
		SetMenuItemText(gSonoDispMenu, DISP_INPUT_ITEM, "\pInput");
	
		SetMenuItemText(gSigDispMenu, DISP_OUTPUT_ITEM, "\p ");
		SetMenuItemText(gSpectDispMenu, DISP_OUTPUT_ITEM, "\p ");
		SetMenuItemText(gSonoDispMenu, DISP_OUTPUT_ITEM, "\p ");
		
		EnableItem(gSigDispMenu, DISP_INPUT_ITEM);
		EnableItem(gSpectDispMenu, DISP_INPUT_ITEM);
		EnableItem(gSonoDispMenu, DISP_INPUT_ITEM);
		DisableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
		DisableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
		DisableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	}

	MenuUpdate();
		
	/* Set up variables */
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gAnalDialog));
#else
	SetPort((GrafPtr)gAnalDialog);
#endif
	if(gAnaPI.analysisType == PICT_ANALYSIS)
		gAnaPI.analysisType = SOUNDHACK_ANALYSIS;
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == SDIFF)
	{
		gAnaPI.points = inSIPtr->spectFrameSize;
		gAnaPI.decimation = gAnaPI.interpolation = inSIPtr->spectFrameIncr;
		if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPFRQ)
			gAnaPI.analysisType = CSOUND_ANALYSIS;
		else if(inSIPtr->packMode == SF_FORMAT_SPECT_AMPPHS)
			gAnaPI.analysisType = SOUNDHACK_ANALYSIS;
		else
			gAnaPI.analysisType = SDIF_ANALYSIS;
		if(gAnaPI.analysisType == CSOUND_ANALYSIS)
			gAnaPI.windowSize = gAnaPI.decimation * 4;
		else
			gAnaPI.windowSize = gAnaPI.decimation * 8;
		GetDialogItem(gAnalDialog, A_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);		
		GetDialogItem(gAnalDialog, A_TYPE_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);		
		GetDialogItem(gAnalDialog, A_BANDS_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);
#if TARGET_API_MAC_CARBON == 1
		SetWTitle(GetDialogWindow(gAnalDialog), "\pSpectral Resynthesis");
#else
		SetWTitle(gAnalDialog, "\pSpectral Resynthesis");
#endif
	}
	else
	{
		GetDialogItem(gAnalDialog, A_TYPE_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);		
		GetDialogItem(gAnalDialog, A_BANDS_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);
#if TARGET_API_MAC_CARBON == 1
		SetWTitle(GetDialogWindow(gAnalDialog), "\pSpectral Analysis");
#else
		SetWTitle(gAnalDialog, "\pSpectral Analysis");
#endif
		GetDialogItem(gAnalDialog,A_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);	
	}
	GetDialogItem(gAnalDialog, A_WINDOW_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gAnaPI.windowType);
	GetDialogItem(gAnalDialog, A_TYPE_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gAnaPI.analysisType);
	
	tmpDouble =	LLog2((double)gAnaPI.points);
	tmpFloat = tmpDouble;
	tmpLong = (long)tmpFloat;	// 10 for 1024
	GetDialogItem(gAnalDialog, A_BANDS_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, (14 - tmpLong));
	
	tmpLong = (gAnaPI.windowSize * 2)/gAnaPI.points;
	GetDialogItem(gAnalDialog, A_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
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
	
	frequency = inSIPtr->sRate/gAnaPI.points;
	FixToString(frequency, errStr);
	GetDialogItem(gAnalDialog, A_FREQ_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, errStr);
		
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gAnalDialog));
	SelectWindow(GetDialogWindow(gAnalDialog));
#else
	ShowWindow(gAnalDialog);
	SelectWindow(gAnalDialog);
#endif
}

void	SynthAnalDialogEvent(short itemHit)
{
	short	itemType;
	Rect	itemRect, dialogRect;
	Handle	itemHandle;
	Str255	tmpStr;
	double	tmpDouble, inLength;
	static short	choice = 4, scaleChoice = 1;
	static long	oldWindowChoice;
	static Boolean	altScale = FALSE;
	WindInfo	*myWI;
	
	inLength = inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(GetWindowPort(GetDialogWindow(gAnalDialog)), &dialogRect);
#else
	dialogRect = gAnalDialog->portRect;
#endif
	switch(itemHit)
	{
		case A_BANDS_MENU:
			GetDialogItem(gAnalDialog, A_BANDS_MENU, &itemType, &itemHandle, &itemRect);
			choice = GetControlValue((ControlHandle)itemHandle);
			gAnaPI.points = 2<<(13 - choice);
			tmpDouble = inSIPtr->sRate/gAnaPI.points;
			FixToString(tmpDouble, tmpStr);
			GetDialogItem(gAnalDialog, A_FREQ_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			break;
		case A_WINDOW_MENU:
			GetDialogItem(gAnalDialog,A_WINDOW_MENU, &itemType, &itemHandle, &itemRect);
			gAnaPI.windowType = GetControlValue((ControlHandle)itemHandle);
			break;
		case A_OVERLAP_MENU:
			break;
		case A_TYPE_MENU:
			GetDialogItem(gAnalDialog,A_TYPE_MENU, &itemType, &itemHandle, &itemRect);
			gAnaPI.analysisType = GetControlValue((ControlHandle)itemHandle);
			GetDialogItem(gAnalDialog,A_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
			HiliteControl((ControlHandle)itemHandle,0);		
			break;
		case A_CANCEL_BUTTON:
			GetDialogItem(gAnalDialog,A_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
			choice = GetControlValue((ControlHandle)itemHandle);
			switch(choice)
			{
				case 1:
					gAnaPI.windowSize = (long)(gAnaPI.points * 4.0);
					break;
				case 2:
					gAnaPI.windowSize = (long)(gAnaPI.points * 2.0);
					break;
				case 3:
					gAnaPI.windowSize = gAnaPI.points;
					break;
				case 4:
					gAnaPI.windowSize = (long)(gAnaPI.points * 0.5);
					break;
			}
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gAnalDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gAnalDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gAnalDialog);
			gAnalDialog = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			inSIPtr = nil;
			MenuUpdate();
			break;
		case A_PROCESS_BUTTON:
			GetDialogItem(gAnalDialog,A_OVERLAP_MENU, &itemType, &itemHandle, &itemRect);
			choice = GetControlValue((ControlHandle)itemHandle);
			switch(choice)
			{
				case 1:
					gAnaPI.windowSize = (long)(gAnaPI.points * 4.0);
					break;
				case 2:
					gAnaPI.windowSize = (long)(gAnaPI.points * 2.0);
					break;
				case 3:
					gAnaPI.windowSize = gAnaPI.points;
					break;
				case 4:
					gAnaPI.windowSize = (long)(gAnaPI.points * 0.5);
					break;
			}
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gAnalDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gAnalDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gAnalDialog);
			gAnalDialog = nil;
			if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT || inSIPtr->sfType == QKTMV || inSIPtr->sfType == SDIFF)
				InitSynthProcess();
			else
				InitAnalProcess();
			break;
		}
}

void	SoundPICTDialogInit(void)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	float	frequency;
	double	tmpDouble;
	float	tmpFloat;
	short	maxLinesPerFFT;
	long	tmpLong;
	Str255	errStr;
	WindInfo *theWindInfo;

	gSoundPICTDialog = GetNewDialog(SOUNDPICT_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)SOUNDPICT_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gSoundPICTDialog), (long)theWindInfo);
#else
	SetWRefCon(gSoundPICTDialog, (long)theWindInfo);
#endif

	gProcessDisabled = UTIL_PROCESS;
	// Rename Display MENUs
	SetMenuItemText(gSigDispMenu, DISP_AUX_ITEM, "\p ");
	SetMenuItemText(gSpectDispMenu, DISP_AUX_ITEM, "\p ");
	SetMenuItemText(gSonoDispMenu, DISP_AUX_ITEM, "\p ");
	DisableItem(gSigDispMenu, DISP_AUX_ITEM);
	DisableItem(gSpectDispMenu, DISP_AUX_ITEM);
	DisableItem(gSonoDispMenu, DISP_AUX_ITEM);
	
	
	if(inSIPtr->sfType == PICT || inSIPtr->sfType == QKTMV)
	{
		SetMenuItemText(gSigDispMenu, DISP_INPUT_ITEM, "\p ");
		SetMenuItemText(gSpectDispMenu, DISP_INPUT_ITEM, "\p ");
		SetMenuItemText(gSonoDispMenu, DISP_INPUT_ITEM, "\p ");
	
		SetMenuItemText(gSigDispMenu, DISP_OUTPUT_ITEM, "\pOutput ");
		SetMenuItemText(gSpectDispMenu, DISP_OUTPUT_ITEM, "\pOutput ");
		SetMenuItemText(gSonoDispMenu, DISP_OUTPUT_ITEM, "\pOutput ");

		DisableItem(gSigDispMenu, DISP_INPUT_ITEM);
		DisableItem(gSpectDispMenu, DISP_INPUT_ITEM);
		DisableItem(gSonoDispMenu, DISP_INPUT_ITEM);
		EnableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
		EnableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
		EnableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	}
	else
	{
		SetMenuItemText(gSigDispMenu, DISP_INPUT_ITEM, "\pInput");
		SetMenuItemText(gSpectDispMenu, DISP_INPUT_ITEM, "\pInput");
		SetMenuItemText(gSonoDispMenu, DISP_INPUT_ITEM, "\pInput");
	
		SetMenuItemText(gSigDispMenu, DISP_OUTPUT_ITEM, "\p ");
		SetMenuItemText(gSpectDispMenu, DISP_OUTPUT_ITEM, "\p ");
		SetMenuItemText(gSonoDispMenu, DISP_OUTPUT_ITEM, "\p ");
		
		EnableItem(gSigDispMenu, DISP_INPUT_ITEM);
		EnableItem(gSpectDispMenu, DISP_INPUT_ITEM);
		EnableItem(gSonoDispMenu, DISP_INPUT_ITEM);
		DisableItem(gSigDispMenu, DISP_OUTPUT_ITEM);
		DisableItem(gSpectDispMenu, DISP_OUTPUT_ITEM);
		DisableItem(gSonoDispMenu, DISP_OUTPUT_ITEM);
	}

	MenuUpdate();
		
	/* Set up variables */
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gSoundPICTDialog));
#else
	SetPort((GrafPtr)gSoundPICTDialog);
#endif
	if(inSIPtr->sfType == PICT || inSIPtr->sfType == QKTMV )
	{
		gAnaPI.points = inSIPtr->spectFrameSize;
		gAnaPI.decimation = gAnaPI.interpolation = inSIPtr->spectFrameIncr;
		gAnaPI.analysisType = PICT_ANALYSIS;
		gAnaPI.windowSize = gAnaPI.decimation * 8;
		GetDialogItem(gSoundPICTDialog, SP_BANDS_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,255);
#if TARGET_API_MAC_CARBON == 1
		SetWTitle(GetDialogWindow(gSoundPICTDialog), "\pQuickTime Decoder");
#else
		SetWTitle(gSoundPICTDialog, "\pQuickTime Decoder");
#endif
	}
	else
	{
		GetDialogItem(gSoundPICTDialog, SP_BANDS_MENU, &itemType, &itemHandle, &itemRect);
		HiliteControl((ControlHandle)itemHandle,0);
#if TARGET_API_MAC_CARBON == 1
		SetWTitle(GetDialogWindow(gSoundPICTDialog), "\pQuickTime Coder");
#else
		SetWTitle(gSoundPICTDialog, "\pQuickTime Coder");
#endif
	}
	// set up initial control values
	// window type
	GetDialogItem(gSoundPICTDialog, SP_WINDOW_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gAnaPI.windowType);
	// number of fft bands
	tmpDouble =	LLog2((double)gAnaPI.points);
	tmpFloat = tmpDouble;
	tmpLong = (long)tmpFloat;	// 10 for 1024
	GetDialogItem(gSoundPICTDialog, SP_BANDS_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, (14 - tmpLong));
	// frequency separation
	frequency = inSIPtr->sRate/gAnaPI.points;
	FixToString(frequency, errStr);
	GetDialogItem(gSoundPICTDialog, SP_FREQ_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, errStr);
	// moving sonogram box
	GetDialogItem(gSoundPICTDialog, SP_MOVING_BOX, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gPCI.moving);
	// center color	
	GetDialogItem(gSoundPICTDialog, SP_COLOR_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gPCI.centerColor);
	// amplitude range
	FixToString(gPCI.amplitudeRange, errStr);
	GetDialogItem(gSoundPICTDialog, SP_AMPLITUDE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, errStr);
	// color inversion box
	GetDialogItem(gSoundPICTDialog, SP_INVERT_BOX, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gPCI.inversion);
	// amp to hue and phase to hue
	if(gPCI.type == 1)
	{
		GetDialogItem(gSoundPICTDialog, SP_AMPHUE_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle, TRUE);
		GetDialogItem(gSoundPICTDialog, SP_PHASEHUE_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle, FALSE);
	}
	else
	{
		GetDialogItem(gSoundPICTDialog, SP_AMPHUE_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle, FALSE);
		GetDialogItem(gSoundPICTDialog, SP_PHASEHUE_RADIO, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle, TRUE);
		gPCI.type = 0;
	}
	// lines per fft slider and box
	gSpectPICTWidth = (gAnaPI.points * 2)/3;
	if(gPCI.moving)
		maxLinesPerFFT = gSpectPICTWidth/30;
	else
		maxLinesPerFFT = gSpectPICTWidth;
	if(gPCI.linesPerFFT > maxLinesPerFFT)
		gPCI.linesPerFFT = maxLinesPerFFT;
	GetDialogItem(gSoundPICTDialog, SP_LINES_SLIDER, &itemType, &itemHandle, &itemRect);
	SetControlMaximum((ControlHandle)itemHandle, maxLinesPerFFT);
	SetControlValue((ControlHandle)itemHandle, gPCI.linesPerFFT);
	NumToString(gPCI.linesPerFFT, errStr);
	GetDialogItem(gSoundPICTDialog, SP_LINES_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, errStr);
	
	gAnaPI.analysisType = PICT_ANALYSIS;
		
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gSoundPICTDialog));
	SelectWindow(GetDialogWindow(gSoundPICTDialog));
#else
	ShowWindow(gSoundPICTDialog);
	SelectWindow(gSoundPICTDialog);
#endif
}

void	SoundPICTDialogEvent(short itemHit)
{
	short	itemType, maxLinesPerFFT;
	Rect	itemRect, dialogRect;
	Handle	itemHandle;
	Str255	tmpStr, errStr;
	double	tmpDouble, inLength;
	static short	choice = 4, scaleChoice = 1;
	static long	oldWindowChoice;
	static Boolean	altScale = FALSE;
	WindInfo	*myWI;
	
	inLength = inSIPtr->numBytes/(inSIPtr->sRate * inSIPtr->nChans * inSIPtr->frameSize);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(GetWindowPort(GetDialogWindow(gSoundPICTDialog)), &dialogRect);
#else
	dialogRect = gSoundPICTDialog->portRect;
#endif
	switch(itemHit)
	{
		case SP_BANDS_MENU:
			GetDialogItem(gSoundPICTDialog, SP_BANDS_MENU, &itemType, &itemHandle, &itemRect);
			choice = GetControlValue((ControlHandle)itemHandle);
			gAnaPI.points = 2<<(13 - choice);
			tmpDouble = inSIPtr->sRate/gAnaPI.points;
			FixToString(tmpDouble, tmpStr);
			GetDialogItem(gSoundPICTDialog, SP_FREQ_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, tmpStr);
			// lines per fft slider and box
			gSpectPICTWidth = (gAnaPI.points * 2)/3;
			if(gPCI.moving)
				maxLinesPerFFT = gSpectPICTWidth/30;
			else
				maxLinesPerFFT = gSpectPICTWidth;
			if(gPCI.linesPerFFT > maxLinesPerFFT)
				gPCI.linesPerFFT = maxLinesPerFFT;
			if(gPCI.linesPerFFT < 1)
				gPCI.linesPerFFT = 1;
			GetDialogItem(gSoundPICTDialog, SP_LINES_SLIDER, &itemType, &itemHandle, &itemRect);
			SetControlMaximum((ControlHandle)itemHandle, maxLinesPerFFT);
			SetControlValue((ControlHandle)itemHandle, gPCI.linesPerFFT);
			NumToString(gPCI.linesPerFFT, errStr);
			GetDialogItem(gSoundPICTDialog, SP_LINES_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, errStr);
			break;
		case SP_LINES_SLIDER:
			GetDialogItem(gSoundPICTDialog, SP_LINES_SLIDER, &itemType, &itemHandle, &itemRect);
			gPCI.linesPerFFT = GetControlValue((ControlHandle)itemHandle);
			NumToString(gPCI.linesPerFFT, errStr);
			GetDialogItem(gSoundPICTDialog, SP_LINES_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, errStr);
			break;
		case SP_WINDOW_MENU:
			GetDialogItem(gSoundPICTDialog,SP_WINDOW_MENU, &itemType, &itemHandle, &itemRect);
			gAnaPI.windowType = GetControlValue((ControlHandle)itemHandle);
			break;
		case SP_AMPHUE_RADIO:
			GetDialogItem(gSoundPICTDialog, SP_AMPHUE_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, TRUE);
			GetDialogItem(gSoundPICTDialog, SP_PHASEHUE_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, FALSE);
			gPCI.type = 1;
			break;
		case SP_PHASEHUE_RADIO:
			GetDialogItem(gSoundPICTDialog, SP_AMPHUE_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, FALSE);
			GetDialogItem(gSoundPICTDialog, SP_PHASEHUE_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, TRUE);
			gPCI.type = 0;
			break;
		case SP_COLOR_MENU:
			GetDialogItem(gSoundPICTDialog, SP_COLOR_MENU, &itemType, &itemHandle, &itemRect);
			gPCI.centerColor = GetControlValue((ControlHandle)itemHandle);
			break;
		case SP_AMPLITUDE_FIELD:
			GetDialogItem(gSoundPICTDialog, SP_AMPLITUDE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, tmpStr);
			StringToFix(tmpStr, &gPCI.amplitudeRange);
			break;
		case SP_INVERT_BOX:
			GetDialogItem(gSoundPICTDialog, SP_INVERT_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
				gPCI.inversion = FALSE;
			else
				gPCI.inversion = TRUE;
			SetControlValue((ControlHandle)itemHandle, gPCI.inversion);
			break;
		case SP_MOVING_BOX:
			GetDialogItem(gSoundPICTDialog, SP_MOVING_BOX, &itemType, &itemHandle, &itemRect);
			if(GetControlValue((ControlHandle)itemHandle) == TRUE)
				gPCI.moving = FALSE;
			else
				gPCI.moving = TRUE;
			SetControlValue((ControlHandle)itemHandle, gPCI.moving);
			// lines per fft slider and box
			gSpectPICTWidth = (gAnaPI.points * 2)/3;
			if(gPCI.moving)
				maxLinesPerFFT = gSpectPICTWidth/30;
			else
				maxLinesPerFFT = gSpectPICTWidth;
			if(gPCI.linesPerFFT > maxLinesPerFFT)
				gPCI.linesPerFFT = maxLinesPerFFT;
			if(gPCI.linesPerFFT < 1)
				gPCI.linesPerFFT = 1;
			GetDialogItem(gSoundPICTDialog, SP_LINES_SLIDER, &itemType, &itemHandle, &itemRect);
			SetControlMaximum((ControlHandle)itemHandle, maxLinesPerFFT);
			SetControlValue((ControlHandle)itemHandle, gPCI.linesPerFFT);
			NumToString(gPCI.linesPerFFT, errStr);
			GetDialogItem(gSoundPICTDialog, SP_LINES_FIELD, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, errStr);
			break;
		case SP_CANCEL_BUTTON:
			gAnaPI.windowSize = gAnaPI.points;
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gSoundPICTDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gSoundPICTDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gSoundPICTDialog);
			gSoundPICTDialog = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			inSIPtr = nil;
			MenuUpdate();
			break;
		case SP_PROCESS_BUTTON:
			gAnaPI.windowSize = gAnaPI.points;
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gSoundPICTDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gSoundPICTDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gSoundPICTDialog);
			gSoundPICTDialog = nil;
			if(inSIPtr->sfType == PICT || inSIPtr->sfType == QKTMV)
				InitSynthProcess();
			else
				InitAnalProcess();
			break;
		}
}

short	InitAnalProcess(void)
{
	long	blockSize, maxCompressionSize, error;
	
	SoundInfoPtr	tmpSIPtr;
	PixMapHandle	thePixMap;
	Rect			movieBounds;
	
	// Set all derived constants
	gScaleDivisor = 1.0;
	gAnaPI.halfPoints = gAnaPI.points >>1;
	
	// windowSize/8 seems to be the largest value possible for time stretching
	// without distortion
		
	if(gAnaPI.analysisType == CSOUND_ANALYSIS)
	{
		gAnaPI.decimation = gAnaPI.windowSize/4;
		gAnaPI.interpolation = gAnaPI.decimation;
		outLSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
		outLSIPtr->sfType = CS_PVOC;
		outLSIPtr->packMode = SF_FORMAT_SPECT_AMPFRQ;
		StringCopy("\ppvoc.1", outLSIPtr->sfSpec.name);
		outLSIPtr->sRate = inSIPtr->sRate;
		outLSIPtr->nChans = MONO;
		outLSIPtr->numBytes = 0;
		outLSIPtr->spectFrameSize = gAnaPI.points;
		outLSIPtr->spectFrameIncr = gAnaPI.decimation;
		if(CreateSoundFile(&outLSIPtr, SPECT_DIALOG) == -1)
		{
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			DisposePtr((Ptr)outLSIPtr);
			outLSIPtr = nil;
			return(-1);
		}
		WriteHeader(outLSIPtr);
		UpdateInfoWindow(outLSIPtr);
		if(inSIPtr->nChans == STEREO)
		{
			outRSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
			outRSIPtr->sfType = CS_PVOC;
			outRSIPtr->packMode = SF_FORMAT_SPECT_AMPFRQ;
			StringCopy("\ppvoc.2", outRSIPtr->sfSpec.name);
			outRSIPtr->sRate = inSIPtr->sRate;
			outRSIPtr->nChans = MONO;
			outRSIPtr->numBytes = 0;
			outRSIPtr->spectFrameSize = gAnaPI.points;
			outRSIPtr->spectFrameIncr = gAnaPI.decimation;
			if(CreateSoundFile(&outRSIPtr, SPECT_DIALOG) == -1)
			{
				gProcessDisabled = gProcessEnabled = NO_PROCESS;
				MenuUpdate();
				DisposePtr((Ptr)outRSIPtr);
				outRSIPtr = nil;
				tmpSIPtr = outLSIPtr;
				outLSIPtr = nil;
				CloseSoundFile(tmpSIPtr);
				return(-1);
			}
			WriteHeader(outRSIPtr);
			UpdateInfoWindow(outRSIPtr);
		}
	}
	else if(gAnaPI.analysisType == SOUNDHACK_ANALYSIS)
	{
		gAnaPI.decimation = gAnaPI.windowSize/8;
		gAnaPI.interpolation = gAnaPI.decimation;
		outSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
		outSIPtr->sfType = CS_PVOC;
		outSIPtr->packMode = SF_FORMAT_SPECT_AMPPHS;
		NameFile(inSIPtr->sfSpec.name, "\pSA", outSIPtr->sfSpec.name);
		outSIPtr->sRate = inSIPtr->sRate;
		outSIPtr->nChans = inSIPtr->nChans;
		outSIPtr->numBytes = 0;
		outSIPtr->spectFrameSize = gAnaPI.points;
		outSIPtr->spectFrameIncr = gAnaPI.decimation;
		if(CreateSoundFile(&outSIPtr, SPECT_DIALOG) == -1)
		{
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			DisposePtr((Ptr)outSIPtr);
			outSIPtr = nil;
			return(-1);
		}
		WriteHeader(outSIPtr);
		UpdateInfoWindow(outSIPtr);
	}
	else if(gAnaPI.analysisType == SDIF_ANALYSIS)
	{
		gAnaPI.decimation = gAnaPI.windowSize/8;
		gAnaPI.interpolation = gAnaPI.decimation;
		outSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
		outSIPtr->sfType = SDIFF;
		outSIPtr->packMode = SF_FORMAT_SPECT_COMPLEX;
		NameFile(inSIPtr->sfSpec.name, "\p.sdif", outSIPtr->sfSpec.name);
		outSIPtr->sRate = inSIPtr->sRate;
		outSIPtr->nChans = inSIPtr->nChans;
		outSIPtr->numBytes = 0;
		outSIPtr->spectFrameSize = gAnaPI.points;
		outSIPtr->spectFrameIncr = gAnaPI.decimation;
		if(CreateSoundFile(&outSIPtr, SPECT_DIALOG) == -1)
		{
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			DisposePtr((Ptr)outSIPtr);
			outSIPtr = nil;
			return(-1);
		}
		WriteHeader(outSIPtr);
		UpdateInfoWindow(outSIPtr);
	}
	else if(gAnaPI.analysisType == PICT_ANALYSIS)
	{
		gAnaPI.decimation = gAnaPI.windowSize/8;
		gAnaPI.interpolation = gAnaPI.decimation;
		gSpectPICTWidth = (gAnaPI.points * 2)/3;
		outLSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
		outLSIPtr->sfType = QKTMV;
		outLSIPtr->packMode = SF_FORMAT_PICT;
		NameFile(inSIPtr->sfSpec.name, "\pPCAL", outLSIPtr->sfSpec.name);
		outLSIPtr->sRate = inSIPtr->sRate;
		outLSIPtr->nChans = 1;
		outLSIPtr->numBytes = 0;
		outLSIPtr->frameSize = 2.0;
		outLSIPtr->spectFrameSize = gAnaPI.points;
		outLSIPtr->spectFrameIncr = gAnaPI.decimation;
		if(CreateSoundFile(&outLSIPtr, SPECT_DIALOG) == -1)
		{
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			DisposePtr((Ptr)outLSIPtr);
			outLSIPtr = nil;
			return(-1);
		}
		WriteHeader(outLSIPtr);
		UpdateInfoWindow(outLSIPtr);
		error = CreateSpectDispWindow(&gAnalPICTDispL, 40, outLSIPtr->sfSpec.name);
		if(error != 0)
		{
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			DisposePtr((Ptr)outLSIPtr);
			outLSIPtr = nil;
			return(-1);
		}
		if(inSIPtr->nChans == STEREO)
		{
			outRSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
			outRSIPtr->sfType = QKTMV;
			outRSIPtr->packMode = SF_FORMAT_PICT;
			NameFile(inSIPtr->sfSpec.name, "\pPCAR", outRSIPtr->sfSpec.name);
			outRSIPtr->sRate = inSIPtr->sRate;
			outRSIPtr->nChans = 1;
			outRSIPtr->numBytes = 0;
			outRSIPtr->frameSize = 2.0;
			outRSIPtr->spectFrameSize = gAnaPI.points;
			outRSIPtr->spectFrameIncr = gAnaPI.decimation;
			if(CreateSoundFile(&outRSIPtr, SPECT_DIALOG) == -1)
			{
				gProcessDisabled = gProcessEnabled = NO_PROCESS;
				MenuUpdate();
				DisposePtr((Ptr)outRSIPtr);
				outRSIPtr = nil;
				DisposePtr((Ptr)outLSIPtr);
				outLSIPtr = nil;
				return(-1);
			}
			WriteHeader(outRSIPtr);
			UpdateInfoWindow(outRSIPtr);
			error = CreateSpectDispWindow(&gAnalPICTDispR, 300, outRSIPtr->sfSpec.name);
			if(error != 0)
			{
				gProcessDisabled = gProcessEnabled = NO_PROCESS;
				MenuUpdate();
				DisposePtr((Ptr)outRSIPtr);
				outRSIPtr = nil;
				DisposePtr((Ptr)outLSIPtr);
				outLSIPtr = nil;
				return(-1);
			}
		}	
		// get memory for compressed image
		movieBounds.left = movieBounds.top = 0;
		movieBounds.bottom = (gAnaPI.points>>1) + 1;
		movieBounds.right = gSpectPICTWidth;
	
		thePixMap = GetGWorldPixMap(gAnalPICTDispL.offScreen);
		LockPixels(thePixMap);
		GetMaxCompressionSize(thePixMap, &(movieBounds), 0, codecNormalQuality,
							'raw ', (CompressorComponent)anyCodec, &maxCompressionSize);
		UnlockPixels(thePixMap);
		gCompressedImage = NewHandle(maxCompressionSize);
		if (gCompressedImage == nil)
		{
			DrawErrorMessage("\pTrouble allocating QuickTime frame memory, more memory is needed for large images.");
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			if(inSIPtr->nChans == STEREO)
			{
				DisposePtr((Ptr)outRSIPtr);
				DisposeWindWorld(&gAnalPICTDispR);
				outRSIPtr = nil;
			}
			DisposePtr((Ptr)outLSIPtr);
			DisposeWindWorld(&gAnalPICTDispL);
			outLSIPtr = nil;
			return(-1);
		}
	}

	gAnaPI.halfPoints = gAnaPI.points>>1;
    blockSize = gAnaPI.decimation;

/*
 * allocate memory
 */
	analysisWindow = synthesisWindow = inputL = nil;
	spectrum = polarSpectrum = displaySpectrum = lastPhaseInL = nil;
 	inputR = lastPhaseInR = outputL = outputR = nil;

	
	analysisWindow = (float *)NewPtr(gAnaPI.windowSize*sizeof(float));
	synthesisWindow = (float *)NewPtr(gAnaPI.windowSize*sizeof(float));
	inputL = (float *)NewPtr(gAnaPI.windowSize*sizeof(float));
	spectrum = (float *)NewPtr(gAnaPI.points*sizeof(float));
	polarSpectrum = (float *)NewPtr((gAnaPI.points+2)*sizeof(float));		/* analysis polarSpectrum */
	displaySpectrum = (float *)NewPtr((gAnaPI.halfPoints+1)*sizeof(float));	/* For display of amplitude */
	lastPhaseInL = (float *)NewPtr((gAnaPI.halfPoints+1) * sizeof(float));
	
	if(inSIPtr->nChans == STEREO)
	{
		inputR = (float *)NewPtr(gAnaPI.windowSize*sizeof(float));	/* inputR buffer */
		lastPhaseInR = (float *)NewPtr((gAnaPI.halfPoints+1) * sizeof(float));
		if(lastPhaseInR == 0)
		{
			DrawErrorMessage("\pNot enough memory for processing");
			DeAllocAnalMem();
			DisposeWindWorld(&gAnalPICTDispR);
			DisposePtr((Ptr)outRSIPtr);
			outRSIPtr = nil;
			DisposeWindWorld(&gAnalPICTDispL);
			DisposePtr((Ptr)outLSIPtr);
			outLSIPtr = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			return(-1);
		}
	}
	
	if(displaySpectrum == 0 || lastPhaseInL == 0)
	{
		DrawErrorMessage("\pNot enough memory for processing");
		DeAllocAnalMem();
		DisposeWindWorld(&gAnalPICTDispL);
		DisposePtr((Ptr)outLSIPtr);
		outLSIPtr = nil;
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		return(-1);
	}
	ZeroFloatTable(lastPhaseInL, gAnaPI.halfPoints+1);
	ZeroFloatTable(spectrum, gAnaPI.points);
	ZeroFloatTable(polarSpectrum, (gAnaPI.points+2));
	ZeroFloatTable(inputL, gAnaPI.windowSize);
	if(inSIPtr->nChans == STEREO)
	{
		ZeroFloatTable(lastPhaseInR, gAnaPI.halfPoints+1);
		ZeroFloatTable(inputR, gAnaPI.windowSize);
	}

	UpdateProcessWindow("\p", "\p", "\pspectral analysis", 0.0);
/*
 * create and scale windows
 */
 	GetWindow(analysisWindow, gAnaPI.windowSize, gAnaPI.windowType);
 	GetWindow(synthesisWindow, gAnaPI.windowSize, gAnaPI.windowType);
    ScaleWindows(analysisWindow, synthesisWindow, gAnaPI);
/*
 * initialize input and output time values (in samples)
 */
    gOutPointer = gInPointer = -gAnaPI.windowSize;
	gInPosition	= 0;
	
	gNumberBlocks = 0;	
	gProcessEnabled = ANAL_PROCESS;
	gProcessDisabled = NO_PROCESS;
	MenuUpdate();
	return(TRUE);
}

short	CreateSpectDispWindow(SoundDisp *newSpectDisp, short left, Str255 windowStr)
{
	OSErr	error;
	Rect	windowRect, offScreenRect;
	WindInfo	*theWindInfo;
	
	
	offScreenRect.left = offScreenRect.top = 0;
	offScreenRect.bottom = (gAnaPI.points>>1) + 1;
	offScreenRect.right = gSpectPICTWidth;
	windowRect.top = 40;
	windowRect.left = left;
	windowRect.bottom = offScreenRect.bottom + 40;
	windowRect.right = offScreenRect.right + left;
	newSpectDisp->windo = NewCWindow(nil,&windowRect, windowStr,FALSE,noGrowDocProc,(WindowPtr)(-1L),FALSE,999);
	// Get port and device of current set port for graphics world
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetWindowPort(newSpectDisp->windo));
#else
	SetPort((GrafPtr)newSpectDisp->windo);
#endif
	GetGWorld(&newSpectDisp->winPort, &newSpectDisp->winDevice );
	error = NewGWorld(&(newSpectDisp->offScreen), 32, &(offScreenRect), NULL, NULL, 0L );
	if(error)
	{
		DrawErrorMessage("\pTrouble allocating offscreen memory, more memory is needed for large images.");
		DisposeWindWorld(newSpectDisp);
		return(-1);
	}
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = ANALWIND;
	theWindInfo->structPtr = (Ptr)newSpectDisp;
	SetWRefCon(newSpectDisp->windo,(long)theWindInfo);
	return(0);
}

void	UpdateSpectDispWindow(SoundDisp *spectDisp)
{
	RGBColor	whiteRGB, blackRGB;
	PixMapHandle	thePixMap;
	Rect windowRect, offScreenRect;

	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;

	if(spectDisp->offScreen == NIL_POINTER)
		return;
	thePixMap = GetGWorldPixMap(spectDisp->offScreen);
	LockPixels(thePixMap);
	SetGWorld(spectDisp->winPort, spectDisp->winDevice );
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);

#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(spectDisp->offScreen, &offScreenRect);
	GetPortBounds(GetWindowPort(spectDisp->windo), &windowRect);
	CopyBits(
			GetPortBitMapForCopyBits(spectDisp->offScreen),//bitmap of gworld
			GetPortBitMapForCopyBits(GetWindowPort(spectDisp->windo)),//bitmap of gworld
			&offScreenRect,			//portRect of gworld
			&windowRect,			//portRect of gworld
			srcCopy, NULL );
#else
	CopyBits(&(((GrafPtr)spectDisp->offScreen)->portBits),//bitmap of gworld
			&(((GrafPtr)spectDisp->windo)->portBits),	//bitmap of window
			&(spectDisp->offScreen->portRect),			//portRect of gworld
			&(spectDisp->windo->portRect),				//portRect of window
			srcCopy, NULL );
#endif
	UnlockPixels(thePixMap);
	ShowWindow(spectDisp->windo);
}

/*
 * main loop--perform phase vocoder analysis
 */
short
AnalBlock(void)
{
    short	curMark;
	long	numSampsRead, numSampsWritten;
	float 	length, displayScale, inTime;
	Str255	errStr;
	static long validSamples, outSamples;

	if(gNumberBlocks == 0)
	{
		validSamples = -1L;
		outSamples = 0;
	}
	if(gAnaPI.analysisType == CSOUND_ANALYSIS)
		displayScale = 8.0/gAnaPI.points;
	else if(gAnaPI.analysisType == SOUNDHACK_ANALYSIS || gAnaPI.analysisType == PICT_ANALYSIS || gAnaPI.analysisType == SDIF_ANALYSIS)
		displayScale = 1.0;
	
// increment times
	gInPointer += gAnaPI.decimation;
	
/*
 * analysis: input gAnaPI.decimation samples; window, fold and rotate input
 * samples into FFT buffer; take FFT; and convert to
 * amplitude-frequency (phase vocoder) form
 */
	SetFPos(inSIPtr->dFileNum, fsFromStart, (inSIPtr->dataStart + gInPosition));
	numSampsRead =
	 ShiftIn(inSIPtr, inputL, inputR, gAnaPI.windowSize, gAnaPI.decimation, &validSamples);
	if(gStopProcess == TRUE)
		numSampsRead = -2L;
	if(numSampsRead > 0)
	{
		inTime = (numSampsRead + (gInPosition/(inSIPtr->nChans*inSIPtr->frameSize)))/inSIPtr->sRate;
		HMSTimeString(inTime, errStr);
		length = (numSampsRead*inSIPtr->nChans*inSIPtr->frameSize + gInPosition)/inSIPtr->numBytes;
		UpdateProcessWindow(errStr, "\p", "\pspectral analysis", length);
	}
	
	// analysis
	WindowFold(inputL, analysisWindow, spectrum, gInPointer, gAnaPI.points, gAnaPI.windowSize);
	RealFFT(spectrum, gAnaPI.halfPoints, TIME2FREQ);
	CartToPolar(spectrum, polarSpectrum, gAnaPI.halfPoints);

	// spectral display routines	
	GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(polarSpectrum, displaySpectrum, gAnaPI.halfPoints, inSIPtr, LEFT, displayScale, "\pInput Channel 1");
	GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(polarSpectrum, displaySpectrum, gAnaPI.halfPoints, inSIPtr, LEFT, displayScale, "\pInput Channel 1");

	// analysis output
	if(gAnaPI.analysisType == CSOUND_ANALYSIS)
		numSampsWritten = WriteCSAData(outLSIPtr, polarSpectrum, lastPhaseInL, gAnaPI.halfPoints);
	else if(gAnaPI.analysisType == SOUNDHACK_ANALYSIS)
		numSampsWritten = WriteSHAData(outSIPtr, polarSpectrum, LEFT);
	else if(gAnaPI.analysisType == SDIF_ANALYSIS)
		numSampsWritten = WriteSDIFData(outSIPtr, spectrum, polarSpectrum, inTime, LEFT);
	else if(gAnaPI.analysisType == PICT_ANALYSIS)
		numSampsWritten = WriteSpectLine(outLSIPtr, polarSpectrum, lastPhaseInL, &gAnalPICTDispL, gAnaPI.halfPoints);
	outSamples += numSampsWritten;
    if(inSIPtr->nChans == STEREO)
    {
		// analysis
		WindowFold(inputR, analysisWindow, spectrum, gInPointer, gAnaPI.points, gAnaPI.windowSize);
		RealFFT(spectrum, gAnaPI.halfPoints, TIME2FREQ);
		CartToPolar(spectrum, polarSpectrum, gAnaPI.halfPoints);
		
		// spectral display routines	
		GetItemMark(gSpectDispMenu, DISP_INPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(polarSpectrum, displaySpectrum, gAnaPI.halfPoints, inSIPtr, RIGHT, displayScale, "\pInput Channel 2");
		GetItemMark(gSonoDispMenu, DISP_INPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(polarSpectrum, displaySpectrum, gAnaPI.halfPoints, inSIPtr, RIGHT, displayScale, "\pInput Channel 2");

		// analysis output
		if(gAnaPI.analysisType == CSOUND_ANALYSIS)
			WriteCSAData(outRSIPtr, polarSpectrum, lastPhaseInR, gAnaPI.halfPoints);
		else if(gAnaPI.analysisType == SOUNDHACK_ANALYSIS)
			WriteSHAData(outSIPtr, polarSpectrum, RIGHT);
		else if(gAnaPI.analysisType == SDIF_ANALYSIS)
			numSampsWritten = WriteSDIFData(outSIPtr, spectrum, polarSpectrum, inTime, RIGHT);
		else if(gAnaPI.analysisType == PICT_ANALYSIS)
			WriteSpectLine(outRSIPtr, polarSpectrum, lastPhaseInR, &gAnalPICTDispR, gAnaPI.halfPoints);
	}
	if(numSampsRead > 0)
	{
		if(gAnaPI.analysisType == SOUNDHACK_ANALYSIS || gAnaPI.analysisType == SDIF_ANALYSIS)
			length = outSamples/(outSIPtr->sRate);
		else
			length = outSamples/(outLSIPtr->sRate);
		HMSTimeString(length, errStr);
		UpdateProcessWindow("\p", errStr, "\pspectral analysis", -1.0);
	}
	
//	Finish it
	if(numSampsRead == -2L || numSampsWritten == -2L)
	{
		if(gAnaPI.analysisType == PICT_ANALYSIS)
		{
			WriteSpectFrame(outLSIPtr, &gAnalPICTDispL);
   			if(inSIPtr->nChans == STEREO)
				WriteSpectFrame(outRSIPtr, &gAnalPICTDispR);
			DisposeWindWorld(&gAnalPICTDispL);
   			if(inSIPtr->nChans == STEREO)
				DisposeWindWorld(&gAnalPICTDispR);
			ReadMovieHeader(outLSIPtr);
   			if(inSIPtr->nChans == STEREO)
				ReadMovieHeader(outRSIPtr);
		}
		DeAllocAnalMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	gNumberBlocks++;
 	gInPosition += inSIPtr->nChans * gAnaPI.decimation * inSIPtr->frameSize;
	return(TRUE);
}

//	Write a 262144 pixel PICT
//	Read in the PICT handle, write (draw) a new column, release the resource, 
//	increment pictColumn. Make a new resource after the 512th column.

long	WriteSpectLine(SoundInfo *mySI, float polarSpectrum[], float lastPhaseIn[], SoundDisp *spectDisp, long halfLengthFFT)
{
	long numSamples, bandNumber, ampIndex, phaseIndex, error;
	double phaseDifference, logAmp;
	static float factor, fundamental;
	static double ampFactor1, ampFactor2;
	RGBColor	myRGBColor, whiteRGB, blackRGB;
	PenState	myPenState, oldPenState;
	Rect		theRect;
	PixMapHandle	thePixMap;
	Pattern		whitePat, blackPat;
	RgnHandle	updateRgn;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	// initialize drawing variables if neccesary
    if(gNumberBlocks == 0)
    {
    	spectDisp->spare = 0;
		fundamental = inSIPtr->sRate/(halfLengthFFT<<1);
        factor = inSIPtr->sRate/(gAnaPI.decimation*twoPi);
        ampFactor1 = gPCI.amplitudeRange * 0.05;
        ampFactor2 = pow(10.0, ampFactor1);
    }
    error = 0;
	if(spectDisp->offScreen == NIL_POINTER)
		return(-1);
	thePixMap = GetGWorldPixMap(spectDisp->offScreen);
	LockPixels(thePixMap);
	SetGWorld(spectDisp->offScreen, NULL );	// Set port to the offscreen
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(spectDisp->offScreen, &theRect);
#else
	theRect = spectDisp->offScreen->portRect;
#endif
		
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;

	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);

#if TARGET_API_MAC_CARBON == 1
	if(spectDisp->spare == 0 && gPCI.inversion == FALSE)	// at column 0, clear out the screen
		FillRect(&theRect, &whitePat);
	else if(spectDisp->spare == 0 && gPCI.inversion == TRUE)
		FillRect(&theRect, &blackPat);
#else
	if(spectDisp->spare == 0 && gPCI.inversion == FALSE)	// at column 0, clear out the screen
		FillRect(&theRect,&(qd.white));
	else if(spectDisp->spare == 0 && gPCI.inversion == TRUE)
		FillRect(&theRect,&(qd.black));
#endif
	if(gPCI.moving && spectDisp->spare == 0)
		spectDisp->spare = (gSpectPICTWidth - (gSpectPICTWidth/30));

	
	if(gPCI.moving && spectDisp->spare == (gSpectPICTWidth - (gSpectPICTWidth/30)))
	{
		updateRgn = NewRgn();
		ScrollRect(&theRect,-gSpectPICTWidth/30, 0, updateRgn);
#if TARGET_API_MAC_CARBON == 1
		if(gPCI.inversion == FALSE)	// at column 0, clear out the screen
			FillRgn(updateRgn, &whitePat);
		else if(gPCI.inversion == TRUE)
			FillRgn(updateRgn, &blackPat);
#else
		if(gPCI.inversion == FALSE)	// at column 0, clear out the screen
			FillRgn(updateRgn,&(qd.white));
		else if(gPCI.inversion == TRUE)
			FillRgn(updateRgn,&(qd.black));
#endif
		DisposeRgn(updateRgn);
	}
	
	GetPenState(&oldPenState);
	GetPenState(&myPenState);
	myPenState.pnSize.h = 1;
	myPenState.pnSize.v = 1;
	
	SetPenState(&myPenState);
	PenMode(patCopy);
		
#if TARGET_API_MAC_CARBON == 1
	PenPat(&blackPat);
#else
	PenPat(&(qd.black));
#endif
		
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
		
		logAmp = log10(polarSpectrum[ampIndex] * ampFactor2)/ampFactor1;
		if(gPCI.type == 1)
			myRGBColor = PhaseAmp2RGB(logAmp, phaseDifference);
		else
			myRGBColor = AmpPhase2RGB(logAmp, phaseDifference);
		RGBForeColor(&myRGBColor);
		MoveTo(spectDisp->spare,(halfLengthFFT - bandNumber));
		LineTo(spectDisp->spare+gPCI.linesPerFFT, (halfLengthFFT - bandNumber));
	}
	
	if(spectDisp->offScreen)
		UnlockPixels(thePixMap);
	SetPenState(&oldPenState);
	PenNormal();
	
	UpdateSpectDispWindow(spectDisp);
	
	spectDisp->spare += gPCI.linesPerFFT;
	if(spectDisp->spare >= gSpectPICTWidth)	// at last column, write picture to resource file
	{	
		error = WriteSpectFrame(mySI, spectDisp);
		if(gPCI.moving)
			spectDisp->spare = gSpectPICTWidth - (gSpectPICTWidth/30);
		else
			spectDisp->spare = 0;
	}
	if(error == 0)
	{
		// set up return values
		mySI->numBytes += mySI->frameSize * mySI->spectFrameIncr;
		numSamples = mySI->spectFrameIncr;
		return(numSamples);
	}
	else
		return(error);
}

void	WriteSpectPICT(SoundInfo *mySI, SoundDisp *spectDisp)
{
	PicHandle aPictHandle; 
	OpenCPicParams	aCPParam;
	short pictIndex;
	long pictCount;
	Str255	pictName;
	long	error;

#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(spectDisp->offScreen, &(aCPParam.srcRect));
#else
	aCPParam.srcRect = spectDisp->offScreen->portRect;
#endif
	aCPParam.hRes = aCPParam.vRes = 0x00480000;
	aCPParam.version = -2;
	
	// Make a new picture	
	aPictHandle = OpenCPicture(&aCPParam);	
	UpdateSpectDispWindow(spectDisp);
	ClosePicture();
	
	mySI->rFileNum = FSpOpenResFile(&(mySI->sfSpec),fsCurPerm);
	UseResFile(mySI->rFileNum);
	pictIndex = Unique1ID('PICT');
	pictCount = Count1Resources('PICT');
	pictCount++;
	NumToString(pictCount, pictName);
	AddResource((Handle)aPictHandle, 'PICT' , pictIndex, pictName);
	CloseResFile(mySI->rFileNum);
	error = ResError();
	ReleaseResource((Handle)aPictHandle);
	error = ResError();
	DisposeHandle((Handle)aPictHandle);
	error = MemError();
	aPictHandle = nil;
	UseResFile(gAppFileNum);
}

long	WriteSpectFrame(SoundInfo *mySI, SoundDisp *spectDisp)
{
	Boolean wasChanged, gotVideo;
	long	error, width, height, i;
	short	movieResID = 0;	// want first movie */	

	CGrafPtr				oldPort;
	GDHandle				oldGDevice;
	ImageDescriptionHandle	imageDescH;
	Media 					theMedia;
	Movie					theMovie;
	OSType					myMediaType;
	PixMapHandle			thePixMap;
	Rect					movieBounds;
	Size					dataSize, sizeLeft, sizeMore;
	Str255					movieName;
	TimeValue 				sampleDuration, sampleTime;
	Track 					theTrack;
		
	
	// Close the data fork
	// QT uses the data fork for media storage and
	// needs unrestricted access.
	sizeLeft = MaxMem(&sizeMore);
	FSClose(mySI->dFileNum);
	
	// Open the file and try to get a movie from it
	error = OpenMovieFile (&(mySI->sfSpec), &mySI->rFileNum, fsWrPerm);
	error = NewMovieFromFile (&theMovie, mySI->rFileNum, &movieResID, 
					movieName, newMovieActive, &wasChanged);
	if (error)
	{
		DrawErrorMessage("\pCan't open QuickTime movie");
		CloseMovieFile(mySI->rFileNum);
		return(-2L);
	}
	
	// Get the first Video track and corresponding media
	gotVideo = false;
	for (i = 1; ((i <= GetMovieTrackCount(theMovie)) && (!gotVideo)); i++)
	{
		theTrack = GetMovieIndTrack(theMovie, i);
		theMedia = GetTrackMedia(theTrack);
		GetMediaHandlerDescription(theMedia, &myMediaType, nil, nil);
		if (myMediaType == VideoMediaType)
			gotVideo = true;
	}
	// If no video track, create one (probably the first frame)
	if (gotVideo == false)
	{
		height = (mySI->spectFrameSize >> 1) + 1;
		width = gSpectPICTWidth;
		theTrack = NewMovieTrack(theMovie, FixRatio(width,1), FixRatio(height,1), 0);
		if(error = GetMoviesError())    
		{
			DrawErrorMessage("\pCan't create QuickTime movie track");
			CloseMovieFile(mySI->rFileNum);
			return(-2L);
		}
		theMedia = NewTrackMedia(theTrack, VideoMediaType, 300L, nil, nil);
	}
	// Now we can start editing
	// BeginMediaEdits accesses the data fork and will give
	// an error if the file is open or funky	
	if (error = BeginMediaEdits(theMedia))
	{
		DrawErrorMessage("\pCan't add frames to QuickTime movie");
		CloseMovieFile(mySI->rFileNum);
		return(-2L);
	}
	
	// Now we need to grab routines from the AddNewSample code
	// to draw the Picture in the QT frame and compress the
	// frame (as little as possible)
	
	// determine the frame duration (check the hint you may have been called with)
	// With a time scale equal to the sample rate
	GetMovieBox(theMovie, &movieBounds);
	GetGWorld(&oldPort, &oldGDevice);
	thePixMap = GetGWorldPixMap(spectDisp->offScreen);
	LockPixels(thePixMap);

	MoveHHi(gCompressedImage);
	HLock(gCompressedImage);
	imageDescH = (ImageDescriptionHandle)NewHandle(4);
#if TARGET_API_MAC_CARBON == 1
	error = CompressImage(thePixMap, &(movieBounds), codecNormalQuality, 
							'raw ', imageDescH, *gCompressedImage);
#else
	error = CompressImage(thePixMap, &(movieBounds), codecNormalQuality, 
							'raw ', imageDescH, StripAddress(*gCompressedImage));
#endif
//	sampleDuration = ((movieBounds.bottom - movieBounds.top) - 1) 
//						* (movieBounds.right - movieBounds.left) >> 2;
	sampleDuration = 10L;
	HLock((Handle)imageDescH);
	dataSize = (**imageDescH).dataSize;
	HUnlock((Handle)imageDescH);
	error = AddMediaSample(theMedia, gCompressedImage, 0, dataSize, sampleDuration,
					(SampleDescriptionHandle)imageDescH, 1, 0, &sampleTime);

	// unlock handles
	HUnlock(gCompressedImage);
	UnlockPixels(thePixMap);

	error = EndMediaEdits(theMedia);
	error = InsertMediaIntoTrack(theTrack, -1, sampleTime, sampleDuration, 0x00010000);
	error = UpdateMovieResource (theMovie, mySI->rFileNum, movieResID, nil);
	error = CloseMovieFile(mySI->rFileNum);
	
	// get rid of allocated temporary memory
	DisposeHandle((Handle)imageDescH);
	DisposeMovie(theMovie);
	// Re-open the data fork
	error = FSpOpenDF(&(mySI->sfSpec), fsCurPerm, &mySI->dFileNum);
	SetGWorld(oldPort, oldGDevice);
	return(error);
}

RGBColor	AmpPhase2RGB(double logAmp, double phaseDifference)
{
	float tmpBlue, tmpGreen, tmpRed;
	unsigned long blue, red, green;
	RGBColor myRGBColor;
	if(logAmp < 0.0)
	{
		if(gPCI.inversion == TRUE)
		{
			myRGBColor.red = (unsigned short)(0UL);
			myRGBColor.green = (unsigned short)(0UL);
			myRGBColor.blue = (unsigned short)(0UL);
		}
		else
		{
			myRGBColor.red = (unsigned short)(65535UL);
			myRGBColor.green = (unsigned short)(65535UL);
			myRGBColor.blue = (unsigned short)(65535UL);
		}
		return(myRGBColor);
	}	
	// phase 0 (red) to -�/4 (blue)
	if(phaseDifference <= 0.0 && phaseDifference > -(QTRPI))
	{
		phaseDifference = phaseDifference * FOUROVERPI;
		tmpRed = 65535.0 * logAmp * (1.0 + phaseDifference);
		tmpBlue = 65535.0 * logAmp * -phaseDifference;
		if(tmpBlue > tmpRed)	// blue is brightest
		{
			tmpGreen = 65535.0 - tmpBlue;
			tmpRed = tmpGreen + tmpRed;
			tmpBlue = 65535.0;
		}
		else					// red is brightest
		{
			tmpGreen = 65535.0 - tmpRed;
			tmpBlue = tmpGreen + tmpBlue;
			tmpRed = 65535.0;
		}
	}
	// phase 0 (red) to �/4 (green)
	else if(phaseDifference > 0.0 && phaseDifference <= QTRPI)
	{
		phaseDifference = phaseDifference * FOUROVERPI;
		tmpRed = 65535.0 * logAmp * (1.0 - phaseDifference);
		tmpGreen = 65535.0 * logAmp * phaseDifference;
		if(tmpRed > tmpGreen)
		{
			tmpBlue = 65535.0 - tmpRed;
			tmpGreen = tmpGreen + tmpBlue;
			tmpRed = 65535.0;
		}
		else
		{
			tmpBlue = 65535.0 - tmpGreen;
			tmpRed = tmpRed + tmpBlue;
			tmpGreen = 65535.0;
		}
	}
	// phase -�/4 (blue) to �/4 (green)
	else
	{
		if(phaseDifference < 0.0)
			phaseDifference += twoPi;
		phaseDifference = (phaseDifference - QTRPI) * TWOOVERTHREEPI;
		tmpBlue = 65535.0 * logAmp * phaseDifference;
		tmpGreen = 65535.0 * logAmp * (1.0 - phaseDifference);
		if(tmpBlue > tmpGreen)	// blue is brightest
		{
			tmpRed = 65535.0 - tmpBlue;
			tmpGreen = tmpGreen + tmpRed;
			tmpBlue = 65535.0;
		}
		else					// green is brightest
		{
			tmpRed = 65535.0 - tmpGreen;
			tmpBlue = tmpRed + tmpBlue;
			tmpGreen = 65535.0;
		}
	}
	if(gPCI.centerColor == RED_MENU)
	{
		red = (unsigned long)(tmpRed);
		green = (unsigned long)(tmpGreen);
		blue = (unsigned long)(tmpBlue);
	}
	else if(gPCI.centerColor == GREEN_MENU)
	{
		green = (unsigned long)(tmpRed);
		blue = (unsigned long)(tmpGreen);
		red = (unsigned long)(tmpBlue);
	}
	else
	{
		blue = (unsigned long)(tmpRed);
		red = (unsigned long)(tmpGreen);
		green = (unsigned long)(tmpBlue);
	}
	if(gPCI.inversion == TRUE)
	{
		myRGBColor.red = (unsigned short)(65535 - red);
		myRGBColor.green = (unsigned short)(65535 - green);
		myRGBColor.blue = (unsigned short)(65535 - blue);
	}
	else
	{
		myRGBColor.red = (unsigned short)(red);
		myRGBColor.green = (unsigned short)(green);
		myRGBColor.blue = (unsigned short)(blue);
	}
	return(myRGBColor);
}

RGBColor	PhaseAmp2RGB(double logAmp, double phaseDifference)
{
	float tmpBlue, tmpGreen, tmpRed;
	float phaseShaped;
	unsigned long blue, red, green;
	RGBColor myRGBColor;
	// phase �/4 to -�/4
	if(phaseDifference <= QTRPI && phaseDifference > -(QTRPI))
		// scale phaseDifference to run from .25 to .75
		phaseShaped = (phaseDifference * ONEOVERPI) + .5;
	// phase -� to -�/4 (0 to .25)
	else if(phaseDifference < -QTRPI)
		phaseShaped = (phaseDifference * ONEOVERTHREEPI) + 0.333333333333;
	// phase -�/4 (blue) to �/4 (green)
	else if(phaseDifference > QTRPI)
		phaseShaped = (phaseDifference * ONEOVERTHREEPI) + 0.666666666666;
	phaseShaped *= 0.66666666666;
	if(logAmp > 0.66666666666)
	{
		logAmp -= 0.66666666666;
		tmpRed = logAmp + phaseShaped;
		tmpGreen = (0.333333333333 - logAmp) + phaseShaped;
		tmpBlue = phaseShaped;  
	}
	else if(logAmp > 0.33333333333)
	{
		logAmp -= 0.33333333333;
		tmpGreen = logAmp + phaseShaped;
		tmpBlue = (0.333333333333 - logAmp) + phaseShaped;
		tmpRed = phaseShaped;  
	}
	else
	{
		tmpBlue = logAmp + phaseShaped;
		tmpRed = (0.333333333333 - logAmp) + phaseShaped;
		tmpGreen = phaseShaped;  
	}
	tmpRed *= 65535.0;
	tmpGreen *= 65535.0;
	tmpBlue *= 65535.0;
	if(gPCI.centerColor == RED_MENU)
	{
		red = (unsigned long)(tmpRed);
		green = (unsigned long)(tmpGreen);
		blue = (unsigned long)(tmpBlue);
	}
	else if(gPCI.centerColor == GREEN_MENU)
	{
		green = (unsigned long)(tmpRed);
		blue = (unsigned long)(tmpGreen);
		red = (unsigned long)(tmpBlue);
	}
	else
	{
		blue = (unsigned long)(tmpRed);
		red = (unsigned long)(tmpGreen);
		green = (unsigned long)(tmpBlue);
	}
	if(gPCI.inversion == TRUE)
	{
		myRGBColor.red = (unsigned short)(65535 - red);
		myRGBColor.green = (unsigned short)(65535 - green);
		myRGBColor.blue = (unsigned short)(65535 - blue);
	}
	else
	{
		myRGBColor.red = (unsigned short)(red);
		myRGBColor.green = (unsigned short)(green);
		myRGBColor.blue = (unsigned short)(blue);
	}
	return(myRGBColor);
}

short	InitSynthProcess(void)
{
	long	blockSize, error;
	
	// Set all derived constants
	gScaleDivisor = 1.0;
	gAnaPI.halfPoints = gAnaPI.points >>1;
	gAnaPI.interpolation = gAnaPI.decimation;
	// windowSize/8 seems to be the largest value possible for time stretching
	// without distortion
		
	outSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
	if(gPreferences.defaultType == 0)
	{
		outSIPtr->sfType = NEXT;
		outSIPtr->packMode = SF_FORMAT_32_FLOAT;
	}
	else
	{
		outSIPtr->sfType = gPreferences.defaultType;
		outSIPtr->packMode = gPreferences.defaultFormat;
	}
	NameFile(inSIPtr->sfSpec.name, "\pPCS", outSIPtr->sfSpec.name);
	outSIPtr->sRate = inSIPtr->sRate;
	outSIPtr->nChans = inSIPtr->nChans;
	outSIPtr->numBytes = 0;
	if(CreateSoundFile(&outSIPtr, SOUND_CUST_DIALOG) == -1)
	{
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		DisposePtr((Ptr)outSIPtr);
		outSIPtr = nil;
		return(-1);
	}
	WriteHeader(outSIPtr);
	UpdateInfoWindow(outSIPtr);
	SetOutputScale(outSIPtr->packMode);
		
    blockSize = gAnaPI.decimation;
    
    if(gAnaPI.analysisType == PICT_ANALYSIS)
	{
		error = CreateSpectDispWindow(&gSynthPICTDisp, 40, inSIPtr->sfSpec.name);
		if(error != 0)
		{
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			DisposePtr((Ptr)outSIPtr);
			outSIPtr = nil;
			return(-1);
		}
	}

/*
 * allocate memory
 */
	analysisWindow = synthesisWindow = inputL = nil;
	spectrum = polarSpectrum = displaySpectrum = lastPhaseInL = nil;
 	inputR = lastPhaseInR = outputL = outputR = nil;

	analysisWindow = (float *)NewPtr(gAnaPI.windowSize*sizeof(float));
	synthesisWindow = (float *)NewPtr(gAnaPI.windowSize*sizeof(float));
	outputL = (float *)NewPtr(gAnaPI.windowSize*sizeof(float));
	spectrum = (float *)NewPtr(gAnaPI.points*sizeof(float));
	polarSpectrum = (float *)NewPtr((gAnaPI.points+2)*sizeof(float));		/* analysis polarSpectrum */
	displaySpectrum = (float *)NewPtr((gAnaPI.halfPoints+1)*sizeof(float));	/* For display of amplitude */
	lastPhaseInL = (float *)NewPtr((gAnaPI.halfPoints+1) * sizeof(float));
	
	if(inSIPtr->nChans == STEREO)
	{
		outputR = (float *)NewPtr(gAnaPI.windowSize*sizeof(float));	/* inputR buffer */
		lastPhaseInR = (float *)NewPtr((gAnaPI.halfPoints+1) * sizeof(float));
		if(lastPhaseInR == 0)
		{
			DrawErrorMessage("\pNot enough memory for processing");
			DeAllocAnalMem();
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			DisposePtr((Ptr)outSIPtr);
			outSIPtr = nil;
			return(-1);
		}
	}
	
	if(displaySpectrum == 0 || lastPhaseInL == 0)
	{
		DrawErrorMessage("\pNot enough memory for processing");
		DeAllocAnalMem();
    	if(gAnaPI.analysisType == PICT_ANALYSIS)
			DisposeWindWorld(&gSynthPICTDisp);
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		DisposePtr((Ptr)outSIPtr);
		outSIPtr = nil;
		return(-1);
	}
	ZeroFloatTable(lastPhaseInL, gAnaPI.halfPoints+1);
	ZeroFloatTable(spectrum, gAnaPI.points);
	ZeroFloatTable(polarSpectrum, (gAnaPI.points+2));
	ZeroFloatTable(outputL, gAnaPI.windowSize);
	if(inSIPtr->nChans == STEREO)
	{
		ZeroFloatTable(lastPhaseInR, gAnaPI.halfPoints+1);
		ZeroFloatTable(outputR, gAnaPI.windowSize);
	}

	UpdateProcessWindow("\p", "\p", "\pspectral resynthesis", 0.0);
/*
 * create and scale windows
 */
 	GetWindow(analysisWindow, gAnaPI.windowSize, gAnaPI.windowType);
 	GetWindow(synthesisWindow, gAnaPI.windowSize, gAnaPI.windowType);
    ScaleWindows(analysisWindow, synthesisWindow, gAnaPI);
/*
 * initialize input and output time values (in samples)
 */
    gOutPointer = gInPointer = -gAnaPI.windowSize;
	gInPosition	= 0;
	
	gNumberBlocks = 0;	
	gProcessEnabled = SYNTH_PROCESS;
	gProcessDisabled = NO_PROCESS;
	MenuUpdate();
	return(TRUE);
}

// main loop--perform phase vocoder resynthesis
short
SynthBlock(void)
{
    short	curMark;
	long	numSampsRead;
	float 	length;
	Str255	errStr;
	static long validSamples;
	static long	sOutPos;
	double seconds;
	

	if(gStopProcess == TRUE)
	{
		if(gAnaPI.analysisType == PICT_ANALYSIS)
		{
			DisposeWindWorld(&gSynthPICTDisp);
		}
		DeAllocAnalMem();
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	if(gNumberBlocks == 0)
	{
		sOutPos = outSIPtr->dataStart;
		validSamples = -1L;
	}
	SetFPos(outSIPtr->dFileNum, fsFromStart, sOutPos);

	// increment times

	gOutPointer += gAnaPI.interpolation;
		
	// analysis input
	if(gAnaPI.analysisType == CSOUND_ANALYSIS)
		numSampsRead = ReadCSAData(inSIPtr, polarSpectrum, lastPhaseInL, gAnaPI.halfPoints);
	else if(gAnaPI.analysisType == SOUNDHACK_ANALYSIS)
		numSampsRead = ReadSHAData(inSIPtr, polarSpectrum, LEFT);
	else if(gAnaPI.analysisType == SDIF_ANALYSIS)
	{
		numSampsRead = ReadSDIFData(inSIPtr, spectrum, polarSpectrum, &seconds, LEFT);
		CartToPolar(spectrum, polarSpectrum, gAnaPI.halfPoints);
	}
	else if(gAnaPI.analysisType == PICT_ANALYSIS)
		numSampsRead = ReadSpectLine(inSIPtr, polarSpectrum, lastPhaseInL, gAnaPI.halfPoints);
	
	// show amount read
	if(numSampsRead > 0)
	{
		length = (numSampsRead + (gInPosition/(inSIPtr->nChans*inSIPtr->frameSize)))/inSIPtr->sRate;
		HMSTimeString(length, errStr);
		length = (numSampsRead*inSIPtr->nChans*inSIPtr->frameSize + gInPosition)/inSIPtr->numBytes;
		UpdateProcessWindow(errStr, "\p", "\pspectral resynthesis", length);
	}
	
	// display spectra
	GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySpectrum(polarSpectrum, displaySpectrum, gAnaPI.halfPoints, outSIPtr, LEFT, 1.0, "\pOutput Channel 1");
	GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
	if(curMark != noMark)
		DisplaySonogram(polarSpectrum, displaySpectrum, gAnaPI.halfPoints, outSIPtr, LEFT, 1.0, "\pOutput Channel 1");

	// resynthesize
	PolarToCart(polarSpectrum, spectrum, gAnaPI.halfPoints);
	RealFFT(spectrum, gAnaPI.halfPoints, FREQ2TIME);
	OverlapAdd(spectrum, synthesisWindow, outputL, gOutPointer, gAnaPI.points, gAnaPI.windowSize);

    if(inSIPtr->nChans == STEREO)
    {
		if(gAnaPI.analysisType == SOUNDHACK_ANALYSIS)
			numSampsRead = ReadSHAData(inSIPtr, polarSpectrum, RIGHT);
		else if(gAnaPI.analysisType == SDIF_ANALYSIS)
		{
			numSampsRead = ReadSDIFData(inSIPtr, spectrum, polarSpectrum, &seconds, RIGHT);
			CartToPolar(spectrum, polarSpectrum, gAnaPI.halfPoints);
		}
	
		// display spectra
		GetItemMark(gSpectDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySpectrum(polarSpectrum, displaySpectrum, gAnaPI.halfPoints, outSIPtr, RIGHT, 1.0, "\pOutput Channel 2");
		GetItemMark(gSonoDispMenu, DISP_OUTPUT_ITEM, &curMark);
		if(curMark != noMark)
			DisplaySonogram(polarSpectrum, displaySpectrum, gAnaPI.halfPoints, outSIPtr, RIGHT, 1.0, "\pOutput Channel 2");
		
		// resynthesize
		PolarToCart(polarSpectrum, spectrum, gAnaPI.halfPoints);
		RealFFT(spectrum, gAnaPI.halfPoints, FREQ2TIME);
		OverlapAdd(spectrum, synthesisWindow, outputR, gOutPointer, gAnaPI.points, gAnaPI.windowSize);
	}
	
	// data output
	ShiftOut(outSIPtr, outputL, outputR, gOutPointer+gAnaPI.interpolation, gAnaPI.interpolation, gAnaPI.windowSize);
	length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));

	HMSTimeString(length, errStr);
	UpdateProcessWindow("\p", errStr, "\pspectral resynthesis", -1.0);
	
	if(numSampsRead == -2L)
	{
		DeAllocAnalMem();
		if(gAnaPI.analysisType == PICT_ANALYSIS)
		{
			DisposeWindWorld(&gSynthPICTDisp);
		}
		FinishProcess();
		gNumberBlocks = 0;
		return(-1);
	}
	
	gNumberBlocks++;
	GetFPos(outSIPtr->dFileNum, &sOutPos);
 	gInPosition += inSIPtr->nChans * gAnaPI.decimation * inSIPtr->frameSize;
 	return(TRUE);
}

long	ReadSpectLine(SoundInfo *mySI, float polarSpectrum[], float lastPhaseIn[], long halfLengthFFT)
{
	long numSamples, bandNumber, ampIndex, phaseIndex, error;
	double phaseDifference, logAmp;
	static float factor, fundamental, averageFactor;
	static double ampFactor1, ampFactor2;
	PixMapHandle	thePixMap;
	RGBColor	pixelRGB;
	long red, green, blue, line;
	
	// first, initialize the static variables
    if(gNumberBlocks == 0)
    {
		fundamental = mySI->sRate/(halfLengthFFT<<1);
        factor = mySI->sRate/(gAnaPI.decimation*twoPi);
        ampFactor1 = gPCI.amplitudeRange * 0.05;
        ampFactor2 = pow(10.0, ampFactor1);
        averageFactor = 1.0 / gPCI.linesPerFFT;
    }
    // now check to see if we are at the right edge a pict,
    // if so, read in a new PICT and display, if no more PICTs
    // drop out gracefully
    if(gSynthPICTDisp.spare >= gSpectPICTWidth || gNumberBlocks == 0)
    {
		error = ReadSpectPICT(mySI, &gSynthPICTDisp);
		if(error != 0)
			return(error);
		if(gPCI.moving)
			gSynthPICTDisp.spare = gSpectPICTWidth - (gSpectPICTWidth/30);
	}
	if(gSynthPICTDisp.offScreen == NIL_POINTER)
		return(-1);
	thePixMap = GetGWorldPixMap(gSynthPICTDisp.offScreen);
	LockPixels(thePixMap);
	SetGWorld(gSynthPICTDisp.offScreen, NULL );	// Set port to the offscreen

    for(bandNumber = 0;  bandNumber<=halfLengthFFT; bandNumber++)
    {
    	ampIndex = bandNumber<<1;
		phaseIndex = ampIndex + 1;
		red = green = blue = 0;
		for(line = 0; line < gPCI.linesPerFFT; line++)
		{
			GetCPixel(gSynthPICTDisp.spare+line, (halfLengthFFT - bandNumber), &pixelRGB);
			red += pixelRGB.red;
			green += pixelRGB.green;
			blue += pixelRGB.blue;
		}
		pixelRGB.red = red * averageFactor;
		pixelRGB.green = green * averageFactor;
		pixelRGB.blue = blue * averageFactor;
		if(gPCI.type == 1)
			RGB2PhaseAmp(pixelRGB, &logAmp, &phaseDifference);
		else
			RGB2AmpPhase(pixelRGB, &logAmp, &phaseDifference);
		logAmp = powf(10.0, (logAmp*ampFactor1))/ampFactor2;
		polarSpectrum[ampIndex] = (float)logAmp;
		if (polarSpectrum[ampIndex] <= 0.0)
	    	phaseDifference = 0.0;
		else
		{
	    	polarSpectrum[phaseIndex] = lastPhaseIn[bandNumber] + phaseDifference;
	    	while(polarSpectrum[phaseIndex] > Pi)
	    		polarSpectrum[phaseIndex] -= twoPi;
	    	while(polarSpectrum[phaseIndex] < -Pi)
	    		polarSpectrum[phaseIndex] += twoPi;
	    	lastPhaseIn[bandNumber] = polarSpectrum[phaseIndex];
		}
	}
	UnlockPixels(thePixMap);
	
	gSynthPICTDisp.spare += gPCI.linesPerFFT;	
	numSamples = mySI->spectFrameIncr;
	if(numSamples == 0)
		return(-2L);
	else
		return(numSamples);
}

long	ReadSpectPICT(SoundInfo *mySI, SoundDisp *spectDisp)
{
	Boolean		gotVideo;
	long		readSize, endOfFile, error, i;
	static long frameNumber;
	
	FInfo		fndrInfo;
	Media		theMedia;
	Movie		theMovie;
	OSType		myMediaType;
	PixMapHandle	thePixMap;
	Rect		theRect;
	RGBColor	whiteRGB, blackRGB;
	Str255		frameName;
	Track		theTrack;
	static	TimeValue	time;
	
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
    if(gNumberBlocks == 0)
    {
    	frameNumber = 1;
    	time=0;
    }
    
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;
	spectDisp->spare = 0;
	// a) find the next PICT to read
	FSpGetFInfo(&(mySI->sfSpec),&fndrInfo);	// read the file type
	if(fndrInfo.fdType == 'SCRN' || fndrInfo.fdType == 'CSCR')
	{
		mySI->rFileNum = FSpOpenResFile(&(mySI->sfSpec),fsCurPerm);
		if(mySI->rFileNum == -1)
			return(-2L);
		UseResFile(mySI->rFileNum);
		NumToString(frameNumber, frameName);
		spectDisp->thePict = (PicHandle)Get1NamedResource('PICT', frameName);
		// if there is no titled PICT look for the numbered PICT
		if(spectDisp->thePict == nil)
			spectDisp->thePict = (PicHandle)Get1IndResource('PICT', frameNumber);
		// if there are no titled or numbered PICTs, time to get out...
		if(spectDisp->thePict == nil)
			return(-2L);
	}
	else if(fndrInfo.fdType == 'MooV')
	{
		error = OpenMovieFile (&(mySI->sfSpec), &mySI->rFileNum, fsCurPerm);
		if (error == noErr) 
		{
			short	movieResID = 0;	// want first movie */	
			Str255	movieName;
			Boolean wasChanged;
		
			error = NewMovieFromFile (&theMovie, mySI->rFileNum, &movieResID,
					movieName, newMovieActive, &wasChanged);
		
		}
		/* create a track and PICT media */
		gotVideo = false;
		for (i = 1; ((i <= GetMovieTrackCount(theMovie)) && (!gotVideo)); i++)
		{
			theTrack = GetMovieIndTrack(theMovie, i);
			theMedia = GetTrackMedia(theTrack);
			GetMediaHandlerDescription(theMedia, &myMediaType, nil, nil);
			if (myMediaType == VideoMediaType)
				gotVideo = true;
		}

		if(error = GetMoviesError()) 
		{
			DrawErrorMessage("\pCan't read video track in movie");
			CloseMovieFile(mySI->rFileNum);
			return(-2L);
		}
		
		if(frameNumber != 1)
			GetTrackNextInterestingTime(theTrack, nextTimeMediaSample, time, 1, &time, nil);
		if(time == -1)
		{
			CloseMovieFile(mySI->rFileNum);
			return(-2L);
		}
		else	
			spectDisp->thePict = GetTrackPict(theTrack, time);
		error = GetMoviesError();
		if(error == -108)
		{
			DrawErrorMessage("\pNot enough memory to read QuickTime movie frame");
			CloseMovieFile(mySI->rFileNum);
			return(-2L);
		}
	}
	else
	{
		if(frameNumber != 1)
			return(-2L);		// only one PICT in a PICT file
		error = FSpOpenDF(&(mySI->sfSpec),fsCurPerm, &mySI->dFileNum);
		spectDisp->thePict = (PicHandle)NewHandle(sizeof(Picture));
		GetEOF(mySI->dFileNum, &endOfFile);
		readSize = endOfFile - 512;
		SetHandleSize((Handle)spectDisp->thePict, readSize);
		HLock((Handle)spectDisp->thePict);
		SetFPos(mySI->dFileNum,fsFromStart,512);
		// U.B. - ??
        error = FSRead(mySI->dFileNum, &readSize, *(spectDisp->thePict));
		HUnlock((Handle)spectDisp->thePict);
	}
	// c) draw the picture off screen	
	thePixMap = GetGWorldPixMap(gSynthPICTDisp.offScreen);
	LockPixels(thePixMap);
	SetGWorld(gSynthPICTDisp.offScreen, NULL );	// Set port to the offscreen
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(gSynthPICTDisp.offScreen, &theRect);
#else
	theRect = gSynthPICTDisp.offScreen->portRect;
#endif

	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
#if TARGET_API_MAC_CARBON == 1
	FillRect(&theRect,&blackPat);
#else
	FillRect(&theRect,&(qd.black));
#endif
    DrawPicture(gSynthPICTDisp.thePict, &theRect);
	if(fndrInfo.fdType == 'SCRN' || fndrInfo.fdType == 'CSCR')
	{
		CloseResFile(mySI->rFileNum);
		error = ResError();
		UseResFile(gAppFileNum);
		ReleaseResource((Handle)gSynthPICTDisp.thePict);
		error = ResError();
		DisposeHandle((Handle)gSynthPICTDisp.thePict);
		error = MemError();
	}
	else if(fndrInfo.fdType == 'MooV')
	{
		DisposeMovieTrack (theTrack);
		CloseMovieFile(mySI->rFileNum);
		DisposeHandle((Handle)gSynthPICTDisp.thePict);
	}
	else
	{
		FSClose(mySI->dFileNum);
		DisposeHandle((Handle)gSynthPICTDisp.thePict);
	}
	gSynthPICTDisp.thePict = nil;
	UnlockPixels(thePixMap);
	// d) now put it on-screen
	UpdateSpectDispWindow(spectDisp);
	frameNumber++;
	return(0);
}

void	RGB2AmpPhase(RGBColor pixel, double *logAmp, double *phaseDifference)
{
	float red, blue, green;
	
	if(gPCI.inversion == TRUE)
	{
		pixel.red = 65535UL - pixel.red;
		pixel.green = 65535UL - pixel.green;
		pixel.blue = 65535UL - pixel.blue;
	}
	if(pixel.blue == 0 && pixel.red == 0 && pixel.green == 0)
	{
		*phaseDifference = *logAmp = 0.0;
		return;
	}
	if(gPCI.centerColor == RED_MENU)
	{
		red = (float)pixel.red;
		blue = (float)pixel.blue;
		green = (float)pixel.green;
	}
	else if(gPCI.centerColor == GREEN_MENU)
	{
		blue = (float)pixel.red;
		green = (float)pixel.blue;
		red = (float)pixel.green;
	}
	else
	{
		green = (float)pixel.red;
		red = (float)pixel.blue;
		blue = (float)pixel.green;
	}
	if(blue < green) 
		if(red <= blue) // red is the smallest, between green and blue
		{
			green = green - red;
			blue = blue - red;
			*logAmp = (green + blue)/65535.0;
			if(*logAmp == 0.0)
				*phaseDifference = 0.0;
			else
			{
				*phaseDifference = (((blue - green)/(131070.0 * *logAmp) + 0.5) * THREEPIOVERTWO) + QTRPI;
				if(*phaseDifference > Pi)
					*phaseDifference -= twoPi;
			}
		}
		else // blue is the smallest, between red and green
		{
			green = green - blue;
			red = red - blue;
			*logAmp = (green + red)/65535.0;
			if(*logAmp == 0.0)
				*phaseDifference = 0.0;
			else
				*phaseDifference = (0.5 - (red - green)/(131070.0 * *logAmp)) * QTRPI;
		}
	else
		if(red <= green) // red is the smallest, between green and blue		
		{
			green = green - red;
			blue = blue - red;
			*logAmp = (green + blue)/65535.0;
			if(*logAmp == 0.0)
				*phaseDifference = 0.0;
			else
			{
				*phaseDifference = (((blue - green)/(131070.0 * *logAmp) + 0.5) * THREEPIOVERTWO) + QTRPI;
				if(*phaseDifference > Pi)
					*phaseDifference -= twoPi;
			}
		}
		else // green is the smallest, between blue and red
		{
			red = red - green;
			blue = blue - green;
			*logAmp = (blue + red)/65535.0;
			if(*logAmp == 0.0)
				*phaseDifference = 0.0;
			else
				*phaseDifference = ((red - blue)/(131070.0 * *logAmp) - 0.5) * QTRPI;
		}
}

void	RGB2PhaseAmp(RGBColor pixel, double *logAmp, double *phaseDifference)
{
	float red, blue, green, phaseShaped;
	
	if(gPCI.inversion == TRUE)
	{
		pixel.red = 65535UL - pixel.red;
		pixel.green = 65535UL - pixel.green;
		pixel.blue = 65535UL - pixel.blue;
	}
	if(pixel.blue == 0 && pixel.red == 0 && pixel.green == 0)
	{
		*phaseDifference = *logAmp = 0.0;
		return;
	}
	if(gPCI.centerColor == RED_MENU)
	{
		red = (float)pixel.red;
		blue = (float)pixel.blue;
		green = (float)pixel.green;
	}
	else if(gPCI.centerColor == GREEN_MENU)
	{
		blue = (float)pixel.red;
		green = (float)pixel.blue;
		red = (float)pixel.green;
	}
	else
	{
		green = (float)pixel.red;
		red = (float)pixel.blue;
		blue = (float)pixel.green;
	}
	if(blue < green) 
		if(red <= blue) // red is the smallest, between green and blue
		{
			phaseShaped = (red * 1.5)/65535.0;
			*logAmp = (green - red)/65535.0 + 0.333333333333;
		}
		else // blue is the smallest, between red and green
		{
			phaseShaped = (blue * 1.5)/65535.0;
			*logAmp = (red - blue)/65535.0 + 0.66666666666;
		}
	else
		if(red <= green) // red is the smallest, between green and blue		
		{
			phaseShaped = (red * 1.5)/65535.0;
			*logAmp = (green - red)/65535.0 + 0.333333333333;
		}
		else // green is the smallest, between blue and red
		{
			phaseShaped = (green * 1.5)/65535.0;
			*logAmp = (blue - green)/65535.0;
		}
	if(phaseShaped <= 0.75 && phaseShaped >= 0.25)
		*phaseDifference = (phaseShaped * Pi) - (Pi * 0.25);
	else if(phaseShaped > 0.75)
		*phaseDifference = (phaseShaped * Pi * 3.0) - (2.0 * Pi);
	else if(phaseShaped < 0.25)
		*phaseDifference = (phaseShaped * Pi * 3.0) - Pi;
	if(*logAmp == 0.0)
		*phaseDifference = 0.0;
}

void
DeAllocAnalMem(void)
{
	RemovePtr((Ptr)analysisWindow);
	RemovePtr((Ptr)synthesisWindow);
	if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT || inSIPtr->sfType == QKTMV)
		RemovePtr((Ptr)outputL);
	else
		RemovePtr((Ptr)inputL);
	RemovePtr((Ptr)spectrum);
	RemovePtr((Ptr)polarSpectrum);
	RemovePtr((Ptr)displaySpectrum);
	RemovePtr((Ptr)lastPhaseInL);
	if(inSIPtr->sfType != CS_PVOC && inSIPtr->sfType != PICT && inSIPtr->sfType != QKTMV)
		DisposeHandle(gCompressedImage);
	gCompressedImage = nil;
	if(inSIPtr->nChans == STEREO)
	{
		if(inSIPtr->sfType == CS_PVOC || inSIPtr->sfType == PICT || inSIPtr->sfType == QKTMV)
			RemovePtr((Ptr)outputR);
		else
			RemovePtr((Ptr)inputR);
		RemovePtr((Ptr)lastPhaseInR);
	}

	analysisWindow = synthesisWindow = inputL = nil;
	spectrum = polarSpectrum = displaySpectrum = lastPhaseInL = nil;
 	outputR = outputL = inputR = lastPhaseInR = nil;
}
