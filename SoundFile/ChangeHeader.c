#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>

//#include <AppleEvents.h>
#include "SoundFile.h"
#include "WriteHeader.h"
#include "SoundHack.h"
#include "Dialog.h"
#include "ChangeHeader.h"
#include "Menu.h"
#include "Misc.h"
#include "Version.h"
#include "CarbonGlue.h"


DialogPtr	gHeaderDialog;
extern MenuHandle	gFileMenu, gProcessMenu, gTypeMenu, gFormatMenu;
extern SoundInfoPtr	inSIPtr;
extern short			gProcessDisabled, gProcessEnabled;
extern struct
{
	Boolean	soundPlay;
	Boolean	soundRecord;
	Boolean	soundCompress;
	Boolean	movie;
}	gGestalt;

SoundInfo			headerSI;
void	
HandleHeaderDialog(void)
{
	Str255	sRateString, sfLengthString;
	double	sfLength;
	short	itemType, n;
	Rect	itemRect;
	Handle	itemHandle;
	WindInfo *theWindInfo;
	
	gHeaderDialog = GetNewDialog(HEADER_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	
	theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	theWindInfo->windType = PROCWIND;
	theWindInfo->structPtr = (Ptr)HEADER_DLOG;

#if TARGET_API_MAC_CARBON == 1
	SetWRefCon(GetDialogWindow(gHeaderDialog), (long)theWindInfo);
#else
	SetWRefCon(gHeaderDialog, (long)theWindInfo);
#endif

	gProcessDisabled = UTIL_PROCESS;
	MenuUpdate();
	headerSI.nChans = inSIPtr->nChans;
	headerSI.frameSize = inSIPtr->frameSize;
	headerSI.sRate = inSIPtr->sRate;
	headerSI.numBytes = inSIPtr->numBytes;
	headerSI.packMode = inSIPtr->packMode;
	sfLength = headerSI.numBytes/(headerSI.nChans*headerSI.sRate*headerSI.frameSize);
	
	/* Set Up initial window */
	FixToString(inSIPtr->sRate, sRateString);
	HMSTimeString(sfLength, sfLengthString);
	
	GetDialogItem(gHeaderDialog, H_SRATE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, sRateString);
	GetDialogItem(gHeaderDialog, H_SFLENGTH_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, sfLengthString);

	switch(inSIPtr->nChans)
	{
		case MONO:
			GetDialogItem(gHeaderDialog,ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gHeaderDialog,TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gHeaderDialog,FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			break;
		case STEREO:
			GetDialogItem(gHeaderDialog,ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gHeaderDialog,TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gHeaderDialog,FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			break;
		case QUAD:
			GetDialogItem(gHeaderDialog,ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gHeaderDialog,TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gHeaderDialog,FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			break;
	}

#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gHeaderDialog));
	SelectWindow(GetDialogWindow(gHeaderDialog));
	SetPort(GetDialogPort(gHeaderDialog));
#else
	ShowWindow(gHeaderDialog);
	SelectWindow(gHeaderDialog);
	SetPort(gHeaderDialog);
#endif
		
	for(n = 1; n < 17; n++)
		DisableItem(gFormatMenu,n);
	switch(inSIPtr->sfType)
	{
		case AIFF:
		case QKTMA:
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
			break;
		case AIFC:
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_4_ADDVI);
			EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
			EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
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
		case SDII:
		case MMIX:
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
		case TEXT:
		case RAW:
			EnableItem(gFormatMenu,SF_FORMAT_TEXT);
			EnableItem(gFormatMenu,SF_FORMAT_4_ADDVI);
			EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
			EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
			EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
			EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_16_SWAP);
			EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
			EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
			EnableItem(gFormatMenu,SF_FORMAT_24_SWAP);
			EnableItem(gFormatMenu,SF_FORMAT_32_SWAP);
			if(gGestalt.soundCompress == TRUE)
			{
				EnableItem(gFormatMenu,SF_FORMAT_4_ADIMA);
				EnableItem(gFormatMenu,SF_FORMAT_MACE3);
				EnableItem(gFormatMenu,SF_FORMAT_MACE6);
			}
			break;
		default:
			break;
	}
	GetDialogItem(gHeaderDialog,H_FORMAT_MENU, &itemType, &itemHandle, &itemRect);
	SetControlValue((ControlHandle)itemHandle, inSIPtr->packMode);
}

void
HandleHeaderDialogEvent(short itemHit)
{
	double	sfLength;
	short	itemType;
	
	Handle	itemHandle;
	Rect	itemRect;
	Str255	sRateString, sfLengthString;
	WindInfo	*myWI;
	

	switch(itemHit)
	{
		case H_CANCEL_BUTTON:
			RestoreHeader();
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gHeaderDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gHeaderDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gHeaderDialog);
			gHeaderDialog = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			inSIPtr = nil;
			break;
		case H_APPLY_BUTTON:
			inSIPtr->packMode = headerSI.packMode;
			inSIPtr->frameSize = headerSI.frameSize;
			ChangeHeader();
			break;
		case H_SAVE_BUTTON:
			inSIPtr->packMode = headerSI.packMode;
			inSIPtr->frameSize = headerSI.frameSize;
			ChangeHeader();
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gHeaderDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gHeaderDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gHeaderDialog);
			gHeaderDialog = nil;
			gProcessDisabled = gProcessEnabled = NO_PROCESS;
			MenuUpdate();
			inSIPtr = nil;
			break;
		case H_SRATE_FIELD:
			GetDialogItem(gHeaderDialog,H_SRATE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, sRateString);
			StringToFix(sRateString, &headerSI.sRate);
			sfLength = headerSI.numBytes/(headerSI.sRate*headerSI.nChans*headerSI.frameSize);
			HMSTimeString(sfLength, sfLengthString);
			GetDialogItem(gHeaderDialog,H_SFLENGTH_FIELD, &itemType, &itemHandle, &itemRect);				
			SetDialogItemText(itemHandle, sfLengthString);
			break;
		case ONECHAN_RADIO:
			GetDialogItem(gHeaderDialog,ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gHeaderDialog,TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gHeaderDialog,FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			headerSI.nChans=MONO;
			sfLength = (headerSI.numBytes/(headerSI.sRate*headerSI.nChans*headerSI.frameSize));
			HMSTimeString(sfLength, sfLengthString);
			GetDialogItem(gHeaderDialog,H_SFLENGTH_FIELD, &itemType, &itemHandle, &itemRect);			
			SetDialogItemText(itemHandle, sfLengthString);
			break;			
		case TWOCHAN_RADIO:
			GetDialogItem(gHeaderDialog,ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gHeaderDialog,TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			GetDialogItem(gHeaderDialog,FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			headerSI.nChans=STEREO;
			sfLength = (headerSI.numBytes/(headerSI.sRate*headerSI.nChans*headerSI.frameSize));
			HMSTimeString(sfLength, sfLengthString);
			GetDialogItem(gHeaderDialog,H_SFLENGTH_FIELD, &itemType, &itemHandle, &itemRect);			
			SetDialogItemText(itemHandle, sfLengthString);
			break;			
		case FOURCHAN_RADIO:
			GetDialogItem(gHeaderDialog,ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gHeaderDialog,TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,OFF);
			GetDialogItem(gHeaderDialog,FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle,ON);
			headerSI.nChans=QUAD;
			sfLength = (headerSI.numBytes/(headerSI.sRate*headerSI.nChans*headerSI.frameSize));
			HMSTimeString(sfLength, sfLengthString);
			GetDialogItem(gHeaderDialog,H_SFLENGTH_FIELD, &itemType, &itemHandle, &itemRect);			
			SetDialogItemText(itemHandle, sfLengthString);
			break;
		case H_FORMAT_MENU:
			GetDialogItem(gHeaderDialog,H_FORMAT_MENU, &itemType, &itemHandle, &itemRect);
			headerSI.packMode = GetControlValue((ControlHandle)itemHandle);
			switch(headerSI.packMode)
			{
				case SF_FORMAT_MACE6:
					headerSI.frameSize = 1.0/6.0;
					break;
				case SF_FORMAT_MACE3:
					headerSI.frameSize = 1.0/3.0;
					break;
				case SF_FORMAT_4_ADDVI:
					headerSI.frameSize = 0.5;
					break;
				case SF_FORMAT_4_ADIMA:
					headerSI.frameSize = 0.53125;
					break;
				case SF_FORMAT_8_LINEAR:
				case SF_FORMAT_8_UNSIGNED:
				case SF_FORMAT_8_MULAW:
				case SF_FORMAT_8_ALAW:
					headerSI.frameSize = 1;
					break;
				case SF_FORMAT_3DO_CONTENT:
				case SF_FORMAT_16_LINEAR:
				case SF_FORMAT_16_SWAP:
					headerSI.frameSize = 2;
					break;
				case SF_FORMAT_24_LINEAR:
				case SF_FORMAT_24_SWAP:
				case SF_FORMAT_24_COMP:
					headerSI.frameSize = 3;
					break;
				case SF_FORMAT_32_LINEAR:
				case SF_FORMAT_32_COMP:
				case SF_FORMAT_32_SWAP:
				case SF_FORMAT_32_FLOAT:
					headerSI.frameSize = 4;
					break;
			}
			sfLength = (headerSI.numBytes/(headerSI.sRate*headerSI.nChans*headerSI.frameSize));
			HMSTimeString(sfLength, sfLengthString);
			GetDialogItem(gHeaderDialog,H_SFLENGTH_FIELD, &itemType, &itemHandle, &itemRect);			
			SetDialogItemText(itemHandle, sfLengthString);
			break;
		default:
			break;
	}
}

void
ChangeHeader(void)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	Str255	sRateString;
	
	GetDialogItem(gHeaderDialog, H_SRATE_FIELD, &itemType, &itemHandle, &itemRect);
	GetDialogItemText(itemHandle, sRateString);
	StringToFix(sRateString, &inSIPtr->sRate);
	
	GetDialogItem(gHeaderDialog, ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
	if(GetControlValue((ControlHandle)itemHandle))
		inSIPtr->nChans = MONO;
	GetDialogItem(gHeaderDialog, TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
	if(GetControlValue((ControlHandle)itemHandle))
		inSIPtr->nChans = STEREO;
	GetDialogItem(gHeaderDialog, FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
	if(GetControlValue((ControlHandle)itemHandle))
		inSIPtr->nChans = QUAD;
	UpdateInfoWindow(inSIPtr);
	if(inSIPtr->sfType != RAW)
		WriteHeader(inSIPtr);
}

void
RestoreHeader(void)
{
	short	itemType;
	Rect	itemRect;
	Handle	itemHandle;
	Str255	sRateString;
	
	HMSTimeString(inSIPtr->sRate, sRateString);
	GetDialogItem(gHeaderDialog, H_SRATE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, sRateString);
	switch(inSIPtr->nChans)
	{
		case MONO:
			GetDialogItem(gHeaderDialog, ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, ON);
			GetDialogItem(gHeaderDialog, TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, OFF);
			GetDialogItem(gHeaderDialog, FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, OFF);
			break;
		case STEREO:
			GetDialogItem(gHeaderDialog, TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, ON);
			GetDialogItem(gHeaderDialog, ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, OFF);
			GetDialogItem(gHeaderDialog, FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, OFF);
			break;
		case QUAD:
			GetDialogItem(gHeaderDialog, FOURCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, ON);
			GetDialogItem(gHeaderDialog, ONECHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, OFF);
			GetDialogItem(gHeaderDialog, TWOCHAN_RADIO, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, OFF);
			break;
	}
}
