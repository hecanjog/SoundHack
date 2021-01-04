/**************************************************************************
  
	Mac IMA4 Variant - Decompression code
	� 1996 Mark F. Coniglio
	all rights reserved
	
	Compression Code
	in the public domain
	by Mark Stout, Compaq Computer, 1/92

****************************************************************************/

#include "Macros.h"
#include "SoundFile.h"
#include "ADPCMIMA.h"
#include "ADPCM.h"

//	GetIMA4LoopOffset
//	given the sample number this returns the byte offset, 
//	from frame 0, of the beginning of a frame of IMA sound data.
//	Note that sampNum is forced to a 64 sample boundardy,
//	the beginning of a block

long GetIMA4LoopOffset(long sampNum, Boolean stereo)
{
	long	blocks, byteNum;
	
	if (!stereo)
	{							// if mono
		// we have a sample number, find the byte boundary?
		sampNum -= sampNum % 64;		// force sample number to a 64 sample boundary
		byteNum = sampNum >> 1;			// 2 frames per byte
		// now account for the two bytes of state info in each block
		blocks = byteNum / 32;			// number of blocks till loop point
		byteNum += blocks * 2;			// add two bytes of state info for each block				
	}
	else
	{									// if stereo
		sampNum -= sampNum % 128;		// loop points are on a 128 frame boundary
		byteNum = sampNum >> 1;			// 2 frames per byte
		blocks = byteNum / 32;			// number of blocks till loop point
		byteNum += blocks * 2;			// add two bytes of state info for each block				
		byteNum *= 2;					// double offset for stereo
	}
	return(byteNum);
}

/*********************************************************************************
* ExpIMA4OneBlock
*
*	Expands one IMA4 data block.
*
*	imaBlock points to a 34 byte data block. Each block consists of
*	otwo bytes of state information followed by 64 samples (32 bytes)
*	of data.
*
*	shortBlock points to an 16 bit output buffer. Note that if the stereo
*	flag is true, this skips every other word to facilitate stereo
*	decoding.
*
*********************************************************************************/
void ExpIMA4OneBlock(register char *imaBlock, register short *shortBlock, Boolean stereo)
{
	register short	encodedSample;
	register short	stepSize;
	register short	stepIndex;
	
	long			stateWord;
	long			outputSample;
	long			count = kIMABlockSize;				// each block has 34 data bytes
	Boolean			odd = TRUE;
	
//	state word is upper 9 bits from sample value, 
//	followed by 7 lower bits to set the stepIndex
	stateWord = *((short*) (imaBlock));			// grab state information... (first two chars)
	outputSample = stateWord & 0xFFFFFF80;		// top 9 go to outputSample
	stepIndex = stateWord & 0x007F;				// bottom seven to stepIndex
	stepSize = gStepSizes[stepIndex];
		
	imaBlock += 2;								// increment position of buffer
	count -= 2;									// decrement byte count
	
	while(count > 0)
	{
		if(odd)									// if on the odd sample
		{					
			encodedSample = (*imaBlock) & 0x0F;	// use low nybble
			odd = FALSE;						// set odd to FALSE
		}
		else									// if on the even sample
		{							
			encodedSample = ((*(imaBlock++)) >> 4) & 0x0F;	// use high nybble
			count--;							// decrement count
			odd = TRUE;							// set odd to TRUE
		}
		
		////////////////////////////////////////////////
		// compute new sample estimate startSam 
		////////////////////////////////////////////////
		outputSample += DecodeDelta(stepSize, encodedSample);
		CLIP(outputSample, -32768L, 32767L);			
		*(shortBlock++) = outputSample;
		if (stereo)								// if decoding to stereo
			shortBlock++;							// skip an extra word
		
		////////////////////////////////////////////////
		// compute new stepsize
		////////////////////////////////////////////////
		stepIndex += gIndexDeltas[encodedSample];		
		CLIP(stepIndex, 0, 88);
		stepSize = gStepSizes[stepIndex];
	}
}

