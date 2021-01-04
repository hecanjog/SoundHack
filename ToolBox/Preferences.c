#include "SoundFile.h"
#include "Convolve.h"
#include "Analysis.h"
#include "PhaseVocoder.h"
#include "Dynamics.h"
#include "Extract.h"
#include "Mutate.h"
#include "Gain.h"
#include "SampleRateConvert.h"
#include "Preferences.h"
#include "SoundHack.h"
#include "Dialog.h"
#include "Misc.h"
#include "CarbonGlue.h"

// Save Preferences and Settings
extern ConvolveInfo		gCI;	// Convolve, Binaural and Ring
extern PvocInfo 		gAnaPI;	// Analysis - Resynthesis
extern DynamicsInfo		gDI;	// Dynamics
extern PvocInfo			gDynPI;	// Dynamics
extern ExtractInfo		gEI;	// Extraction
extern PvocInfo			gExtPI;	// Extraction
extern MutateInfo		gMI;	// Mutation
extern PvocInfo			gMutPI;	// Mutation
extern GainInfo			gGI;	// Gain
extern PvocInfo			gPI;	// Phase Vocoder
extern SRCInfo			gSRCI;	// Sample Rate Conversion
extern PictCoderInfo	gPCI; // PICT Coder

extern DialogPtr		gPrefDialog;
extern MenuHandle		gFormatMenu;
extern short			gAppFileNum;
extern short			gProcessDisabled;

extern struct
{
	Boolean	soundPlay;
	Boolean	soundRecord;
	Boolean	soundCompress;
	Boolean	movie;
}	gGestalt;

struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

void
HandlePrefDialog(void)
{
	short	itemType, n;
	Rect	itemRect;
	Handle	itemHandle;
	
	gProcessDisabled = UTIL_PROCESS;
	if(gPreferences.defaultType == 0)
	{
		GetDialogItem(gPrefDialog, PR_SFTYPE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle, 0);
		HideDialogItem(gPrefDialog, PR_SFTYPE_CNTL);
		HideDialogItem(gPrefDialog, PR_SFFORMAT_CNTL);
		gPreferences.defaultFormat = 0;
	}
	else
	{
		GetDialogItem(gPrefDialog, PR_SFTYPE_BOX, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle, 1);
		ShowDialogItem(gPrefDialog, PR_SFTYPE_CNTL);
		ShowDialogItem(gPrefDialog, PR_SFFORMAT_CNTL);
		
		for(n = 1; n < 17; n++)
			DisableItem(gFormatMenu,n);
		
		switch(gPreferences.defaultType)
		{
			case AIFF:
			case QKTMA:
				EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
				break;
			case AIFC:
				EnableItem(gFormatMenu,SF_FORMAT_4_ADDVI);
				EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
				EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
				EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
				if(gGestalt.soundCompress == TRUE)
				{
					EnableItem(gFormatMenu,SF_FORMAT_4_ADIMA);
					EnableItem(gFormatMenu,SF_FORMAT_MACE3);
					EnableItem(gFormatMenu,SF_FORMAT_MACE6);
				}
				break;
			case AUDMED:
			case MMIX:
			case SDII:
				EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
				break;
			case DSPD:
				EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
				break;
			case BICSF:
				EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
				break;
			case NEXT:
			case SUNAU:
				EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
				EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
				EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
				break;
			case WAVE:
				EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
				EnableItem(gFormatMenu,SF_FORMAT_16_SWAP);
				EnableItem(gFormatMenu,SF_FORMAT_24_SWAP);
				EnableItem(gFormatMenu,SF_FORMAT_32_SWAP);
				break;
			case MAC:
				EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
				break;
			case RAW:
				EnableItem(gFormatMenu,SF_FORMAT_4_ADDVI);
				EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
				EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
				EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
				EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_16_SWAP);
				EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
				EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
				if(gGestalt.soundCompress == TRUE)
				{
					EnableItem(gFormatMenu,SF_FORMAT_4_ADIMA);
					EnableItem(gFormatMenu,SF_FORMAT_MACE3);
					EnableItem(gFormatMenu,SF_FORMAT_MACE6);
				}
			case TEXT:
				EnableItem(gFormatMenu,SF_FORMAT_TEXT);
				break;
			default:
				break;
		}
	}
	GetDialogItem(gPrefDialog, PR_SFTYPE_CNTL, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gPreferences.defaultType);
	GetDialogItem(gPrefDialog, PR_SFFORMAT_CNTL, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gPreferences.defaultFormat);
	GetDialogItem(gPrefDialog, PR_OPENACT_CNTL, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gPreferences.openPlay);
	GetDialogItem(gPrefDialog, PR_PROCPLAY_CNTL, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, gPreferences.procPlay);
	GetDialogItem(gPrefDialog, PR_EDIT_TEXT, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle,gPreferences.editorName);
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gPrefDialog));
	SelectWindow(GetDialogWindow(gPrefDialog));
