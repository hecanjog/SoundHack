// MPEG.c
// Routines to read and write MPEG encoded files

#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
#include <stdio.h>
#include <SoundInput.h>
#include "SoundFile.h"
#include "CreateSoundFile.h"
#include "WriteHeader.h"
#include "SoundHack.h"
#include "Misc.h"
#include "Dialog.h"
#include "MPEG.h"
#include "MPEGTables.h"

extern long	gNumberBlocks, gInPosition;
extern short gProcessEnabled, gProcessDisabled, gStopProcess;
extern SoundInfoPtr	outSIPtr;
#ifdef powerc
extern FileFilterUPP	gMPEGFileFilter;
#endif

// these should be allocated as needed
unsigned long *BitAlloc, *ScaleIndex, *ScaleFactInfo, *Sample;
float	*Quant, *SynthBuffer, *MatrixBuffer, *SFC, *SFCD,*Cosine32;
short	*OutBuffer;

struct
{
	FSSpec			inFile;
	short			inRefNum;
	unsigned long	mpegHeader;
	unsigned short	layer;
	Boolean			protection;
	unsigned long	bitRate;
	unsigned long	sampleRate;
	unsigned short	mode;
	unsigned short	crc;
	Boolean			pad;
	tableIndexBand	tableIndex[32];
	long			chans;
	long			jSBound;
	long			sBLimit;
	long			bufferSize;		// in bytes
	long			currentWord;	// current long word in buffer
	long			nextSyncPos;
	long			currentBit;		// current bit (0-31) in buffer
	long			outPointer;
	unsigned long	*buffer;
}	gMPI;

OSErr	InitMPEGImport(void)
{
	long				readSize, i, j, eofPos;
	double				piOver64, oneOver32768;
	float				length;
	SHStandardFileReply	reply;
	short				numTypes = -1;
	SFTypeList			typeList;
	OSErr				iErr;
	Str255				errStr;
	
	// Open the file, using MPEGFileFilter to grab only files of type 'MPEG' or that end
	// with .mpg, .mpeg, .mp2, .mp1 (are there more?)
#ifdef powerc
	StandardGetFile((FileFilterUPP)gMPEGFileFilter,numTypes,typeList,&reply);
#else
	StandardGetFile((FileFilterUPP)MPEGFileFilter,numTypes,typeList,&reply);
#endif
	if(reply.sfGood)
	{
		iErr = FSpOpenDF(&reply.sfFile, fsCurPerm ,&gMPI.inRefNum);
		if(iErr)
		{
			FSClose(gMPI.inRefNum);
			FlushVol((StringPtr)nil, reply.sfFile.vRefNum);
			DrawErrorMessage("\perror opening file");
			return(-1);
		}
	}
	else
		return(-1);
	gMPI.inFile = reply.sfFile;
	
	// get the MPEG header (first 32 bits) everything else is based on it.
	SetFPos(gMPI.inRefNum, fsFromStart, 0L);
	readSize = sizeof(long);
	FSRead(gMPI.inRefNum, &readSize, &gMPI.mpegHeader);
	if((gMPI.mpegHeader & MPEG_SYNC_WORD) != MPEG_SYNC_WORD)
	{
		FSClose(gMPI.inRefNum);
		FlushVol((StringPtr)nil, reply.sfFile.vRefNum);
		DrawErrorMessage("\pbad mpeg header");
		return(-1);
	}
		
	// find some of the commonly used variables
	// sample rate
	if((gMPI.mpegHeader & MPEG_SAMP_RATE) == MPEG_SR_44K)
		gMPI.sampleRate = 44100;
	else if((gMPI.mpegHeader & MPEG_SAMP_RATE) == MPEG_SR_48K)
		gMPI.sampleRate = 48000;
	else
		gMPI.sampleRate = 32000;
		
	// bitrate
	gMPI.bitRate = GetMPEGBitRate(gMPI.mpegHeader);
	
	// number of channels
	if((gMPI.mpegHeader & MPEG_MODE) == MPEG_MODE_MONO)
		gMPI.chans = 1;
	else
		gMPI.chans = 2;
	
	// location in bitstream	
	gMPI.currentWord = 0L;
	gMPI.currentBit = 0L;
	
	outSIPtr = (SoundInfo *)NewPtr(sizeof(SoundInfo));
	outSIPtr->sfType = AIFF;
	outSIPtr->packMode = SF_FORMAT_16_LINEAR;
	outSIPtr->frameSize = 2.0;
	StringAppend(gMPI.inFile.name, "\p.aiff", outSIPtr->sfSpec.name);

	outSIPtr->sRate = gMPI.sampleRate;
	outSIPtr->nChans = gMPI.chans;
	outSIPtr->numBytes = 0;
	if(CreateSoundFile(&outSIPtr, FALSE) == -1)
	{
		gProcessDisabled = gProcessEnabled = NO_PROCESS;
		MenuUpdate();
		RemovePtr((Ptr)outSIPtr);
		FSClose(gMPI.inRefNum);
		FlushVol((StringPtr)nil, gMPI.inFile.vRefNum);
		outSIPtr = nil;
		return(-1);
	}
	WriteHeader(outSIPtr);
	UpdateInfoDialog(outSIPtr);
    
	
	// set up and allocate all working memory
	gMPI.bufferSize = GetMPEGBitRate(gMPI.mpegHeader) >> 3;	// bitRate divided by 8, byteRate
														// one second of sound
	gMPI.buffer =	(unsigned long *)NewPtr(gMPI.bufferSize);
	
	BitAlloc = 		(unsigned long *)NewPtr(2 * 32 * sizeof(long));
	ScaleFactInfo =	(unsigned long *)NewPtr(2 * 32 * sizeof(long));
	ScaleIndex =	(unsigned long *)NewPtr(3 * 2 * 32 * sizeof(long));
	Sample =		(unsigned long *)NewPtr(3 * 2 * 32 * sizeof(long));
	Quant =			(float *)NewPtr(3 * 2 * 32 * sizeof(float));
	MatrixBuffer =	(float *)NewPtr(2 * 1024 * sizeof(float));
	SynthBuffer =	(float *)NewPtr(2 * 512 * sizeof(float));
	SFC =			(float *)NewPtr(18 * 64 * sizeof(float));
	SFCD =			(float *)NewPtr(18 * 64 * sizeof(float));
	Cosine32 =		(float *)NewPtr(32 * 64 * sizeof(float));
	OutBuffer =		(short *)NewPtr(gMPI.sampleRate * gMPI.chans * sizeof(short));
	
	ZeroFloatTable(MatrixBuffer, 2048);
	ZeroFloatTable(SynthBuffer, 1024);
	
	// initialize the requant and scalefactor tables
	oneOver32768 = 1.0/32768.0;
	if((gMPI.mpegHeader & MPEG_LAYER) == MPEG_LAYER_I)
	{
		for(i = 0; i < 18; i++)
			for(j = 0; j < 64; j++)
			{
				*(SFC + (i<<6) + j) = scaleFactors[j] * deQuantCI[i] * oneOver32768;
				*(SFCD + (i<<6) + j) = scaleFactors[j] * deQuantCI[i] * deQuantDI[i];
			}
	}
	else
	{
		for(i = 0; i < 18; i++)
			for(j = 0; j < 64; j++)
			{
				*(SFC + (i<<6) + j) = scaleFactors[j] * deQuantCII[i] * oneOver32768;
				*(SFCD + (i<<6) + j) = scaleFactors[j] * deQuantCII[i] * deQuantDII[i];
			}
	}
	
	// initialize the cosine based subband synthesis table
	piOver64 = atan(1.0) * 0.0625;
	// i is the subBand and j is the sample in the cosine table
	for(i = 0; i < 32; i++)
		for(j = 0; j < 64; j++)
			*(Cosine32 + (i<<6) + j) = (float)cos((double)((16+j)*(2*i+1)*piOver64));

	// read the first block in
	UpdateProcessWindow("\p", "\p", "\pimporting mpeg", 0.0);
	SetFPos(gMPI.inRefNum, fsFromStart, 0L);
	readSize = gMPI.bufferSize;
	FSRead(gMPI.inRefNum, &readSize, gMPI.buffer);
	GetFPos(gMPI.inRefNum, &gInPosition);
	length = (float)gInPosition/gMPI.bufferSize;
	FixToString(length, errStr);
	GetEOF(gMPI.inRefNum, &eofPos);
	length = (float)gInPosition/eofPos;
	UpdateProcessWindow(errStr, "\p", "\pimporting mpeg", length);
	gProcessEnabled = MPEG_IMPORT_PROCESS;
	return(0);
}

