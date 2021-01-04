/* For Mutation Dialog Display */
#define M_BANDS_ITEM		8
#define	M_TYPE_ITEM			12
#define	M_OMEGA_FIELD		9
#define	M_FUNC_BOX			10
#define	M_FUNC_BUTTON		11
#define M_BPERSI_TEXT		18
#define M_BPERSI_FIELD		19
#define M_ABSO_BOX			13
#define M_SAV_TEXT			14
#define M_SAV_FIELD			15
#define M_TAV_TEXT			21
#define M_TAV_FIELD			20
#define M_EMPHA_TEXT		16
#define M_EMPHA_FIELD		17
#define	M_SCALE_BOX			6
#define	M_FILE_BUTTON		5
#define	M_FILE_FIELD		3
#define	M_CANCEL_BUTTON		2
#define	M_PROCESS_BUTTON	1

#define	USIM 1
#define ISIM 2
#define IUIM 3
#define UUIM 4
#define	LCM	5
#define	LCMIUIM	6
#define	LCMUUIM	7

/*Function prototypes for Mutate.c*/
void	HandleMutateDialog(void);
void	HandleMutateDialogEvent(short itemHit);
void	SetMutateDefaults(void);
short		InitMutateProcess(void);
short		MutateBlock(void);
void	PickMutateTable(float omega, float persist, long bands, Boolean decisionTable[], Boolean smallDecision[]);
void	DeAllocMutateMem(void);
void	MutateSpectrum(short channel, float sourceSpectrum[], float sourcejSpectrum[], float targetSpectrum[], float targetjSpectrum[], float mutantjSpectrum[], float omega);
float	ISIMutate(float targetDelta);
float	IUIMutate(float sourceDelta, float targetDelta);
float	LCMutate(float sourceDelta, float targetDelta);
float	USIMutate(float sourceDelta, float targetDelta, float omega);
float	UUIMutate(float sourceDelta, float targetDelta, float omega);

typedef struct
{
	long	fDecimation;
	short	type;
	float	omega;
	float	bandPersistence;		/* Only if irregular */
	float	deltaEmphasis;			/* 1 = all delta, 0 = all old stuff */
	float	sourceAbsoluteValue;	/* Only if absolute */
	float	targetAbsoluteValue;	/* Only if absolute */
	Boolean	absolute;
	Boolean	useFunction;
	Boolean	scale;
} MutateInfo;
