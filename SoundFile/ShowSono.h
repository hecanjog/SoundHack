short		CreateSonoWindow(SoundInfo *mySI);
short		DrawSonoBorders(SoundInfo *mySI);
void	DrawOneSonoBorder(Rect drawRect, Str255 windowStr);
void	DisplayMonoSono(float Spect[], SoundInfo *mySI, long length, Str255 legend);
short		DisplaySonogram(float spectrum[], float displaySpectrum[], long halfLengthFFT, SoundInfo *mySI, short channel, float scale, Str255 legend);
void	DrawSono(float Spect[], Rect drawRect, long length, long hPos);
short		UpdateSonoDisplay(SoundInfo *mySI);