pascal Boolean	MPEGFileFilter(FileParam *pBP)
{
	short i;
	Str255 testString;

	if(pBP->ioFlAttrib & 0x10)	// Is it a directory?
		return(FALSE);
	if(pBP->ioFlFndrInfo.fdType == 'MPEG' || pBP->ioFlFndrInfo.fdType == 'MPII')
		return(FALSE);
	for(i = 1; i <= 3; i++)
		testString[i] = pBP->ioNamePtr[i+(pBP->ioNamePtr[0] - 3)];
	testString[0] = 3;
	if(EqualString(testString,"\p.mp",FALSE,TRUE))
		return(FALSE);
	for(i = 1; i <= 4; i++)
		testString[i] = pBP->ioNamePtr[i+(pBP->ioNamePtr[0] - 4)];
	testString[0] = 4;
	if(EqualString(testString,"\p.mpg",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.mpa",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.mp1",FALSE,TRUE))
		return(FALSE);
	if(EqualString(testString,"\p.mp2",FALSE,TRUE))
		return(FALSE);
	for(i = 1; i <= 5; i++)
		testString[i] = pBP->ioNamePtr[i+(pBP->ioNamePtr[0] - 5)];
	testString[0] = 5;
	if(EqualString(testString,"\p.mpeg",FALSE,TRUE))
		return(FALSE);
	return(TRUE);
}

void	MPEGImportBlock(void)
{
	unsigned long slotsFrame, frameSize;
	long sample, writeSize;
	float	length;
	Str255	errStr;
				
	if(gStopProcess == TRUE)
	{
		FinishMPEGImport();
		FinishProcess();
		gNumberBlocks = 0;
		return;
	}

	if((gMPI.mpegHeader & MPEG_LAYER) == MPEG_LAYER_I)
	{
		for(; gMPI.outPointer <(gMPI.sampleRate >>2);)
		{
			gMPI.mpegHeader = SeekNextSyncWord();
			gMPI.sBLimit = 32;
			slotsFrame = (long)(12 * (gMPI.bitRate/gMPI.sampleRate));
			if((gMPI.mpegHeader & MPEG_PAD) == MPEG_PAD)
				slotsFrame++;
			frameSize = 4 * slotsFrame;
			if((gMPI.mpegHeader & MPEG_MODE) == MPEG_MODE_JST)
			{
				if((gMPI.mpegHeader & MPEG_MODE_EXT) == MPEG_MODE_EXT4)
					gMPI.jSBound = 4;
				else if((gMPI.mpegHeader & MPEG_MODE_EXT) == MPEG_MODE_EXT8)
					gMPI.jSBound = 8;
				else if((gMPI.mpegHeader & MPEG_MODE_EXT) == MPEG_MODE_EXT12)
					gMPI.jSBound = 12;
				else if((gMPI.mpegHeader & MPEG_MODE_EXT) == MPEG_MODE_EXT16)
					gMPI.jSBound = 16;
			}
			else
				gMPI.jSBound = gMPI.sBLimit;
			// index past the header
			gMPI.currentWord++;
			if((gMPI.mpegHeader & MPEG_PROTECTION) == MPEG_PROTECTION)
				gMPI.crc = (unsigned short)GetBits(16);
			GetMPEGIBitAllocScale();
			for(sample = 0; sample < 12; sample++)
			{
				GetMPEGISample();
				ReQuantMPEGISample();
				MPEGISynthesis();
			}
		}
	}
	else if((gMPI.mpegHeader & MPEG_LAYER) == MPEG_LAYER_II)
	{
		
		for(; gMPI.outPointer <(gMPI.sampleRate >>2);)
		{
			gMPI.mpegHeader = SeekNextSyncWord();
			gMPI.sBLimit = GetMPEGIITable(gMPI.sampleRate, gMPI.bitRate, gMPI.chans);
			slotsFrame = (long)(144 * (gMPI.bitRate/gMPI.sampleRate));
			if((gMPI.mpegHeader & MPEG_PAD) == MPEG_PAD)
				slotsFrame++;
			frameSize = slotsFrame;
			gMPI.nextSyncPos = gMPI.currentWord + (frameSize >> 2);
			if((gMPI.mpegHeader & MPEG_MODE) == MPEG_MODE_JST)
			{
				if((gMPI.mpegHeader & MPEG_MODE_EXT) == MPEG_MODE_EXT4)
					gMPI.jSBound = 4;
				else if((gMPI.mpegHeader & MPEG_MODE_EXT) == MPEG_MODE_EXT8)
					gMPI.jSBound = 8;
				else if((gMPI.mpegHeader & MPEG_MODE_EXT) == MPEG_MODE_EXT12)
					gMPI.jSBound = 12;
				else if((gMPI.mpegHeader & MPEG_MODE_EXT) == MPEG_MODE_EXT16)
					gMPI.jSBound = 16;
			}
			else
				gMPI.jSBound = gMPI.sBLimit;
			// index past the header
			gMPI.currentWord++;
 			if((gMPI.mpegHeader & MPEG_PROTECTION) == MPEG_PROTECTION)
				gMPI.crc = (unsigned short)GetBits(16);
			GetMPEGIIBitAllocScale();
			for(sample = 0; sample < 12; sample++)
			{
				GetMPEGIISample();
				if(sample >= 0 && sample < 4)
					ReQuantMPEGIISample(0);
				else if(sample >= 4 && sample < 8)
					ReQuantMPEGIISample(64);
				else if(sample >= 9 && sample < 12)
					ReQuantMPEGIISample(128);
				MPEGIISynthesis();
			}
		}
	}
	writeSize = gMPI.outPointer * sizeof(short);
	FSWrite(outSIPtr->dFileNum, &writeSize, OutBuffer);
	outSIPtr->numBytes += writeSize;
	length = (outSIPtr->numBytes/(outSIPtr->frameSize*outSIPtr->nChans*outSIPtr->sRate));
	FixToString(length, errStr);
	UpdateProcessWindow("\p", errStr, "\p", -1.0);

	gMPI.outPointer = 0;
}

