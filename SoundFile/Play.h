enum
{
	kNoPlayer,
	kQuickTimePlayer,
	kPortAudioPlayer
};

typedef struct
{
	Boolean looping;
	Boolean playing;
	long	player;
	float	length;
	long	position;
	SoundInfo	*file;
}	PlayInfo;
	

/*Function prototypes for Play.c*/

void InitializePlay(void);
void TerminatePlay(void);
Boolean	StartPlay(SoundInfo *mySI, double startTime, double endTime);
void	UpdatePlay();
void	SetPlayLooping(Boolean looping);
Boolean	GetPlayLooping();
float	GetPlayTime();
Boolean	GetPlayState();
long GetPlayerType(SoundInfo *theSI);
Boolean	GetFilePlayState(SoundInfo *mySI);
void	StopPlay(Boolean wait);
void	FinishPlay(void);
void	RestartPlay(void);


