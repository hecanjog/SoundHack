#ifdef mc68881
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* mc68881 */
#include <math.h>

/* For Pvoc Dialog Display */
#define P_BANDS_MENU		3
#define	P_FREQ_FIELD		10
#define P_WINDOW_MENU		13
#define P_OVERLAP_MENU		4
#define	P_SCALE_MENU		5
#define P_SCALE_FIELD		6
#define P_TIME_RADIO		7
#define P_PITCH_RADIO		8
#define P_DRAW_BOX			11
#define P_DRAW_BUTTON		12
#define P_GATE_BOX			18
#define P_MINAMP_TEXT		14
#define P_MINAMP_FIELD		16
#define P_RATIO_TEXT		15
#define P_RATIO_FIELD		17
#define P_ANALYSIS_RATE_FIELD	21
#define P_SYNTHESIS_RATE_FIELD	22
#define P_CANCEL_BUTTON		2
#define P_PROCESS_BUTTON	1

typedef struct
{
	long	points;
	long	halfPoints;
	long	windowSize;
	long	decimation;
	long	interpolation;
	long	windowType;
	Boolean	phaseLocking;
	double	scaleFactor;
	double	maskRatio;
	double	minAmplitude;
	short		analysisType;
	short		time;
	Boolean	useFunction;
} PvocInfo;

/*Function prototypes for PhaseVocoder.c*/
void	HandlePvocDialog(void);
void	HandlePvocDialogEvent(short itemHit);
void	SetPvocDefaults(void);
short	InitPvocProcess(void);
short	PvocBlock(void);
float	FindBestRatio(void);
void	SimpleSpectralGate(float polarSpectrum[]);
void	PhaseInterpolate(float polarSpectrum[], float lastPhaseIn[], float lastPhaseOut[]);
void	DeAllocPvocMem(void);

/*Function prototypes for PhaseVocoderRoutines.c*/
short	ScaleWindows(float analysisWindow[], float synthesisWindow[], PvocInfo myPI);
long	ShiftIn(SoundInfo *mySI, float leftBlock[], float rightBlock[], long windowSize, long decimation, long *validSamples);
void	WindowFold(float input[], float window[], float output[], long currentTime, long points, long windowSize);
void 	CartToPolar(float spectrum[], float polarSpectrum[], long halfLengthFFT);
void	PolarToCart(float polarSpectrum[], float spectrum[], long halfLengthFFT);
void	AddSynth(float polarSpectrum[], float output[], float lastAmp[], float lastFreq[], float lastPhase[], float sineIndex[], float sineTable[], PvocInfo myPI);
void	OverlapAdd(float input[], float synthesisWindow[], float output[], long currentTime, long points, long windowSize);
long	ShiftOut(SoundInfo *mySI, float leftBlock[], float rightBlock[], long currentTime, long interpolation, long windowSize);

