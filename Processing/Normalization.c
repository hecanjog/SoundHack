#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
//#include <AppleEvents.h>
#include "SoundFile.h"
#include "Dialog.h"
#include "Gain.h"
#include "Menu.h"
#include "SoundHack.h"
#include "Normalization.h"
#include "Misc.h"

long				gNumSamps, gNumBytes;
float				*normBlock;
extern Boolean		gNormalize;
extern long			gNumberBlocks;
extern MenuHandle	gAppleMenu, gFileMenu, gEditMenu, gProcessMenu, gControlMenu;
extern GainInfo		gGI;
extern short		gProcessEnabled, gStopProcess;
extern float		gScale, gScaleL, gScaleR;

SoundInfoPtr		gNormSI;

void
InitNormProcess(SoundInfo *mySI)
{	
	long	blockSize;

	gNumBytes = mySI->numBytes;	/* A steady copy */
	gNormSI = mySI;
	
	blockSize = 16384L;
	
	normBlock = (float *)NewPtr(blockSize * sizeof(float) * mySI->nChans);

	UpdateProcessWindow("\p", "\p", "\pnormalzing file", 0.0);
	
	gGI.peakCh2 = gGI.peakCh1 = 0.0;

	gNumSamps = gNumberBlocks = 0;
	gProcessEnabled = NORM_PROCESS;
	MenuUpdate();
}

short
NormBlock(void)
{
	long	readSize, writeSize, blockSize, n;
	Str255	errStr;
	float	length;
	
	blockSize = 16384L;
	if(gNormalize == TRUE)
	{
		UpdateProcessWindow("\p", "\p", "\plooking for peak", -1.0);
		SetFPos(gNormSI->dFileNum, fsFromStart, (gNormSI->dataStart 
			+ (blockSize * gNormSI->frameSize * gNumberBlocks)));
		readSize = ReadMonoBlock(gNormSI, blockSize, normBlock);
		for(n = 0; n < readSize; n++)
		{
			if((gGI.peakCh1 < *(normBlock + n)) && (0.0 < *(normBlock + n)))
				gGI.peakCh1 = *(normBlock + n);
			else if((gGI.peakCh1 < -(*(normBlock + n))) && (0.0 < -(*(normBlock + n))))
				gGI.peakCh1 = -(*(normBlock + n));
		}
		gNumSamps = gNumSamps + readSize;
		length = (float)(gNumSamps/(gNormSI->nChans*gNormSI->sRate));
		HMSTimeString(length, errStr);
		length = length / (gNormSI->numBytes/(gNormSI->sRate * gNormSI->nChans * gNormSI->frameSize));
		UpdateProcessWindow(errStr, "\p", "\p", length);
		gNumberBlocks++;
		if(readSize != blockSize)
		{
			if(gNormSI->packMode == SF_FORMAT_8_LINEAR)
				gScale = gScaleL= gScaleR = 127.0/gGI.peakCh1;
			else if(gNormSI->packMode == SF_FORMAT_24_LINEAR || gNormSI->packMode == SF_FORMAT_24_COMP 
				|| gNormSI->packMode == SF_FORMAT_32_LINEAR || gNormSI->packMode == SF_FORMAT_32_COMP
				|| gNormSI->packMode == SF_FORMAT_24_SWAP || gNormSI->packMode == SF_FORMAT_32_SWAP)
				gScale = gScaleL = gScaleR = 2147483648.0/gGI.peakCh1;
			else
				gScale = gScaleL= gScaleR = 32767.0/gGI.peakCh1;
			gNormalize = FALSE;
			gNumberBlocks = 0;
			gNumSamps = 0;
		}else
			return(0);
	}else if(gStopProcess == TRUE)
	{
		FinishProcess();
		gNumberBlocks = 0;
		RemovePtr((Ptr)normBlock);
		return(-1);
	}
	UpdateProcessWindow("\p", "\p", "\pchanging gain", -1.0);
	SetFPos(gNormSI->dFileNum, fsFromStart, (gNormSI->dataStart 
		+ (blockSize * gNormSI->frameSize * gNumberBlocks)));
	readSize = ReadMonoBlock(gNormSI, blockSize, normBlock);
	
	SetFPos(gNormSI->dFileNum, fsFromStart, (gNormSI->dataStart 
		+ (blockSize * gNormSI->frameSize * gNumberBlocks)));
	writeSize = WriteMonoBlock(gNormSI, readSize, normBlock);
	gNormSI->numBytes = gNumBytes;	/* WriteMonoBlock screws with numBytes */
	gNumSamps = gNumSamps + readSize;
	length = (float)(gNumSamps/(gNormSI->nChans*gNormSI->sRate));
	HMSTimeString(length, errStr);
	length = length / (gNormSI->numBytes/(gNormSI->sRate * gNormSI->nChans * gNormSI->frameSize));
	UpdateProcessWindow("\p", errStr, "\p", length);
	gNumberBlocks++;
	if(writeSize != blockSize)
	{
		RemovePtr((Ptr)normBlock);
		FinishProcess();
		gNumberBlocks = 0;
	}
	return(TRUE);
}
