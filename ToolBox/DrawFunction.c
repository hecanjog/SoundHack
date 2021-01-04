#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>

//#include <AppleEvents.h>
#include "SoundFile.h"
#include "OpenSoundFile.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "SoundHack.h"
#include "DrawFunction.h"
#include "Misc.h"
#include "Dialog.h"

#define	NIL_MOUSE_REGION	0L

#define	DRAW_TOP	0
#define	DRAW_BOTTOM	274
#define	DRAW_LEFT	0
#define	DRAW_RIGHT	400

extern DialogPtr	gDrawFunctionDialog;
extern SoundInfoPtr	inSIPtr, frontSIPtr;
extern EventRecord	gTheEvent;
extern float		twoPi, Pi;
extern float		gScale, gScaleL, gScaleR, gScaleDivisor;
extern short		gProcessEnabled;
extern FSSpec		nilFSSpec;
extern struct
{
	Str255	editorName;
	long	editorID;
	short	openPlay;
	short	procPlay;
	short	defaultType;
	short	defaultFormat;
}	gPreferences;

float gTimeRange;
extern float *gFunction;
DialogPtr	gValueDialog;
SoundInfoPtr	funcSIPtr;
struct
{
	double	topRange;
	double	botRange;
	double	scale;
	double	cycle;
	double	zeroPoint;
	float	topRangeMax;
	float	botRangeMax;
	Boolean	wrap;
	Boolean initialized;
}	gDFI;

void
HandleDrawFunctionDialog(float topRangeMax, float botRangeMax, float percentRange, Str255 labelStr, float timeRange, Boolean wrap)
{
	short	itemType;
	Handle	itemHandle;
	Rect	itemRect;
	Str255	topRangeStr, botRangeStr, errStr;
	
	/* Initialize variables */
	funcSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gDrawFunctionDialog));
#else
	SetPort((GrafPtr)gDrawFunctionDialog);
#endif
   	gTimeRange = timeRange;
   	gDFI.wrap = wrap;
	gDFI.initialized = FALSE;
	gDFI.topRange = topRangeMax * percentRange;
	gDFI.topRangeMax = topRangeMax;
	gDFI.botRange = botRangeMax / percentRange;
	gDFI.botRangeMax = botRangeMax;
	gDFI.scale = gDFI.topRange - gDFI.botRange;
	gDFI.zeroPoint = gDFI.scale/2.0 + gDFI.botRange;
	gDFI.cycle = 1.0;
	FixToString(gDFI.topRange, topRangeStr);
	FixToString(gDFI.botRange, botRangeStr);
	FixToString(gDFI.cycle, errStr);

/*	Initialize Dialog */
	
#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gDrawFunctionDialog));
	SelectWindow(GetDialogWindow(gDrawFunctionDialog));
#else
	SetPort((GrafPtr)gDrawFunctionDialog);
	SelectWindow(gDrawFunctionDialog);
#endif
	GetDialogItem(gDrawFunctionDialog, DF_TOPRANGE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, topRangeStr);
	GetDialogItem(gDrawFunctionDialog, DF_BOTRANGE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, botRangeStr);
	GetDialogItem(gDrawFunctionDialog, DF_CYCLE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, errStr);
	GetDialogItem(gDrawFunctionDialog, DF_SCALE_LABEL, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, labelStr);
#if TARGET_API_MAC_CARBON == 1
	ShowWindow(GetDialogWindow(gDrawFunctionDialog));
#else
	ShowWindow(gDrawFunctionDialog);
#endif
	gProcessEnabled = DRAW_PROCESS;
}

