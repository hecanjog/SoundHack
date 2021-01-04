/* For Dynamics Dialog display */
#define EX_BANDS_ITEM		3
#define	EX_FRAMES_ITEM		4
#define	EX_RATE_TEXT		6
#define	EX_TRANS_DEV_FIELD	7
#define	EX_STEAD_DEV_FIELD	14
#define	EX_CANCEL_BUTTON	2
#define	EX_PROCESS_BUTTON	1

/*Function prototypes for Dynamics.c*/
void	HandleExtractDialog(void);
void	HandleExtractDialogEvent(short itemHit);
short		InitExtractProcess(void);
short		ExtractBlock(void);
void	SpectralExtract(float polarSpectrum[], float steadSpectrum[], float transSpectrum[], float spectralFrames[]);
void	SetExtractDefaults(void);
void	DeAllocExtractMem(void);

/*
 * ExtractInfo - structure typedef for spectral rate extraction processing information
 */
 
typedef struct
{
	long	frames;
	float	transDev;
	float	steadDev;
} ExtractInfo;