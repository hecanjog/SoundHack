/* For Phase & TimeNorm Dialog Display */
#define VALUE_FIELD			3
#define VALUE_PROMPT_FIELD	2
#define VALUE_OK_BUTTON		1
#define	VALUE_CANCEL_BUTTON	4

/* For DrawFunction Dialog Display */
#define	DF_DRAW_PICT		2
#define	DF_TOPRANGE_FIELD	3
#define	DF_BOTRANGE_FIELD	4
#define DF_READ_BUTTON		5
#define DF_TRI_BUTTON		6
#define DF_RAMP_BUTTON		7
#define	DF_SINE_BUTTON		8
#define DF_SCALE_TEXT		9
#define	DF_SCALE_LABEL		10
#define	DF_TIME_TEXT		11
#define	DF_TIME_LABEL		12
#define	DF_INVERT_BUTTON	13
#define	DF_AVG_BUTTON		14
#define	DF_CYCLE_FIELD		15
#define	DF_REVERSE_BUTTON	17
#define DF_SET_BUTTON		18
#define DF_PHASE_BUTTON		19
#define DF_WRITE_BUTTON		20
#define DF_SAW_BUTTON		21
#define DF_TRIFULL_BUTTON	22
#define DF_SQUARE_BUTTON	23
#define DF_NORM_BUTTON		24
#define DF_SETMEAN_BUTTON	25
#define	DF_RAND_BUTTON		28
#define	DF_DONE_BUTTON		1


/*Function prototypes for DrawFunction.c*/
float	InterpFunctionValue(long inPosition,Boolean wrap);
void	HandleDrawFunctionDialog(float topRangeMax, float botRangeMax, float percentRange, Str255 labelStr, float timeRange, Boolean wrap);
void	HandleDrawFunctionDialogEvent(short itemHit);
short		DrawFunction(void);
void	DrawTrackMouse(void);
void	HandleValueDialog(Str255 promptString, Str255 *valueString);

