// soundwindow.c
#include "SoundFile.h"
#include "SoundHack.h"

long	NewSoundView(SoundInfo *thisSI, short left, short bottom)
{
	Rect	windRect;
	Str255	menuString, windowTitle;
	WindInfo	*myWindInfo;
	
	windRect.top = 40;
	windRect.bottom = bottom;
	windRect.left = left;
	windRect.right = left + (80 * thisSI->nChans) + 20;
	thisSI->view.windo = NewCWindow(nil,&windRect,nil,true,floatProc,(WindowPtr)-1L,true,nil);	
	myWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	myWindInfo->windType = VIEWWIND;
	myWindInfo->structPtr = (Ptr)thisSI;
	
	// Get port and device of current set port for graphics world
	SetPort(thisSI->view.windo);
	GetGWorld(&thisSI->view.winPort, &thisSI->view.winDevice );
	error = NewGWorld( &thisSI->view.offScreen, 0, &(thisSI->view.windo->portRect), NULL, NULL, 0L );
	// we might attact a info structure here
	thisSI->view.spare = 0;
	if(error)
	{
		DrawErrorMessage("\pTrouble in offscreen land");
		DisposeWindWorld(&thisSI->view);
		return(error);
	}
	
	myWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	myWindInfo->windType = soundView;
	myWindInfo->structPtr = (Ptr)thisSI;
	SetWTitle(thisSI->view.windo, windowTitle);

	SetWRefCon(thisSI->view.windo,(long)myWindInfo);
	ShowWindow(thisSI->view.windo);
	MenuUpdate();
	DrawViewBorders(thisSI);
	return(1);
}