#else
	ShowWindow(gPrefDialog);
	SelectWindow(gPrefDialog);
#endif
}

void
HandlePrefDialogEvent(short itemHit)
{
	short	itemType, n;
	Rect	itemRect;
	Handle	itemHandle;
	FInfo	fndrInfo;
	SHSFTypeList	typeList = {'APPL'};
	SHStandardFileReply reply;
	short	typeBox;

	switch(itemHit)
	{
		case PR_SFTYPE_BOX:
			GetDialogItem(gPrefDialog, PR_SFTYPE_BOX, &itemType, &itemHandle, &itemRect);
			typeBox = GetControlValue((ControlHandle)itemHandle);
			if(typeBox == 0)
			{
				SetControlValue((ControlHandle)itemHandle, 1);
				gPreferences.defaultType = AIFF;
				gPreferences.defaultFormat = SF_FORMAT_16_LINEAR;
				GetDialogItem(gPrefDialog, PR_SFTYPE_CNTL, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle, gPreferences.defaultType);
				GetDialogItem(gPrefDialog, PR_SFFORMAT_CNTL, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle, gPreferences.defaultFormat);
				ShowDialogItem(gPrefDialog, PR_SFTYPE_CNTL);
				ShowDialogItem(gPrefDialog, PR_SFFORMAT_CNTL);
			}
			else
			{
				SetControlValue((ControlHandle)itemHandle, 0);
				gPreferences.defaultType = 0;
				gPreferences.defaultFormat = 0;
				HideDialogItem(gPrefDialog, PR_SFTYPE_CNTL);
				HideDialogItem(gPrefDialog, PR_SFFORMAT_CNTL);
			}
			break;
		case PR_SFTYPE_CNTL:
			GetDialogItem(gPrefDialog, PR_SFTYPE_CNTL, &itemType, &itemHandle, &itemRect);
			gPreferences.defaultType = GetControlValue((ControlHandle)itemHandle);
			break;
		case PR_SFFORMAT_CNTL:
			GetDialogItem(gPrefDialog, PR_SFFORMAT_CNTL, &itemType, &itemHandle, &itemRect);
			gPreferences.defaultFormat = GetControlValue((ControlHandle)itemHandle);
			break;
		case PR_EDIT_BUTTON:
			SHStandardGetFile(nil, 1, typeList, &reply);
			if(reply.sfGood)
			{
				FSpGetFInfo(&reply.sfFile, &fndrInfo);
				gPreferences.editorID = fndrInfo.fdCreator;
				StringCopy(reply.sfFile.name, gPreferences.editorName);
				GetDialogItem(gPrefDialog, PR_EDIT_TEXT, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, gPreferences.editorName);
			}
			else
			{
				gPreferences.editorID = 'SDHK';
				StringCopy("\pNo Editor Selected", gPreferences.editorName);
			}
			break;
		case PR_OPENACT_CNTL:
			if(gPreferences.openPlay == FALSE)
			{
				GetDialogItem(gPrefDialog, PR_OPENACT_CNTL, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,TRUE);
				gPreferences.openPlay = TRUE;
			}
			else
			{
				GetDialogItem(gPrefDialog, PR_OPENACT_CNTL, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,FALSE);
				gPreferences.openPlay = FALSE;
			}
			break;
		case PR_PROCPLAY_CNTL:
			if(gPreferences.procPlay == FALSE)
			{
				GetDialogItem(gPrefDialog, PR_PROCPLAY_CNTL, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,TRUE);
				gPreferences.procPlay = TRUE;
			}
			else
			{
				GetDialogItem(gPrefDialog, PR_PROCPLAY_CNTL, &itemType, &itemHandle, &itemRect);
				SetControlValue((ControlHandle)itemHandle,FALSE);
				gPreferences.procPlay = FALSE;
			}
			break;
		case PR_SET_BUTTON:
			gProcessDisabled = NO_PROCESS;
#if TARGET_API_MAC_CARBON == 1
			HideWindow(GetDialogWindow(gPrefDialog));
#else
			HideWindow(gPrefDialog);
#endif
			MenuUpdate();
			break;
	}
	for(n = 1; n < 17; n++)
		DisableItem(gFormatMenu,n);
	switch(gPreferences.defaultType)
	{
		case AIFF:
		case QKTMA:
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
			break;
		case AIFC:
			EnableItem(gFormatMenu,SF_FORMAT_4_ADDVI);
			EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
			EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
			if(gGestalt.soundCompress == TRUE)
			{
				EnableItem(gFormatMenu,SF_FORMAT_4_ADIMA);
				EnableItem(gFormatMenu,SF_FORMAT_MACE3);
				EnableItem(gFormatMenu,SF_FORMAT_MACE6);
			}
			break;
		case AUDMED:
		case MMIX:
		case SDII:
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
			break;
		case DSPD:
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			break;
		case BICSF:
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
			break;
		case NEXT:
		case SUNAU:
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
			EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
			EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
			break;
		case WAVE:
			EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
			EnableItem(gFormatMenu,SF_FORMAT_16_SWAP);
			EnableItem(gFormatMenu,SF_FORMAT_24_SWAP);
			EnableItem(gFormatMenu,SF_FORMAT_32_SWAP);
			break;
		case MAC:
			EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
			break;
		case RAW:
			EnableItem(gFormatMenu,SF_FORMAT_4_ADDVI);
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
			EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
			EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_16_SWAP);
			EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
			if(gGestalt.soundCompress == TRUE)
			{
				EnableItem(gFormatMenu,SF_FORMAT_4_ADIMA);
				EnableItem(gFormatMenu,SF_FORMAT_MACE3);
				EnableItem(gFormatMenu,SF_FORMAT_MACE6);
			}
		case TEXT:
			EnableItem(gFormatMenu,SF_FORMAT_TEXT);
			break;
		default:
			break;
	}
}
void
OpenPreferences(void)
{
	short	sysVRefNum, rRefNum;
	long	prefDirID;
	FSSpec	prefFSS;
	
	FindFolder(kOnSystemDisk, 'pref', kDontCreateFolder, &sysVRefNum, &prefDirID);
	FSMakeFSSpec(sysVRefNum, prefDirID, PREF_FILE_NAME, &prefFSS);
	// Open file to check for existence, then close for ReadSettings, which opens it
	// again.
	rRefNum = FSpOpenResFile(&prefFSS, fsCurPerm);
	if(rRefNum != -1)
	{
		CloseResFile(rRefNum);
		FlushVol((StringPtr)nil, prefFSS.vRefNum);
		ReadSettings(prefFSS);
	}
	else
		// No preferences file, get defaults from internal settings
		SetDefaults();
	HandleAboutWindow(1, "\pPreferences Loaded");
}

