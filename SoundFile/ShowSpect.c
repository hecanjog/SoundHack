/* 
 *	SoundHack- soon to be many things to many people
 *	Copyright �1994 Tom Erbe - California Institute of the Arts
 *
 *	ShowSpect.c - show spectral data in a window.
 *	10/1/94 - fixed color map on border drawing. Reduced the number of redundant functions.
 *	10/22/94 - fixed bug with uninitialized penstate which used to cause random crashes.
 */
#include <math.h>
#include "SoundFile.h"
#include "SoundHack.h"
//#include <AppleEvents.h>
#include "Misc.h"
#include "Dialog.h"
#include "Menu.h"
#include "ShowSpect.h"

extern SoundInfoPtr	inSIPtr, filtSIPtr, outSIPtr, outSteadSIPtr, outTransSIPtr, outLSIPtr, outRSIPtr;

extern MenuHandle	gSpectDispMenu;
extern long			gProcessEnabled;
extern WindowPtr	gGrowWindow;
extern long			gNumberBlocks;

short
CreateSpectWindow(SoundInfo *mySI)
{
	Str255		menuString, windowTitle;
	OSErr		error;
	WindInfo	*myWindInfo;
	Rect		windowRect;
	
	if(mySI->nChans == MONO)
		mySI->spectDisp.windo = GetNewCWindow(MONO_DISP_WIND, NIL_POINTER, (WindowPtr)(-1L));
	else
		mySI->spectDisp.windo = GetNewCWindow(STEREO_DISP_WIND, NIL_POINTER, (WindowPtr)(-1L));

	// Get port and device of current set port for graphics world
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetWindowPort(mySI->spectDisp.windo));
	GetGWorld(&mySI->spectDisp.winPort, &mySI->spectDisp.winDevice );
	GetPortBounds(GetWindowPort(mySI->spectDisp.windo), &windowRect);
	error = NewGWorld( &mySI->spectDisp.offScreen, 0, &windowRect, NULL, NULL, 0L );
#else
	SetPort(mySI->spectDisp.windo);
	GetGWorld(&mySI->spectDisp.winPort, &mySI->spectDisp.winDevice );
	error = NewGWorld( &mySI->spectDisp.offScreen, 0, &(mySI->spectDisp.windo->portRect), NULL, NULL, 0L );
#endif

	if(error)
	{
		DrawErrorMessage("\pTrouble in offscreen land");
		DisposeWindWorld(&mySI->spectDisp);
		return(-1);
	}
	myWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	myWindInfo->windType = SPECTWIND;
	myWindInfo->structPtr = (Ptr)mySI;

	// input window: input, source
	if(mySI == inSIPtr)
	{
		mySI->spectDisp.spareB = 1;
		GetMenuItemText(gSpectDispMenu, DISP_INPUT_ITEM, menuString);
		StringAppend(menuString, "\p Spectrum", windowTitle);
		SetWTitle(mySI->spectDisp.windo, windowTitle);
	}
	// auxilary window: impulse response, target, steady
	else if((mySI == filtSIPtr) || (mySI == outSteadSIPtr) || (mySI == outLSIPtr))
	{
		mySI->spectDisp.spareB = 2;
		GetMenuItemText(gSpectDispMenu, DISP_AUX_ITEM, menuString);
		StringAppend(menuString, "\p Spectrum", windowTitle);
		SetWTitle(mySI->spectDisp.windo, windowTitle);
	}
	// output window: output, transient, mutant
	else if((mySI == outSIPtr) || (mySI == outTransSIPtr) || (mySI == outRSIPtr))
	{
		mySI->spectDisp.spareB = 3;
		GetMenuItemText(gSpectDispMenu, DISP_OUTPUT_ITEM, menuString);
		StringAppend(menuString, "\p Spectrum", windowTitle);
		SetWTitle(mySI->spectDisp.windo, windowTitle);
	}
	// Put in a pointer to the parent soundinfo structure
	SetWRefCon(mySI->spectDisp.windo,(long)myWindInfo);
	ShowWindow(mySI->spectDisp.windo);
	MenuUpdate();
	DrawSpectBorders(mySI);
	return(1);
}

