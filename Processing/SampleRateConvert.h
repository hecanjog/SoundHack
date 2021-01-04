/* For Varispeed Dialog Display */
#define V_BEST_RADIO		4
#define V_MEDIUM_RADIO		5
#define V_FAST_RADIO		6
#define V_RATE_FIELD		8
#define	V_VARI_BOX			9
#define	V_VARI_BUTTON		10
#define	V_PITCH_RADIO		11
#define	V_SCALE_RADIO		12
#define	V_CANCEL_BUTTON		2
#define	V_PROCESS_BUTTON	1

#define BUFFERSIZE 1024L
#define HALFBUFFERSIZE 512L

/*Function prototypes for SampleRateConvert.c*/
void	HandleSRConvDialog(void);
void	HandleSRConvDialogEvent(short itemHit);
short	InitSRConvProcess(void);
short	SRConvBlock(void);
void	DeAllocSRConvMem(void);

typedef struct
{
	short		curSampInBuf;
	short		internalSRFactor;
	short		halfSampWinSize;
	short		maxCurSamp;
	short		variSpeed;
	short		offset;
	short		pitchScale;
	short		quality;
	long		numberSamplesIn;
	float		meanScale;
	double		newSRate;
	float		index;
	float		increment;
} SRCInfo;
