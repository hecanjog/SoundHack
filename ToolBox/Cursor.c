#include "Cursors.h"

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