void	FinishMPEGImport(void)
{
	DisposePtr((Ptr)gMPI.buffer);
	DisposePtr((Ptr)BitAlloc);
	DisposePtr((Ptr)ScaleFactInfo);
	DisposePtr((Ptr)ScaleIndex);
	DisposePtr((Ptr)Sample);
	DisposePtr((Ptr)Quant);
	DisposePtr((Ptr)MatrixBuffer);
	DisposePtr((Ptr)SynthBuffer);
	DisposePtr((Ptr)SFC);
	DisposePtr((Ptr)SFCD);
	DisposePtr((Ptr)Cosine32);
	DisposePtr((Ptr)OutBuffer);
}

//	this tries to estimate how many bytes are needed for each sample.
//	this is mainly used to display the sound length
//	an MPEG frame is a block of data that contains info for a number
//	of samples: 384 for layer 1, 1152 for layers 2 and 3

float	GetMPEGSampleSize(unsigned long mpegHeader)
{
	unsigned short	sampleRate;
	float	bitRate, bytesPerSample;
	
	if((mpegHeader & MPEG_SAMP_RATE) == MPEG_SR_44K)
		sampleRate = 44100;
	else if((mpegHeader & MPEG_SAMP_RATE) == MPEG_SR_48K)
		sampleRate = 48000;
	else
		sampleRate = 32000;
		
	if((mpegHeader & MPEG_MODE) == MPEG_MODE_MONO)
		gMPI.chans = 1;
	else
		gMPI.chans = 2;
		
	bitRate = (float)GetMPEGBitRate(mpegHeader);
	bytesPerSample	= bitRate/(sampleRate * gMPI.chans * 8.0);		

	return(bytesPerSample);
}

long	GetMPEGBitRate(unsigned long mpegHeader)
{
	unsigned short	layer;
	unsigned char	bitCode;
	
	if((mpegHeader & MPEG_LAYER) == MPEG_LAYER_I)
		layer = SF_FORMAT_MPEG_I;
	else if((mpegHeader & MPEG_LAYER) == MPEG_LAYER_II)
		layer = SF_FORMAT_MPEG_II;
		
	bitCode = (mpegHeader & MPEG_BIT_RATE) >> 12;
	
	if(layer == SF_FORMAT_MPEG_I)
		switch(bitCode)
		{
			case 0x00:	// free - SoundHack will bomb on this
				return(-2);
				break;
			case 0x01:	// 32 kbps
				return(32000);		
				break;
			case 0x02:	// 64 kbps
				return(64000);		
				break;
			case 0x03:	// 96 kbps
				return(96000);		
				break;
			case 0x04:	// 128 kbps
				return(128000);		
				break;
			case 0x05:	// 160 kbps
				return(160000);		
				break;
			case 0x06:	// 192 kbps
				return(192000);		
				break;
			case 0x07:	// 224 kbps
				return(224000);		
				break;
			case 0x08:	// 256 kbps
				return(256000);		
				break;
			case 0x09:	// 288 kbps
				return(288000);		
				break;
			case 0x0a:	// 320 kbps
				return(320000);		
				break;
			case 0x0b:	// 352 kbps
				return(352000);		
				break;
			case 0x0c:	// 384 kbps
				return(384000);		
				break;
			case 0x0d:	// 416 kbps
				return(416000);		
				break;
			case 0x0e:	// 448 kbps
				return(448000);		
				break;
			case 0x0f:	// forbidden
				return(-1);
				break;
		}
	else if(layer == SF_FORMAT_MPEG_II)
		switch(bitCode)
		{
			case 0x00:	// free - SoundHack will bomb on this
				return(-2);
				break;
			case 0x01:	// 32 kbps
				return(32000);		
				break;
			case 0x02:	// 48 kbps
				return(48000);		
				break;
			case 0x03:	// 56 kbps
				return(56000);		
				break;
			case 0x04:	// 64 kbps
				return(64000);		
				break;
			case 0x05:	// 80 kbps
				return(80000);		
				break;
			case 0x06:	// 96 kbps
				return(96000);		
				break;
			case 0x07:	// 112 kbps
				return(112000);		
				break;
			case 0x08:	// 128 kbps
				return(128000);		
				break;
			case 0x09:	// 160 kbps
				return(160000);		
				break;
			case 0x0a:	// 192 kbps
				return(192000);		
				break;
			case 0x0b:	// 224 kbps
				return(224000);		
				break;
			case 0x0c:	// 256 kbps
				return(256000);		
				break;
			case 0x0d:	// 320 kbps
				return(320000);		
				break;
			case 0x0e:	// 384 kbps
				return(384000);		
				break;
			case 0x0f:	// forbidden
				return(-1);
				break;
		}
	else
		return(-1);
	return(0);
}