short
DrawSpectBorders(SoundInfo *mySI)
{	
	Rect	myRect, drawRect, windowRect, offScreenRect;
	PenState	oldPenState;
	Str255		windowStr;
	RGBColor	blackRGB, whiteRGB;
	PixMapHandle		thePixMap;
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	if(mySI->spectDisp.offScreen == NIL_POINTER)
		return(-1);
	thePixMap = GetGWorldPixMap(mySI->spectDisp.offScreen);
	LockPixels(thePixMap);
	SetGWorld(mySI->spectDisp.offScreen, NULL );	// Set port to the offscreen
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->spectDisp.offScreen, &myRect);
#else
	myRect = mySI->spectDisp.offScreen->portRect;
#endif
	
	
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;
	
	RGBBackColor(&blackRGB);
	RGBForeColor(&whiteRGB);
	ClipRect(&myRect);
#if TARGET_API_MAC_CARBON == 1
	FillRect(&myRect,&whitePat);
	
	GetPenState(&oldPenState);
	PenMode(patCopy);
	PenPat(&blackPat);
#else
	FillRect(&myRect,&(qd.white));
	
	GetPenState(&oldPenState);
	PenMode(patCopy);
	PenPat(&(qd.black));
#endif
	ShowPen();
	
	drawRect.left = myRect.left + 24;
	drawRect.right = myRect.right - 5;
	if(mySI->nChans == MONO)
	{
		drawRect.bottom = myRect.bottom - 29;
		drawRect.top = myRect.top + 5;
		DrawOneSpectBorder(drawRect, windowStr);
	}
	else
	{
		drawRect.bottom = ((myRect.bottom - myRect.top) >> 1) - 29;
		drawRect.top = myRect.top + 5;
		DrawOneSpectBorder(drawRect, windowStr);
		drawRect.bottom = myRect.bottom - 29;
		drawRect.top = ((myRect.bottom - myRect.top) >> 1) + 5;
		DrawOneSpectBorder(drawRect, windowStr);
	}
	SetGWorld(mySI->spectDisp.winPort, mySI->spectDisp.winDevice );
	
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->spectDisp.offScreen, &offScreenRect);
	GetPortBounds(GetWindowPort(mySI->spectDisp.windo), &windowRect);
	CopyBits(
			GetPortBitMapForCopyBits(mySI->spectDisp.offScreen),//bitmap of gworld
			GetPortBitMapForCopyBits(GetWindowPort(mySI->spectDisp.windo)),//bitmap of gworld
			&offScreenRect,			//portRect of gworld
			&windowRect,			//portRect of gworld
			srcCopy, NULL );
#else
	CopyBits(&(((GrafPtr)mySI->spectDisp.offScreen)->portBits),//bitmap of gworld
			&(((GrafPtr)mySI->spectDisp.windo)->portBits),	//bitmap of window
			&(mySI->spectDisp.offScreen->portRect),			//portRect of gworld
			&(mySI->spectDisp.windo->portRect),				//portRect of window
			srcCopy, NULL );
#endif
	if(mySI->spectDisp.offScreen)
		UnlockPixels(thePixMap);
	SetPenState(&oldPenState);
	return(1);
}

