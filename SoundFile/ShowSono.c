#include <math.h>
#include "SoundFile.h"
#include "SoundHack.h"
#include "Menu.h"
#include "Misc.h"
#include "Dialog.h"
#include "ShowSono.h"

extern short		gProcessEnabled;
extern long			gNumberBlocks;
extern MenuHandle	gSonoDispMenu;
extern WindowPtr	gGrowWindow;
extern SoundInfoPtr	inSIPtr, filtSIPtr, outSIPtr, outSteadSIPtr, outTransSIPtr, outLSIPtr, outRSIPtr;

short
CreateSonoWindow(SoundInfo *mySI)
{
	OSErr	error;
	Str255	menuString, windowTitle;
	WindInfo	*myWindInfo;
	Rect		windowRect;
	
	if(mySI->nChans == MONO)
		mySI->sonoDisp.windo = GetNewCWindow(MONO_DISP_WIND, NIL_POINTER, (WindowPtr)(-1L));
	else
		mySI->sonoDisp.windo = GetNewCWindow(STEREO_DISP_WIND, NIL_POINTER, (WindowPtr)(-1L));
	
	// Get port and device of current set port for graphics world
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetWindowPort(mySI->sonoDisp.windo));
	GetGWorld(&mySI->sonoDisp.winPort, &mySI->sonoDisp.winDevice );
	GetPortBounds(GetWindowPort(mySI->sonoDisp.windo), &windowRect);
	error = NewGWorld( &mySI->sonoDisp.offScreen, 0, &windowRect, NULL, NULL, 0L );
#else
	SetPort(mySI->sonoDisp.windo);
	GetGWorld(&mySI->sonoDisp.winPort, &mySI->sonoDisp.winDevice );
	error = NewGWorld( &mySI->sonoDisp.offScreen, 0, &(mySI->sonoDisp.windo->portRect), NULL, NULL, 0L );
#endif
	// initialize horizontal pointer
	mySI->sonoDisp.spare = 0;

	if(error)
	{
		DrawErrorMessage("\pTrouble in offscreen land");
		DisposeWindWorld(&mySI->sonoDisp);
		return(-1);
	}
	
	myWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	myWindInfo->windType = SONOWIND;
	myWindInfo->structPtr = (Ptr)mySI;

	// input window: input, source
	if(mySI == inSIPtr)
	{
		mySI->sonoDisp.spareB = 1;
		GetMenuItemText(gSonoDispMenu, DISP_INPUT_ITEM, menuString);
		StringAppend(menuString, "\p Sonogram", windowTitle);
		SetWTitle(mySI->sonoDisp.windo, windowTitle);
	}
	// auxilary window: impulse response, target, steady
	else if((mySI == filtSIPtr) || (mySI == outSteadSIPtr) || (mySI == outLSIPtr))
	{
		mySI->sonoDisp.spareB = 2;
		GetMenuItemText(gSonoDispMenu, DISP_AUX_ITEM, menuString);
		StringAppend(menuString, "\p Sonogram", windowTitle);
		SetWTitle(mySI->sonoDisp.windo, windowTitle);
	}
	// output window: output, transient, mutant
	else if((mySI == outSIPtr) || (mySI == outTransSIPtr) || (mySI == outRSIPtr))
	{
		mySI->sonoDisp.spareB = 3;
		GetMenuItemText(gSonoDispMenu, DISP_OUTPUT_ITEM, menuString);
		StringAppend(menuString, "\p Sonogram", windowTitle);
		SetWTitle(mySI->sonoDisp.windo, windowTitle);
	}
	// Put in a pointer to the parent soundinfo structure
	SetWRefCon(mySI->sonoDisp.windo,(long)myWindInfo);
	ShowWindow(mySI->sonoDisp.windo);
	MenuUpdate();
	DrawSonoBorders(mySI);
	return(1);
}