void	
HandleDrawFunctionDialogEvent(short itemHit)
{
	double	vPosition, vPositionOld, hPosition, hPositionOld, tmpFloat, tmpFloat2;
	float	tmpFunction[400];
	short		drawDone=FALSE, mouseLocOldH = 0, mouseLocOldV = 0;
	long	i, j, shift;
	short	itemType;
			
	Boolean up = TRUE;
	Handle	itemHandle;
	Point	mouseLoc;
	Rect	itemRect;
	Str255	scaleStr, timeStr, topRangeStr, botRangeStr, errStr;
	
	SoundInfoPtr	oldFrontSIPtr;

#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gDrawFunctionDialog));
	SelectWindow(GetDialogWindow(gDrawFunctionDialog));
#else
	SetPort((GrafPtr)gDrawFunctionDialog);
	SelectWindow(gDrawFunctionDialog);
#endif
	DrawFunction();
	switch(itemHit)
	{
		case DF_DONE_BUTTON:
#if TARGET_API_MAC_CARBON == 1
			HideWindow(GetDialogWindow(gDrawFunctionDialog));
#else
			HideWindow(gDrawFunctionDialog);
#endif
			gProcessEnabled = NO_PROCESS;
			break;
		case DF_TOPRANGE_FIELD:
			GetDialogItem(gDrawFunctionDialog,DF_TOPRANGE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle, topRangeStr);
			StringToFix(topRangeStr, &gDFI.topRange);
			if(gDFI.topRange > gDFI.topRangeMax || gDFI.topRange < gDFI.botRange)
			{
				gDFI.topRange = gDFI.topRangeMax;
				FixToString(gDFI.topRange, topRangeStr);
				SetDialogItemText(itemHandle, topRangeStr);
			}
			gDFI.scale = gDFI.topRange - gDFI.botRange;
			gDFI.zeroPoint = gDFI.scale/2.0 + gDFI.botRange;
			DrawFunction();
			break;
		case DF_BOTRANGE_FIELD:
			GetDialogItem(gDrawFunctionDialog,DF_BOTRANGE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle,botRangeStr);
			StringToFix(botRangeStr, &gDFI.botRange);
			if(gDFI.botRange < gDFI.botRangeMax || gDFI.botRange > gDFI.topRange)
			{
				gDFI.botRange = gDFI.botRangeMax;
				FixToString(gDFI.botRange, botRangeStr);
				SetDialogItemText(itemHandle, botRangeStr);
			}
			gDFI.scale = gDFI.topRange - gDFI.botRange;
			gDFI.zeroPoint = gDFI.scale/2.0 + gDFI.botRange;
			DrawFunction();
			break;
		case DF_CYCLE_FIELD:
			GetDialogItem(gDrawFunctionDialog,DF_CYCLE_FIELD, &itemType, &itemHandle, &itemRect);
			GetDialogItemText(itemHandle,errStr);
			StringToFix(errStr, &gDFI.cycle);
			break;
		case DF_READ_BUTTON:
			oldFrontSIPtr = frontSIPtr;
			if(OpenSoundFile(nilFSSpec, FALSE) != -1)
			{
				funcSIPtr = frontSIPtr;
				frontSIPtr = oldFrontSIPtr;
				if(funcSIPtr->nChans != MONO)
				{
					DrawErrorMessage("\pUse only Monaural SoundFiles");
					break;
				}
				SetFPos(funcSIPtr->dFileNum, fsFromStart, funcSIPtr->dataStart);
				ReadMonoBlock(funcSIPtr, 400L, gFunction);
				if(funcSIPtr->packMode != TEXT)
					for(i=0; i<400; i++)
						gFunction[i] = (((gFunction[i] + 1.0) * 0.5 * gDFI.scale) + gDFI.botRange);
				DrawFunction();
				gDFI.initialized = TRUE;
			}
#if TARGET_API_MAC_CARBON == 1
			SelectWindow(GetDialogWindow(gDrawFunctionDialog));
#else
			SelectWindow(gDrawFunctionDialog);
#endif
			break;				
		case DF_WRITE_BUTTON:
			funcSIPtr = nil;
			funcSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
			if(gPreferences.defaultType == 0)
			{
				funcSIPtr->sfType = AIFF;
				funcSIPtr->packMode = SF_FORMAT_16_LINEAR;
			}
			else
			{
				funcSIPtr->sfType = gPreferences.defaultType;
				funcSIPtr->packMode = gPreferences.defaultFormat;
			}
			NameFile(inSIPtr->sfSpec.name, "\pFUNC", funcSIPtr->sfSpec.name);
			funcSIPtr->sRate = inSIPtr->sRate;
			funcSIPtr->nChans = 1;
			funcSIPtr->numBytes = 0;
			if(CreateSoundFile(&funcSIPtr, SOUND_CUST_DIALOG) == -1)
			{
				DrawFunction();
				break;
			}
			WriteHeader(funcSIPtr);
			UpdateInfoWindow(funcSIPtr);
			for(i = 0; i < 400; i++)
				tmpFunction[i] = (gFunction[i] + gDFI.botRange)/(gDFI.scale/2.0) - 1.0;
			gScaleDivisor = 1.0;			
			SetOutputScale(funcSIPtr->packMode);
			SetFPos(funcSIPtr->dFileNum, fsFromStart, funcSIPtr->dataStart);
			WriteMonoBlock(funcSIPtr, 400L, tmpFunction);
			WriteHeader(funcSIPtr);
			UpdateInfoWindow(funcSIPtr);
			DrawFunction();
			gDFI.initialized = TRUE;
#if TARGET_API_MAC_CARBON == 1
			SelectWindow(GetDialogWindow(gDrawFunctionDialog));
#else
			SelectWindow(gDrawFunctionDialog);
#endif
			break;				
		case DF_RAMP_BUTTON:
			for(i = 0; i < 400; i++)
			{
				tmpFloat = (i*gDFI.cycle)/800.0;
				while(tmpFloat>.5)
					tmpFloat = tmpFloat - 0.5;
				if(gDFI.initialized)
					gFunction[i] = (gFunction[i] - gDFI.zeroPoint)*(1.0 - 2.0*tmpFloat) + gDFI.zeroPoint;
				else
					gFunction[i] = (((1.0 - tmpFloat) * gDFI.scale) + gDFI.botRange);
			}
			DrawFunction();
			gDFI.initialized = TRUE;
			break;				
		case DF_SAW_BUTTON:
			for(i = 0; i < 400; i++)
			{
				tmpFloat = (i*gDFI.cycle)/400.0;
				while(tmpFloat>1.0)
					tmpFloat = tmpFloat - 1.0;
				if(gDFI.initialized)
					gFunction[i] = (gFunction[i]-gDFI.zeroPoint)*(1.0 - 2.0*tmpFloat)+gDFI.zeroPoint;
				else
					gFunction[i] = (((1.0 - tmpFloat) * gDFI.scale) + gDFI.botRange);
			}
			DrawFunction();
			gDFI.initialized = TRUE;
			break;				
		case DF_SQUARE_BUTTON:
			tmpFloat = 1.0;
			tmpFloat2 = 0.0;
			for(i = 0; i < 400; i++)
			{
				tmpFloat2 += gDFI.cycle/400.0;
				if(tmpFloat2 > 0.5)
				{
					tmpFloat2 -= 0.5;
					tmpFloat *= -1.0;
				}
				if(gDFI.initialized)
					gFunction[i] = (gFunction[i]-gDFI.zeroPoint)*tmpFloat+gDFI.zeroPoint;
				else
					gFunction[i] = (((tmpFloat+1.0)/2.0) * gDFI.scale) + gDFI.botRange;
			}
			DrawFunction();
			gDFI.initialized = TRUE;
			break;				
		case DF_TRI_BUTTON:
			up = TRUE;
			tmpFloat = 0.5;
			for(i = 0; i < 400; i++)
			{
				if(up)
				{
					tmpFloat = tmpFloat + gDFI.cycle/400.0;
					if(tmpFloat>1.0)
					{
						tmpFloat = 2.0 - tmpFloat;
						up = FALSE;
					}
				}
				else
				{
					tmpFloat = tmpFloat - gDFI.cycle/400.0;
					if(tmpFloat<0.5)
					{
						tmpFloat = 1.0 - tmpFloat;
						up = TRUE;
					}
				}
				if(gDFI.initialized)
					gFunction[i] = (gFunction[i] - gDFI.zeroPoint)*(2.0 * (tmpFloat-0.5)) + gDFI.zeroPoint;
				else
					gFunction[i] = ((tmpFloat * gDFI.scale) + gDFI.botRange);
			}
			DrawFunction();
			gDFI.initialized = TRUE;
			break;				
		case DF_TRIFULL_BUTTON:
			up = TRUE;
			tmpFloat = 0.0;
			for(i = 0; i < 400; i++)
			{
				if(up)
				{
					tmpFloat = tmpFloat + gDFI.cycle/200.0;
					if(tmpFloat>1.0)
					{
						tmpFloat = 2.0 - tmpFloat;
						up = FALSE;
					}
				}
				else
				{
					tmpFloat = tmpFloat - gDFI.cycle/200.0;
					if(tmpFloat<0.0)
					{
						tmpFloat = 0.0 - tmpFloat;
						up = TRUE;
					}
				}
				if(gDFI.initialized)
					gFunction[i] = (gFunction[i]-gDFI.zeroPoint)*((2.0 * tmpFloat) - 1.0)+gDFI.zeroPoint;
				else
					gFunction[i] = ((tmpFloat * gDFI.scale) + gDFI.botRange);
			}
			DrawFunction();
			gDFI.initialized = TRUE;
			break;				
		case DF_SINE_BUTTON:
			for(i = 0; i < 400; i++)
			{
				if(gDFI.initialized)
					gFunction[i] = (gFunction[i] - gDFI.zeroPoint)*sin(twoPi*(i*gDFI.cycle)/400.0) + gDFI.zeroPoint;
				else
					gFunction[i] = (((sin(twoPi*(i*gDFI.cycle)/400.0)+1.0)/2.0 * gDFI.scale) + gDFI.botRange);
			}
			DrawFunction();
			gDFI.initialized = TRUE;
			break;
		case DF_RAND_BUTTON:
			for(i = 0; i < 400; i++)
			{
				tmpFloat = Random()/32767.0;
				if(gDFI.initialized)
					gFunction[i] = (gFunction[i] - gDFI.zeroPoint)*tmpFloat + gDFI.zeroPoint;
				else
					gFunction[i] = (((tmpFloat +1.0)/2.0)* gDFI.scale) + gDFI.botRange;
			}
			DrawFunction();
			gDFI.initialized = TRUE;
			break;
		case DF_SETMEAN_BUTTON:
			tmpFloat = tmpFloat2 = 0.0;
			HandleValueDialog("\pEnter a new mean value", &errStr);
			StringToFix(errStr, &tmpFloat);
			if(tmpFloat == 0.0)
			{
				DrawFunction();
				break;
			}
			for (i=0; i < 400; i++)
				tmpFloat2 += gFunction[i];
			if(tmpFloat2 == 0.0)
			{
				DrawFunction();
				break;
			}
			if (gDFI.botRange >= 0.0)
			{
				tmpFloat2 = (400.0 * tmpFloat)/tmpFloat2;
				for (i=0; i < 400; i++)
					gFunction[i] *= tmpFloat2;
			}
			else
			{
				tmpFloat2 = tmpFloat2/400.0;
				for (i=0; i < 400; i++)
					gFunction[i] = (gFunction[i] - tmpFloat2) + tmpFloat;
			}
			DrawFunction();
			break;
		case DF_PHASE_BUTTON:
			HandleValueDialog("\pEnter a shift value from 0 to 399", &errStr);
			StringToNum(errStr, &shift);
			if(shift == 0)
			{
				DrawFunction();
				break;
			}
			for(i = 0; i < 400; i++)
				tmpFunction[i] = gFunction[i];
			for(i = 0, j = shift; i < 400; i++)
			{
				gFunction[j] = tmpFunction[i];
				if(++j == 400)
					j = 0;
			}
			DrawFunction();
			break;
		case DF_NORM_BUTTON:
		{
			float maximum, minimum, normScale, normOffset;
			maximum = minimum = gFunction[0];
			for(i = 0; i < 400; i++)
			{
				if(gFunction[i] > maximum)
					maximum = gFunction[i];
				if(gFunction[i] < minimum)
					minimum = gFunction[i];
			}
			normScale = gDFI.scale/(maximum - minimum);
			normOffset = (normScale * (maximum + minimum)/2.0) - gDFI.zeroPoint;
			for(i = 0; i < 400; i++)
				gFunction[i] = (gFunction[i] * normScale) - normOffset;
			DrawFunction();
		}
			break;
		case DF_REVERSE_BUTTON:
			for(i = 0; i < 400; i++)
				tmpFunction[i] = gFunction[i];
			for(i = 0; i < 400; i++)
				gFunction[i] = tmpFunction[399 - i];
			DrawFunction();
			break;
		case DF_SET_BUTTON:
			HandleValueDialog("\pSet all table entries to", &errStr);
			StringToFix(errStr, &tmpFloat);
			for(i = 0; i < 400; i++)
				gFunction[i] = tmpFloat;
			DrawFunction();
			gDFI.initialized = FALSE;
			break;				
		case DF_INVERT_BUTTON:
			for(i = 0; i < 400; i++)
				gFunction[i] = -gFunction[i] + (gDFI.topRange + gDFI.botRange);
			DrawFunction();
			break;				
		case DF_AVG_BUTTON:
			for(i = 1; i < 399; i++)
			{
				if(gDFI.wrap == TRUE)
				{
					if((gFunction[i-1]<gFunction[i+1]) && ((gFunction[i-1] - gDFI.botRange + gDFI.topRange - gFunction[i+1]) < (gFunction[i+1] - gFunction[i-1])))
					{
						tmpFloat = gFunction[i-1] + 360.0;
						gFunction[i] = (tmpFloat + gFunction[i+1])/2.0;
						if(gFunction[i] > 360.0)
							gFunction[i] = gFunction[i] - 360.0;
					}
					else if((gFunction[i+1]<gFunction[i-1]) && ((gFunction[i+1] - gDFI.botRange + gDFI.topRange - gFunction[i-1]) < (gFunction[i-1] - gFunction[i+1])))
					{
						tmpFloat = gFunction[i+1] + 360.0;
						gFunction[i] = (gFunction[i-1] + tmpFloat)/2.0;
						if(gFunction[i] > 360.0)
							gFunction[i] = gFunction[i] - 360.0;
					}
					else
						gFunction[i] = (gFunction[i-1] + gFunction[i+1])/2.0;
				}
				else
					gFunction[i] = (gFunction[i-1] + gFunction[i+1])/2.0;
			}
			DrawFunction();
			break;				
		case DF_DRAW_PICT:
			drawDone = FALSE;
			while(drawDone == FALSE)
			{
				WaitNextEvent(everyEvent, &gTheEvent, SLEEP, NIL_MOUSE_REGION);
				switch(gTheEvent.what)
				{
					case mouseUp:
						drawDone = TRUE;
						mouseLocOldH = 0;
						break;
					default:
						GetMouse(&mouseLoc);
						if(mouseLoc.v > DRAW_TOP && mouseLoc.v < DRAW_BOTTOM && 
							mouseLoc.h > DRAW_LEFT && mouseLoc.h < DRAW_RIGHT)
						{
							if(mouseLocOldH == 0)
							{
								mouseLocOldH = mouseLoc.h;
								mouseLocOldV = mouseLoc.v;
							}
							MoveTo(mouseLocOldH,mouseLocOldV);
							LineTo(mouseLoc.h, mouseLoc.v);
							mouseLocOldV = mouseLoc.v;
							vPosition = ((DRAW_BOTTOM - mouseLoc.v) * gDFI.scale)/274.0 + gDFI.botRange;
							hPosition = ((mouseLoc.h - DRAW_LEFT) * gTimeRange)/400.0;
							if(mouseLoc.h < mouseLocOldH)
								for(i = mouseLoc.h; i < mouseLocOldH; i++)
									gFunction[i - DRAW_LEFT] = vPosition 
										+ ((vPositionOld-vPosition)*((float)(i-mouseLoc.h)/(mouseLocOldH-mouseLoc.h)));
							else
								for(i = mouseLocOldH; i < mouseLoc.h; i++)
									gFunction[i - DRAW_LEFT] = vPositionOld 
										+ ((vPosition-vPositionOld)*((float)(i-mouseLocOldH)/(mouseLoc.h-mouseLocOldH)));
							mouseLocOldH = mouseLoc.h;
							if(vPosition != vPositionOld)
							{
								vPositionOld = vPosition;
								FixToString(vPosition,scaleStr);
								GetDialogItem(gDrawFunctionDialog, DF_SCALE_TEXT, &itemType, &itemHandle, &itemRect);
								SetDialogItemText(itemHandle, scaleStr);
							}
							if(hPosition != hPositionOld)
							{
								hPositionOld = hPosition;
								FixToString(hPosition,timeStr);
								GetDialogItem(gDrawFunctionDialog, DF_TIME_TEXT, &itemType, &itemHandle, &itemRect);
								SetDialogItemText(itemHandle, timeStr);
							}
							/* Big Fudge */
							gFunction[0] = gFunction[1];
							gFunction[399] = gFunction[398];
						}
						break;
				}
			}
			gDFI.initialized = TRUE;
			DrawFunction();
			break;
		default:
			break;
	}
}

