/* For Marker & Loop Dialog Display */
#define	MK_MARKER_DOWN_BUTTON	3
#define	MK_MARKER_UP_BUTTON		4
#define	MK_MARKER_NUMBER_FIELD	5
#define	MK_MARKER_TIME_FIELD	6
#define	MK_LOOP_DOWN_BUTTON		7
#define	MK_LOOP_UP_BUTTON		8
#define	MK_LOOP_NUMBER_FIELD	9
#define	MK_LOOP_START_TIME_FIELD	10
#define	MK_LOOP_END_TIME_FIELD	11
#define	MK_LOW_NOTE_FIELD		12
#define	MK_BASE_NOTE_FIELD		13
#define	MK_HIGH_NOTE_FIELD		14
#define	MK_DETUNE_FIELD			15
#define	MK_GAIN_FIELD			16
#define	MK_HIGH_VEL_FIELD		17
#define	MK_LOW_VEL_FIELD			18
#define	MK_CANCEL_BUTTON			2
#define	MK_OK_BUTTON				1

typedef struct		/* based on the AIFF Instrument chunk */
{
	char		baseNote;
	char		lowNote;
	char		highNote;
	char		lowVelocity;
	char		highVelocity;
	char		detune;
	short		gain;
	short		numLoops;
	short		numMarkers;
	long		*loopStart;
	long		*loopEnd;
	long		*markPos;
	long		currentMarker;
	long		currentLoop;
}	MarkerInfo;

void	HandleMarkerDialog(void);
void	HandleMarkerDialogEvent(short itemHit);
void	InitMarkerStruct(MarkerInfo *myMarkerInfo);
short		ReadMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		ReadAIFFMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		ReadSDIIMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		ReadWAVEMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		WriteMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		WriteAIFFMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		WriteWAVEMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		WriteSDIIMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		CreateMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		CreateAIFFMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		CreateWAVEMarkers(SoundInfo *mySI,MarkerInfo *myMarkerInfo);
short		CreateSDIIMarkers(SoundInfo *mySI);
void	CopyMarkers(void);
