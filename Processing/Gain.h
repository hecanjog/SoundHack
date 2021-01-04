/* For Gain Change Dialog Box */
#define	G_DB_RADIO			18
#define	G_FIXED_RADIO		19
#define	PEAK_VAL_1_FIELD	6
#define	PEAK_VAL_2_FIELD	9
#define	PEAK_POS_1_FIELD	7
#define	PEAK_POS_2_FIELD	10
#define	RMS_VAL_1_FIELD		8
#define	RMS_VAL_2_FIELD		11
#define OFFSET_1_FIELD		21
#define	OFFSET_2_FIELD		22
#define ADD_OFFSET_1_FIELD	24
#define	ADD_OFFSET_2_FIELD	25
#define GAIN_FACT_1_FIELD	16
#define	GAIN_FACT_2_FIELD	17
#define	G_ANALYZE_BUTTON	12
#define	G_CANCEL_BUTTON		2
#define	G_PROCESS_BUTTON	1

/*Function prototypes for Gain.c*/
void	HandleGainDialog(void);
void	HandleGainDialogEvent(short itemHit);
void	ShowGainDB(void);
void	ShowGainFixed(void);
short		InitGainProcess(double gainL, double gainR);
short		GainBlock(void);
void	GainAnalyze(void);

typedef struct
{
	double	peakCh1;
	double	peakCh2;
	long	peakPosCh1;
	long	peakPosCh2;
	double	rmsCh1;
	double	rmsCh2;
	double	offsetCh1;
	double	offsetCh2;
	double	addOffsetCh1;
	double	addOffsetCh2;
	double	factor1;
	double	factor2;
}	GainInfo;
