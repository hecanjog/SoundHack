#include "SoundFile.h"
#include "CarbonGlue.h"
#include "Dialog.h"
#include "Play.h"
#include "Misc.h"
#include "OpenSoundFile.h"

extern SndChannelPtr	gPermChannel;
extern SHStandardFileReply gOpenReply;
extern MenuHandle	gTypeMenu, gFormatMenu;

// For OpenSoundFile.c and CreateSoundFile.c
#ifdef powerc
FileFilterUPP	gOpenSoundFileFilter;
DlgHookYDUPP	gOpenSoundDialogProc;
DlgHookYDUPP	gCreateSoundDialogProc;
#endif

extern struct
{
	Boolean	soundPlay;
	Boolean	soundRecord;
	Boolean	soundCompress;
	Boolean	movie;
}	gGestalt;

/*pascal void StandardPutTypeFile(
	OSType fileType,
	ConstStr255Param prompt,
	ConstStr255Param defaultName,
	SHStandardFileReply *sfreply)
{
	StandardPutFile(prompt, defaultName, sfreply);
}*/

pascal long InvalWindowRect(WindowPtr whichWindow, const Rect *portRect)
{
	InvalRect(portRect);
	return(0);
}


void	OpenSoundFileCallBackInit()
{
#ifdef powerc
	gOpenSoundFileFilter = NewRoutineDescriptor((ProcPtr)OpenSoundClassicFileFilter, uppFileFilterYDProcInfo, GetCurrentISA());
	gOpenSoundDialogProc = NewRoutineDescriptor((ProcPtr)OpenSoundClassicDialogProc, uppDlgHookYDProcInfo, GetCurrentISA());
#endif
}

void OpenSoundGetFile(
	short numTypes, 
	const SFTypeList typeList, 
	SHStandardFileReply *openReply, 
	long *openCount,
	FSSpec *files)
{
	Point myPoint;
	
	myPoint.h = -1;
	myPoint.v = -1;
#ifdef powerc
	CustomGetFile((FileFilterYDUPP)(gOpenSoundFileFilter), numTypes, typeList, openReply, 
		MY_OPEN_DLOG, myPoint, (DlgHookYDUPP)gOpenSoundDialogProc, nil, nil, nil, openCount);
#else
	CustomGetFile((FileFilterYDUPP)(OpenSoundClassicFileFilter), numTypes, typeList, openReply, 
		MY_OPEN_DLOG, myPoint, (DlgHookYDUPP)OpenSoundClassicDialogProc, nil, nil, nil, openCount);
#endif
}

