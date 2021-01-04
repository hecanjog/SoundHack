/* For Binaural Dialog Display */
#define B_0_RADIO			3
#define B_30_RADIO			14
#define B_60_RADIO			13
#define B_90_RADIO			12
#define B_120_RADIO			11
#define B_150_RADIO			10
#define B_180_RADIO			9
#define B_210_RADIO			8
#define B_240_RADIO			7
#define B_270_RADIO			6
#define B_300_RADIO			5
#define B_330_RADIO			4
#define B_DEGREE_FIELD		16
#define	B_MOVE_BOX			18
#define	B_MOVE_BUTTON		19
#define	B_HEIGHT_CNTL		20
#define B_CANCEL_BUTTON		2
#define B_PROCESS_BUTTON	1

#define	ELEVATED	0
#define	EAR_LEVEL	1
#define	LOWERED		2

/*Function prototypes for Binaural.c*/
void	HandleBinauralDialog(void);
void	HandleBinauralDialogEvent(short itemHit);
void	SetBinauralButtons(long position);
void	BinauralButtonsOff(void);
short		InitBinauralProcess(long position);
short		NewImpulse(float position);
void	CopyImpulse(float source[], float target[], long delay);
void	MixImpulse(float sourceA[], float sourceB[], float target[], float percentB, long delayA, long delayB);
