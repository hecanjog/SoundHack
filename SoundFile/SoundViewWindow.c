// soundwindow.c
#include "SoundFile.h"
#include "Dialog.h"
#include "SoundHack.h"
#include "Misc.h"
#include "SoundViewWindow.h"

long	NewSoundView(SoundInfo *thisSI, short left, short bottom)
{
	Rect	windRect;
	Str255	windTitle;
	WindInfo	*windInfo;
	long	error;
	
	windRect.top = 40;
	windRect.bottom = bottom;
	windRect.left = left;
	windRect.right = left + (80 * thisSI->nChans) + 20;
	thisSI->view.windo = NewCWindow(nil,&windRect,nil,true,floatProc,(WindowPtr)-1L,true,nil);	
	
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
	
	windInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	windInfo->windType = VIEWWIND;
	windInfo->structPtr = (Ptr)thisSI;
	StringCopy(thisSI->sfSpec.name, windTitle);
	SetWTitle(thisSI->view.windo, windTitle);

	SetWRefCon(thisSI->view.windo,(long)windInfo);
	ShowWindow(thisSI->view.windo);
	MenuUpdate();
	DrawSoundViewBorder(thisSI);
	return(1);
}

// on the left will be the zoom and overview
// in the left 20 pixels
// markers will appear in the audio
// the top will be some sort of pallete

void DrawSoundViewBorder(SoundInfo *thisSI)
{
	Rect	thisRect, drawRect;
	PenState	oldPenState;
	Str255		windowStr;
	RGBColor	blackRGB, whiteRGB;
	PixMapHandle		thePixMap;
	long borderWidth = 5;
	
	if(thisSI->view.offScreen == NIL_POINTER)
		return(-1);
	thePixMap = GetGWorldPixMap(thisSI->view.offScreen);
	LockPixels(thePixMap);
	SetGWorld(thisSI->view.offScreen, NULL );	// Set port to the offscreen
	thisRect = thisSI->view.offScreen->portRect;
	
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;
	
	RGBBackColor(&blackRGB);
	RGBForeColor(&whiteRGB);
	
	ClipRect(&thisRect);
	FillRect(&thisRect,&(qd.white));

	GetPenState(&oldPenState);
	PenMode(patCopy);
	PenPat(&(qd.black));
	ShowPen();

// border for sound
	MoveTo(borderWidth,borderWidth);					// top left point
	LineTo(thisRect.right - borderWidth, borderWidth); 	// top right
	LineTo(thisRect.right - borderWidth, thisRect.bottom - 30); // bottom right
	LineTo(borderWidth, thisRect.bottom - 30); // bottom left
	LineTo(borderWidth, borderWidth); // bottom right
	
	SetGWorld(thisSI->view.winPort, thisSI->view.winDevice );
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
	CopyBits(&(((GrafPtr)thisSI->view.offScreen)->portBits),//bitmap of gworld
			&(((GrafPtr)thisSI->view.windo)->portBits),	//bitmap of window
			&(thisSI->view.offScreen->portRect),			//portRect of gworld
			&(thisSI->view.windo->portRect),				//portRect of window
			srcCopy, NULL );
	if(thisSI->view.offScreen)
		UnlockPixels(thePixMap);
	SetPenState(&oldPenState);
	return(1);

}
