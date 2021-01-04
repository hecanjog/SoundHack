#include "SoundFile.h"
#include "SoundHack.h"
#include "Misc.h"
#include "Menu.h"
//#include <AppleEvents.h>
#include "ShowWave.h"

extern float	gScale, gScaleL, gScaleR, gScaleDivisor;
extern MenuHandle	gSigDispMenu;
extern SoundInfoPtr	inSIPtr, filtSIPtr, outSIPtr, outSteadSIPtr, outTransSIPtr;

void
CreateSigWindow(SoundInfo *mySI)
{
	Str255	menuString, windowTitle;
	WindInfo	*myWindInfo;
	
	if(mySI->nChans == MONO)
		mySI->sigDisp.windo = GetNewCWindow(MONO_DISP_WIND, NIL_POINTER, (WindowPtr)(-1L));
	else
		mySI->sigDisp.windo = GetNewCWindow(STEREO_DISP_WIND, NIL_POINTER, (WindowPtr)(-1L));
	
	myWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
	myWindInfo->windType = WAVEWIND;
	myWindInfo->structPtr = (Ptr)mySI;

	// input window: input, source
	if(mySI == inSIPtr)
	{
		mySI->sigDisp.spareB = 1;
		GetMenuItemText(gSigDispMenu, DISP_INPUT_ITEM, menuString);
		StringAppend(menuString, "\p Signal", windowTitle);
		SetWTitle(mySI->sigDisp.windo, windowTitle);
	}
	// auxilary window: impulse response, target, steady
	else if((mySI == filtSIPtr) || (mySI == outSteadSIPtr))
	{
		mySI->sigDisp.spareB = 2;
		GetMenuItemText(gSigDispMenu, DISP_AUX_ITEM, menuString);
		StringAppend(menuString, "\p Signal", windowTitle);
		SetWTitle(mySI->sigDisp.windo, windowTitle);
	}
	// output window: output, transient, mutant
	else if((mySI == outSIPtr) || (mySI == outTransSIPtr))
	{
		mySI->sigDisp.spareB = 3;
		GetMenuItemText(gSigDispMenu, DISP_OUTPUT_ITEM, menuString);
		StringAppend(menuString, "\p Signal", windowTitle);
		SetWTitle(mySI->sigDisp.windo, windowTitle);
	}
	// Put in a pointer to the parent soundinfo structure
	SetWRefCon(mySI->sigDisp.windo,(long)myWindInfo);
	ShowWindow(mySI->sigDisp.windo);
	MenuUpdate();
	DrawWaveBorders(mySI);
}


void
DrawWaveBorders(SoundInfo *mySI)
{	
	Rect		myRect, drawRect;
	PenState	oldPenState;
	RGBColor	blackRGB, whiteRGB;
	WindowPtr	myWindow;
	
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	myWindow = mySI->sigDisp.windo;
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetWindowPort(myWindow));
	GetPortBounds(GetWindowPort(myWindow), &myRect);
	BackPat(&blackPat);
#else
	SetPort(myWindow);
	myRect = myWindow->portRect;
	BackPat(&(qd.black));
#endif
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;
	
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);

	ClipRect(&myRect);
	EraseRect(&myRect);
	
	drawRect.bottom = myRect.bottom - 14;
	drawRect.top = myRect.top + 2;
	drawRect.left = myRect.left + 5;
	drawRect.right = myRect.right - 5;
	
	GetPenState(&oldPenState);
	PenMode(patCopy);
#if TARGET_API_MAC_CARBON == 1
	PenPat(&whitePat);
#else
	PenPat(&(qd.white));
#endif
	
	/*Draw a line for bottom scale marks*/
	MoveTo(drawRect.left, drawRect.bottom);
	LineTo(drawRect.right, drawRect.bottom);
	
	/* Draw the scale marks on bottom */
	MoveTo(drawRect.left, drawRect.bottom);
	LineTo(drawRect.left, drawRect.bottom + 3);
	MoveTo(drawRect.right, drawRect.bottom);
	LineTo(drawRect.right, drawRect.bottom + 3);
	
	SetPenState(&oldPenState);
	PenNormal();
}

