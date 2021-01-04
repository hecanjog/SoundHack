/* For Import Dialog Display */
#define	IP_ID_FIELD			6
#define	IP_PLAY_BUTTON		9
#define	IP_NAME_FIELD		5
#define	IP_NEXT_FIELD		4
#define	IP_PREV_FIELD		3
#define	IP_RATE_FIELD		12
#define	IP_CHAN_FIELD		13
#define	IP_TYPE_FIELD		15
#define	IP_CANCEL_BUTTON	2
#define	IP_OK_BUTTON		1

/* For Export Dialog Display */
#define	E_MAX_FIELD			5
#define	E_START_FIELD		6
#define	E_END_FIELD			8
#define	E_CANCEL_BUTTON		2
#define	E_OK_BUTTON			1

/* For Play Dialog Display */
#define PL_FF_ICON			2
#define PL_STOP_ICON		3
#define PL_RTZ_ICON			4
#define PL_REWIND_ICON		5
#define PL_DONE_BUTTON		6
#define	PL_TIME_FIELD		8
#define	PL_PLAY_ICON		1

// Replacement structs for Apple's or Metrowerk's awful use of extended80

struct ErbeCmpSoundHeader {
	Ptr								samplePtr;					/*if nil then samples are in sample area*/
	unsigned long					numChannels;				/*number of channels i.e. mono = 1*/
	UnsignedFixed					sampleRate;					/*sample rate in Apples Fixed point representation*/
	unsigned long					loopStart;					/*loopStart of sound before compression*/
	unsigned long					loopEnd;					/*loopEnd of sound before compression*/
	unsigned char					encode;						/*data structure used , stdSH, extSH, or cmpSH*/
	unsigned char					baseFrequency;				/*same meaning as regular SoundHeader*/
	unsigned long					numFrames;					/*length in frames ( packetFrames or sampleFrames )*/
	char							AIFFSampleRate[10];			/*IEEE sample rate*/
	Ptr								markerChunk;				/*sync track*/
	OSType							format;						/*data format type, was futureUse1*/
	unsigned long					futureUse2;					/*reserved by Apple*/
	StateBlockPtr					stateVars;					/*pointer to State Block*/
	LeftOverBlockPtr				leftOverSamples;			/*used to save truncated samples between compression calls*/
	short							compressionID;				/*0 means no compression, non zero means compressionID*/
	unsigned short					packetSize;					/*number of bits in compressed sample packet*/
	unsigned short					snthID;						/*resource ID of Sound Manager snth that contains NRT C/E*/
	unsigned short					sampleSize;					/*number of bits in non-compressed sample*/
	unsigned char					sampleArea[1];				/*space for when samples follow directly*/
};
typedef struct ErbeCmpSoundHeader ErbeCmpSoundHeader;

struct ErbeExtSoundHeader {
	Ptr								samplePtr;					/*if nil then samples are in sample area*/
	unsigned long					numChannels;				/*number of channels,  ie mono = 1*/
	UnsignedFixed					sampleRate;					/*sample rate in Apples Fixed point representation*/
	unsigned long					loopStart;					/*same meaning as regular SoundHeader*/
	unsigned long					loopEnd;					/*same meaning as regular SoundHeader*/
	unsigned char					encode;						/*data structure used , stdSH, extSH, or cmpSH*/
	unsigned char					baseFrequency;				/*same meaning as regular SoundHeader*/
	unsigned long					numFrames;					/*length in total number of frames*/
	char							AIFFSampleRate[10];				/*IEEE sample rate*/
	Ptr								markerChunk;				/*sync track*/
	Ptr								instrumentChunks;			/*AIFF instrument chunks*/
	Ptr								AESRecording;
	unsigned short					sampleSize;					/*number of bits in sample*/
	unsigned short					futureUse1;					/*reserved by Apple*/
	unsigned long					futureUse2;					/*reserved by Apple*/
	unsigned long					futureUse3;					/*reserved by Apple*/
	unsigned long					futureUse4;					/*reserved by Apple*/
	unsigned char					sampleArea[1];				/*space for when samples follow directly*/
};
typedef struct ErbeExtSoundHeader ErbeExtSoundHeader;

/*Function prototypes for SndResource.c*/
void	HandleExportDialog(void);
short		ExportSndResource(double startTime, double endTime);
short		HandleImportDialog(void);
short		ImportSndResource(Handle mySndHandle);
short		ReadSndResourceInfo(Handle mySndHandle);

