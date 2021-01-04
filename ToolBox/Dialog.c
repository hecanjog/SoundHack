#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>

//#include <Sound.h>
//#include <SoundComponents.h>
#include <stdio.h>
#include <time.h>
#include "SoundFile.h"
#include "Play.h"
#include "Dialog.h"
#include "SoundHack.h"
#include "Misc.h"
#include "Macros.h"
#include "CarbonGlue.h"


extern short		gAppFileNum;
extern short		gProcessEnabled, gProcessDisabled;
DialogPtr	gErrorDialog, gBiblioDialog;
extern MenuHandle	gTypeMenu, gFormatMenu;
extern WindowPtr	gProcessWindow, gAboutWindow;
extern PlayInfo		gPlayInfo;
extern SoundInfoPtr	frontSIPtr, outLSIPtr, outRSIPtr, inSIPtr, filtSIPtr, outSIPtr, outSteadSIPtr, outTransSIPtr, firstSIPtr, lastSIPtr;

PicHandle		gAboutPicture;
ControlHandle	gSliderCntl;
PixPatHandle	barPP, backPP;
Str255			inStr = "\pin: 00:00:00.000", outStr = "\pout: 00:00:00.000", playStr = "\pplay: 00:00:00.000";
Str255			titleStr;
float			gBarLength;
CGrafPtr		gProcessPort;
GDHandle		gProcessDevice;
GWorldPtr		gProcessOffScreen;


