#include "Cursors.h"
extern ProcessSerialNumber	gPSN;

CCrsrHandle	gSmileyCursor, gACursor, gBCursor, gCCursor, gDCursor, gECursor, gFCursor, gGCursor, gHCursor;

void
InitCursors(void)
{
	gSmileyCursor = GetCCursor(128);
	gACursor = GetCCursor(129);
	gBCursor = GetCCursor(130);
	gCCursor = GetCCursor(131);
	gDCursor = GetCCursor(132);
	gECursor = GetCCursor(133);
	gFCursor = GetCCursor(134);
	gGCursor = GetCCursor(135);
	gHCursor = GetCCursor(136);
}

void
ShowSmiley(void)
{
	SetCCursor(gSmileyCursor);
}

void
SpinWart(void)
{
	static short n = 1;
	ProcessSerialNumber PSN;
	
	GetFrontProcess(&PSN);
	if(gPSN.highLongOfPSN != PSN.highLongOfPSN || gPSN.lowLongOfPSN != PSN.lowLongOfPSN)
		return;
	
	if(n > 8 || n < 1)
		n = 1;
	switch(n)
	{
		case 1:
			SetCCursor(gACursor);
			break;
		case 2:
			SetCCursor(gBCursor);
			break;
		case 3:
			SetCCursor(gCCursor);
			break;
		case 4:
			SetCCursor(gDCursor);
			break;
		case 5:
			SetCCursor(gECursor);
			break;
		case 6:
			SetCCursor(gFCursor);
			break;
		case 7:
			SetCCursor(gGCursor);
			break;
		case 8:
			SetCCursor(gHCursor);
			break;
	}
	n++;
}