void
DrawOneSpectBorder(Rect drawRect, Str255 windowStr)
{
	/*Draw a line for bottom scale marks*/
	MoveTo(drawRect.left, drawRect.bottom);
	LineTo(drawRect.right, drawRect.bottom);
	
	/*Draw a line for left scale marks*/
	MoveTo(drawRect.left, drawRect.top);
	LineTo(drawRect.left, drawRect.bottom);
	
	/* Draw the scale marks on bottom */
	MoveTo(drawRect.left, drawRect.bottom);
	LineTo(drawRect.left, drawRect.bottom + 3);
	MoveTo((drawRect.right - drawRect.left)/4 + drawRect.left, drawRect.bottom);
	LineTo((drawRect.right - drawRect.left)/4 + drawRect.left, drawRect.bottom + 3);
	MoveTo((drawRect.right - drawRect.left)/2 + drawRect.left, drawRect.bottom);
	LineTo((drawRect.right - drawRect.left)/2 + drawRect.left, drawRect.bottom + 3);
	MoveTo((drawRect.right - drawRect.left)*3/4 + drawRect.left, drawRect.bottom);
	LineTo((drawRect.right - drawRect.left)*3/4 + drawRect.left, drawRect.bottom + 3);
	MoveTo(drawRect.right, drawRect.bottom);
	LineTo(drawRect.right, drawRect.bottom + 3);
	
	/* Draw the scale marks on left */
	MoveTo(drawRect.left, drawRect.bottom);
	LineTo(drawRect.left - 3, drawRect.bottom);
	MoveTo(drawRect.left, (drawRect.bottom - drawRect.top)/4 + drawRect.top);
	LineTo(drawRect.left - 3, (drawRect.bottom - drawRect.top)/4 + drawRect.top);
	MoveTo(drawRect.left, (drawRect.bottom - drawRect.top)/2 + drawRect.top);
	LineTo(drawRect.left - 3, (drawRect.bottom - drawRect.top)/2 + drawRect.top);
	MoveTo(drawRect.left, (drawRect.bottom - drawRect.top)*3/4 + drawRect.top);
	LineTo(drawRect.left - 3, (drawRect.bottom - drawRect.top)*3/4 + drawRect.top);
	MoveTo(drawRect.left, drawRect.top);
	LineTo(drawRect.left - 3, drawRect.top);
	
	/*Draw the scale text */
	TextFont(kFontIDGeneva);
	TextSize(9);
	TextMode(srcCopy);
	
	/* Frequency Scale */
	MoveTo(drawRect.left, drawRect.bottom + 14);
	DrawString("\p0");
	NumToString((long)(inSIPtr->sRate/8.0), windowStr);
	MoveTo(((drawRect.right - drawRect.left)/4 + drawRect.left - StringWidth(windowStr)/2), drawRect.bottom+14);
	DrawString(windowStr);
	NumToString((long)(inSIPtr->sRate/4.0), windowStr);
	MoveTo(((drawRect.right - drawRect.left) - StringWidth(windowStr))/2 + drawRect.left, drawRect.bottom+14);
	DrawString(windowStr);
	NumToString((long)(inSIPtr->sRate * 3.0/8.0), windowStr);
	MoveTo(((drawRect.right - drawRect.left)*3/4 + drawRect.left - StringWidth(windowStr)/2), drawRect.bottom+14);
	DrawString(windowStr);
	NumToString((long)(inSIPtr->sRate/2.0), windowStr);
	MoveTo(drawRect.right - StringWidth(windowStr), drawRect.bottom+14);
	DrawString(windowStr);
	
	/* Amplitude Scale */
	MoveTo(drawRect.left - 23, drawRect.bottom + 4);
	DrawString("\p-80");
	MoveTo(drawRect.left - 23, (drawRect.bottom - drawRect.top)*3/4 + 4 + drawRect.top);
	DrawString("\p-60");
	MoveTo(drawRect.left - 23, (drawRect.bottom - drawRect.top)/2 + 4 + drawRect.top);
	DrawString("\p-40");
	MoveTo(drawRect.left - 23, (drawRect.bottom - drawRect.top)/4 + 4 + drawRect.top);
	DrawString("\p-20");
	MoveTo(drawRect.left - 23, drawRect.top + 4);
	DrawString("\p0 dB");
}

// this will display spectral data from both pvoc type routines and convolution based routines
// so the data might be cartesian or polar.
short
DisplaySpectrum(float spectrum[], float displaySpectrum[], long halfLengthFFT, SoundInfo *mySI, short channel, float scale, Str255 legend)
{		
	long	realIndex, imagIndex, ampIndex, phaseIndex, bandNumber, maxBand, lengthFFT;
	float	realPart, imagPart, maxAmp = 0.0, magnitude, maxFreq, logAmp;
	double	hypot();
	
	Rect	myRect, drawRect, windowRect, offScreenRect;
	RGBColor	whiteRGB, blackRGB;	
	Str255	oneStr, twoStr, threeStr, fourStr;
	PixMapHandle		thePixMap;
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	
	if(mySI->spectDisp.windo == (WindowPtr)(-1L))
		CreateSpectWindow(mySI);
	if(mySI->spectDisp.offScreen == NIL_POINTER)
		return(-1);
	
	thePixMap = GetGWorldPixMap(mySI->spectDisp.offScreen);
	LockPixels(thePixMap);
	SetGWorld(mySI->spectDisp.offScreen, NULL );	// Set port to the offscreen
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->spectDisp.offScreen, &myRect);
#else
	myRect = mySI->spectDisp.offScreen->portRect;
