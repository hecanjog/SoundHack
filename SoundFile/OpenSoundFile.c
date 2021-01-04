#if powerc
#include <fp.h>
#endif	/*powerc*/

//#include <Sound.h>
#include "SoundFile.h"
#include "ReadHeader.h"
#include "SpectralHeader.h"
#include "OpenSoundFile.h"
#include "SoundHack.h"
#include "Play.h"
#include "Misc.h"
#include "Menu.h"
#include "Dialog.h"
#include "CarbonGlue.h"

extern short gProcessEnabled;
extern long	gNumFilesOpen;
extern EventRecord	gTheEvent;
extern SoundInfoPtr	frontSIPtr, firstSIPtr, lastSIPtr;
extern MenuHandle	gFileMenu, gSoundFileMenu;
extern WindowPtr	gProcessWindow;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

SHStandardFileReply gOpenReply;

// if the vRefNum is set, skip the standard file shit, this file has already been found
short
OpenSoundFile(FSSpec fileSpec, Boolean any)
{		
	FInfo		fndrInfo;
	OSErr		error;
	SHSFTypeList	typeList;
	
	SoundInfo	*mySI;
	SoundInfoPtr	previousSIPtr, currentSIPtr;
	WindInfo	*myWindInfo;
	FSSpec		*files;
	
	static short topSoundWindow = 40;
	static short leftSoundWindow = 5;

	short		numTypes, i;
	long		openCount = 1;
	Str255		menuName;
	Rect		infoRect;
#if TARGET_API_MAC_CARBON == 1
	BitMap screenBits;
	
	GetQDGlobalsScreenBits(&screenBits);
#endif

	mySI = (SoundInfo *)NewPtr(sizeof(SoundInfo));
	// This WindInfo structure describes the Sound Information Dialog window and points
	// back to the soundfile described by it. It is deleted by CloseSoundFile().
	myWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	if(fileSpec.vRefNum == nil)
	{		
		SetDialogFont(systemFont);
		numTypes = -1;
		if(any)
			SHStandardGetFile(nil, numTypes, typeList, &gOpenReply);
		else
		{
			OpenSoundGetFile(numTypes, typeList, &gOpenReply, &openCount, &files);
			OpenManySoundFiles(files, openCount);
			if(openCount > 0)
				DisposePtr((Ptr)files);
			return(true);
		}			
		if(gOpenReply.sfGood == FALSE)
		{
			RemovePtr((Ptr)mySI);
			RemovePtr((Ptr)myWindInfo);
			return(-1);
		}
		StringCopy(gOpenReply.sfFile.name, mySI->sfSpec.name);
		mySI->sfSpec.vRefNum = gOpenReply.sfFile.vRefNum;
		mySI->sfSpec.parID = gOpenReply.sfFile.parID;
	}
	else
	{
		StringCopy(fileSpec.name, mySI->sfSpec.name);
		mySI->sfSpec.vRefNum = fileSpec.vRefNum;
		mySI->sfSpec.parID = fileSpec.parID;
	}
	error = FSpOpenDF(&(mySI->sfSpec), fsCurPerm, &mySI->dFileNum);
	if(error != 0 && error != opWrErr)
	{
		DrawErrorMessage("\pProblems opening the file");
		{
			RemovePtr((Ptr)mySI);
			RemovePtr((Ptr)myWindInfo);
			return(-1);
		}
	}
	
	// check to see if this file has been opened before
	previousSIPtr = currentSIPtr = firstSIPtr;
	for(i = 0; i < gNumFilesOpen; i++)
	{
		if(currentSIPtr->sfSpec.vRefNum == mySI->sfSpec.vRefNum && currentSIPtr->sfSpec.parID == mySI->sfSpec.parID
			&& EqualString(currentSIPtr->sfSpec.name, mySI->sfSpec.name, FALSE, FALSE))
		{
			frontSIPtr = currentSIPtr;
			SelectWindow(currentSIPtr->infoWindow);
			RemovePtr((Ptr)mySI);
			RemovePtr((Ptr)myWindInfo);
			return(TRUE);
		}
		else
		{
			previousSIPtr = currentSIPtr;
			currentSIPtr = (SoundInfoPtr)previousSIPtr->nextSIPtr;
		}
	}
	
	if(!any)
	{
		StringCopy("\p", mySI->compName);
		
		// now we filter through different file types from most to least likely
		// but first separate out data fork less files, files without dataforks
		// sometimes fool the magic number tests randomly
		
		FSpGetFInfo(&(mySI->sfSpec),&fndrInfo);	// read the file type
		if(fndrInfo.fdType == 'SCRN' || fndrInfo.fdType == 'PICT' || fndrInfo.fdType == 'CSCR')
		{
			if(ReadPICTHeader(mySI) != -1)
				mySI->sfType = PICT;
		}
		else if(fndrInfo.fdType == 'MooV')
		{
			if(ReadMovieHeader(mySI) != -1)
				mySI->sfType = QKTMV;
		}
		else
		{
			if(ReadNEXTHeader(mySI) != -1)
				mySI->sfType = NEXT;
			else if(ReadBICSFHeader(mySI) != -1)
				mySI->sfType = BICSF;
			else if(ReadAIFFHeader(mySI) != -1)
				mySI->sfType = AIFF;
			else if(ReadAIFCHeader(mySI) != -1)
				mySI->sfType = AIFC;
			else if(ReadSDIIHeader(mySI) != -1)
			{
				switch(fndrInfo.fdCreator)
				{
					case 'Sd2b':
						mySI->sfType = AUDMED;
						break;
					case 'Sd2a':
						mySI->sfType = SDII;
						break;
					case 'MMIX':
						mySI->sfType = MMIX;
						break;
					case 'DSP!':
						mySI->sfType = DSPD;
						break;
					default:
						mySI->sfType = SDII;
						break;
				}
			}
			else if(ReadWAVEHeader(mySI) != -1)
				mySI->sfType = WAVE;
			else if(ReadMPEGHeader(mySI) != -1)
				mySI->sfType = MPEG;
			else if(ReadTX16WHeader(mySI) != -1)
				mySI->sfType = TX16W;
			else if(ReadSDIHeader(mySI) != -1)
				mySI->sfType = SDI;
			else if(ReadLemurHeader(mySI) != -1)
				mySI->sfType = LEMUR;
			else if(ReadPVAHeader(mySI) != -1)
				mySI->sfType = CS_PVOC;
			else if(ReadSDIFHeader(mySI) != -1)
				mySI->sfType = SDIFF;
			else
			{
				mySI->sfType = OpenSoundFindType(mySI->sfSpec);
				mySI->sRate = 44100.0;
				mySI->nChans = 1;
				mySI->dataStart = 0;
				if(mySI->sfType == RAW)
				{
					mySI->packMode = SF_FORMAT_8_UNSIGNED;
					mySI->frameSize = 1;
				}
				else if(mySI->sfType == TEXT)
				{
					mySI->packMode = SF_FORMAT_TEXT;
					mySI->frameSize = 1;
				}
				else
				{
					mySI->sfType = RAW;
					mySI->packMode = SF_FORMAT_16_LINEAR;
					mySI->frameSize = 2;
				}
				GetEOF(mySI->dFileNum, &mySI->numBytes);
				DrawErrorMessage("\pThis is a headerless soundfile, make sure to set the proper header values with Header Change...");
			}
		}
	}
	else
	{
		if(ReadLemurHeader(mySI) != -1)
			mySI->sfType = LEMUR;
		else if(ReadPVAHeader(mySI) != -1)
			mySI->sfType = CS_PVOC;
		else if(ReadSDIFHeader(mySI) != -1)
			mySI->sfType = SDIFF;
		else
		{
			mySI->sfType = RAW;
			mySI->sRate = 44100.0;
			mySI->nChans = 1;
			mySI->dataStart = 0;
			mySI->packMode = SF_FORMAT_8_UNSIGNED;
			mySI->frameSize = 1;
			GetEOF(mySI->dFileNum, &mySI->numBytes);
			DrawErrorMessage("\pThis is a headerless soundfile, make sure to set the proper header values with Header Change...");
		}
	}
	if(mySI->compName[0]==0)
		StringCopy("\pnot compressed", mySI->compName);
		
	// If we are this far, the soundfile has been successfully opened and identified
	// Initialize the display pointers, so we know nothing is there yet.
	mySI->sigDisp.windo = (WindowPtr)(-1L);
	mySI->sigDisp.offScreen = NIL_POINTER;
	mySI->spectDisp.windo = (WindowPtr)(-1L);
	mySI->spectDisp.offScreen = NIL_POINTER;
	mySI->sonoDisp.windo = (WindowPtr)(-1L);
	mySI->sonoDisp.offScreen = NIL_POINTER;

	// Initialize playPosition
	mySI->playPosition = 0;
	
	// Now let's put the SoundInfo structure in the linked list
	// Check to see if this is the first file open
	if(gNumFilesOpen == 0)
	{
		firstSIPtr = lastSIPtr = frontSIPtr = mySI;
	}
	else
	{
		lastSIPtr->nextSIPtr = (Ptr)mySI;			
		lastSIPtr = frontSIPtr = mySI;
	}
	
	// Allocate a new info dialog and put it up
	mySI->infoWindow = GetNewCWindow(INFO_WIND,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	// We now attach the WindInfo structure to the infoWindow and attach the soundinfo structure
	// to the WindInfo structure
	SetWRefCon(mySI->infoWindow,(long)myWindInfo);
	myWindInfo->windType = INFOWIND;
	myWindInfo->structPtr = (Ptr)mySI;
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetWindowPort(mySI->infoWindow));
#else
	SetPort((GrafPtr)mySI->infoWindow);
#endif
	infoRect.top = 0;
	infoRect.left = 0;
	infoRect.bottom = 85;
	infoRect.right = 230;

	GetGWorld(&mySI->infoPort, &mySI->infoDevice );
	error = NewGWorld( &mySI->infoOffScreen, 0, &infoRect, NULL, NULL, 0L );
	if(error)
	{
		DrawErrorMessage("\pTrouble in offscreen land");
		DisposeWindow(mySI->infoWindow);
		DisposeGWorld(mySI->infoOffScreen);
		return(-1);
	}

	for(i=0; i < 256; i++)
		mySI->infoSamps[i] = 0.0;
	MoveWindow(mySI->infoWindow, leftSoundWindow, topSoundWindow, FALSE);
	ShowWindow(mySI->infoWindow);
	UpdateInfoWindow(mySI);
	
	// Add this to the SoundFile Menu, but first remove semicolons from the name
	RemoveMenuChars(mySI->sfSpec.name, menuName);
	InsertMenuItem(gSoundFileMenu, menuName, gNumFilesOpen);

	gNumFilesOpen++;
	EnableItem(gSoundFileMenu, gNumFilesOpen);
	if(gNumFilesOpen < 10)
		SetItemCmd(gSoundFileMenu, gNumFilesOpen, (gNumFilesOpen + '0'));
	// Do a few things if this is the first soundfile
	if(gNumFilesOpen == 1)
	{
		ShowWindow(gProcessWindow);
		DrawControls(gProcessWindow);
		topSoundWindow = 40;
		leftSoundWindow = 5;
	}
	
	// Reset the process window.
	HandleProcessWindowEvent(gTheEvent.where, TRUE);

	topSoundWindow += 85;
#if TARGET_API_MAC_CARBON == 1
	if(topSoundWindow > (screenBits.bounds.bottom - 150))
#else
	if(topSoundWindow > (qd.screenBits.bounds.bottom - 150))
#endif
	{
		topSoundWindow = 40;
		leftSoundWindow += 230;
#if TARGET_API_MAC_CARBON == 1
		if(leftSoundWindow > (screenBits.bounds.right - 230))
#else
		if(leftSoundWindow > (qd.screenBits.bounds.right - 230))
#endif
			leftSoundWindow = 5;
	}
	SetDialogFont(systemFont);
	if(gPreferences.openPlay && gProcessEnabled == NO_PROCESS)
	{
		StopPlay(FALSE);			// Stop any playing file
		SetMenuItemText(gFileMenu, PLAY_ITEM, "\pStop Play (space)");
		StartPlay(mySI, 0.0, 0.0);
	}
	return(TRUE);
}

