/*Function prototypes for ShowSpect.c*/
short		CreateSpectWindow(SoundInfo *mySI);
short		DrawSpectBorders(SoundInfo *mySI);
void	DrawOneSpectBorder(Rect drawRect, Str255 windowStr);
short		DisplaySpectrum(float spectrum[], float displaySpectrum[], long halfLengthFFT, SoundInfo *mySI, short channel, float scale, Str255 legend);
void	DrawSpect(float Spect[], Rect drawRect, long length);
short		UpdateSpectDisplay(SoundInfo *mySI);