#endif
	
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;

	lengthFFT = halfLengthFFT<<1;
/*
 * unravel RealFFT-format cartSpectrum: note that halfLengthFFT+1 pairs of values are produced
 */
    for (bandNumber = 0; bandNumber <= halfLengthFFT; bandNumber++)
    {
		realIndex = ampIndex = bandNumber<<1;
		imagIndex = phaseIndex = realIndex + 1;
		if(gProcessEnabled == FIR_PROCESS)
		{
    		if(bandNumber == 0)
    		{
    			realPart = spectrum[realIndex];
    			imagPart = 0.0;
    		}
    		else if(bandNumber == halfLengthFFT)
    		{
    			realPart = spectrum[1];
    			imagPart = 0.0;
    		}
    		else
    		{
				realPart = spectrum[realIndex];
				imagPart = spectrum[imagIndex];
			}
			//	compute magnitude from real and imaginary parts
			magnitude = hypot(realPart, imagPart) * scale;
		}
		else
			// this assumes that the scale has been prefigured for pvoc.
			// scale = 1.0 for pvoc, 8.0/gPI.points for csound analysis
			magnitude = spectrum[ampIndex] * scale;
			
		if (magnitude > maxAmp)
		{
			maxAmp = magnitude;
			maxBand = bandNumber;
		}
		if(magnitude <= 0.0)
			displaySpectrum[bandNumber] = 0.0;
		else
		{
			//	To get a logarithmic range for 80 dB converted to a range from 0.0 to 1.0,
			//	multiply by 10.0 ^ 4.0 inside and divide by 4.0 outside.
			logAmp = log10(magnitude * 10000.0)/4.0;
			if(logAmp < 0.0)
				displaySpectrum[bandNumber] = 0.0;
			else
				displaySpectrum[bandNumber] = logAmp;
		}
    }
    
	//	Now draw everything!
	//	Set up the scale text
#if TARGET_API_MAC_CARBON == 1
	BackPat(&blackPat);
	PenPat(&whitePat);
#else
	BackPat(&(qd.black));
	PenPat(&(qd.white));
#endif
	TextFont(kFontIDGeneva);
	TextSize(9);
	TextMode(srcCopy);
	ForeColor(whiteColor);
	
	maxFreq = maxBand*inSIPtr->sRate/lengthFFT;
	FixToString(maxFreq, oneStr);
	StringAppend(oneStr, "\p        ", fourStr);
	StringAppend(legend, "\p     Peak Frequency Band: ", twoStr);
	StringAppend(twoStr, fourStr, threeStr);

	//	Set up and display the spectrum and legend
	drawRect.left = myRect.left + 25;
	drawRect.right = myRect.right - 5;
	if(mySI->nChans == MONO)
	{
		drawRect.bottom = myRect.bottom - 29;
		drawRect.top = myRect.top + 5;
		DrawSpect(displaySpectrum, drawRect, halfLengthFFT);
		MoveTo(myRect.left + 25, myRect.bottom - 5);
		DrawString(threeStr);
	}
	else
	{
		if(channel == LEFT)
		{
			drawRect.bottom = ((myRect.bottom - myRect.top) >> 1) - 29;
			drawRect.top = myRect.top + 5;
			DrawSpect(displaySpectrum, drawRect, halfLengthFFT);
			MoveTo(myRect.left + 25, ((myRect.bottom - myRect.top) >> 1) - 5);
			DrawString(threeStr);
		}
		else
		{
			drawRect.bottom = myRect.bottom - 29;
			drawRect.top = (myRect.top + (myRect.bottom - myRect.top >> 1)) + 5;
			DrawSpect(displaySpectrum, drawRect, halfLengthFFT);
			MoveTo(myRect.left + 25, myRect.bottom - 5);
			DrawString(threeStr);
		}
	}
	PenNormal();	
	SetGWorld(mySI->spectDisp.winPort, mySI->spectDisp.winDevice );
	
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->spectDisp.offScreen, &offScreenRect);
	GetPortBounds(GetWindowPort(mySI->spectDisp.windo), &windowRect);
	CopyBits(
			GetPortBitMapForCopyBits(mySI->spectDisp.offScreen),//bitmap of gworld
			GetPortBitMapForCopyBits(GetWindowPort(mySI->spectDisp.windo)),//bitmap of gworld
			&offScreenRect,			//portRect of gworld
			&windowRect,			//portRect of gworld
			srcCopy, NULL );