/* TRUE MEANS DON'T SHOW THE FILE */
pascal Boolean
OpenSoundClassicFileFilter(FileParam *pBP, Ptr myDataPtr)
{
	short i;
	Str255 testString;

	if(pBP->ioFlAttrib & 0x10)	// Is it a directory?
		return(FALSE);
	if(pBP->ioFlFndrInfo.fdType == 'Sd2f' || pBP->ioFlFndrInfo.fdType == 'AIFF' || pBP->ioFlFndrInfo.fdType == 'AIFC' ||
	   pBP->ioFlFndrInfo.fdType == 'DSPs' || pBP->ioFlFndrInfo.fdType == 'MSND' || pBP->ioFlFndrInfo.fdType == 'RIFF' ||
	   pBP->ioFlFndrInfo.fdType == '.WAV' || pBP->ioFlFndrInfo.fdType == 'IRCM' || pBP->ioFlFndrInfo.fdType == 'NxTS' ||
	   pBP->ioFlFndrInfo.fdType == 'LMAN' || pBP->ioFlFndrInfo.fdType == 'DATA' || pBP->ioFlFndrInfo.fdType == 'ULAW' ||
	   pBP->ioFlFndrInfo.fdType == 'SCRN' || pBP->ioFlFndrInfo.fdType == 'CSCR' || pBP->ioFlFndrInfo.fdType == 'PICT' || 
	   pBP->ioFlFndrInfo.fdType == 'TEXT' || pBP->ioFlFndrInfo.fdType == 'MooV' || pBP->ioFlFndrInfo.fdType == 'WAVE' ||
	   pBP->ioFlFndrInfo.fdType == 'MPG3' || pBP->ioFlFndrInfo.fdType == 'MPEG')
		return(FALSE);
	for(i = 1; i <= 3; i++)
		testString[i] = pBP->ioNamePtr[i+(pBP->ioNamePtr[0] - 3)];
	testString[0] = 3;
	if(EqualString(testString,"\p.au",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.sf",FALSE,TRUE))
		return(FALSE);
	for(i = 1; i <= 4; i++)
		testString[i] = pBP->ioNamePtr[i+(pBP->ioNamePtr[0] - 4)];
	testString[0] = 4;
	if(EqualString(testString,"\p.mp3",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.snd",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.irc",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.wav",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.pvc",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.aif",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.aic",FALSE,TRUE))
		return(FALSE);
	testString[0] = 2;
	if(EqualString(testString,"\p.W",false,true))
		return(FALSE);
	if(EqualString(testString,"\p.w",false,true))
		return(FALSE);
	for(i = 1; i <= 5; i++)
		testString[i] = pBP->ioNamePtr[i+(pBP->ioNamePtr[0] - 5)];
	testString[0] = 5;
	if(EqualString(testString,"\p.aiff",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.aifc",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.sdif",FALSE,TRUE))
		return(FALSE);
	return(TRUE);
}

pascal short
OpenSoundClassicDialogProc(short item, DialogPtr theDialog, long *openCount)
{
	Rect	textRect;
	static	SoundInfo	currentSI;
	static	Boolean	fileOpen = FALSE, playing = FALSE;
	Str255	aStr, bStr, cStr;
	OSErr	error;
	Handle	itemHandle;
	short	itemType, returnItem;
	long	longSRate;
	SCStatus	status;
	
	if(GetWRefCon((theDialog)) != sfMainDialogRefCon)
		return(item);

	switch(item)
	{
		case MO_PLAY_BUTTON:
			GetDialogItem(theDialog,MO_PLAY_BUTTON,&itemType,&itemHandle,&textRect);	// Set up area in dialog to draw in
			GetControlTitle((ControlHandle)itemHandle, aStr);
			if(EqualString(aStr, "\pPlay", FALSE, TRUE))
			{
				if(currentSI.packMode == SF_FORMAT_16_LINEAR || currentSI.packMode == SF_FORMAT_8_LINEAR 
					|| currentSI.packMode == SF_FORMAT_8_UNSIGNED || currentSI.packMode == SF_FORMAT_8_MULAW
					|| currentSI.packMode == SF_FORMAT_16_SWAP || currentSI.packMode == SF_FORMAT_4_ADDVI
					|| currentSI.packMode == SF_FORMAT_8_ALAW)
				{
					StartDBPlay(&currentSI, 0.0, FALSE);
					SetControlTitle((ControlHandle)itemHandle, "\pStop Play");
					playing = TRUE;
				}
			}
			else
			{
				StopDBPlay(FALSE, FALSE);
				SetControlTitle((ControlHandle)itemHandle, "\pPlay");
				playing = FALSE;
			}
			return(sfHookNullEvent);
			break;
		case MO_OPEN_ALL_BUTTON:
			*openCount = TRUE;
			if(playing)
			{
				StopDBPlay(FALSE, FALSE);
				GetDialogItem(theDialog,MO_PLAY_BUTTON,&itemType,&itemHandle,&textRect);
				SetControlTitle((ControlHandle)itemHandle, "\pPlay");
				playing = FALSE;
			}
			return(sfItemOpenButton);
			break;
		case sfItemOpenButton:
		case sfItemCancelButton:
			if(playing)
			{
				StopDBPlay(FALSE, FALSE);
				GetDialogItem(theDialog,MO_PLAY_BUTTON,&itemType,&itemHandle,&textRect);
				SetControlTitle((ControlHandle)itemHandle, "\pPlay");
				playing = FALSE;
			}
			if(fileOpen == TRUE)
			{
				error = FSClose(currentSI.dFileNum);
				if(error == 0)
					fileOpen = FALSE;
				currentSI.dFileNum = 0;
				error = FlushVol("\p",currentSI.sfSpec.vRefNum);
			}
			return(item);
			break;
		case sfHookNullEvent:
			if(playing)
			{
				SndChannelStatus(gPermChannel, sizeof (status), &status);
				if(!status.scChannelBusy)
				{
					GetDialogItem(theDialog,MO_PLAY_BUTTON,&itemType,&itemHandle,&textRect);
					SetControlTitle((ControlHandle)itemHandle, "\pPlay");
					playing = FALSE;
				}
			}			
			returnItem = sfHookNullEvent;
			break;
		default:
			if(playing)
			{
				StopDBPlay(FALSE, FALSE);
				GetDialogItem(theDialog,MO_PLAY_BUTTON,&itemType,&itemHandle,&textRect);
				SetControlTitle((ControlHandle)itemHandle, "\pPlay");
				playing = FALSE;
			}
			returnItem = item;
			break;

	}
	if(EqualString(gOpenReply.sfFile.name,currentSI.sfSpec.name,TRUE,TRUE) == FALSE)
	{	
		SetPort(theDialog);
		GetDialogItem(theDialog,MO_TEXT_FIELD,&itemType,&itemHandle,&textRect);	// Set up area in dialog to draw in
		EraseRect(&textRect);
		
		// Close previous soundfile
		if(fileOpen == TRUE)
		{
			error = FSClose(currentSI.dFileNum);
			if(error == 0)
				fileOpen = FALSE;
			currentSI.dFileNum = 0;
			error = FlushVol("\p",currentSI.sfSpec.vRefNum);
		}
		// Get soundfile info
		StringCopy(gOpenReply.sfFile.name, currentSI.sfSpec.name);
		currentSI.sfSpec.parID = gOpenReply.sfFile.parID;
		currentSI.sfSpec.vRefNum = gOpenReply.sfFile.vRefNum;
		error = FSpOpenDF(&(currentSI.sfSpec), fsRdPerm, &currentSI.dFileNum);
		if(error == 0)
		{
			fileOpen = TRUE;
			if(OpenSoundReadHeader(&currentSI) != -1)
			{
				GetDialogItem(theDialog,MO_PLAY_BUTTON,&itemType,&itemHandle,&textRect);	// Set up area in dialog to draw in
				HiliteControl((ControlHandle)itemHandle,0);
				switch(currentSI.sfType)
				{
					case AIFF:
						StringCopy("\pAudio IFF, ", aStr);
						break;
					case AIFC:
						StringCopy("\pAudio IFC, ", aStr);
						break;
					case SDII:
						StringCopy("\pSound Designer II, ", aStr);
						break;
					case DSPD:
						StringCopy("\pDSP Designer, ", aStr);
						break;
					case MMIX:
						StringCopy("\pMacMix, ", aStr);
						break;
					case WAVE:
						StringCopy("\pMicrosoft WAVE, ", aStr);
						break;
					case BICSF:
						StringCopy("\pBICSF, ", aStr);
						break;
					case SUNAU:
						StringCopy("\pSun .au, ", aStr);
						break;
					case NEXT:
						StringCopy("\pNeXT .snd, ", aStr);
						break;
					case LEMUR:
						StringCopy("\pLemur MQ, ", aStr);
						break;
					case CS_PVOC:
						StringCopy("\pCsound Pvoc, ", aStr);
						break;
					case PICT:
						StringCopy("\pPICT, ", aStr);
						break;
					case QKTMV:
						StringCopy("\pQuicktime Movie, ", aStr);
						break;
					case RAW:
						StringCopy("\pNo Header, ", aStr);
						currentSI.packMode = SF_FORMAT_16_LINEAR;
						currentSI.sRate = 44100;
						currentSI.nChans = MONO;
						break;
					case TEXT:
						StringCopy("\pNo Header, ", aStr);
						currentSI.packMode = SF_FORMAT_TEXT;
						currentSI.sRate = 44100;
						currentSI.nChans = MONO;
						break;
					default:
						StringCopy("\pNo Header, ", aStr);
						break;
				}
				longSRate = (long)currentSI.sRate;
				NumToString(longSRate, bStr);
				StringAppend(aStr, bStr, cStr);
				StringAppend(cStr, "\p, ", bStr);
				if(currentSI.nChans == MONO)
					StringAppend(bStr, "\pMono, ", cStr);
				else if(currentSI.nChans == STEREO)
					StringAppend(bStr, "\pStereo, ", cStr);
				else if(currentSI.nChans == QUAD)
					StringAppend(bStr, "\pQuad, ", cStr);
				else
					StringAppend(bStr, "\p, ", cStr);
				switch(currentSI.packMode)
				{
					case SF_FORMAT_MACE6:
						StringAppend(cStr, "\pMACE 6:1 Compression.", aStr);
						break;
					case SF_FORMAT_MACE3:
						StringAppend(cStr, "\pMACE 3:1 Compression.", aStr);
						break;
					case SF_FORMAT_4_ADIMA:
						StringAppend(cStr, "\p4 Bit 4:1 IMA4 ADPCM.", aStr);
						break;
					case SF_FORMAT_4_ADDVI:
						StringAppend(cStr, "\p4-Bit Intel/DVI ADPCM.", aStr);
						break;
					case SF_FORMAT_8_LINEAR:
						StringAppend(cStr, "\p8-Bit Linear.", aStr);
						break;
					case SF_FORMAT_8_UNSIGNED:
						StringAppend(cStr, "\p8-Bit Unsigned.", aStr);
						break;
					case SF_FORMAT_8_ALAW:
						StringAppend(cStr, "\p8-Bit aLaw.", aStr);
						break;
					case SF_FORMAT_8_MULAW:
						StringAppend(cStr, "\p8-Bit �Law.", aStr);
						break;
					case SF_FORMAT_16_LINEAR:
						StringAppend(cStr, "\p16-Bit Linear.", aStr);
						break;
					case SF_FORMAT_16_SWAP:
						StringAppend(cStr, "\p16-Bit ByteSwap.", aStr);
						break;
					case SF_FORMAT_24_LINEAR:
						StringAppend(cStr, "\p24-Bit Linear.", aStr);
						break;
					case SF_FORMAT_32_LINEAR:
						StringAppend(cStr, "\p32-Bit Linear.", aStr);
						break;
					case SF_FORMAT_32_FLOAT:
						StringAppend(cStr, "\p32-Bit Float.", aStr);
						break;
					case SF_FORMAT_TEXT:
						StringAppend(cStr, "\pText.", aStr);
						break;
					case SF_FORMAT_SPECT_AMP:
						StringAppend(cStr, "\pAmplitude.", aStr);
						break;
					case SF_FORMAT_SPECT_AMPPHS:
						StringAppend(cStr, "\pAmp/Phase Pairs.", aStr);
						break;
					case SF_FORMAT_SPECT_COMPLEX:
						StringAppend(cStr, "\pReal/Imag Pairs.", aStr);
						break;
					case SF_FORMAT_SPECT_AMPFRQ:
						StringAppend(cStr, "\pAmp/Freq Pairs.", aStr);
						break;
					case SF_FORMAT_SPECT_MQ:
						StringAppend(cStr, "\pMQ Spectra.", aStr);
						break;
					case SF_FORMAT_PICT:
						StringAppend(cStr, "\pSonogram.", aStr);
						break;
					default:
						StringAppend(bStr, "\p.", cStr);
						break;
				}
				GetDialogItem(theDialog,MO_TEXT_FIELD,&itemType,&itemHandle,&textRect);	// Set up area in dialog to draw in
				SetDialogItemText(itemHandle,aStr);
			}
			else
			{
				GetDialogItem(theDialog,MO_PLAY_BUTTON,&itemType,&itemHandle,&textRect);
				HiliteControl((ControlHandle)itemHandle,255);
			}
		}
		else
		{
			GetDialogItem(theDialog,MO_PLAY_BUTTON,&itemType,&itemHandle,&textRect);
			HiliteControl((ControlHandle)itemHandle,255);
		}
	}
	return(returnItem);
	// what we want to do here is first print out some basic info on the soundfile
	// then sense whether play or open all was selected, and perform the proper action
}

#ifdef powerc

void	CreateSoundFileCallBackInit()
{
	gCreateSoundDialogProc = NewRoutineDescriptor((ProcPtr)MySaveDialogProc, uppDlgHookYDProcInfo, GetCurrentISA());
}
#endif

void	CreateSoundPutFile(
			Str255 saveStr, 
			Str255 fileName, 
			SHStandardFileReply *createReply,
			SoundInfoPtr	*soundInfo)
{
	Point	myPoint;

	myPoint.h = -1;
	myPoint.v = -1;

#ifdef powerc
	CustomPutFile(saveStr, fileName, createReply, MY_SAVE_DLOG,
		myPoint, (DlgHookYDUPP)gCreateSoundDialogProc, nil, nil, nil, (Ptr)soundInfo);
#else
	CustomPutFile(saveStr, fileName, createReply, MY_SAVE_DLOG,
		myPoint, (DlgHookYDUPP)CreateSoundDialogProc, nil, nil, nil, (Ptr)soundInfo);
#endif
}

// item returned means process as normal, otherwise hand back sfHookNullEvent
pascal short
CreateSoundDialogProc(short itemHit, DialogPtr theDialog, SoundInfoPtr *mySIPtr)
{
	short	itemType, n;
	Rect	itemRect;
	Handle	itemHandle;

	if(GetWRefCon(GetDialogWindow(theDialog)) != sfMainDialogRefCon)
		return(itemHit);
	if(itemHit == sfHookFirstCall)
	{	
		GetDialogItem(theDialog,MS_TYPE_MENU, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle, (*mySIPtr)->sfType);
		GetDialogItem(theDialog,MS_FORMAT_MENU, &itemType, &itemHandle, &itemRect);
		SetControlValue((ControlHandle)itemHandle, (*mySIPtr)->packMode);
	
		for(n = 1; n < 15; n++)
			DisableItem(gFormatMenu,n);
		
		switch((*mySIPtr)->sfType)
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
	switch(itemHit)
	{
		case sfItemOpenButton:
			switch((*mySIPtr)->packMode)
			{
				case SF_FORMAT_MACE6:
					(*mySIPtr)->frameSize = 1.0/6.0;
					StringCopy("\p6:1 MACE",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_MACE3:
					(*mySIPtr)->frameSize = 1.0/3.0;
					StringCopy("\p3:1 MACE",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_4_ADIMA:
					(*mySIPtr)->frameSize = 0.53125;
					StringCopy("\p4:1 IMA4 ADPCM",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_4_ADDVI:
					(*mySIPtr)->frameSize = 0.5;
					StringCopy("\p4:1 Intel/DVI ADPCM",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_8_LINEAR:
				case SF_FORMAT_8_UNSIGNED:
					(*mySIPtr)->frameSize = 1;
					StringCopy("\pnot compressed",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_8_MULAW:
					(*mySIPtr)->frameSize = 1;
					StringCopy("\pCCITT G711 �Law",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_8_ALAW:
					(*mySIPtr)->frameSize = 1;
					StringCopy("\pCCITT G711 aLaw",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_16_SWAP:
				case SF_FORMAT_16_LINEAR:
				case SF_FORMAT_3DO_CONTENT:
					(*mySIPtr)->frameSize = 2;
					StringCopy("\pnot compressed",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_24_LINEAR:
					(*mySIPtr)->frameSize = 3;
					StringCopy("\pnot compressed",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_32_LINEAR:
					(*mySIPtr)->frameSize = 4;
					StringCopy("\pnot compressed",(*mySIPtr)->compName);
					break;
				case SF_FORMAT_32_FLOAT:
					(*mySIPtr)->frameSize = 4;
					StringCopy("\pFloat 32",(*mySIPtr)->compName);
					break;
			}
			return(itemHit);
			break;
		case MS_TYPE_MENU:
			GetDialogItem(theDialog,MS_TYPE_MENU, &itemType, &itemHandle, &itemRect);
			(*mySIPtr)->sfType =  GetControlValue((ControlHandle)itemHandle);
			for(n = 1; n < 15; n++)
				DisableItem(gFormatMenu,n);
			switch((*mySIPtr)->sfType)
			{
				case AIFF:
				case QKTMA:
					EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
					if((*mySIPtr)->packMode != SF_FORMAT_8_LINEAR && (*mySIPtr)->packMode != SF_FORMAT_16_LINEAR && (*mySIPtr)->packMode != SF_FORMAT_24_LINEAR && (*mySIPtr)->packMode != SF_FORMAT_32_LINEAR)
						(*mySIPtr)->packMode = SF_FORMAT_16_LINEAR;
					break;
				case AIFC:
					EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
					EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
					EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
					EnableItem(gFormatMenu,SF_FORMAT_4_ADDVI);
					if(gGestalt.soundCompress == TRUE)
					{
						EnableItem(gFormatMenu,SF_FORMAT_4_ADIMA);
						EnableItem(gFormatMenu,SF_FORMAT_MACE3);
						EnableItem(gFormatMenu,SF_FORMAT_MACE6);
					}
					if((*mySIPtr)->packMode != SF_FORMAT_8_LINEAR 
						&& (*mySIPtr)->packMode != SF_FORMAT_16_LINEAR 
						&& (*mySIPtr)->packMode != SF_FORMAT_4_ADDVI 
						&& (*mySIPtr)->packMode != SF_FORMAT_24_LINEAR 
						&& (*mySIPtr)->packMode != SF_FORMAT_32_LINEAR 
						&& (*mySIPtr)->packMode != SF_FORMAT_32_FLOAT 
						&& (*mySIPtr)->packMode != SF_FORMAT_8_MULAW 
						&& (*mySIPtr)->packMode != SF_FORMAT_8_ALAW)
							(*mySIPtr)->packMode = SF_FORMAT_16_LINEAR;
					break;
				case AUDMED:
				case SDII:
				case MMIX:
					EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
					if((*mySIPtr)->packMode != SF_FORMAT_8_LINEAR && (*mySIPtr)->packMode != SF_FORMAT_16_LINEAR)
						(*mySIPtr)->packMode = SF_FORMAT_16_LINEAR;
					break;
				case DSPD:
					EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
					(*mySIPtr)->packMode = SF_FORMAT_16_LINEAR;
					break;
				case BICSF:
					EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
					if((*mySIPtr)->packMode != SF_FORMAT_8_LINEAR && (*mySIPtr)->packMode != SF_FORMAT_16_LINEAR && (*mySIPtr)->packMode != SF_FORMAT_32_FLOAT)
						(*mySIPtr)->packMode = SF_FORMAT_16_LINEAR;
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
					if((*mySIPtr)->packMode != SF_FORMAT_8_LINEAR && (*mySIPtr)->packMode != SF_FORMAT_16_LINEAR && (*mySIPtr)->packMode != SF_FORMAT_32_FLOAT && (*mySIPtr)->packMode != SF_FORMAT_8_MULAW && (*mySIPtr)->packMode != SF_FORMAT_8_ALAW)
						(*mySIPtr)->packMode = SF_FORMAT_16_LINEAR;
					break;
				case WAVE:
					EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
					EnableItem(gFormatMenu,SF_FORMAT_16_SWAP);
					if((*mySIPtr)->packMode != SF_FORMAT_8_UNSIGNED && (*mySIPtr)->packMode != SF_FORMAT_16_SWAP)
						(*mySIPtr)->packMode = SF_FORMAT_8_UNSIGNED;
					break;
				case MAC:
					EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
					(*mySIPtr)->packMode = SF_FORMAT_8_UNSIGNED;
					break;
				case TEXT:
					EnableItem(gFormatMenu,SF_FORMAT_TEXT);
					(*mySIPtr)->packMode = SF_FORMAT_TEXT;
					break;
				case RAW:
					EnableItem(gFormatMenu,SF_FORMAT_4_ADDVI);
					EnableItem(gFormatMenu,SF_FORMAT_8_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_8_UNSIGNED);
					EnableItem(gFormatMenu,SF_FORMAT_8_MULAW);
					EnableItem(gFormatMenu,SF_FORMAT_8_ALAW);
					EnableItem(gFormatMenu,SF_FORMAT_24_LINEAR);
 					EnableItem(gFormatMenu,SF_FORMAT_32_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_16_LINEAR);
					EnableItem(gFormatMenu,SF_FORMAT_16_SWAP);
					EnableItem(gFormatMenu,SF_FORMAT_32_FLOAT);
					if(gGestalt.soundCompress == TRUE)
					{
						EnableItem(gFormatMenu,SF_FORMAT_4_ADIMA);
						EnableItem(gFormatMenu,SF_FORMAT_MACE3);
						EnableItem(gFormatMenu,SF_FORMAT_MACE6);
					}
					if((*mySIPtr)->packMode == 0)
						(*mySIPtr)->packMode = SF_FORMAT_8_UNSIGNED;
					break;
				default:
					break;
			}
			GetDialogItem(theDialog,MS_FORMAT_MENU, &itemType, &itemHandle, &itemRect);
			SetControlValue((ControlHandle)itemHandle, (*mySIPtr)->packMode);
			return(sfHookNullEvent);
			break;
		case MS_FORMAT_MENU:
			GetDialogItem(theDialog,MS_FORMAT_MENU, &itemType, &itemHandle, &itemRect);
			(*mySIPtr)->packMode = GetControlValue((ControlHandle)itemHandle);
			return(sfHookNullEvent);
			break;
		default:
			return(itemHit);
			break;
	}
} 


