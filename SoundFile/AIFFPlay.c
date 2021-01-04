#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
//#include <AppleEvents.h>
//#include <Sound.h>
#include "SoundFile.h"
#include "Dialog.h"
#include "SoundHack.h"
#include "Play.h"

#define	SIZE_BLOCK	131072L

extern WindowPtr	gPlayWindow;

short
StartAIFFPlay(SoundInfo *mySI)
{
	EventRecord	myEvent;
	Rect		myRect;
	SCStatus	status;
	SndChannelPtr	myChannel;
	OSErr		myErr;
	
	Boolean	exit, aSync = TRUE;
	gPlayWindow = GetNewCWindow(CD_IMPORT_WIND,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);

	exit = FALSE;	
	myChannel = 0L;
	myErr = SndNewChannel(&myChannel, sampledSynth, 0, nil);
	if(myErr != 0)
	{
		DrawErrorMessage("\pError Allocating Sound Channel");
		return(-1);
	}	
	
	ShowWindow(gPlayWindow);	
	SelectWindow(gPlayWindow);	
	SetPort(gPlayWindow);
	myRect = gPlayWindow->portRect;
	/*Draw the text */
	TextFont(kFontIDGeneva);
	TextSize(9);
	TextMode(srcXor);
	
	/* Time Scale */
	MoveTo(myRect.left + 5, ((myRect.bottom - myRect.top)/2 + 5));
	DrawString("\pClick mouse to stop playback");
	
	myErr = SndStartFilePlay(myChannel, mySI->dFileNum, 0, SIZE_BLOCK, nil, nil, nil, aSync);

 	do
	{
		SndChannelStatus(myChannel, sizeof(status), &status);
		if(status.scChannelBusy == FALSE)
			exit = TRUE;
		if(GetOSEvent(mDownMask, &myEvent) == TRUE) 
			exit = TRUE;
	}while(exit == FALSE);
	
	DisposeWindow(gPlayWindow);
	SndStopFilePlay(myChannel, TRUE);
	SndDisposeChannel(myChannel,FALSE);
	return(TRUE);
}	
