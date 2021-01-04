/* For Analysis Dialog Display */
#define A_BANDS_MENU		3
#define	A_FREQ_FIELD		5
#define A_WINDOW_MENU		6
#define A_OVERLAP_MENU		7
#define A_TYPE_MENU			8
#define A_CANCEL_BUTTON		2
#define A_PROCESS_BUTTON	1

/* For Sound<>PICT Dialog Display */
#define SP_BANDS_MENU		3
#define	SP_FREQ_FIELD		5
#define SP_WINDOW_MENU		6
#define SP_COLOR_MENU		9
#define SP_INVERT_BOX		10
#define SP_MOVING_BOX		14
#define SP_LINES_SLIDER		11
#define SP_LINES_FIELD		12
#define SP_AMPHUE_RADIO		15
#define SP_PHASEHUE_RADIO	16
#define	SP_AMPLITUDE_FIELD	8
#define SP_CANCEL_BUTTON	2
#define SP_PROCESS_BUTTON	1

// Analysis types
#define	NO_ANALYSIS			0
#define	CSOUND_ANALYSIS		1
#define	SOUNDHACK_ANALYSIS	2
#define SDIF_ANALYSIS		3
#define	PICT_ANALYSIS		4

#define	RED_MENU			1
#define GREEN_MENU			2
#define BLUE_MENU			3

typedef struct
{
	long	centerColor;
	double	amplitudeRange;
	long	linesPerFFT;
	long	type;
	Boolean	inversion;
	Boolean	moving;
}	PictCoderInfo;

void	SynthAnalDialogInit(void);
void	SynthAnalDialogEvent(short itemHit);
void	SoundPICTDialogInit(void);
void	SoundPICTDialogEvent(short itemHit);
short	CreateSpectDispWindow(SoundDisp *newSpectDisp, short left, Str255 windowStr);
void 	UpdateSpectDispWindow(SoundDisp *newSpectDisp);

short	InitAnalProcess(void);
short	AnalBlock(void);
long	WriteSpectLine(SoundInfo *mySI, float polarSpectrum[], float lastPhaseIn[], SoundDisp *spectDisp, long halfLengthFFT);
void	WriteSpectPICT(SoundInfo *mySI, SoundDisp *spectDisp);
long	WriteSpectFrame(SoundInfo *mySI, SoundDisp *spectDisp);
RGBColor	AmpPhase2RGB(double logAmp, double phaseDifference);
RGBColor	PhaseAmp2RGB(double logAmp, double phaseDifference);

void	DeAllocAnalMem(void);
short	InitSynthProcess(void);
short	SynthBlock(void);
long	ReadSpectLine(SoundInfo *mySI, float polarSpectrum[], float lastPhaseIn[], long halfLengthFFT);
long	ReadSpectPICT(SoundInfo *mySI, SoundDisp *spectDisp);
void	RGB2AmpPhase(RGBColor pixelRGB, double *logAmp, double *phaseDifference);
void	RGB2PhaseAmp(RGBColor pixelRGB, double *logAmp, double *phaseDifference);

