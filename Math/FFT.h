#define	TIME2FREQ			1
#define	FREQ2TIME			0

/*Function prototypes for FFT.c*/
void	InitFFTTable(void);
void	FFT(float data[], long numberPoints, short direction);
void	bitreverse(float data[], long numberData);
void	RealFFT(float data[], long numberPoints, short direction);

