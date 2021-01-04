/* For Convolve Dialog Display */
#define	F_FILTLENGTH_FIELD	4
#define F_MEMORY_FIELD		7
#define	F_LOW_RADIO			9
#define	F_MED_RADIO			11
#define	F_HIGH_RADIO		12
#define	F_WINDOW_MENU		15
#define F_RING_BOX			14
#define F_BRIGHTEN_BOX		16
#define F_MOVING_BOX		13
#define F_NORM_BOX			8
#define	F_CANCEL_BUTTON		2
#define	F_FILTOPEN_BUTTON	5
#define	F_PROCESS_BUTTON 	1

/*Function prototypes for Convolve.c*/
void	HandleFIRDialog(void);
void	HandleFIRDialogEvent(short itemHit);
short		InitFIRProcess(void);
short		ConvolveBlock(void);
short		AllocConvMem(void);
void	DeAllocConvMem(void);
void	BrightenFFT(float cartSpectrum[], long halfSizeFFT);
typedef struct
{
	long	sizeImpulse;
	long	sizeConvolution;
	long	sizeFFT;
	long	halfSizeFFT;
	long	incWin;
	long	windowType;
	long	binPosition;
	long	binHeight;
	short	moving;
	short	ringMod;
	short	windowImpulse;
	short	binaural;
	short	brighten;
} ConvolveInfo;