//	this always gets bits from gMPI.buffer
//	this should maintain a pointer to the current bit and word and increment both when
//	neccesary
unsigned long	GetBits(long numBits)
{
	unsigned long	currentWord, returnValue, getBits;
	long			readSize, eofPos;
	Str255			errStr;
	float			length;
	unsigned long	bitMask[16]={MASK_1, MASK_2, MASK_3, MASK_4, MASK_5, MASK_6, MASK_7,
			MASK_8, MASK_9, MASK_10, MASK_11, MASK_12, MASK_13, MASK_14, MASK_15, MASK_16};
	
	if(numBits == 0)
		return(0x00000000);
	currentWord = (unsigned long)(*(gMPI.buffer + gMPI.currentWord));
	
	if(gMPI.currentWord+1 >= (gMPI.bufferSize >> 2))
	{
		*(gMPI.buffer) = currentWord;
		readSize = gMPI.bufferSize - 4;
		FSRead(gMPI.inRefNum, &readSize, (gMPI.buffer+1));
		if(readSize == 0)
		{
			gStopProcess = TRUE;
			return(0x00000000);
		}
		GetFPos(gMPI.inRefNum, &gInPosition);
		length = (float)gInPosition/gMPI.bufferSize;
		FixToString(length, errStr);
		GetEOF(gMPI.inRefNum, &eofPos);
		length = (float)gInPosition/eofPos;
		UpdateProcessWindow(errStr, "\p", "\pimporting mpeg", length);
		gMPI.currentWord = 0L;
		gMPI.nextSyncPos -= 11999UL;
	}
// we are shifting right from the left edge (bit 0)
// if we are too far to the right, shift those bits left, then get the others
// in the next word
	if((gMPI.currentBit + numBits) > 32)
	{
		getBits = gMPI.currentBit + numBits - 32;
		returnValue = (currentWord << getBits) & bitMask[numBits - 1];
		// number of bits left....
		numBits = numBits - (32 - gMPI.currentBit);
		gMPI.currentWord++;
		gMPI.currentBit = 0;
		currentWord = (unsigned long)(*(gMPI.buffer + gMPI.currentWord));
	}
	else
		returnValue = 0x0UL;

	getBits = 32 - (numBits + gMPI.currentBit);
	returnValue += (currentWord >> getBits) & bitMask[numBits - 1];
	gMPI.currentBit += numBits;
	if(gMPI.currentBit > 31)
	{
		gMPI.currentBit -= 32;
		if(gMPI.currentBit == 0)
			gMPI.currentWord++;
	}
	
	return(returnValue);
}

// this finds the next sync word and returns the MPEG header
unsigned long SeekNextSyncWord(void)
{
	unsigned long shiftWord = 0UL, i = 0;
	
	gMPI.currentWord = gMPI.nextSyncPos;
	gMPI.currentBit = 0;
	
	while((shiftWord & 0xfff) != 0xfff)
	{
		shiftWord = shiftWord << 1;
		shiftWord += GetBits(1);
		i++;
		if(i>1028)
		{
			DrawErrorMessage("\pmissing sync word");
			return(0UL);
		}
	}
	shiftWord = (unsigned long)(*(gMPI.buffer + gMPI.currentWord));
	gMPI.currentBit = 0;
	return(shiftWord);
}

// The bit allocation information is decoded. Layer I
// has 4 bit per subband whereas Layer II is Ws and bit rate
// dependent.

void GetMPEGIBitAllocScale(void)
{
    long subBand;
    
    for(subBand = 0; subBand<gMPI.jSBound; subBand++)
    {
       	*(BitAlloc+subBand) = GetBits(4);
       	if(gMPI.chans == 2)
       		*(BitAlloc+32+subBand) = GetBits(4);
    }
    for(; subBand<SUBBAND_LIMIT; subBand++)
        *(BitAlloc+subBand) = *(BitAlloc+32+subBand) = GetBits(4);

	for(subBand = 0; subBand<SUBBAND_LIMIT; subBand++)
	{
        if(!*(BitAlloc+subBand))
            *(ScaleIndex+subBand) = SCALE_RANGE-1;
        else                    /* 6 bit per scale factor */
            *(ScaleIndex+subBand) =  GetBits(6);
       	if(gMPI.chans == 2)
       	{
        	if(!*(BitAlloc+32+subBand))
            	*(ScaleIndex+32+subBand) = SCALE_RANGE-1;
        	else                    /* 6 bit per scale factor */
            	*(ScaleIndex+32+subBand) =  GetBits(6);
		}
	}
}

// The following routine takes care of reading the compressed sample from
// the bit stream. For layer 1, read the number of bits as indicated
// by the BitAlloc information.

