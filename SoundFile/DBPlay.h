void	DBPlayCallBackInit(void);
short	StartDBPlay(SoundInfo *mySI, double time, Boolean open);
void	StopDBPlay(Boolean wait, Boolean open);
void	FinishPlay(void);
void	RestartPlay(void);
pascal void	DoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer);
pascal void	Sw16DoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer);
pascal void	S8DoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer);
pascal void MuDoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer);
pascal void ADoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer);
pascal void ADDVIDoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer);
pascal void FltDoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer);
void SignToUnsignBlock(Ptr block, long size);
void Sw16BlockMove(Ptr inBlock, Ptr outBlock, long size);
void Flt2ShortBlock(float *floatSams, short *shortSams, long size, float gain);

typedef struct
{
	SndDoubleBufferHeader2	doubleHeader;
	ParmBlkPtr				inSParmBlk;
	long					numFrames;
	long					numBytes;
	long					filePos;
	short					dFileNum;
	Boolean					lastBlock;
	Boolean					nextBlock;
	char					*fileBlock;
	float					timeDivisor;
	unsigned long			ticksStart;
	Boolean					loop;
} DoubleBufPlayInfo;