short
DrawFunction(void)
{

	short		i, vPos,lastVPos;
	short	itemType;
	Handle	itemHandle;
	Rect drawRect,dialogRect, myRect;

#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gDrawFunctionDialog));
	GetPortBounds(GetDialogPort(gDrawFunctionDialog), &dialogRect);
#else
	SetPort(gDrawFunctionDialog);
	dialogRect = gDrawFunctionDialog->portRect;
#endif
	GetDialogItem(gDrawFunctionDialog, DF_DRAW_PICT, &itemType, &itemHandle, &drawRect);
	FrameRect(&drawRect);
	myRect.top = drawRect.top + 1;
	myRect.bottom = drawRect.bottom - 1;
	myRect.left = drawRect.left + 1;
	myRect.right = drawRect.right - 1;
	EraseRect(&myRect);
	ClipRect(&myRect);
	vPos = 279 - (short)(274.0*(gFunction[0]-gDFI.botRange)/gDFI.scale);
	MoveTo(0, vPos);
	for(i=0; i<400; i++)
	{
		lastVPos = vPos;
		vPos = 274 - (short)(274.0*(gFunction[i] - gDFI.botRange)/gDFI.scale);
		if(gDFI.wrap == TRUE)
		{
			if((vPos < lastVPos) && ((vPos - myRect.top + myRect.bottom - lastVPos) < (lastVPos - vPos)))
			{
				LineTo(i,myRect.bottom);
				MoveTo(i,myRect.top);
				LineTo(i,vPos);
			}
			else if ((vPos > lastVPos) && ((lastVPos - myRect.top + myRect.bottom - vPos) < (vPos - lastVPos)))
			{
				LineTo(i,myRect.top);
				MoveTo(i,myRect.bottom);
				LineTo(i,vPos);
			}
			else
				LineTo(i,vPos);
		}
		else
			LineTo(i,vPos);
	}
	ShowDialogItem(gDrawFunctionDialog, DF_DRAW_PICT);
	ClipRect(&dialogRect);
	return(1);
}

