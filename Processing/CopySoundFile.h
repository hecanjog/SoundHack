/*Function prototypes for CopySoundFile.c*/
short	InitCopySoundFile(void);
short	CopyBlock(void);
void	FinishCopySoundFile(void);
void	DeAllocCopyMemory(void);

short	InitSplitSoundFile(void);
short	SplitBlock(void);
void	FinishSplitSoundFile(void);
void	DeAllocSplitMemory(void);
