short	CreateSoundFile(SoundInfoPtr *mySI, short dialog);
short	CreateAIFF(SoundInfo *mySI);
short	CreateAIFC(SoundInfo *mySI);
short	CreateWAVE(SoundInfo *mySI);
short	CreateBICSF(SoundInfo *mySI);
short	CreateSDII(SoundInfo *mySI);
short	CreateDSPD(SoundInfo *mySI);
short	CreateNEXT(SoundInfo *mySI);
short	CreateTEXT(SoundInfo *mySI);
short	CreateRAW(SoundInfo *mySI);
short	CreatePVAFile(SoundInfo *mySI);
short	CreateSDIFFile(SoundInfo *mySI);
short	CreatePICT(SoundInfo *mySI);
short	CreateMovie(SoundInfo *mySI);

#define NO_DIALOG			0
#define SOUND_SIMP_DIALOG	1
#define SOUND_CUST_DIALOG	2
#define SPECT_DIALOG		3