short
DrawSonoBorders(SoundInfo *mySI)
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
	if(mySI->sonoDisp.offScreen == NIL_POINTER)
		return(-1);
	thePixMap = GetGWorldPixMap(mySI->sonoDisp.offScreen);
	LockPixels(thePixMap);
	SetGWorld(mySI->sonoDisp.offScreen, NULL );	// Set port to the offscreen
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->sonoDisp.offScreen, &myRect);
#else
	myRect = mySI->sonoDisp.offScreen->portRect;
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
	
	drawRect.left = myRect.left + 39;
	drawRect.right = myRect.right - 5;
	if(mySI->nChans == MONO)
	{
		drawRect.bottom = myRect.bottom - 19;
		drawRect.top = myRect.top + 5;
		DrawOneSonoBorder(drawRect, windowStr);
	}
	else
	{
		drawRect.bottom = ((myRect.bottom - myRect.top) >> 1) - 19;
		drawRect.top = myRect.top + 5;
		DrawOneSonoBorder(drawRect, windowStr);
		drawRect.bottom = myRect.bottom - 19;
		drawRect.top = ((myRect.bottom - myRect.top) >> 1) + 5;
		DrawOneSonoBorder(drawRect, windowStr);
	}
	SetGWorld(mySI->sonoDisp.winPort, mySI->sonoDisp.winDevice );
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->sonoDisp.offScreen, &offScreenRect);
	GetPortBounds(GetWindowPort(mySI->sonoDisp.windo), &windowRect);
	CopyBits(
			GetPortBitMapForCopyBits(mySI->sonoDisp.offScreen),//bitmap of gworld
			GetPortBitMapForCopyBits(GetWindowPort(mySI->sonoDisp.windo)),//bitmap of gworld
			&offScreenRect,			//portRect of gworld
			&windowRect,			//portRect of gworld
			srcCopy, NULL );
#else
	CopyBits(&(((GrafPtr)mySI->sonoDisp.offScreen)->portBits),//bitmap of gworld
			&(((GrafPtr)mySI->sonoDisp.windo)->portBits),	//bitmap of window
			&(mySI->sonoDisp.offScreen->portRect),			//portRect of gworld
			&(mySI->sonoDisp.windo->portRect),				//portRect of window
			srcCopy, NULL );
#endif
	if(mySI->sonoDisp.offScreen)
		UnlockPixels(thePixMap);
	SetPenState(&oldPenState);
	return(1);
}

void
DrawOneSonoBorder(Rect drawRect, Str255 windowStr)
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
	
	MoveTo(drawRect.left - 35, drawRect.bottom + 4);
	DrawString("\p0");
	NumToString((long)(inSIPtr->sRate/8.0), windowStr);
	MoveTo(drawRect.left - 35, (drawRect.bottom - drawRect.top)*3/4 + 4 + drawRect.top);
	DrawString(windowStr);
	NumToString((long)(inSIPtr->sRate/4.0), windowStr);
	MoveTo(drawRect.left - 35, (drawRect.bottom - drawRect.top)/2 + 4 + drawRect.top);
	DrawString(windowStr);
	NumToString((long)(inSIPtr->sRate * 3.0/8.0), windowStr);
	MoveTo(drawRect.left - 35, (drawRect.bottom - drawRect.top)/4 + 4 + drawRect.top);
	DrawString(windowStr);
	NumToString((long)(inSIPtr->sRate/2.0), windowStr);
	MoveTo(drawRect.left - 35, drawRect.top + 4);
	DrawString(windowStr);
}