void
LoadSettings(void)
{
	SHStandardFileReply	reply;
	short	rRefNum;
	short		numTypes = 1;
	SHSFTypeList	typeList = {'pref'};
	
	SHStandardGetFile(nil,numTypes,typeList,&reply);
	if(reply.sfGood)
	{
		rRefNum = FSpOpenResFile(&reply.sfFile, fsCurPerm);
		if(rRefNum != -1)
		{
			CloseResFile(rRefNum);
			FlushVol((StringPtr)nil, reply.sfFile.vRefNum);
			ReadSettings(reply.sfFile);
		}
	}
}

void
SavePreferences(void)
{
	short	sysVRefNum, rRefNum;
	long	prefDirID;
	FSSpec	prefFSS;
	
	FindFolder(kOnAppropriateDisk, 'pref', kDontCreateFolder, &sysVRefNum, &prefDirID);
	FSMakeFSSpec(sysVRefNum, prefDirID, PREF_FILE_NAME, &prefFSS);
	rRefNum = FSpOpenResFile(&prefFSS, fsCurPerm);
	if(rRefNum != -1)
	{
		CloseResFile(rRefNum);
		FlushVol((StringPtr)nil, prefFSS.vRefNum);
		FSpDelete(&prefFSS);
		FlushVol((StringPtr)nil, prefFSS.vRefNum);
	}
	FSpCreateResFile(&prefFSS, 'SDHK', 'pref' , smSystemScript);
	WriteSettings(prefFSS);
}