void
DrawTrackMouse(void)
{
	short	itemType;
	Handle	itemHandle;
	Str255	topRangeStr, botRangeStr, scaleStr, timeStr;
	double	topRange, botRange, scale, vPosition, 
			hPosition;
	static	double	hPositionOld, vPositionOld; 
	Rect	itemRect;
	Point	mouseLoc;
	
	GetDialogItem(gDrawFunctionDialog,DF_TOPRANGE_FIELD, &itemType, &itemHandle, &itemRect);
	GetDialogItemText(itemHandle, topRangeStr);
	StringToFix(topRangeStr, &topRange);
	GetDialogItem(gDrawFunctionDialog,DF_BOTRANGE_FIELD, &itemType, &itemHandle, &itemRect);
	GetDialogItemText(itemHandle,botRangeStr);
	StringToFix(botRangeStr, &botRange);
	scale = topRange - botRange;

#if TARGET_API_MAC_CARBON == 1
	SetPort(GetDialogPort(gDrawFunctionDialog));
#else
	SetPort(gDrawFunctionDialog);
#endif
	GetMouse(&mouseLoc);
	if(mouseLoc.v > DRAW_TOP && mouseLoc.v < DRAW_BOTTOM && 
			mouseLoc.h > DRAW_LEFT && mouseLoc.h < DRAW_RIGHT)
	{
		vPosition = ((DRAW_BOTTOM - mouseLoc.v) * scale)/270.0 + botRange;
		hPosition = ((mouseLoc.h - DRAW_LEFT) * gTimeRange)/400.0;
		if(vPosition != vPositionOld)
		{
			vPositionOld = vPosition;
			FixToString(vPosition,scaleStr);
			GetDialogItem(gDrawFunctionDialog, DF_SCALE_TEXT, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, scaleStr);
		}
		if(hPosition != hPositionOld)
		{
			hPositionOld = hPosition;
			HMSTimeString(hPosition,timeStr);
			GetDialogItem(gDrawFunctionDialog, DF_TIME_TEXT, &itemType, &itemHandle, &itemRect);
			SetDialogItemText(itemHandle, timeStr);
		}
	}
}