void GetMPEGISample(void)
{
    long subBand, bits;

    for(subBand = 0; subBand<gMPI.jSBound; subBand++)
    {
		if((bits = *(BitAlloc+subBand)) == 0)
            *(Sample+subBand) = 0;
        else
        { 
            *(Sample+subBand) = GetBits(bits+1) << (15 - bits);
            if(*(Sample+subBand) & 0x00008000)
            // the sign bit, when inverted will be zero, make the rest of the word
            // as max positive as possible
            {
            	*(Sample+subBand) = *(Sample+subBand) & 0x00007fff;
            	*(Sample+subBand) = *(Sample+subBand) | maxPosMask[bits+1];
            }
            else
            // the sign bit, when inverted will be one, make the rest of the word
            // as max negative as possible
            {
            	*(Sample+subBand) = *(Sample+subBand) | 0xffff8000;
            	*(Sample+subBand) = *(Sample+subBand) & maxNegMask[bits+1];
            }
        }
       	if(gMPI.chans == 2)
			if((bits = *(BitAlloc+32+subBand)) == 0)
            	*(Sample+32+subBand) = 0;
        	else 
        	{ 
				*(Sample+32+subBand) = GetBits(bits+1) << (15 - bits);
            	if(*(Sample+32+subBand) & 0x00008000)
            	{
            		*(Sample+32+subBand) = *(Sample+32+subBand) & 0x00007fff;
 		           	*(Sample+32+subBand) = *(Sample+32+subBand) | maxPosMask[bits+1];
 		        }
            	else
            	{
            		*(Sample+32+subBand) = *(Sample+32+subBand) | 0xffff8000;
            		*(Sample+32+subBand) = *(Sample+32+subBand) & maxNegMask[bits+1];
				}            		
			}
    }
    for(;subBand<SUBBAND_LIMIT;subBand++)
    {
        if ((bits = *(BitAlloc+subBand)) == 0)
            *(Sample+subBand) = 0;
        else 
        { 
            *(Sample+subBand) = GetBits(bits+1) << (15 - bits);
            if(*(Sample+subBand) & 0x00008000)
            // the sign bit, when inverted will be zero, make the rest of the word
            // as max positive as possible
            {
            	*(Sample+subBand) = *(Sample+subBand) & 0x00007fff;
            	*(Sample+subBand) = *(Sample+subBand) | maxPosMask[bits+1];
            }
            else
            // the sign bit, when inverted will be one, make the rest of the word
            // as max negative as possible
            {
            	*(Sample+subBand) = *(Sample+subBand) | 0xffff8000;
            	*(Sample+subBand) = *(Sample+subBand) & maxNegMask[bits+1];
            }            
        }
       	if(gMPI.chans == 2)
            *(Sample+32+subBand) = *(Sample+subBand);
    }
}

// SFCD is combined tables deQuantCI[18] * deQuantDI[18] * scaleFactors[64]
// SFA is combined tables deQuantCI[18] * scaleFactors[64]
// C and D are indexed by BitAlloc[channel][subBand]
// scale factor is indexed by ScaleIndex[channel][0][subBand] (center index for level ii triple
// packed sample)
void	ReQuantMPEGISample(void)
{
    long subBand, bits;
	
    for(subBand = 0; subBand<gMPI.jSBound; subBand++)
    {
		if((bits = *(BitAlloc+subBand)) == 0)
            *(Quant+subBand) = 0.0;
        else 
            *(Quant+subBand) = *(SFC + (bits<<6) + *(ScaleIndex+subBand)) 
            	* (signed)*(Sample+subBand) + *(SFCD + (bits<<6) + *(ScaleIndex+subBand));
       	if(gMPI.chans == 2)
			if((bits = *(BitAlloc+32+subBand)) == 0)
            	*(Quant+32+subBand) = 0.0;
        	else 
            	*(Quant+32+subBand) = *(SFC + (bits<<6) + *(ScaleIndex+32+subBand))
            		* (signed)*(Sample+32+subBand) + *(SFCD + (bits<<6) + *(ScaleIndex+32+subBand));
    }
    for(;subBand<SUBBAND_LIMIT;subBand++)
    {
        if ((bits = *(BitAlloc+subBand)) == 0)
            *(Quant+subBand) = 0.0;
        else 
            *(Quant+subBand) = *(SFC + (bits<<6) +*(ScaleIndex+subBand))
            	* (signed)*(Sample+subBand) 
            	+ *(SFCD + (bits<< 6) + *(ScaleIndex+subBand));
       	if(gMPI.chans == 2)
            *(Quant+32+subBand) = *(Quant+subBand);
    }
}

void	MPEGISynthesis(void)
{
    long subBand, sample;
	
	for(sample = 64; sample<1024; sample++)
	{
		*(MatrixBuffer+sample) = *(MatrixBuffer+sample-64);
       	if(gMPI.chans == 2)
			*(MatrixBuffer+1024+sample) = *(MatrixBuffer+1024+sample-64);
	}
	for(sample = 0; sample<64; sample++)
	{
		*(MatrixBuffer+sample) = 0.0;
       	if(gMPI.chans == 2)
			*(MatrixBuffer+1024+sample) = 0.0;
    	for(subBand = 0; subBand<gMPI.jSBound; subBand++)
    	{
        	if(*(BitAlloc+subBand))
        	{
				*(MatrixBuffer+sample) += *(Quant+subBand) * *(Cosine32 + (subBand<<6) + sample);
       			if(gMPI.chans == 2)
            		*(MatrixBuffer+1024+sample) += *(Quant+32+subBand) * *(Cosine32 + (subBand<<6) + sample);
            }
		}
		for(;subBand<SUBBAND_LIMIT;subBand++)
		{
        	if(*(BitAlloc+subBand))
        	{
				*(MatrixBuffer+sample) += *(Quant+subBand) * *(Cosine32 + (subBand<<6) + sample);
       			if(gMPI.chans == 2)
            		*(MatrixBuffer+1024+sample) = *(MatrixBuffer+sample);
            }
		}
	}
}

long
GetMPEGIITable(unsigned long sampleRate, unsigned long bitRate, unsigned long channels)
{
	long	bitRateChan, subBand;
	
	if(channels == 1)
		bitRateChan = bitRate;
	if(channels == 2)
		bitRateChan = bitRate>>1;
	
	if ((sampleRate == 48000 && bitRateChan >= 56000)
		|| (bitRateChan >= 56000 && bitRateChan <= 80000))
	{
		for(subBand = 0; subBand < 3; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 4;
			gMPI.tableIndex[subBand].bandQuant = quantA;
		}
		for(; subBand < 11; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 4;
			gMPI.tableIndex[subBand].bandQuant = quantB;
		}
		for(; subBand < 23; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 3;
			gMPI.tableIndex[subBand].bandQuant = quantC;
		}
		for(; subBand < 27; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 2;
			gMPI.tableIndex[subBand].bandQuant = quantD;
		}
		return(27L);
	}
	else if (sampleRate != 48000 && bitRateChan >= 96000)
	{
		for(subBand = 0; subBand < 3; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 4;
			gMPI.tableIndex[subBand].bandQuant = quantA;
		}
		for(; subBand < 11; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 4;
			gMPI.tableIndex[subBand].bandQuant = quantB;
		}
		for(; subBand < 23; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 3;
			gMPI.tableIndex[subBand].bandQuant = quantC;
		}
		for(; subBand < 30; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 2;
			gMPI.tableIndex[subBand].bandQuant = quantD;
		}
		return(30L);
	}
	else if (sampleRate != 32000 && bitRateChan <= 48000)
	{
		for(subBand = 0; subBand < 2; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 4;
			gMPI.tableIndex[subBand].bandQuant = quantE;
		}
		for(; subBand < 8; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 3;
			gMPI.tableIndex[subBand].bandQuant = quantF;
		}
		return(8L);
	}
	else
	{
		for(subBand = 0; subBand < 2; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 4;
			gMPI.tableIndex[subBand].bandQuant = quantE;
		}
		for(; subBand < 12; subBand++)
		{
			gMPI.tableIndex[subBand].indexBits = 3;
			gMPI.tableIndex[subBand].bandQuant = quantF;
		}
		return(12L);
	}
}