short
DisplaySonogram(float spectrum[], float displaySpectrum[], long halfLengthFFT, SoundInfo *mySI, short channel, float scale, Str255 legend)
{
	long	realIndex, imagIndex, ampIndex, phaseIndex, bandNumber, maxBand, lengthFFT;
	float	realPart, imagPart, maxAmp = 0.0, magnitude, maxFreq, logAmp;
	double	hypot();

	Rect	myRect, drawRect, windowRect, offScreenRect;
	RGBColor	myRGBColor, whiteRGB, blackRGB;	
	Str255	oneStr, twoStr, threeStr, fourStr;
	PixMapHandle		thePixMap;
	
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	if(mySI->sonoDisp.windo == (WindowPtr)(-1L))
		CreateSonoWindow(mySI);
	if(mySI->sonoDisp.offScreen == NIL_POINTER)
		return(-1);
	
	thePixMap = GetGWorldPixMap(mySI->sonoDisp.offScreen);
	LockPixels(thePixMap);
	SetGWorld(mySI->sonoDisp.offScreen, NULL );	// Set port to the offscreen
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->sonoDisp.offScreen, &myRect);
#else
	myRect = mySI->sonoDisp.offScreen->portRect;
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
	drawRect.left = myRect.left + 40;
	drawRect.right = myRect.right - 5;
	if(mySI->nChans == MONO)
	{
		mySI->sonoDisp.spare++;
		if(mySI->sonoDisp.spare > drawRect.right || mySI->sonoDisp.spare < drawRect.left || gNumberBlocks == 0)
			mySI->sonoDisp.spare = drawRect.left;
		drawRect.bottom = myRect.bottom - 20;
		drawRect.top = myRect.top + 5;
		DrawSono(displaySpectrum, drawRect, halfLengthFFT, mySI->sonoDisp.spare);
		GetForeColor(&myRGBColor);
		myRGBColor.red = 0xffff;
		RGBForeColor(&myRGBColor);
		MoveTo(myRect.left + 25, myRect.bottom - 5);
		DrawString(threeStr);
	}
	else
	{
		if(channel == LEFT)
		{
			mySI->sonoDisp.spare++;
			if(mySI->sonoDisp.spare > drawRect.right || mySI->sonoDisp.spare < drawRect.left || gNumberBlocks == 0)
				mySI->sonoDisp.spare = drawRect.left;
			drawRect.bottom = ((myRect.bottom - myRect.top) >> 1) - 20;
			drawRect.top = myRect.top + 5;
			DrawSono(displaySpectrum, drawRect, halfLengthFFT, mySI->sonoDisp.spare);
			GetForeColor(&myRGBColor);
			myRGBColor.red = 0xffff;
			RGBForeColor(&myRGBColor);
			MoveTo(myRect.left + 25, ((myRect.bottom - myRect.top) >> 1) - 5);
			DrawString(threeStr);
		}
		else
		{
			drawRect.bottom = myRect.bottom - 20;
			drawRect.top = (myRect.top + (myRect.bottom - myRect.top >> 1)) + 5;
			DrawSono(displaySpectrum, drawRect, halfLengthFFT, mySI->sonoDisp.spare);
			GetForeColor(&myRGBColor);
			myRGBColor.red = 0xffff;
			RGBForeColor(&myRGBColor);
			MoveTo(myRect.left + 25, myRect.bottom - 5);
			DrawString(threeStr);
		}
	}
	PenNormal();
	SetGWorld(mySI->sonoDisp.winPort, mySI->sonoDisp.winDevice );
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->sonoDisp.offScreen, &offScreenRect);
	GetPortBounds(GetWindowPort(mySI->sonoDisp.windo), &windowRect);
	CopyBits(
			GetPortBitMapForCopyBits(mySI->sonoDisp.offScreen),//bitmap of gworld
			GetPortBitMapForCopyBits(GetWindowPort(mySI->sonoDisp.windo)),//bitmap of gworld
			&offScreenRect,			//portRect of gworld
			&windowRect,			//portRect of gworld
			srcCopy, NULL );
#else
	CopyBits(&(((GrafPtr)mySI->sonoDisp.offScreen)->portBits),//bitmap of gworld
			&(((GrafPtr)mySI->sonoDisp.windo)->portBits),	//bitmap of window
			&(mySI->sonoDisp.offScreen->portRect),			//portRect of gworld
			&(mySI->sonoDisp.windo->portRect),				//portRect of window
			srcCopy, NULL );