short 
OpenManySoundFiles(FSSpec *files, long count)
{
	long	index;
	
	for(index = 0; index < count; index++)
	{
		if(OpenSoundFindType(files[index]) != RAW)
			OpenSoundFile(files[index], FALSE);
	}
	return(true);
}


long
OpenSoundFindType(FSSpec fileSpec)
{
	FInfo	fndrInfo;
	long	sfType;
	Str255 testString;
	short i;

	FSpGetFInfo(&fileSpec,&fndrInfo);
	switch(fndrInfo.fdType)
	{
		case 'AIFF':
			sfType = AIFF;
			break;
		case 'AIFC':
			sfType = AIFC;
			break;
		case 'Sd2f':
			sfType = SDII;
			break;
		case 'DSPs':
			sfType = DSPD;
			break;
		case 'MSND':
			sfType = MMIX;
			break;
		case 'RIFF':
		case '.WAV':
		case 'WAVE':
			sfType = WAVE;
			break;
		case 'IRCM':
			sfType = BICSF;
			break;
		case 'NxTS':
		case 'ULAW':
			sfType = NEXT;
			break;
		case 'LMAN':
			sfType = LEMUR;
			break;
		case 'DATA':
			sfType = RAW;
			break;
		case 'MooV':
			sfType = QKTMV;
			break;
		case 'RIF':
		case 'MPEG':
		case 'MPG ':
		case 'Mp3 ':
		case 'MPG3':
			sfType = MPEG;
			break;
		case 'SFIL':
			sfType = SDI;
			break;
		case 'SCRN':
		case 'CSCR':
		case 'PICT':
			sfType = PICT;
			break;
		case 'TEXT':
			sfType = TEXT;
			break;
		default:		//Unknown file type, match text
			sfType = RAW;	// if it matches nothing else, set this.

			for(i = 1; i <= 3; i++)
				testString[i] = fileSpec.name[i+(fileSpec.name[0] - 3)];
			testString[0] = 3;
			if(EqualString(testString,"\p.au",FALSE,TRUE))
				sfType = SUNAU;
			if(EqualString(testString,"\p.sf",FALSE,TRUE))
				sfType = BICSF;
			if(sfType == RAW)
			{
				for(i = 1; i <= 4; i++)
					testString[i] = fileSpec.name[i+(fileSpec.name[0] - 4)];
				testString[0] = 2;
				if(EqualString(testString,"\p.W",false,true))
					sfType = TX16W;
				if(EqualString(testString,"\p.w",false,true))
					sfType = TX16W;
				testString[0] = 4;
				if(EqualString(testString,"\p.snd",FALSE,TRUE))
					sfType = NEXT;
				if(EqualString(testString,"\p.irc",FALSE,TRUE))
					sfType = BICSF;
				if(EqualString(testString,"\p.wav",FALSE,TRUE))
					sfType = WAVE;
				if(EqualString(testString,"\p.pvc",FALSE,TRUE))
					sfType = CS_PVOC;
				if(EqualString(testString,"\p.aic",FALSE,TRUE))
					sfType = AIFC;
				if(EqualString(testString,"\p.aif",FALSE,TRUE))
					sfType = AIFF;
				if(EqualString(testString,"\p.mp3",FALSE,TRUE))
					sfType = MPEG;
			}
			if(sfType == RAW)
			{
				for(i = 1; i <= 5; i++)
					testString[i] = fileSpec.name[i+(fileSpec.name[0] - 5)];
				testString[0] = 5;
				if(EqualString(testString,"\p.mpeg",FALSE,TRUE))
					sfType = MPEG;
				if(EqualString(testString,"\p.aiff",FALSE,TRUE))
					sfType = AIFF;
				if(EqualString(testString,"\p.cdda",FALSE,TRUE))
					sfType = AIFF;
				if(EqualString(testString,"\p.aifc",FALSE,TRUE))
					sfType = AIFC;
				if(EqualString(testString,"\p.sdif",FALSE,TRUE))
					sfType = SDIFF;
			}
			if(sfType == MPEG)
			{
				fndrInfo.fdType = 'MPG3';
				FSpSetFInfo(&fileSpec,&fndrInfo);
			}
			break;
	}
	return(sfType);
}

