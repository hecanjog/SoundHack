/*Function prototypes for FourToOne.c*/
char	ADDVIEncode(short shortOne, short shortTwo, long channels, Boolean init);
void	ADDVIDecode(unsigned char charDVI, short *shortOne, short *shortTwo, long channels, Boolean init);
void	BlockADDVIEncode(char *charSams, float *blockL, float *blockR, long numSamples, long channels);
void	BlockADDVIDecode(unsigned char *charSams, short *shortSams, long samples, long channels, Boolean init);