// this just gets the indices into the bit allocation table.
// The following function implements the layer II format of scale factor extraction.
// Layer II requires reading first the ScaleFactInfo which in turn indicate the
// number of scale factors transmitted.

void GetMPEGIIBitAllocScale(void)
{
    long subBand;

    for(subBand = 0; subBand<gMPI.jSBound; subBand++)
    {
		*(BitAlloc+subBand) = (char)GetBits(gMPI.tableIndex[subBand].indexBits);
       	if(gMPI.chans == 2)
			*(BitAlloc+32+subBand) = (char)GetBits(gMPI.tableIndex[subBand].indexBits);
	}
    for(; subBand<gMPI.sBLimit; subBand++) /* expand to 2 channels */
        *(BitAlloc+subBand) = *(BitAlloc+32+subBand) = (char)GetBits(gMPI.tableIndex[subBand].indexBits);
	for(; subBand<SUBBAND_LIMIT; subBand++) 
        *(BitAlloc+subBand) = *(BitAlloc+32+subBand) = 0;

    for(subBand = 0; subBand<gMPI.sBLimit; subBand++)
    {
		if(*(BitAlloc+subBand))
			*(ScaleFactInfo+subBand) = (char) GetBits(2);
       	if(gMPI.chans == 2)
			if(*(BitAlloc+32+subBand))
				*(ScaleFactInfo+32+subBand) = (char) GetBits(2);
	}
	for(; subBand<SUBBAND_LIMIT; subBand++)
    {
        *(ScaleFactInfo+subBand) = 0;
       	if(gMPI.chans == 2)
			*(ScaleFactInfo+32+subBand) = 0;
	}

    for(subBand = 0; subBand<gMPI.sBLimit; subBand++)
    {
        if(*(BitAlloc+subBand))   
            switch(*(ScaleFactInfo+subBand))
            {
				/* all three scale factors transmitted */
				case 0:
					*(ScaleIndex+subBand) = GetBits(6);
					*(ScaleIndex+64+subBand) = GetBits(6);
					*(ScaleIndex+128+subBand) = GetBits(6);
					break;
				/* scale factor 1 & 3 transmitted */
				case 1:
					*(ScaleIndex+subBand) = *(ScaleIndex+64+subBand) = GetBits(6);
					*(ScaleIndex+128+subBand) = GetBits(6);
					break;
				/* scale factor 1 & 2 transmitted */
				case 3:
					*(ScaleIndex+subBand) = GetBits(6);
					*(ScaleIndex+64+subBand) = *(ScaleIndex+128+subBand) = GetBits(6);
					break;
				/* only one scale factor transmitted */
				case 2:
					*(ScaleIndex+subBand) = *(ScaleIndex+64+subBand) = *(ScaleIndex+128+subBand) = GetBits(6);
					break;
				default:
					break;
			}
		else
			*(ScaleIndex+subBand) = *(ScaleIndex+64+subBand) = *(ScaleIndex+128+subBand) = SCALE_RANGE-1;
       	if(gMPI.chans == 2)
       	{
        	if(*(BitAlloc+32+subBand))   
            	switch(*(ScaleFactInfo+32+subBand))
            	{
					/* all three scale factors transmitted */
					case 0:
						*(ScaleIndex+32+subBand) = GetBits(6);
						*(ScaleIndex+96+subBand) = GetBits(6);
						*(ScaleIndex+160+subBand) = GetBits(6);
						break;
					/* scale factor 1 & 3 transmitted */
					case 1:
						*(ScaleIndex+32+subBand) = *(ScaleIndex+96+subBand) = GetBits(6);
						*(ScaleIndex+160+subBand) = GetBits(6);
						break;
					/* scale factor 1 & 2 transmitted */
					case 3:
						*(ScaleIndex+32+subBand) = GetBits(6);
						*(ScaleIndex+96+subBand) = *(ScaleIndex+160+subBand) = GetBits(6);
						break;
					/* only one scale factor transmitted */
					case 2:
						*(ScaleIndex+32+subBand) = *(ScaleIndex+96+subBand) = *(ScaleIndex+160+subBand) = GetBits(6);
						break;
					default:
						break;
				}
			else
				*(ScaleIndex+32+subBand) = *(ScaleIndex+96+subBand) = *(ScaleIndex+160+subBand) = SCALE_RANGE-1;
		}
    }
    for(; subBand<SUBBAND_LIMIT; subBand++)
    {
        *(ScaleIndex+subBand) = *(ScaleIndex+64+subBand) = *(ScaleIndex+128+subBand) = SCALE_RANGE-1;
       	if(gMPI.chans == 2)
        	*(ScaleIndex+32+subBand) = *(ScaleIndex+96+subBand) = *(ScaleIndex+160+subBand) = SCALE_RANGE-1;
	}
}

// The following routine takes care of reading the compressed sample from the bit
// stream.  For layer 2, if grouping is indicated for a particular subband, then
// the sample size has to be read from the bits_group and the merged samples has
// to be decompose into the three distinct samples. Otherwise,
// it is the same for as layer one.