void	SaveSettings(void)
{
	SHStandardFileReply	reply;
	short	rRefNum;
	
	SHStandardPutFile("\pSave Settings as", "\pSoundHack Settings",&reply);
	if(reply.sfGood)
	{
		rRefNum = FSpOpenResFile(&reply.sfFile,fsCurPerm);
		if(rRefNum != -1)
		{
			CloseResFile(rRefNum);
			FlushVol((StringPtr)nil, reply.sfFile.vRefNum);
			FSpDelete(&reply.sfFile);
			FlushVol((StringPtr)nil, reply.sfFile.vRefNum);
		}
		FSpCreateResFile(&reply.sfFile, 'SDHK', 'pref' , reply.sfScript);
		WriteSettings(reply.sfFile);
	}
}


void
WriteSettings(FSSpec myFile)
{
	short	rRefNum;
	rRefNum = FSpOpenResFile(&myFile,fsCurPerm);
	UseResFile(rRefNum);
	AddPtrResource((Ptr)(&gCI), "\pconvolve info", sizeof(gCI));
	AddPtrResource((Ptr)(&gAnaPI), "\panalysis pvoc info", sizeof(gAnaPI));
	AddPtrResource((Ptr)(&gDI), "\pdynamics info", sizeof(gDI));
	AddPtrResource((Ptr)(&gDynPI), "\pdynamics pvoc info", sizeof(gDynPI));
	AddPtrResource((Ptr)(&gEI), "\pextract info", sizeof(gEI));
	AddPtrResource((Ptr)(&gExtPI), "\pextract pvoc info", sizeof(gExtPI));
	AddPtrResource((Ptr)(&gMI), "\pmutate info", sizeof(gMI));
	AddPtrResource((Ptr)(&gMutPI), "\pmutate pvoc info", sizeof(gMutPI));
	AddPtrResource((Ptr)(&gGI), "\pgain info", sizeof(gGI));
	AddPtrResource((Ptr)(&gPI), "\ppvoc info", sizeof(gPI));
	AddPtrResource((Ptr)(&gSRCI), "\psample rate info", sizeof(gSRCI));
	AddPtrResource((Ptr)(&gPreferences), "\ppreferences", sizeof(gPreferences));
	AddPtrResource((Ptr)(&gPCI), "\ppict coder info", sizeof(gPCI));
	CloseResFile(rRefNum);
	UseResFile(gAppFileNum);
	FlushVol((StringPtr)nil, myFile.vRefNum);
}

void
AddPtrResource(Ptr data, Str255 name, long size)
{
	Handle	ptrHandle;
	short	rID;
	
	PtrToHand(data, &ptrHandle, size);
	rID = UniqueID('SDHK');
	AddResource(ptrHandle, 'SDHK', rID, name);
}

void
ReadSettings(FSSpec myFile)
{
	short	rRefNum;
	long	error;
	
	rRefNum = FSpOpenResFile(&myFile,fsCurPerm);
	UseResFile(rRefNum);
	error = GetPtrResource((Ptr)(&gCI), "\pconvolve info", sizeof(gCI));
	if(error != 0)
		SetConDefaults();
		
	error = GetPtrResource((Ptr)(&gAnaPI), "\panalysis pvoc info", sizeof(gAnaPI));
	error += GetPtrResource((Ptr)(&gPCI), "\ppict coder info", sizeof(gPCI));
	if(error != 0)
		SetAnaDefaults();
		
	error = GetPtrResource((Ptr)(&gDI), "\pdynamics info", sizeof(gDI));
	error += GetPtrResource((Ptr)(&gDynPI), "\pdynamics pvoc info", sizeof(gDynPI));
	if(error != 0)
		SetDynDefaults();
		
	error = GetPtrResource((Ptr)(&gEI), "\pextract info", sizeof(gEI));
	error += GetPtrResource((Ptr)(&gExtPI), "\pextract pvoc info", sizeof(gExtPI));
	if(error != 0)
		SetExtDefaults();
		
	error = GetPtrResource((Ptr)(&gMI), "\pmutate info", sizeof(gMI));
	error += GetPtrResource((Ptr)(&gMutPI), "\pmutate pvoc info", sizeof(gMutPI));
	if(error != 0)
		SetMutDefaults();
		
	error = GetPtrResource((Ptr)(&gGI), "\pgain info", sizeof(gGI));
	if(error != 0)
		SetGaiDefaults();
		
	error = GetPtrResource((Ptr)(&gPI), "\ppvoc info", sizeof(gPI));
	if(error != 0)
		SetPhaDefaults();
		
	error = GetPtrResource((Ptr)(&gSRCI), "\psample rate info", sizeof(gSRCI));
	if(error != 0)
		SetSamDefaults();
		
	error = GetPtrResource((Ptr)(&gPreferences), "\ppreferences", sizeof(gPreferences));
	if(error != 0)
		SetPreDefaults();
		
	CloseResFile(rRefNum);
	UseResFile(gAppFileNum);
	FlushVol((StringPtr)nil, myFile.vRefNum);
}