long
InitProcessWindow(void)
{
	RGBColor	barColor, backColor;
	OSErr		error;
	Rect		backRect;
#if TARGET_API_MAC_CARBON == 1
	BitMap screenBits;
	GetQDGlobalsScreenBits(&screenBits);
#endif
	
	backRect.top = 0;
	backRect.left = 0;
	backRect.bottom = 15;
	backRect.right = 502;

	// set up some colors
	backPP = NewPixPat();
	barPP = NewPixPat();
	
	LMGetHiliteRGB(&barColor);
	backColor.blue = barColor.blue>>1;
	backColor.green = barColor.green>>1;
	backColor.red = barColor.red>>1;
		
	MakeRGBPat(backPP,&backColor);	
	MakeRGBPat(barPP,&barColor);
	
	gProcessWindow = GetNewCWindow(PLAY_WIND,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
#if TARGET_API_MAC_CARBON == 1
	MoveWindow(gProcessWindow, 5, screenBits.bounds.bottom - 40, FALSE);	
	SetPort(GetWindowPort(gProcessWindow));
#else
	MoveWindow(gProcessWindow, 5, qd.screenBits.bounds.bottom - 40, FALSE);	
	SetPort((GrafPtr)gProcessWindow);
#endif
	
	GetGWorld(&gProcessPort, &gProcessDevice );
	error = NewGWorld( &gProcessOffScreen, 0, &backRect, NULL, NULL, 0L );
	if(error)
	{
		DrawErrorMessage("\pTrouble in offscreen land");
		DisposeWindow(gProcessWindow);
		DisposeGWorld(gProcessOffScreen);
		return(-1);
	}

	gSliderCntl = GetNewControl(TRANSPORT_CNTL, gProcessWindow);
	SetControlValue(gSliderCntl, 0);
	MoveControl(gSliderCntl, 0, 16);
	return(TRUE);
}
long
UpdateProcessWindow(Str255 inTime, Str255 outTime, Str255 title, float barLength)
{
	double			seconds;
	static double oldSeconds;
	
	static float 			lastLength;
	
	Str255		tmpStr;
	
	if(inTime[0] != 0)
		StringAppend("\pin: ", inTime, inStr);
	if(outTime[0] != 0)
		StringAppend("\pout: ", outTime, outStr);
		
	if(gProcessEnabled == PLAY_PROCESS)
	{
		seconds = GetPlayTime();
		if(seconds != oldSeconds)
		{
			HMSTimeString(seconds, tmpStr);
			StringAppend("\pplay: ", tmpStr, playStr);
		}
	}
	if(title[0] != 0)
		StringCopy(title, titleStr);
	if(barLength != -1.0)
		gBarLength = barLength;
	return(TRUE);
}

long
DrawProcessWindow()
{
	unsigned long	controlSet;
	double			seconds;
	static double oldSeconds;
	short			hPos;
	
	static float 			lastLength;
	
	Rect		barRect, backRect, offScreenRect;
	Str255		tmpStr;
	PixMapHandle		thePixMap;
	RGBColor	blackRGB, whiteRGB;
	
	controlSet = -1;
	if(gProcessOffScreen == NIL_POINTER)
		return(-1);
	thePixMap = GetGWorldPixMap(gProcessOffScreen);
	LockPixels(thePixMap);
	SetGWorld(gProcessOffScreen, NULL);	// Set port to the offscreen
	
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;
	
	RGBBackColor(&blackRGB);
	RGBForeColor(&whiteRGB);
	
	barRect.top = backRect.top = 0;
	barRect.left = backRect.left = 0;
	barRect.bottom = backRect.bottom = 15;
	backRect.right = 502;
	if(gProcessEnabled != NO_PROCESS && gBarLength != lastLength)
		lastLength = gBarLength;
	barRect.right = (long)(lastLength * 502);
	FillCRect(&backRect, backPP);
	FillCRect(&barRect, barPP);
	
		
	if(gProcessEnabled == PLAY_PROCESS)
	{
		seconds = GetPlayTime();
		if(seconds != oldSeconds)
		{
			controlSet = (unsigned long)((420 * seconds)/gPlayInfo.length);
			if(controlSet != gPlayInfo.position)
			{
				CLIP(controlSet, 0, 419);
				SetControlValue(gSliderCntl, controlSet);
				gPlayInfo.position = GetControlValue(gSliderCntl);
			}
		}
	}
	TextFont(kFontIDCourier);
	TextSize(14);
	TextMode(srcOr);
	// autoposition depending on string size
	// 2 from left edge
	MoveTo(2, 13);
	DrawString(inStr);
	// center
	hPos = (502 - StringWidth(outStr)) >> 1;
	MoveTo(hPos, 13);
	DrawString(outStr);
	// 2 from right edge
	hPos = 500 - StringWidth(playStr);
	MoveTo(hPos, 13);
	DrawString(playStr);
	
	SetGWorld(gProcessPort, gProcessDevice);
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(gProcessOffScreen, &offScreenRect);
	CopyBits(
			GetPortBitMapForCopyBits(gProcessOffScreen),//bitmap of gworld
			GetPortBitMapForCopyBits(GetWindowPort(gProcessWindow)),//bitmap of gworld
			&offScreenRect,			//portRect of gworld
			&(backRect),				//portRect of window
			srcCopy, NULL );
#else
	CopyBits(&(((GrafPtr)gProcessOffScreen)->portBits),	//bitmap of gworld
			&(((GrafPtr)gProcessWindow)->portBits),		//bitmap of window
			&(gProcessOffScreen->portRect),				//portRect of gworld
			&(backRect),				//portRect of window
			srcCopy, NULL );
#endif
			
	UnlockPixels(thePixMap);
	
	GetWTitle(gProcessWindow, tmpStr);
	if(titleStr[0] != 0 && EqualString(tmpStr, titleStr, TRUE, TRUE) == FALSE)
		SetWTitle(gProcessWindow, titleStr);
	if(controlSet != -1)
		DrawControls(gProcessWindow);
	return(TRUE);
}

void
HandleProcessWindowEvent(Point where, Boolean reset)
{
	double	frontLength, timeSet;
	unsigned long controlValue;
	short	partHit;

	ControlHandle	sliderCntl;
	Str255	tmpStr;

#if TARGET_API_MAC_CARBON == 1
	SetPort(GetWindowPort(gProcessWindow));
#else
	SetPort((GrafPtr)gProcessWindow);
#endif
	if(reset)
		SetControlValue(gSliderCntl, frontSIPtr->playPosition);
	else
	{
		GlobalToLocal(&where);
		partHit = FindControl(where, gProcessWindow, &sliderCntl);
		if(partHit != 0)
		{	
			switch(partHit)
			{
				case kControlIndicatorPart:
					TrackControl(sliderCntl,where,nil);
					frontSIPtr->playPosition = controlValue = GetControlValue(gSliderCntl);
					RestartPlay();
					break;
				case kControlPageUpPart:
					frontSIPtr->playPosition = controlValue = where.h - 40;
					//frontSIPtr->playPosition = controlValue = GetControlValue(gSliderCntl) - 10;
					CLIP(controlValue, 0, 420);
					SetControlValue(gSliderCntl, controlValue);
					RestartPlay();
					break;
				case kControlPageDownPart:
					frontSIPtr->playPosition = controlValue = where.h - 40;
					//frontSIPtr->playPosition = controlValue = GetControlValue(gSliderCntl) + 10;
					CLIP(controlValue, 0, 420);
					SetControlValue(gSliderCntl, controlValue);
					RestartPlay();
					break;
				case kControlUpButtonPart:
					frontSIPtr->playPosition = controlValue = GetControlValue(gSliderCntl) - 5;
					CLIP(controlValue, 0, 420);
					SetControlValue(gSliderCntl, controlValue);
					RestartPlay();
					break;
				case kControlDownButtonPart:
					frontSIPtr->playPosition = controlValue = GetControlValue(gSliderCntl) + 5;
					CLIP(controlValue, 0, 420);
					SetControlValue(gSliderCntl, controlValue);
					RestartPlay();
					break;
				case 0:
					break;
			}
			UpdateInfoWindow(frontSIPtr);
		}
	}
	controlValue = GetControlValue(gSliderCntl);
	frontLength = frontSIPtr->numBytes/(frontSIPtr->sRate * frontSIPtr->nChans * frontSIPtr->frameSize);
	timeSet = (controlValue * frontLength)/420.0;
	HMSTimeString(timeSet, tmpStr);
	StringAppend("\pplay: ", tmpStr, playStr);
	UpdateProcessWindow("\p", "\p", "\p", -1.0);
}

void
DrawErrorMessage(Str255 s)
{
	short	itemType;
	Handle	itemHandle;
	Rect	itemRect;
	WindInfo *theWindInfo;
	
	if(gErrorDialog == nil)
	{
		gErrorDialog = GetNewDialog(ERROR_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
		
		theWindInfo = (WindInfo *)NewPtr(sizeof(WindInfo));
		theWindInfo->windType = PROCWIND;
		theWindInfo->structPtr = (Ptr)ERROR_DLOG;

#if TARGET_API_MAC_CARBON == 1
		SetWRefCon(GetDialogWindow(gErrorDialog), (long)theWindInfo);
#else
		SetWRefCon(gErrorDialog, (long)theWindInfo);
#endif
	}

	GetDialogItem(gErrorDialog, ERROR_TEXT, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, s);
#if TARGET_API_MAC_CARBON == 1
	SelectWindow(GetDialogWindow(gErrorDialog));
	ShowWindow(GetDialogWindow(gErrorDialog));
#else
	SelectWindow(gErrorDialog);
	ShowWindow(gErrorDialog);
#endif
	DrawDialog(gErrorDialog);
}

void
HandleErrorMessageEvent(short itemHit)
{
	switch(itemHit)
	{
		case ERROR_OK:
#if TARGET_API_MAC_CARBON == 1
			HideWindow(GetDialogWindow(gErrorDialog));
#else
			HideWindow(gErrorDialog);
#endif
/*
#if TARGET_API_MAC_CARBON == 1
			myWI = (WindInfoPtr)GetWRefCon(GetDialogWindow(gErrorDialog));
#else
			myWI = (WindInfoPtr)GetWRefCon(gErrorDialog);
#endif
			RemovePtr((Ptr)myWI);
			DisposeDialog(gErrorDialog);
			gErrorDialog = nil;*/
			MenuUpdate();
			break;
		default:
			break;
	}
}

// 0 - initialize window, 1 - update window (check for string), 2 - hide window 
void	HandleAboutWindow(short event, Str255 message)
{	
	unsigned long	lFreeKBytes;
	Str255	memStr;
	RGBColor backGround;
	Rect	msgRect;
	Rect	imageRect;
	EventRecord theEvent;
	short fontNum;
	
	backGround.red = 0xffff;
	backGround.green = 0xccff;
	backGround.blue = 0x0000;
	msgRect.top = 172;
	msgRect.bottom = 195;
	msgRect.left = 99;	// 294 is the center
	msgRect.right = 490;
	
	
	switch(event)
	{
		case 0:
			gAboutWindow = GetNewCWindow(ABOUT_WIND, NIL_POINTER, (WindowPtr)MOVE_TO_FRONT);
			gAboutPicture = (PicHandle)GetResource('PICT', ABOUT_PICT);
			ShowWindow(gAboutWindow);
			SelectWindow(gAboutWindow);
#if TARGET_API_MAC_CARBON == 1
			SetPort(GetWindowPort(gAboutWindow));
			GetPortBounds(GetWindowPort(gAboutWindow), &imageRect);
			DrawPicture(gAboutPicture, &imageRect);
#else
			SetPort((GrafPtr)gAboutWindow);
			DrawPicture(gAboutPicture, &(gAboutWindow->portRect));
#endif
			break;
		case 1:			
#if TARGET_API_MAC_CARBON == 1
			SetPort(GetWindowPort(gAboutWindow));
#else
			SetPort((GrafPtr)gAboutWindow);
#endif
			ShowWindow(gAboutWindow);
#if TARGET_API_MAC_CARBON == 1
			GetPortBounds(GetWindowPort(gAboutWindow), &imageRect);
			DrawPicture(gAboutPicture, &imageRect);
#else
			DrawPicture(gAboutPicture, &(gAboutWindow->portRect));
#endif
			RGBBackColor(&backGround);
			EraseRect(&msgRect);

			UseResFile(gAppFileNum);
			GetFNum("\p04b-09", &fontNum);
			GetFNum("\p04b-20", &fontNum);
			if(fontNum == 0)
			{
				GetFNum("\pHelvetica", &fontNum);
				TextFont(fontNum);
				TextSize(14);
			}
			else
			{
				TextFont(fontNum);
				TextSize(9);
			}
			TextMode(srcCopy);
			if(message == nil)
			{
				StringCopy("\pVersion 0.896 - 2007 Aug 8.", memStr);
				MoveTo((294 - (StringWidth(memStr) >> 1)), 192);
				DrawString(memStr);
				WaitNextEvent( 0, &theEvent, 90, NULL );  /* Use this just to sleep without getting events. */
				EraseRect(&msgRect);
				lFreeKBytes = MaxBlock() >> 10;
				sprintf((char *)memStr, "Free Memory: %ld kBytes.", lFreeKBytes);
				c2pstr((char *)memStr);
				MoveTo((244 - (StringWidth(memStr) >> 1)), 192);
				DrawString(memStr);
			}
			else
			{
				MoveTo((244 - (StringWidth(message) >> 1)), 192);
				DrawString(message);
			}
			PenNormal();
			break;
		case 2:
			HideWindow(gAboutWindow);
			MenuUpdate();
			break;
	}
	WaitNextEvent( 0, &theEvent, 2, NULL );  /* Use this just to sleep without getting events. */

}

void
UpdateInfoWindow(SoundInfo *mySI)
{
	short	fontSize = 13, vertPosition = 0, horizPosition = 0, i;
	Rect	windowRect, imageRect, offScreenRect;
	double	sfLength, mByteSize; 
	Str255	bSizeString, sRateString, sfLengthString, nChansString, tmpStr;
	RGBColor colorOne, colorTwo, colorThree, colorFour, white, whiteRGB, blackRGB;
	PixPatHandle	pixPatOne, pixPatTwo;
	PixMapHandle		thePixMap;
	long colorNumber;
	Pattern		whitePat, blackPat;

	if(mySI->infoOffScreen == NIL_POINTER)
		return;
	thePixMap = GetGWorldPixMap(mySI->infoOffScreen);
	LockPixels(thePixMap);
	SetGWorld(mySI->infoOffScreen, NULL);	// Set port to the offscreen
	
	blackRGB.red = blackRGB.green = blackRGB.blue = 0x0000;
	whiteRGB.red = whiteRGB.green = whiteRGB.blue = 0xFFFF;
	
	RGBBackColor(&blackRGB);
	RGBForeColor(&whiteRGB);

#if TARGET_API_MAC_CARBON == 1
	
	GetQDGlobalsWhite(&whitePat);
	GetQDGlobalsBlack(&blackPat);
#endif
	colorNumber = mySI->numBytes + mySI->sfSpec.name[1] + (mySI->sfSpec.name[2]<<8) + (mySI->sfSpec.name[3]<<16) + (mySI->sfSpec.name[4]<<24);
	
	mySI->infoUpdate = false;
	/*Set up colors based on length of sound*/
	white.red = white.green = white.blue = 0xFFFF;

	colorOne.red = (colorNumber>>10);
	colorOne.green = (colorNumber>>5);
	colorOne.blue = (colorNumber>>8);
	
	colorThree.red = (colorNumber>>6);
	colorThree.green = (colorNumber>>9);
	colorThree.blue = (colorNumber>>9);
	
	colorTwo.red = (colorNumber>>7);
	colorTwo.green = (colorNumber>>10);
	colorTwo.blue = (colorNumber>>6);
	
	colorFour.red = (colorNumber>>5);
	colorFour.green = (colorNumber>>9);
	colorFour.blue = (colorNumber>>7);
	
	ColorBrighten(&colorOne);
	ColorBrighten(&colorTwo);
	
	pixPatOne = NewPixPat();
	pixPatTwo = NewPixPat();
	MakeRGBPat(pixPatOne,&colorOne);	
	MakeRGBPat(pixPatTwo,&colorTwo);

	/*Initialize Strings for Info Box */
	FixToString3(mySI->sRate, tmpStr);
	StringAppend(tmpStr, "\p sps", sRateString);
	
	mByteSize = (double)mySI->numBytes / (1024 * 1024);
	FixToString3(mByteSize, tmpStr);
	StringAppend(tmpStr, "\p mbytes", bSizeString);
	
	NumToString(mySI->nChans, tmpStr);
	StringAppend(tmpStr, "\p channels", nChansString);
	
	sfLength = (mySI->numBytes/(mySI->frameSize*mySI->nChans*mySI->sRate));
	HMSTimeString(sfLength, sfLengthString);
 
	/*Setup and Show Window*/
	SetWTitle(mySI->infoWindow, mySI->sfSpec.name);
#if TARGET_API_MAC_CARBON == 1
//		SetPort(GetWindowPort(mySI->infoWindow));
		GetPortBounds(GetWindowPort(mySI->infoWindow), &windowRect);
#else
//		SetPort((GrafPtr)mySI->infoWindow);
		windowRect = mySI->infoWindow->portRect;
#endif
	imageRect.top = windowRect.top + 5;
	imageRect.bottom = windowRect.bottom - 5;
	imageRect.left = windowRect.left + 5;
	imageRect.right = imageRect.left + (imageRect.bottom - imageRect.top);

	TextFont(kFontIDCourier);
	TextSize(fontSize);
	TextMode(srcOr);
	PenNormal();
	RGBForeColor(&colorOne);
	RGBBackColor(&white);
	EraseRect(&windowRect);

	/*Fill the block for sample drawing*/
	SetSecondsPosition(mySI, (mySI->playPosition * sfLength)/420.0);
	PenMode(srcOr);
	if(mySI->nChans == MONO)
	{
		for(horizPosition = imageRect.left, i = 0; horizPosition < imageRect.right; horizPosition++, i++)
		{
			// samples varies from 1.0 to -1.0
			// vertPosition varies from imageRect.bottom (large) to imageRect.top
			vertPosition = ((-mySI->infoSamps[i] * .5) + .5) * (imageRect.bottom - imageRect.top) + imageRect.top;
			MoveTo(horizPosition, imageRect.bottom);
			RGBForeColor(&colorOne);
			LineTo(horizPosition,vertPosition);
			RGBForeColor(&colorThree);
			LineTo(horizPosition,imageRect.top);
		}
	}
	if(mySI->nChans == STEREO)
	{
		PenMode(addOver);
		for(horizPosition = imageRect.left, i = 0; horizPosition < imageRect.right; horizPosition++, i++)
		{
			// samples varies from 1.0 to -1.0
			// vertPosition varies from imageRect.bottom (large) to imageRect.top
			RGBForeColor(&colorOne);
			vertPosition = ((-mySI->infoSamps[i] * .5) + .5) * (imageRect.bottom - imageRect.top) + imageRect.top;
			MoveTo(horizPosition, imageRect.bottom);
			LineTo(horizPosition,vertPosition);
			RGBForeColor(&colorThree);
			LineTo(horizPosition,imageRect.top);
			
			RGBForeColor(&colorTwo);
			vertPosition = ((-mySI->infoSamps[i+128] * .5) + .5) * (imageRect.bottom - imageRect.top) + imageRect.top;
			MoveTo(horizPosition, imageRect.bottom);
			LineTo(horizPosition,vertPosition);
			RGBForeColor(&colorFour);
			LineTo(horizPosition,imageRect.top);
		}
	}
		
	PenNormal();
	
	RGBForeColor(&colorOne);
	// autoposition vertically depending on string size
	// leave room for the imageRect on left
	horizPosition = imageRect.right + 5;
	vertPosition = fontSize + 1;
	MoveTo(horizPosition, vertPosition);
	DrawString(sfLengthString);
	
	vertPosition += fontSize;
	MoveTo(horizPosition, vertPosition);
	DrawString(sRateString);
	
	vertPosition += fontSize;
	MoveTo(horizPosition, vertPosition);
	DrawString(nChansString);
	
	vertPosition += fontSize;
	MoveTo(horizPosition, vertPosition);
	DrawString(bSizeString);
	
	vertPosition += fontSize;
	MoveTo(horizPosition, vertPosition);
	switch(mySI->sfType)
	{
		case AIFF:
			DrawString("\paudio iff");
			break;
		case AIFC:
			DrawString("\paudio ifc");
			break;
		case SDII:
			DrawString("\psd II�");
			break;
		case AUDMED:
			DrawString("\paudiomedia�");
			break;
		case MMIX:
			DrawString("\pmacmix�");
			break;
		case DSPD:
			DrawString("\pdsp designer�");
			break;
		case BICSF:
			DrawString("\pbicsf");
			break;
		case NEXT:
			DrawString("\pnext");
			break;
		case SUNAU:
			DrawString("\psun");
			break;
		case QKTMA:
			DrawString("\pqt aiff");
			break;
		case SDIFF:
			DrawString("\psdif");
			break;
		case WAVE:
			DrawString("\pwave");
			break;
		case TX16W:
			DrawString("\ptx16w");
			break;
		case SDI:
			DrawString("\psound designer i");
			break;
		case TEXT:
			DrawString("\ptext");
			break;
		case RAW:
			DrawString("\pheaderless data");
			break;
		case CS_PVOC:
			DrawString("\pcsound pvoc");
			break;
		case CS_HTRO:
			DrawString("\pcsound hetro");
			break;
		case LEMUR:
			DrawString("\plemur");
			break;
		case PICT:
			DrawString("\ppict resource");
			break;
		case QKTMV:
			DrawString("\pquicktime movie");
			break;
		case MPEG:
			DrawString("\pmpeg");
			break;
	}
	
	vertPosition += fontSize;
	MoveTo(horizPosition, vertPosition);
	switch(mySI->packMode)
	{
		case SF_FORMAT_MACE6:
			DrawString( "\pmace 6:1");
			break;
		case SF_FORMAT_MACE3:
			DrawString( "\pmace 3:1");
			break;
		case SF_FORMAT_4_ADIMA:
			DrawString( "\pima4 adpcm");
			break;
		case SF_FORMAT_4_ADDVI:
			DrawString( "\pintel/dvi adpcm");
			break;
		case SF_FORMAT_8_LINEAR:
			DrawString( "\p8 bit linear");
			break;
		case SF_FORMAT_8_UNSIGNED:
			DrawString( "\p8 bit unsigned");
			break;
		case SF_FORMAT_8_MULAW:
			DrawString( "\p8 bit �law");
			break;
		case SF_FORMAT_8_ALAW:
			DrawString( "\p8 bit alaw");
			break;
		case SF_FORMAT_16_LINEAR:
		case SF_FORMAT_16_SWAP:
		case SF_FORMAT_3DO_CONTENT:
			DrawString( "\p16 bit linear");
			break;
		case SF_FORMAT_24_LINEAR:
		case SF_FORMAT_24_COMP:
		case SF_FORMAT_24_SWAP:
			DrawString( "\p24 bit linear");
			break;
		case SF_FORMAT_32_LINEAR:
		case SF_FORMAT_32_COMP:
		case SF_FORMAT_32_SWAP:
			DrawString( "\p32 bit linear");
			break;
		case SF_FORMAT_32_FLOAT:
			DrawString( "\p32 bit float");
			break;
		case SF_FORMAT_TEXT:
			DrawString( "\ptext");
			break;
		case SF_FORMAT_SPECT_AMPFRQ:
			DrawString("\pamp/freq pairs");
			break;
		case SF_FORMAT_SPECT_AMP:
			DrawString("\pamplitude");
			break;
		case SF_FORMAT_SPECT_AMPPHS:
			DrawString("\pamp/phase pairs");
			break;
		case SF_FORMAT_SPECT_COMPLEX:
			DrawString("\preal/imag pairs");
			break;
		case SF_FORMAT_SPECT_MQ:
			DrawString("\pmq Data");
			break;
		case SF_FORMAT_MPEG_I:
			DrawString("\pmpeg layer i");
			break;
		case SF_FORMAT_MPEG_II:
			DrawString("\pmpeg layer ii");
			break;
		case SF_FORMAT_MPEG_III:
			DrawString("\pmpeg layer iii");
			break;
		case SF_FORMAT_TX16W:
			DrawString("\p12 bit tx16w");
			break;
		case SF_FORMAT_PICT:
			DrawString("\psonogram");
			break;
	}
	DisposePixPat(pixPatOne);
	DisposePixPat(pixPatTwo);
	SetGWorld(mySI->infoPort, mySI->infoDevice);
	RGBBackColor(&whiteRGB);
	RGBForeColor(&blackRGB);
#if TARGET_API_MAC_CARBON == 1
	GetPortBounds(mySI->infoOffScreen, &offScreenRect);
	CopyBits(
			GetPortBitMapForCopyBits(mySI->infoOffScreen),//bitmap of gworld
			GetPortBitMapForCopyBits(GetWindowPort(mySI->infoWindow)),//bitmap of gworld
			&offScreenRect,			//portRect of gworld
			&(windowRect),				//portRect of window
			srcCopy, NULL );
#else
	CopyBits(&(((GrafPtr)mySI->infoOffScreen)->portBits),	//bitmap of gworld
			&(((GrafPtr)mySI->infoWindow)->portBits),		//bitmap of window
			&(mySI->infoOffScreen->portRect),				//portRect of gworld
			&(windowRect),				//portRect of window
			srcCopy, NULL );
#endif
			
	UnlockPixels(thePixMap);
	UpdateInfoWindowFileType();
}

void
UpdateInfoWindowFileType(void)
{
	Rect	typeRect, windowRect;
	PicHandle	thePicture;
	Boolean	looking = TRUE;
	SoundInfoPtr	currentSIPtr, previousSIPtr;
	long i;
	short processEnabled;
	
	if(gProcessEnabled == NO_PROCESS || gProcessEnabled == PLAY_PROCESS)
		processEnabled = gProcessDisabled;
	else
		processEnabled = gProcessEnabled;
		
	previousSIPtr = currentSIPtr = firstSIPtr;
	for(i = 0; looking == TRUE; i++)
	{
		if(currentSIPtr == lastSIPtr)
			looking = FALSE;
		
		if(currentSIPtr == inSIPtr)
		{
			if(processEnabled == FUDGE_PROCESS || processEnabled == MUTATE_PROCESS)
				thePicture = GetPicture(SOURCE_PICT);
			else
				thePicture = GetPicture(INPUT_PICT);
		}
		else if(currentSIPtr == outSIPtr || currentSIPtr == outLSIPtr || currentSIPtr == outRSIPtr)
		{
			if(processEnabled == FUDGE_PROCESS || processEnabled == MUTATE_PROCESS)
				thePicture = GetPicture(MUTANT_PICT);
			else
				thePicture = GetPicture(OUTPUT_PICT);
		}
		else if(currentSIPtr == filtSIPtr)
		{
			if(processEnabled == FUDGE_PROCESS || processEnabled == MUTATE_PROCESS)
				thePicture = GetPicture(TARGET_PICT);
			else if(processEnabled == LEMON_PROCESS || processEnabled == DYNAMICS_PROCESS)
				thePicture = GetPicture(TRIGGER_PICT);
			else
				thePicture = GetPicture(IMPULSE_PICT);
		}
		else if(currentSIPtr == outSteadSIPtr)
			thePicture = GetPicture(STEADY_PICT);
		else if(currentSIPtr == outTransSIPtr)
			thePicture = GetPicture(TRANSIENT_PICT);
		else
			thePicture = GetPicture(BLANK_PICT);
#if TARGET_API_MAC_CARBON == 1
		SetPort(GetWindowPort(currentSIPtr->infoWindow));
		GetPortBounds(GetWindowPort(currentSIPtr->infoWindow), &windowRect);
#else
		SetPort((GrafPtr)currentSIPtr->infoWindow);
		windowRect = currentSIPtr->infoWindow->portRect;
#endif
		typeRect.right = windowRect.right;
		typeRect.left = typeRect.right - 15;
		typeRect.top = ((windowRect.bottom - windowRect.top) - 75)/2;
		typeRect.bottom = typeRect.top + 75;

		if(processEnabled == NO_PROCESS)
			EraseRect(&typeRect);
		else
			DrawPicture(thePicture,&typeRect);
		ReleaseResource((Handle)thePicture);
		previousSIPtr = currentSIPtr;
		currentSIPtr = (SoundInfoPtr)previousSIPtr->nextSIPtr;
	}
}