void	HandleValueDialog(Str255 promptString, Str255 *valueString)
{
	short	itemType;
	Handle	itemHandle;
	Rect	itemRect;
	short	itemHit, dialogDone = FALSE;
	
	gValueDialog = GetNewDialog(VALUE_DLOG,NIL_POINTER,(WindowPtr)MOVE_TO_FRONT);
	
	GetDialogItem(gValueDialog, VALUE_PROMPT_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, promptString);
	GetDialogItem(gValueDialog, VALUE_FIELD, &itemType, &itemHandle, &itemRect);
	SetDialogItemText(itemHandle, "\p0.0");
#if TARGET_API_MAC_CARBON == 1
	SelectWindow(GetDialogWindow(gValueDialog));
	DrawDialog(gValueDialog);
	ShowWindow(GetDialogWindow(gValueDialog));
#else
	SelectWindow(gValueDialog);
	DrawDialog(gValueDialog);
	ShowWindow(gValueDialog);
#endif
	SetDialogCancelItem(gValueDialog, VALUE_CANCEL_BUTTON);
	while(dialogDone == FALSE)
	{
		ModalDialog(NIL_POINTER,&itemHit);
		switch(itemHit)
		{
			case VALUE_OK_BUTTON:
				GetDialogItem(gValueDialog, VALUE_FIELD, &itemType, &itemHandle, &itemRect);
				GetDialogItemText(itemHandle, (unsigned char *)valueString);
				DisposeDialog(gValueDialog);
				gValueDialog = nil;
				dialogDone = TRUE;
				break;
			case VALUE_CANCEL_BUTTON:
				GetDialogItem(gValueDialog, VALUE_FIELD, &itemType, &itemHandle, &itemRect);
				SetDialogItemText(itemHandle, "\p0.0");
				GetDialogItemText(itemHandle, (unsigned char *)valueString);
				DisposeDialog(gValueDialog);
				gValueDialog = nil;
				dialogDone = TRUE;
				break;
			case VALUE_FIELD:
				break;
			default:
				break;
		}
	}
}