void
DisplayMonoWave(float Wave[], SoundInfo	*mySI, long length, Str255 beginStr, Str255 endStr)
{
	float	drawScale, divisor;
	Rect	myRect, drawRect;
	long	horizSize, vertSize;
	WindowPtr	myWindow;
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	
	if(mySI->sigDisp.windo == (WindowPtr)(-1L))	// needs a better test than this
		CreateSigWindow(mySI);
	myWindow = mySI->sigDisp.windo;
#if TARGET_API_MAC_CARBON == 1
	PenPat(&whitePat);
	SetPort(GetWindowPort(myWindow));
	GetPortBounds(GetWindowPort(myWindow), &myRect);
	BackPat(&blackPat);
#else
	PenPat(&(qd.white));
	SetPort(myWindow);
	myRect = myWindow->portRect;
	BackPat(&(qd.black));
#endif
	drawRect.bottom = myRect.bottom - 15;
	drawRect.top = myRect.top + 2;
	drawRect.left = myRect.left + 5;
	drawRect.right = myRect.right - 5;
	
	DrawWaveBorders(mySI);

	vertSize = drawRect.bottom - drawRect.top;
	horizSize = drawRect.right - drawRect.left;

	if(mySI == inSIPtr || mySI == filtSIPtr)
		divisor = gScaleDivisor;
	else
		divisor = 1.0;
	switch(mySI->packMode)
	{
		case SF_FORMAT_8_LINEAR:
		case SF_FORMAT_8_UNSIGNED:
			drawScale = gScale/(divisor * 128.0);
			break;
		case SF_FORMAT_4_ADDVI:
		case SF_FORMAT_8_MULAW:
		case SF_FORMAT_8_ALAW:
		case SF_FORMAT_16_LINEAR:
		case SF_FORMAT_3DO_CONTENT:
		case SF_FORMAT_16_SWAP:
			drawScale = gScale/(divisor * 32768.0);
			break;
		case SF_FORMAT_24_COMP:
		case SF_FORMAT_24_SWAP:
		case SF_FORMAT_24_LINEAR:
		case SF_FORMAT_32_COMP:
		case SF_FORMAT_32_SWAP:
		case SF_FORMAT_32_LINEAR:
			drawScale = gScale/(divisor * 2147483648.0);
			break;
		case SF_FORMAT_32_FLOAT:
		case SF_FORMAT_TEXT:
			drawScale = gScale/divisor;
			break;
	}
		
	DisplayWave(Wave, myWindow, length, vertSize, (long)drawRect.left, (vertSize/2+drawRect.top), drawScale, horizSize);
	
	/*Draw the scale text */
	TextFont(kFontIDGeneva);
	TextSize(9);
	TextMode(srcXor);
	
	/* Time Scale */
	MoveTo(drawRect.left, drawRect.bottom + 13);
	DrawString(beginStr);
	MoveTo(drawRect.right - StringWidth(endStr), drawRect.bottom+13);
	DrawString(endStr);
	PenNormal();
}

void	
DisplayStereoWave(float WaveL[], float WaveR[], SoundInfo *mySI, long length, Str255 beginStr, Str255 endStr)
{
	float	drawScaleL, drawScaleR, divisor;
	Rect	myRect, drawRect;
	long	horizSize, vertSize;
	WindowPtr	myWindow;
	
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	if(mySI->sigDisp.windo == (WindowPtr)(-1L))	// needs a better test than this
		CreateSigWindow(mySI);
	myWindow = mySI->sigDisp.windo;
#if TARGET_API_MAC_CARBON == 1
	PenPat(&whitePat);
	SetPort(GetWindowPort(myWindow));
	GetPortBounds(GetWindowPort(myWindow), &myRect);
	BackPat(&blackPat);
#else
	PenPat(&(qd.white));
	SetPort(myWindow);
	myRect = myWindow->portRect;
	BackPat(&(qd.black));
#endif
	drawRect.bottom = myRect.bottom - 15;
	drawRect.top = myRect.top + 5;
	drawRect.left = myRect.left + 5;
	drawRect.right = myRect.right - 5;

	DrawWaveBorders(mySI);

	vertSize = drawRect.bottom - drawRect.top;
	horizSize = drawRect.right - drawRect.left;
	
	if(mySI == inSIPtr || mySI == filtSIPtr)
		divisor = gScaleDivisor;
	else
		divisor = 1.0;
	
	switch(mySI->packMode)
	{
		case SF_FORMAT_8_LINEAR:
		case SF_FORMAT_8_UNSIGNED:
			drawScaleL = gScaleL/(divisor * 128.0);
			drawScaleR = gScaleR/(divisor * 128.0);
			break;
		case SF_FORMAT_4_ADDVI:
		case SF_FORMAT_8_MULAW:
		case SF_FORMAT_8_ALAW:
		case SF_FORMAT_16_LINEAR:
		case SF_FORMAT_3DO_CONTENT:
		case SF_FORMAT_16_SWAP:
			drawScaleL = gScaleL/(divisor * 32768.0);
			drawScaleR = gScaleR/(divisor * 32768.0);
			break;
		case SF_FORMAT_24_LINEAR:
		case SF_FORMAT_24_COMP:
		case SF_FORMAT_24_SWAP:
		case SF_FORMAT_32_LINEAR:
		case SF_FORMAT_32_COMP:
		case SF_FORMAT_32_SWAP:
			drawScaleL = gScaleL/(divisor * 2147483648.0);
			drawScaleR = gScaleR/(divisor * 2147483648.0);
			break;
		case SF_FORMAT_32_FLOAT:
		case SF_FORMAT_TEXT:
			drawScaleL = gScaleL/divisor;
			drawScaleR = gScaleR/divisor;
			break;
	}
		
	DisplayWave(WaveL, myWindow, length, vertSize/2, (long)drawRect.left, (vertSize/4 + drawRect.top), drawScaleL, horizSize);
	DisplayWave(WaveR, myWindow, length, vertSize/2, (long)drawRect.left, (3 * vertSize/4 + drawRect.top), drawScaleR, horizSize);

	/*Draw the scale text */
	TextFont(kFontIDGeneva);
	TextSize(9);
	TextMode(srcXor);
	
	/* Time Scale */
	MoveTo(drawRect.left, drawRect.bottom + 13);
	DrawString(beginStr);
	MoveTo(drawRect.right - StringWidth(endStr), drawRect.bottom+13);
	DrawString(endStr);
	PenNormal();
}

