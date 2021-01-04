/**************************************************************************
  
	Mac IMA4 Variant - Decrompresion code
	� 1996 Mark F. Coniglio
	all rights reserved
	
****************************************************************************/

#ifndef _H_IMA4Dec
#define	_H_IMA4Dec

#define	kIMABlockSize		34
#define kIMASamplesPerBlock	64

/////////////////////////////
// Prototypes
/////////////////////////////

long GetIMA4LoopOffset(long loopframe, Boolean stereo);
void ExpIMA4OneBlock(char *inbuf, short *outbuf, Boolean stereo);
long BlockADIMADecodeShort(char *charSams, short *shortSams, long numSamples, long channels);
long BlockADIMADecodeFloat(char *charSams, float *blockL, float *blockR, long numSamples, long channels);
long ExpIMA4Stereo(char *inbuf, short *outbuf, long insize);
long ExpIMA4Mono(char *inbuf, short *outbuf, long insize);

#endif