void GetMPEGIISample(void)
{
    long subBand, chan, bits, chnShft;

    for(subBand = 0; subBand<gMPI.sBLimit; subBand++)
    	for(chan = 0; chan<((subBand<gMPI.jSBound)?gMPI.chans:1); chan++) 
    	{
    		chnShft = chan << 5;
        	if (*(BitAlloc+chnShft+subBand))
        	{
        		if(*(gMPI.tableIndex[subBand].bandQuant + *(BitAlloc+chnShft+subBand)) == 1 ||
        		 	*(gMPI.tableIndex[subBand].bandQuant + *(BitAlloc+chnShft+subBand)) == 2 ||
        		 	*(gMPI.tableIndex[subBand].bandQuant + *(BitAlloc+chnShft+subBand)) == 4)
				{
					// BitAlloc = 3, 5, 9
					unsigned long nLev, c, nLevSq, bitCheck = 0, sampBits;

					// without divison
					// remove squared step value repeatedly then
					// remove step value repeatedly
					
					nLev = quantTable[*(gMPI.tableIndex[subBand].bandQuant 
						+ *(BitAlloc+chnShft+subBand))];
					if(nLev == 3)
					{
						sampBits = 2;
						nLevSq = 9;
					}
					else if(nLev == 5)
					{
						sampBits = 3;
						nLevSq = 25;
					}
					else if(nLev == 9)
					{
						sampBits = 4;
						nLevSq = 81;
					}
					bits = bitsTable[*(gMPI.tableIndex[subBand].bandQuant 
						+ *(BitAlloc+chnShft+subBand))];
					c = (unsigned long) GetBits(bits);
					
					for(*(Sample+128+chnShft+subBand) = 0;
							(signed long)(c - nLevSq) > 0; *(Sample+128+chnShft+subBand) += 1)
						c = c - nLevSq;
					*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand) << (16 - sampBits);
					for(*(Sample+64+chnShft+subBand) = 0; 
							(signed long)(c - nLev) > 0; *(Sample+64+chnShft+subBand) += 1)
						c = c - nLev;
					*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand) << (16 - sampBits);
					*(Sample + chnShft+subBand) = c << (16 - sampBits);
					if(*(Sample + chnShft+subBand) & 0x00008000)	// if the MSB is 1
					{
						*(Sample+chnShft+subBand) = *(Sample+chnShft+subBand) & 0x00007fff;
            			*(Sample+chnShft+subBand) = *(Sample+chnShft+subBand);
            		}
            		else
					{
						*(Sample+chnShft+subBand) = *(Sample+chnShft+subBand) | 0xffff8000;
            			*(Sample+chnShft+subBand) = *(Sample+chnShft+subBand) & maxNegMask[sampBits];
            		}

					if(*(Sample+64+chnShft+subBand) & 0x00008000)
					{
						*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand) & 0x00007fff;
            			*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand);
            		}
           			 else
					{
						*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand) | 0xffff8000;
            			*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand) & maxNegMask[sampBits];
            		}
					if(*(Sample+128+chnShft+subBand) & 0x00008000)
					{
						*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand) & 0x00007fff;
            			*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand);
            		}
           			 else
					{
						*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand) | 0xffff8000;
            			*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand) & maxNegMask[sampBits];
            		}
				}
            	/* check for grouping in subband */
				else
				{
					bits = bitsTable[*(gMPI.tableIndex[subBand].bandQuant 
						+ *(BitAlloc+chnShft+subBand))];
                    *(Sample + chnShft+subBand) = GetBits(bits) << (16 - bits);
                    *(Sample+64+chnShft+subBand) = GetBits(bits) << (16 - bits);
                    *(Sample+128+chnShft+subBand) = GetBits(bits) << (16 - bits);
					if(*(Sample + chnShft+subBand) & 0x00008000)	// if the MSB is 1
					{
						*(Sample+chnShft+subBand) = *(Sample+chnShft+subBand) & 0x00007fff;
            			*(Sample+chnShft+subBand) = *(Sample+chnShft+subBand);
            		}
            		else
					{
						*(Sample+chnShft+subBand) = *(Sample+chnShft+subBand) | 0xffff8000;
            			*(Sample+chnShft+subBand) = *(Sample+chnShft+subBand) & maxNegMask[bits];
            		}

					if(*(Sample+64+chnShft+subBand) & 0x00008000)
					{
						*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand) & 0x00007fff;
            			*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand);
            		}
           			 else
					{
						*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand) | 0xffff8000;
            			*(Sample+64+chnShft+subBand) = *(Sample+64+chnShft+subBand) & maxNegMask[bits];
            		}
					if(*(Sample+128+chnShft+subBand) & 0x00008000)
					{
						*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand) & 0x00007fff;
            			*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand);
            		}
           			 else
					{
						*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand) | 0xffff8000;
            			*(Sample+128+chnShft+subBand) = *(Sample+128+chnShft+subBand) & maxNegMask[bits];
            		}
                }         
        	}
			else
				*(Sample + chnShft+subBand) = *(Sample+64+chnShft+subBand) = *(Sample+128+chnShft+subBand) = 0;
			if(gMPI.chans == 2 && subBand>= gMPI.jSBound) /* joint stereo : copy L to R */
			{
				*(Sample+32+subBand) = *(Sample+subBand);
				*(Sample+96+subBand) = *(Sample+64+subBand);
				*(Sample+160+subBand) = *(Sample+128+subBand);
			}
		}
// a lot of this zeroing of tables is really not neccesray
    for(; subBand<SUBBAND_LIMIT; subBand++)
    	for(chan = 0; chan<gMPI.chans; chan++)
    	{
    	    chnShft = chan << 5;
        	*(Sample + chnShft+subBand) = *(Sample+64+chnShft+subBand) = *(Sample+128+chnShft+subBand) = 0;
        }
}
  
// SFCD is combined tables deQuantCII[18] * deQuantDII[18] * scaleFactors[64]
// SFA is combined tables deQuantCII[18] * scaleFactors[64]
// C and D are indexed by BitAlloc[channel][subBand]
// scale factor is indexed by ScaleIndex[samplegroup][channel][subBand]
// sample 0 left	0		sample 0 right	32
// sample 1	left	64		sample 1 right	96
// sample 2 left	128		sample 2 right	160
// sampleGroup should be 0 for 0-3, 64 for 4-7, 128 for 8-11