void	
DisplayWave(float Wave[], WindowPtr myWindow,long length, long vSize, long hZeroPos, long vZeroPos, float drawScale, long horizSize)
{
	long	vMin = 0, vMax = 0, vPos, hPos, n;
	float	min = 0.0, max = 0.0, hInc, hPosFloat;
	PenState	myPenState, oldPenState;
	Pattern		whitePat, blackPat;
	
#if TARGET_API_MAC_CARBON == 1
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	
	vPos = (long)(vSize/2.0 * -Wave[0] * drawScale) + vZeroPos;
	if(vPos < (vZeroPos - vSize/2)) /* If over top border */
		vPos = vZeroPos - vSize/2;
	if(vPos > (vSize/2 + vZeroPos)) /* If over bottom border */
		vPos = vSize/2 + vZeroPos;
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetWindowPort(myWindow));
#else
	SetPort(myWindow);
#endif
	GetPenState(&oldPenState);
	GetPenState(&myPenState);
	myPenState.pnSize.h = 1;
	myPenState.pnSize.v = 1;
	myPenState.pnLoc.h = hZeroPos;
	myPenState.pnLoc.v = vPos;
	SetPenState(&myPenState);
	PenMode(patCopy);
#if TARGET_API_MAC_CARBON == 1
	PenPat(&whitePat);
#else
	PenPat(&(qd.white));
#endif
	
	hPos = hZeroPos;
	hPosFloat = (float)hPos;
	hInc = (float)horizSize/(length -1);
	
	if(length < horizSize)
	{
		for(n = 1; n < length; n++)
		{
			hPosFloat += hInc;
			hPos = (long)hPosFloat;
			vPos = (long)(vSize/2.0 * -Wave[n] * drawScale) + vZeroPos;
			if(vPos < (vZeroPos - vSize/2)) /* If over top border */
				vPos = vZeroPos - vSize/2;
			if(vPos > (vSize/2 + vZeroPos)) /* If over bottom border */
				vPos = vSize/2 + vZeroPos;
			LineTo(hPos, vPos);
		}
	}
	else
	{
		for(n = 1; n < length;)
		{
			min = 1000000.0;
			max = -1000000.0;
			hPos++;
			while(hPosFloat < (float)hPos)
			{
				hPosFloat += hInc;
				if(min > Wave[n])
					min = Wave[n];
				if(max < Wave[n])
					max = Wave[n];
				n++;
			}
			/* Remember that windows are indexed from the top down so that the maximum
			value will be shown at the minimum index */
			vMin = (long)(vSize/2 * -min * drawScale) + vZeroPos;
			vMax = (long)(vSize/2 * -max * drawScale) + vZeroPos;
			if(vMax < (vZeroPos - vSize/2))
				vMax = vZeroPos - vSize/2;
			else if(vMax > (vZeroPos + vSize/2))
				vMax = vZeroPos + vSize/2;
			if(vMin > (vZeroPos + vSize/2))
				vMin = vZeroPos + vSize/2;
			else if(vMin < (vZeroPos - vSize/2))
				vMin = vZeroPos - vSize/2;
			LineTo((hPos - 1), vMax);
			LineTo((hPos - 1), vMin);
		}
	}
	SetPenState(&oldPenState);
	PenNormal();
}