#else
	CopyBits(&(((GrafPtr)mySI->spectDisp.offScreen)->portBits),//bitmap of gworld
			&(((GrafPtr)mySI->spectDisp.windo)->portBits),	//bitmap of window
			&(mySI->spectDisp.offScreen->portRect),			//portRect of gworld
			&(mySI->spectDisp.windo->portRect),				//portRect of window
			srcCopy, NULL );
#endif
	if(mySI->spectDisp.offScreen)
		UnlockPixels(thePixMap);
	return(1);
}

// This is what actually puts the line in the window
void
DrawSpect(float Spect[], Rect drawRect, long length)
{
	long 	horizPos, horizPosLast, n, horizSize,
			vertSize, vertPos, vZeroPos, vertPosMax;
	float	hInc, horizPosFloat;
	PenState	myPenState, oldPenState;
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif

#if TARGET_API_MAC_CARBON == 1
	FillRect(&drawRect, &whitePat);
#else
	FillRect(&drawRect, &(qd.white));
#endif
	
	GetPenState(&oldPenState);
	GetPenState(&myPenState);
	myPenState.pnSize.h = 1;
	myPenState.pnSize.v = 1;
	
	horizPos = horizPosLast = drawRect.left;
	myPenState.pnLoc.h = horizPos;
	
	vZeroPos = drawRect.bottom;
	myPenState.pnLoc.v = vZeroPos;
	
	SetPenState(&myPenState);
	PenMode(patCopy);
#if TARGET_API_MAC_CARBON == 1
	PenPat(&blackPat);
#else
	PenPat(&(qd.black));
#endif
	
	vertSize = drawRect.bottom - drawRect.top;
	horizSize = drawRect.right - drawRect.left;
	vertPosMax = 16384L;
	hInc = (float)horizSize/(length + 1); //get everything within borders 
	horizPosFloat = (float)horizPos;

	for(n = 0; n <= length; n++)
	{
		horizPos = (long)horizPosFloat;
		if(horizPos != horizPosLast)
		{
			vertPos = (long)(vertSize * -Spect[n]) + vZeroPos;
			// If the vertical position is larger than the largest last vertical position (below
			// that previous value), set it equal to that.
			if(vertPosMax < vertPos)
				vertPos = vertPosMax;
			vertPosMax = 16384L;
			// Keep it below the top border
			if(vertPos < drawRect.top)
				vertPos = drawRect.top;
			// Keep it above the bottom border
			if(vertPos >= drawRect.bottom)
				vertPos = drawRect.bottom - 1;
			// Keep it within the left border
			if(horizPos < drawRect.left)
				horizPos = drawRect.left;
			// Keep it within the right border
			if(horizPos >= drawRect.right)
				horizPos = drawRect.right - 1;
			LineTo(horizPos, vertPos);
		}
		else
		{
			vertPos = (long)(vertSize * -Spect[n]) + vZeroPos;
			if(vertPosMax > vertPos)
				vertPosMax = vertPos;
		}
		horizPosLast = horizPos;
		horizPosFloat += hInc;
	}
	SetPenState(&oldPenState);
	PenNormal();
}

// Copy off screen GWorld to window properly
short
UpdateSpectDisplay(SoundInfo *mySI)
{
	RGBColor	whiteRGB, blackRGB;
	Rect		windowRect;
	
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;

	if(mySI->spectDisp.offScreen == NIL_POINTER)
		return(-1);
	SetGWorld(mySI->spectDisp.winPort, mySI->spectDisp.winDevice );
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(GetWindowPort(mySI->spectDisp.windo), &windowRect);
	UpdateGWorld(&(mySI->spectDisp.offScreen),0, &windowRect,0,NIL_POINTER,clipPix);
#else
	UpdateGWorld(&(mySI->spectDisp.offScreen),0,&mySI->spectDisp.windo->portRect,0,NIL_POINTER,clipPix);
#endif
	if(gGrowWindow == mySI->spectDisp.windo)
	{
		gGrowWindow = (WindowPtr)(NIL_POINTER);
		DrawSpectBorders(mySI);
	}
	SetGWorld(mySI->spectDisp.winPort, mySI->spectDisp.winDevice );
	return(1);
}