short
OpenSoundReadHeader(SoundInfo *mySI)
{
	short	returnValue;
	
	mySI->sfType = OpenSoundFindType(mySI->sfSpec);
	switch(mySI->sfType)
	{
		case AIFF:
			returnValue = ReadAIFFHeader(mySI);
			break;
		case AIFC:
			returnValue = ReadAIFCHeader(mySI);
			break;
		case SDII:
		case DSPD:
		case MMIX:
			returnValue = ReadSDIIHeader(mySI);
			break;
		case WAVE:
			returnValue = ReadWAVEHeader(mySI);
			break;
		case BICSF:
			returnValue = ReadBICSFHeader(mySI);
			break;
		case SUNAU:
		case NEXT:
			returnValue = ReadNEXTHeader(mySI);
			break;
		case LEMUR:
			returnValue = ReadLemurHeader(mySI);
			break;
		case CS_PVOC:
			returnValue = ReadPVAHeader(mySI);
			break;
		case SDIFF:
			returnValue = ReadSDIFHeader(mySI);
			break;
		case PICT:
			returnValue = ReadPICTHeader(mySI);
			break;
		case QKTMV:
			returnValue = ReadMovieHeader(mySI);
			break;
		case SDI:
			returnValue = ReadSDIHeader(mySI);
			break;
		case TX16W:
			returnValue = ReadTX16WHeader(mySI);
			break;
		default:
			returnValue = TRUE;
			break;
	}
	return(returnValue);
}

