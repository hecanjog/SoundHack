/* For Dynamics Dialog display */
#define	D_TYPE_MENU			3
#define D_BANDS_MENU		5
#define D_LOW_BAND_FIELD	17
#define D_LOW_FREQ_TEXT		18
#define D_HIGH_BAND_FIELD	20
#define D_HIGH_FREQ_TEXT	21
#define	D_GAIN_TEXT			7
#define	D_GAIN_FIELD		8
#define D_TIME_FIELD		22
#define D_TIME_TEXT			23
#define	D_BELOW_RADIO		14
#define D_ABOVE_RADIO		15
#define D_RELATIVE_BOX		24
#define D_AVERAGE_BOX		25
#define D_BAND_GROUP_BOX	26
#define	D_THRESH_TEXT		4
#define	D_THRESH_FIELD		6
#define D_FILE_BOX			9
#define D_FILE_TEXT			10
#define	D_FILE_FIELD		11
#define	D_FILE_BUTTON		12
#define	D_CANCEL_BUTTON		2
#define	D_PROCESS_BUTTON	1

/*Function prototypes for Dynamics.c*/
void	HandleDynamicsDialog(void);
void	HandleDynamicsDialogEvent(short itemHit);
short	InitDynamicsProcess(void);
short	SpecGateBlock(void);
void	SpectralGate(float polarSpectrum[], float trigRef[], float curFac[], float spectralFrames[]);
void	SpectralCompand(float polarSpectrum[], float trigRef[], float curFac[], float spectralFrames[]);
void	SetDynamicsDefaults(void);
void	DeAllocSpecGateMem(void);

/*
 * Dynamics.h - structure typedef for dynamic processing information
 */
 
typedef struct
{
	float	factor;
	float	increment;
	float	threshold;
	long	type;
	long	lowBand;
	long	highBand;
	double	time;
	Boolean	below;
	Boolean	useFile;
	Boolean	relative;
	Boolean average;
	Boolean group;
} DynamicsInfo;

#define	GATE		1
#define	EXPAND		2
#define	COMPRESS	3

/* For popup menu */

#define	GATE_ITEM			1
#define	EXPAND_ITEM			2
#define	COMPRESS_ITEM		3