// Decode the requested number of samples into the *shortSam block.
// Interleaved if neccesary.
// This is used only for playback, so we can force alignment.
long
BlockADIMADecodeShort(char *imaBlock, short *shortBlock, long insize, long channels)
{
	long	outcount = 0;
	static short	overFlow[128];
	
	if(channels == STEREO)
	{
		while (insize > 0)
		{
			ExpIMA4OneBlock(imaBlock, shortBlock, TRUE);	// expand the left block
			imaBlock += kIMABlockSize;					// increment the input buffer ptr
			ExpIMA4OneBlock(imaBlock, shortBlock+1, TRUE);	// expand the right block
			imaBlock += kIMABlockSize;					// increment the input buffer ptr
		
			shortBlock += (kIMASamplesPerBlock * 2);		// move to next pair of blocks
			insize -= (kIMABlockSize * 2);				// we took two blocks from the buffer
			outcount += (kIMASamplesPerBlock * 2 * 2);	// samps per block * 2 bytes per samp * 2 chan
		}
	}
	else
	{
		while (insize > 0)
		{
			ExpIMA4OneBlock(imaBlock, shortBlock, FALSE);	// expand the block
			imaBlock += kIMABlockSize;					// increment the input buffer ptr
			shortBlock += (kIMASamplesPerBlock * 1);		// move to next block
			insize -= (kIMABlockSize * 1);				// we took one block from the buffer
			outcount += (kIMASamplesPerBlock * 2 * 1);	// samps per block * 2 bytes per samp * 1 chan
		}
	}
	return outcount;
}

// Decode the requested number of samples into *blockL and, if stereo, *blockR
long
BlockADIMADecodeFloat(char *imaBlock, float *blockL, float *blockR, long numSamples, long channels)
{
}

/*********************************************************************************
* ExpIMA4Stereo
*
*	Expands a stereo IMA4 ADPCM buffer.
*
*	The source buffer contains a series of IMA4 Blocks, each consisting
*	of two bytes of state information followed by 64 samples (32 bytes)
*	of data. This data is decoded and stored in the output buffer
*	in the stereo 16 bit interleaved format.
*
*	** The source buffer must be an integral multiple of IMA4BlkSize * 2 **
*
*********************************************************************************/
long ExpIMA4Stereo(char *imaBlock, short *shortBlock, long insize)
{
	long	outcount = 0;
	
	while (insize > 0)
	{
		
		ExpIMA4OneBlock(imaBlock, shortBlock, TRUE);	// expand the left block
		imaBlock += kIMABlockSize;					// increment the input buffer ptr
		
		ExpIMA4OneBlock(imaBlock, shortBlock+1, TRUE);	// expand the right block
		imaBlock += kIMABlockSize;					// increment the input buffer ptr
		
		shortBlock += (kIMASamplesPerBlock * 2);		// move to next pair of blocks
		insize -= (kIMABlockSize * 2);				// we took two blocks from the buffer
		outcount += (kIMASamplesPerBlock * 2 * 2);	// samps per block * 2 bytes per samp * 2 chan
	}
	
	if (insize != 0)
		DebugStr("\pInvalid Buffer Size in IMA4 Stereo");
		
	return outcount;
}

/*********************************************************************************
* ExpIMA4Mono
*
*	Expands a monophonic IMA4 ADPCM buffer.
*
*	The source buffer contains a series of IMA4 Blocks, each consisting
*	of two bytes of state information followed by 64 samples (32 bytes)
*	of data. This data is decoded and stored in the output buffer
*	as a series of 16 bit samples.
*
*	** The source buffer must be an integral multiple of IMA4BlkSize **
*
*********************************************************************************/
long ExpIMA4Mono(char *imaBlock, short *shortBlock, long insize)
{
	long	outcount = 0;
	
	while (insize > 0)
	{
		
		ExpIMA4OneBlock(imaBlock, shortBlock, FALSE);	// expand the block
		imaBlock += kIMABlockSize;					// increment the input buffer ptr
		
		shortBlock += (kIMASamplesPerBlock * 1);		// move to next block
		insize -= (kIMABlockSize * 1);				// we took one block from the buffer
		outcount += (kIMASamplesPerBlock * 2 * 1);	// samps per block * 2 bytes per samp * 1 chan
	}
	
	if (insize != 0)
		DebugStr("\pInvalid Buffer Size in IMA4 Mono");
		
	return outcount;
}