long
GetPtrResource(Ptr data, Str255 name, long size)
{
	Handle	ptrHandle;
	long	rSize;
	
	ptrHandle = GetNamedResource('SDHK', name);
	if(ptrHandle == 0)
		return(-1);
	rSize = GetHandleSize(ptrHandle);
	if(rSize != size)
		return(-1);
	HLock(ptrHandle);
	BlockMoveData(*ptrHandle, data, size);
	HUnlock(ptrHandle);
	return(0);
}

void
SetDefaults(void)
{
	SetPreDefaults();
	SetAnaDefaults();
	SetConDefaults();
	SetGaiDefaults();
	SetSamDefaults();
	SetDynDefaults();
	SetExtDefaults();
	SetMutDefaults();
	SetPhaDefaults();
}

void
SetPreDefaults(void)
{
	StringCopy("\pNo Editor Selected", gPreferences.editorName);
	gPreferences.editorID = 'SDHK';
	gPreferences.openPlay = FALSE;
	gPreferences.procPlay = FALSE;
	gPreferences.defaultType = 0;
	gPreferences.defaultFormat = 0;
}

void
SetAnaDefaults(void)
{
	gAnaPI.points = 1024;						// Analysis
	gAnaPI.windowSize = 1024;
	gAnaPI.windowType = HAMMING;
	gAnaPI.analysisType = CSOUND_ANALYSIS;
	gPCI.centerColor = RED_MENU;
	gPCI.amplitudeRange = 120.0;
	gPCI.inversion = FALSE;
	gPCI.moving = FALSE;
	gPCI.linesPerFFT = 1;
	gPCI.type = 1;
}

void
SetConDefaults(void)
{
	gCI.binPosition = 0;						// Convolution, Binaural & Ring Modulation
	gCI.binHeight = 2;
	gCI.brighten = FALSE;
	gCI.windowImpulse = FALSE;
	gCI.windowType = RECTANGLE;
	gCI.ringMod = FALSE;
	gCI.moving = FALSE;
}

void
SetGaiDefaults(void)
{
	gGI.peakCh1 = gGI.peakCh2 = 0.0;			// Gain
	gGI.peakPosCh1 = gGI.peakPosCh2 = 0;
	gGI.rmsCh1 = gGI.rmsCh2 = 0.0;
	gGI.offsetCh1 = gGI.offsetCh2 = 0.0;
}

void
SetSamDefaults(void)
{
	gSRCI.quality = 4;							// Sample Rate Conversion
	gSRCI.newSRate = 44100.0;
	gSRCI.pitchScale = FALSE;
	gSRCI.variSpeed = FALSE;
}

void
SetDynDefaults(void)
{
	gDynPI.points = 1024;						// Spectral Dynamics
	gDI.highBand = gDynPI.halfPoints = 512;
	gDI.type = GATE;
	gDI.lowBand = 0;
	gDI.below = TRUE;
	gDI.average = TRUE;
	gDI.threshold = 0.01;
	gDI.factor = 0.01;
	gDI.useFile = FALSE;
	gDI.relative = FALSE;
	gDI.group = TRUE;
}

void
SetExtDefaults(void)
{
	gExtPI.points = 1024;						// Spectral Extraction
	gEI.frames = 6;
	gEI.transDev = 5.0;
	gEI.steadDev = 2.0;
}

void
SetMutDefaults(void)
{
	gMI.omega = 0.5;							// Mutation
	gMI.type = USIM;
	gMutPI.useFunction = FALSE;
	gMI.absolute = TRUE;
	gMI.targetAbsoluteValue = 0.1;
	gMI.sourceAbsoluteValue = 0.1;
	gMutPI.points = 1024;
	gMI.scale = FALSE;
	gMI.bandPersistence = 0.97;
	gMI.deltaEmphasis = -0.02;
}

void
SetPhaDefaults(void)
{
	gPI.points = 1024;							// Phase Vocoder
	gPI.windowSize = 1024;
	gPI.scaleFactor = 1.0;
	gPI.windowType = HAMMING;
	gPI.time = TRUE;
	gPI.useFunction = FALSE;
	gPI.minAmplitude = 0.0001;				
	gPI.maskRatio = 0.001;
}