#if powerc
#include <fp.h>
#endif	/*powerc*/

#include <stdlib.h>
//#include <Sound.h>
#include "SoundFile.h"
#include "CloseSoundFile.h"
#include "WriteHeader.h"
#include "SoundHack.h"
#include "Misc.h"
#include "Dialog.h"

extern long	gNumFilesOpen;
extern short	gStopProcess, gProcessEnabled;
extern SoundInfoPtr	inSIPtr, filtSIPtr, frontSIPtr, firstSIPtr, lastSIPtr, outSIPtr, 
	outLSIPtr, outRSIPtr, outSteadSIPtr, outTransSIPtr;
extern MenuHandle	gSoundFileMenu;
extern WindowPtr	gProcessWindow;
//extern short gTopPosition, gLeftPosition;

short
CloseSoundFile(SoundInfo *mySI)
{	
	short i, numMenu;
	Boolean	looking = TRUE;
	SoundInfoPtr	currentSIPtr, previousSIPtr;
	WindInfo	*myWI;
	
	// if processing, stop on next run through, remember processes get copies
	if((mySI == inSIPtr || mySI == filtSIPtr) && gProcessEnabled != NO_PROCESS)
		gStopProcess = TRUE;
	if(mySI == outSIPtr || mySI == outSteadSIPtr || mySI == outTransSIPtr|| mySI == outLSIPtr|| mySI == outRSIPtr)
	{
		DrawErrorMessage("\pYou can't close the output file while processing");
		return(-1);
	}
	
	// rearrange linked list
	previousSIPtr = currentSIPtr = firstSIPtr;
	for(i = 0; looking == TRUE; i++)
	{
		if(currentSIPtr == mySI)
		{
			previousSIPtr->nextSIPtr = mySI->nextSIPtr;
			looking = FALSE;
			if(mySI == lastSIPtr)
			{
				lastSIPtr = previousSIPtr;
			}
			if(mySI == firstSIPtr)
				firstSIPtr = (SoundInfoPtr)mySI->nextSIPtr;
		}
		else
		{
			previousSIPtr = currentSIPtr;
			currentSIPtr = (SoundInfoPtr)previousSIPtr->nextSIPtr;
		}
	}
	// rearrange menu
	DeleteMenuItem(gSoundFileMenu, i);

	if(gNumFilesOpen > 9)
		numMenu = 10;
	else
		numMenu = gNumFilesOpen;
	for(i=1; i<numMenu; i++)
		SetItemCmd(gSoundFileMenu, i, (i + '0'));
	
	// dispose of infoWindow and WindInfo structure
	HideWindow(mySI->infoWindow);
	myWI = (WindInfoPtr)GetWRefCon(mySI->infoWindow);
	RemovePtr((Ptr)myWI);
	myWI = nil;
	DisposeWindow(mySI->infoWindow);
	DisposeGWorld(mySI->infoOffScreen);
		
	// close file	
//	WriteHeader(mySI);
	FSClose(mySI->dFileNum);
	FlushVol((StringPtr)NIL_POINTER,mySI->sfSpec.vRefNum);
	TouchFolder(mySI->sfSpec.vRefNum, mySI->sfSpec.parID);
	RemovePtr((Ptr)mySI);
	mySI = nil;

	// get new front window
	if(gNumFilesOpen>1)
	{
		frontSIPtr = lastSIPtr;
		SelectWindow(frontSIPtr->infoWindow);
	}
	gNumFilesOpen--;
	if(gNumFilesOpen == 0)
	{
		HideWindow(gProcessWindow);
	}
	MenuUpdate();
	SetDialogFont(systemFont);
	return(TRUE);
}
