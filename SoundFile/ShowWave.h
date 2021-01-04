/*Function prototypes for ShowWave.c*/
void	CreateSigWindow(SoundInfo *mySI);
void	DrawWaveBorders(SoundInfo *mySI);
void	DisplayMonoWave(float Wave[], SoundInfo *mySI, long length, Str255 beginStr, Str255 endStr);
void	DisplayStereoWave(float WaveL[], float WaveR[], SoundInfo *mySI, long length, Str255 beginStr, Str255 endStr);
void	DisplayWave(float Wave[], WindowPtr myWindow,long length, long vSize, long hZeroPos, long vZeroPos, float drawScale, long horizSize);