// The only case in which wrapping is enabled is binaural processsing, so I am hard wiring this
// function to use a topRange of 360 and a bottom range of 0
float
InterpFunctionValue(long inPosition, Boolean wrap)
{
	long intIndex;
	float modIndex, funcIndex, interpValue, topRange = 360.0, botRange = 0.0, tmpValue;
	
	funcIndex = ((float)inPosition * 399.0)/inSIPtr->numBytes;
	if(funcIndex < 0.0)
		return(gFunction[0]);
	if(funcIndex >= 399.0)
		return(gFunction[399]);
	intIndex = (long)funcIndex;
	modIndex = funcIndex - (float)intIndex;
	
	if(wrap == TRUE)
	{
		if((gFunction[intIndex]<gFunction[intIndex+1]) && 
		((gFunction[intIndex] - botRange + topRange - gFunction[intIndex+1]) < (gFunction[intIndex+1] - gFunction[intIndex])))
		{
			tmpValue = gFunction[intIndex] + 360.0;
			interpValue = tmpValue * (1.0-modIndex) + modIndex * gFunction[intIndex+1];
			if(interpValue > 360.0)
				interpValue = interpValue - 360.0;
		}
		else if((gFunction[intIndex+1]<gFunction[intIndex]) && 
		((gFunction[intIndex+1] - botRange + topRange - gFunction[intIndex]) < (gFunction[intIndex] - gFunction[intIndex+1])))
		{
			tmpValue = gFunction[intIndex+1] + 360.0;
			interpValue = gFunction[intIndex] * (1.0-modIndex) + modIndex * tmpValue;
			if(interpValue > 360.0)
				interpValue = interpValue - 360.0;
		}
		else
			interpValue = gFunction[intIndex] * (1.0-modIndex) + modIndex * gFunction[intIndex+1];
	}
	else
		interpValue = gFunction[intIndex] * (1.0-modIndex) + modIndex * gFunction[intIndex+1];
	return(interpValue);
}