void	ReQuantMPEGIISample(short sampleGroup)
{
    long subBand, bits, btsShft;
    float sfcTmp, sfcdTmp;
	
    for(subBand = 0; subBand<gMPI.jSBound; subBand++)
    {
		if((bits = *(gMPI.tableIndex[subBand].bandQuant + *(BitAlloc+subBand))) == 0)
            *(Quant+subBand) = *(Quant+64+subBand) = *(Quant+128+subBand) = 0.0;
        else
        {
        	btsShft = bits << 6;
        	sfcTmp = *(SFC+btsShft+ *(ScaleIndex+sampleGroup+subBand));
        	sfcdTmp = *(SFCD+btsShft+ *(ScaleIndex+sampleGroup+subBand));
            *(Quant+subBand) = sfcTmp * (signed)*(Sample+subBand) + sfcdTmp;
            *(Quant+64+subBand) = sfcTmp * (signed)*(Sample+64+subBand) + sfcdTmp;
            *(Quant+128+subBand) = sfcTmp * (signed)*(Sample+128+subBand) + sfcdTmp;
        }
       	if(gMPI.chans == 2)
			if((bits = *(gMPI.tableIndex[subBand].bandQuant + *(BitAlloc+32+subBand))) == 0)
            	*(Quant+32+subBand) = *(Quant+96+subBand) = *(Quant+160+subBand) = 0.0;
        	else
        	{
        		btsShft = bits << 6;
        		sfcTmp = *(SFC+btsShft+ *(ScaleIndex+sampleGroup+32+subBand));
        		sfcdTmp = *(SFCD+btsShft+ *(ScaleIndex+sampleGroup+32+subBand));
            	*(Quant+32+subBand) = sfcTmp * (signed)*(Sample+32+subBand) + sfcdTmp;
            	*(Quant+96+subBand) = sfcTmp * (signed)*(Sample+96+subBand) + sfcdTmp;
            	*(Quant+160+subBand) = sfcTmp * (signed)*(Sample+160+subBand) + sfcdTmp;
            }
    }
    for(;subBand<gMPI.sBLimit;subBand++)
    {
        if ((bits = *(gMPI.tableIndex[subBand].bandQuant + *(BitAlloc+subBand))) == 0)
            *(Quant+32+subBand) = *(Quant+96+subBand) = *(Quant+160+subBand) = 0.0;
        else
        {
        	btsShft = bits << 6;
        	sfcTmp = *(SFC+btsShft+ *(ScaleIndex+sampleGroup+subBand));
        	sfcdTmp = *(SFCD+btsShft+ *(ScaleIndex+sampleGroup+subBand));
            *(Quant+subBand) = sfcTmp * (signed)*(Sample+subBand) + sfcdTmp;
            *(Quant+64+subBand) = sfcTmp * (signed)*(Sample+64+subBand) + sfcdTmp;
            *(Quant+128+subBand) = sfcTmp * (signed)*(Sample+128+subBand) + sfcdTmp;
        }
       	if(gMPI.chans == 2)
       	{
            *(Quant+32+subBand) = *(Quant+subBand);
            *(Quant+96+subBand) = *(Quant+64+subBand);
            *(Quant+160+subBand) = *(Quant+128+subBand);
        }
    }
    // more senseless setting values to zero
    // this time necessary as it is the last time.
    for(; subBand<SUBBAND_LIMIT; subBand++)
    	*(Quant+32+subBand) = *(Quant+subBand) = *(Quant+96+subBand)
    		= *(Quant+64+subBand) = *(Quant+160+subBand) = *(Quant+128+subBand) = 0.0;

}

void	MPEGIISynthesis(void)
{
    long subBand, sample, i, j, triple, tripleX64, accumL, accumR;
    float	tmpFloat;
    
	for(triple = 0; triple<3; triple++)
	{
		tripleX64 = triple << 6;
		for(sample = 1023; sample>63; sample--)
		{
			*(MatrixBuffer+sample) = *(MatrixBuffer+sample-64);
    	   	if(gMPI.chans == 2)
				*(MatrixBuffer+1024+sample) = *(MatrixBuffer+1024+sample-64);
		}

		for(sample = 0; sample<64; sample++)
		{
			*(MatrixBuffer+sample) = 0.0;
     	  	if(gMPI.chans == 2)
				*(MatrixBuffer+1024+sample) = 0.0;
    		for(subBand = 0; subBand<gMPI.jSBound; subBand++)
    		{
        		if(*(BitAlloc+subBand))
        		{
					*(MatrixBuffer+sample) += *(Quant+tripleX64+subBand) * *(Cosine32 + (subBand<<6) + sample);
       				if(gMPI.chans == 2)
            			*(MatrixBuffer+1024+sample) += *(Quant+tripleX64+32+subBand) * *(Cosine32 + (subBand<<6) + sample);
				}
			}
			for(;subBand<gMPI.sBLimit;subBand++)
			{
        		if(*(BitAlloc+subBand))
        		{
        			tmpFloat = *(Quant+tripleX64+subBand) * *(Cosine32 + (subBand<<6) + sample);
					*(MatrixBuffer+sample) += tmpFloat;
       				if(gMPI.chans == 2)
            			*(MatrixBuffer+1024+sample) += tmpFloat;
            	}
			}
		}
		for(i=0;i<8;i++)
			for(j=0;j<32;j++)
			{
				*(SynthBuffer+(i<<6)+j) = *(MatrixBuffer+(i<<7)+j);
				*(SynthBuffer+(i<<6)+32+j) = *(MatrixBuffer+(i<<7)+96+j);
       			if(gMPI.chans == 2)
       			{
					*(SynthBuffer+512+(i<<6)+j) = *(MatrixBuffer+1024+(i<<7)+j);
					*(SynthBuffer+544+(i<<6)+j) = *(MatrixBuffer+1120+(i<<7)+j);
 				}
 			}

 		for(j=0; j<32; j++)
 		{
 			accumR = accumL = 0L;
 			for(i=0; i<16; i++)
 			{
 				accumL += (long)(*(SynthBuffer+(i<<5)+j) * (deLongWindow[(i<<5)+j]));
       			if(gMPI.chans == 2)
 					accumR += (long)(*(SynthBuffer+512+(i<<5)+j) * (deLongWindow[(i<<5)+j]));
 			}
 			*(OutBuffer+gMPI.outPointer) = accumL >> 1;
       		if(gMPI.chans == 2)
 				*(OutBuffer+gMPI.outPointer+1) = accumR >> 1;
			gMPI.outPointer += gMPI.chans;
 		}
  	}
}
