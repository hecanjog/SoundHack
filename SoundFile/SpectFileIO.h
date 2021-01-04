// SpectFileIO.h

long	WriteCSAData(SoundInfo *mySI, float polarSpectrum[], float lastPhaseIn[], long halfLengthFFT);
long	WriteSHAData(SoundInfo *mySI, float polarSpectrum[], short channel);
long	WriteSDIFData(SoundInfo *mySI, float spectrum[], float polarSpectrum[], double seconds, short channel);
long	ReadCSAData(SoundInfo *mySI, float polarSpectrum[], float lastPhaseIn[], long halfLengthFFT);
long	ReadSHAData(SoundInfo *mySI, float polarSpectrum[], short channel);
long	ReadSDIFData(SoundInfo *mySI, float spectrum[], float polarSpectrum[], double *seconds, short channel);


typedef struct
{
    char matrixType[4];
    long matrixDataType;
    long rowCount;
    long columnCount;
	float	sampleRate;
	float	frameSeconds;
	float	fftSize;
	float	zeroPad;
}	SDIF_STFInfoFloatMatrix;

typedef struct
{
    char matrixType[4];
    long matrixDataType;
    long rowCount;
    long columnCount;
	double	sampleRate;
	double	frameSeconds;
	double	fftSize;
}	SDIF_STFInfoDoubleMatrix;