#endif
	if(mySI->sonoDisp.offScreen)
		UnlockPixels(thePixMap);
	return(-1);
}

void
DrawSono(float Spect[], Rect drawRect, long length, long hPos)
{
	long		vPos = 0, vPosLast = 0, n, vertSize, temp, spectMax;
	float		vInc, vPosFloat = 0.0;
	PenState	myPenState, oldPenState;
	RGBColor	myRGBColor;
	HSVColor	myHSVColor;
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	
	vertSize = drawRect.bottom - drawRect.top;

	GetPenState(&oldPenState);
	GetPenState(&myPenState);
	myPenState.pnSize.h = 1;
	myPenState.pnSize.v = 1;
	myPenState.pnLoc.h = hPos+1;
	myPenState.pnLoc.v = drawRect.top;
	
	SetPenState(&myPenState);
	PenMode(patCopy);
		
#if TARGET_API_MAC_CARBON == 1
	PenPat(&blackPat);
#else
	PenPat(&(qd.black));
#endif
	if(hPos != drawRect.right)
	{
		myRGBColor.blue = 0x0;
		myRGBColor.green = 0x3fff;
		myRGBColor.red = 0x7fff;
		RGBForeColor(&myRGBColor);
		LineTo(hPos+1, drawRect.bottom);
		MoveTo(hPos, drawRect.top);
	}
	vPosLast = -1;
	vInc = (float)vertSize/(length - 1); /* get everything within borders*/
	vPosFloat = (float)drawRect.top;
	for(n = (length - 1); n >= 0; n--)
	{
		vPos = (long)vPosFloat;
		temp = (long)(Spect[n] * 65535.0);
		if(spectMax < temp)
			spectMax = temp;
		if(vPos != vPosLast)
		{
			if(spectMax < 1.0)
			{
				myHSVColor.hue = 0;
				myHSVColor.saturation = 0;
				myHSVColor.value = 0;
			}
			else
			{
				myHSVColor.hue = (unsigned short)(65536 - spectMax);
				myHSVColor.saturation = (unsigned short)(49152 + ((spectMax)>>2));
				myHSVColor.value = (unsigned short)(8192 + ((spectMax * 7)>>3));
				
			}
			HSV2RGB(&myHSVColor,&myRGBColor);
			RGBForeColor(&myRGBColor);
			LineTo(hPos, vPos);
			spectMax = 0;
		}
		vPosLast = vPos;
		vPosFloat += vInc;
	}

	SetPenState(&oldPenState);
	PenNormal();
}


// Copy off screen GWorld to window properly
short
UpdateSonoDisplay(SoundInfo *mySI)
{
	Rect				aRect, windowRect;
	
	if(mySI->sonoDisp.offScreen == NIL_POINTER)
		return(-1);
	SetGWorld(mySI->sonoDisp.winPort, mySI->sonoDisp.winDevice );
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(GetWindowPort(mySI->sonoDisp.windo), &windowRect);
	UpdateGWorld(&(mySI->sonoDisp.offScreen),0,&windowRect,0,NIL_POINTER,clipPix);
#else
	UpdateGWorld(&(mySI->sonoDisp.offScreen),0,&mySI->sonoDisp.windo->portRect,0,NIL_POINTER,clipPix);
#endif
	if(gGrowWindow == mySI->sonoDisp.windo)
	{
		gGrowWindow = (WindowPtr)(NIL_POINTER);
		DrawSonoBorders(mySI);
#if TARGET_API_MAC_CARBON == 1
		GetPortBounds(GetWindowPort(mySI->sonoDisp.windo), &aRect);
		mySI->sonoDisp.spare = aRect.left + 40;
#else
		mySI->sonoDisp.spare = mySI->sonoDisp.windo->portRect.left + 40;
#endif
	}
	SetGWorld(mySI->sonoDisp.winPort, mySI->sonoDisp.winDevice );
	return(1);
}
