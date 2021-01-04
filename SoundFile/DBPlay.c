#include <math.h>
#include <Sound.h>
#include <SoundComponents.h>
#include "SoundFile.h"
#include "AppleCompression.h"
#include "DBPlay.h"
#include "Dialog.h"
#include "Macros.h"
#include "Menu.h"
#include "Misc.h"
#include "SoundHack.h"
#include "�Law.h"
#include "ALaw.h"
#include "ADPCMDVI.h"
#include "CarbonGlue.h"
#if TARGET_API_MAC_CARBON == 1
#include "CarbonSndPlayDB.h"
#endif

#ifdef powerc
SndDoubleBackUPP	gDoubleBackProc;
SndDoubleBackUPP	gSw16DoubleBackProc;
SndDoubleBackUPP	gS8DoubleBackProc;
SndDoubleBackUPP	gADoubleBackProc;
SndDoubleBackUPP	gMuDoubleBackProc;
SndDoubleBackUPP	gADDVIDoubleBackProc;
SndDoubleBackUPP	gFltDoubleBackProc;
#endif

#define	SIZE_BLOCK	24576L

/* Globals */

extern ControlHandle	gSliderCntl;
extern MenuHandle		gControlMenu, gFileMenu;
extern short			gProcessEnabled;
extern short 			*gPlaySams;
extern SoundInfoPtr		frontSIPtr, playSIPtr;
extern EventRecord 		gTheEvent;
extern SndChannelPtr	gPermChannel;
DoubleBufPlayInfo		gDBPI;

long gNumAppleCompSamples;
Boolean gDecompressing;

void	DBPlayCallBackInit(void)
{
#ifdef powerc
#if TARGET_API_MAC_CARBON == 0
	gDoubleBackProc = NewSndDoubleBackUPP(DoubleBackProc);
	gS8DoubleBackProc = NewSndDoubleBackUPP(S8DoubleBackProc);
	gSw16DoubleBackProc = NewSndDoubleBackUPP(Sw16DoubleBackProc);
	gMuDoubleBackProc = NewSndDoubleBackUPP(MuDoubleBackProc);
	gADoubleBackProc = NewSndDoubleBackUPP(ADoubleBackProc);
	gADDVIDoubleBackProc = NewSndDoubleBackUPP(ADDVIDoubleBackProc);
	gFltDoubleBackProc = NewSndDoubleBackUPP(FltDoubleBackProc);
#endif
#endif
}

/* Based on the example in Inside Macintosh VI */
short	StartDBPlay(SoundInfo *mySI, double time, Boolean open)
{
	SndDoubleBufferPtr		doubleBuffer;
	
	short		i;
	unsigned long	startPos, tick;
	OSErr	error;
	Str255	tmpStr;
	
	playSIPtr = mySI;
	if(mySI->packMode == SF_FORMAT_TEXT || mySI->packMode == SF_FORMAT_32_LINEAR 
		|| mySI->packMode == SF_FORMAT_24_LINEAR || mySI->packMode == SF_FORMAT_SPECT_AMP 
		|| mySI->packMode == SF_FORMAT_SPECT_AMPPHS || mySI->packMode == SF_FORMAT_SPECT_AMPFRQ 
		|| mySI->packMode == SF_FORMAT_SPECT_MQ || mySI->packMode == SF_FORMAT_4_ADIMA 
		|| mySI->packMode == SF_FORMAT_MACE3 || mySI->packMode == SF_FORMAT_MACE6 )
	{
		Delay(120,&tick);
		return(-1);
	}
	if(open)
	{
		StringAppend("\pplay � ", mySI->sfSpec.name, tmpStr);
		UpdateProcessWindow("\p", "\p", tmpStr, -1.0);
	}
	// save the file position to restore it later.
	gDBPI.timeDivisor = (float)(mySI->numBytes/(mySI->frameSize * mySI->sRate * mySI->nChans));

	gDBPI.dFileNum = mySI->dFileNum;
	GetFPos(gDBPI.dFileNum, &(gDBPI.filePos));	
	startPos = (long)(time * mySI->frameSize * mySI->sRate * mySI->nChans);
	// the grossest boundary would be 32-bits by two channels (8-bytes) unless IMA, then on a 34 byte boundary
	if(mySI->packMode == SF_FORMAT_4_ADIMA)
		startPos = mySI->dataStart + ((startPos/34) * 34);
	else
		startPos = mySI->dataStart + ((startPos/8) * 8);
	
	gDBPI.numFrames = SIZE_BLOCK/mySI->nChans;
	gDBPI.numBytes = (long)(SIZE_BLOCK * mySI->frameSize);
	gDBPI.fileBlock = nil;
	gDBPI.fileBlock = (char *)NewPtr((long)(SIZE_BLOCK * mySI->frameSize));
	gDBPI.lastBlock = gDBPI.nextBlock = FALSE;
	gDBPI.doubleHeader.dbhNumChannels = mySI->nChans;
	
	switch(mySI->packMode)
	{
		case SF_FORMAT_4_ADDVI:
			gDBPI.doubleHeader.dbhSampleSize = 16L;
			gDBPI.doubleHeader.dbhFormat = kTwosComplement;
#if powerc & TARGET_API_MAC_CARBON == 0
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)gADDVIDoubleBackProc;
#else
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)ADDVIDoubleBackProc;
#endif
			break;
		case SF_FORMAT_8_UNSIGNED:
			gDBPI.doubleHeader.dbhSampleSize = (long)(mySI->frameSize * 8);
			gDBPI.doubleHeader.dbhFormat = kOffsetBinary;
#if powerc & TARGET_API_MAC_CARBON == 0
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)gDoubleBackProc;
#else
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)DoubleBackProc;
#endif
			break;
		case SF_FORMAT_8_LINEAR:
			gDBPI.doubleHeader.dbhSampleSize = (long)(mySI->frameSize * 8);
			gDBPI.doubleHeader.dbhFormat = kTwosComplement;
#if powerc & TARGET_API_MAC_CARBON == 0
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)gS8DoubleBackProc;
#else
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)S8DoubleBackProc;
#endif
			break;
		case SF_FORMAT_8_MULAW:
			gDBPI.doubleHeader.dbhSampleSize = 16L;
			gDBPI.doubleHeader.dbhFormat = kTwosComplement;
#if powerc & TARGET_API_MAC_CARBON == 0
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)gMuDoubleBackProc;
#else
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)MuDoubleBackProc;
#endif
			break;
		case SF_FORMAT_8_ALAW:
			gDBPI.doubleHeader.dbhSampleSize = 16L;
			gDBPI.doubleHeader.dbhFormat = kTwosComplement;
#if powerc & TARGET_API_MAC_CARBON == 0
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)gADoubleBackProc;
#else
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)ADoubleBackProc;
#endif
			break;
		case SF_FORMAT_16_SWAP:
			gDBPI.doubleHeader.dbhSampleSize = (long)(mySI->frameSize * 8);
			gDBPI.doubleHeader.dbhFormat = kTwosComplement;
#if powerc & TARGET_API_MAC_CARBON == 0
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)gSw16DoubleBackProc;
#else
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)Sw16DoubleBackProc;
#endif
			break;
		case SF_FORMAT_16_LINEAR:
		case SF_FORMAT_24_LINEAR:
		case SF_FORMAT_32_LINEAR:
			gDBPI.doubleHeader.dbhSampleSize = (long)(mySI->frameSize * 8);
			gDBPI.doubleHeader.dbhFormat = kTwosComplement;
#if powerc & TARGET_API_MAC_CARBON == 0
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)gDoubleBackProc;
#else
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)DoubleBackProc;
#endif
			break;
		case SF_FORMAT_32_FLOAT:
			gDBPI.doubleHeader.dbhSampleSize = 16L;
			gDBPI.doubleHeader.dbhFormat = kTwosComplement;
#if powerc & TARGET_API_MAC_CARBON == 0
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)gFltDoubleBackProc;
#else
			gDBPI.doubleHeader.dbhDoubleBack = (SndDoubleBackUPP)FltDoubleBackProc;
#endif
			break;
	}
	gDBPI.doubleHeader.dbhCompressionID = 0;
	gDBPI.doubleHeader.dbhPacketSize = 0;
    gDBPI.doubleHeader.dbhSampleRate = (unsigned long)(mySI->sRate * 65536);
	
	/* parameter block stuff for fast file routines */
	gDBPI.inSParmBlk = (ParmBlkPtr) NewPtrClear(sizeof(ParamBlockRec));;
	
	gDBPI.inSParmBlk->ioParam.ioCompletion = nil;
	gDBPI.inSParmBlk->ioParam.ioRefNum = mySI->dFileNum;
	gDBPI.inSParmBlk->ioParam.ioReqCount = (long)(SIZE_BLOCK * mySI->frameSize);
	gDBPI.inSParmBlk->ioParam.ioPosMode = fsFromStart;
	gDBPI.inSParmBlk->ioParam.ioPosOffset = startPos;
	gDBPI.inSParmBlk->ioParam.ioBuffer = gDBPI.fileBlock;
	
	/* Read First Buffer */
	error = PBRead(gDBPI.inSParmBlk, FALSE);
	
	if(mySI->packMode == SF_FORMAT_4_ADDVI)
		BlockADDVIDecode((unsigned char *)(nil), (short *)(nil), gDBPI.numFrames, 2, TRUE);
		
    /* Set up memory for double buffer */
	for (i = 0; i <= 1; ++i)
	{
		if(mySI->packMode == SF_FORMAT_8_MULAW || mySI->packMode == SF_FORMAT_8_ALAW)
			doubleBuffer = (SndDoubleBufferPtr)NewPtrClear(sizeof(SndDoubleBuffer) + (long)(SIZE_BLOCK * 2));
		else if(mySI->packMode == SF_FORMAT_4_ADDVI || mySI->packMode == SF_FORMAT_4_ADIMA 
			|| mySI->packMode == SF_FORMAT_MACE3 || mySI->packMode == SF_FORMAT_MACE6)
			doubleBuffer = (SndDoubleBufferPtr)NewPtrClear(sizeof(SndDoubleBuffer) + (long)(SIZE_BLOCK * 4));
		else if(mySI->packMode == SF_FORMAT_32_FLOAT)
			doubleBuffer = (SndDoubleBufferPtr)NewPtrClear(sizeof(SndDoubleBuffer) + (long)(SIZE_BLOCK * 2));
		else
			doubleBuffer = (SndDoubleBufferPtr)NewPtrClear(sizeof(SndDoubleBuffer) + (long)(SIZE_BLOCK * mySI->frameSize));
	
		if (doubleBuffer == nil || MemError() != 0)
    	{
    		DrawErrorMessage("\pNo Memory");
    		return(-1);
    	}
		
		doubleBuffer->dbNumFrames = 0;
		doubleBuffer->dbFlags = 0;
		doubleBuffer->dbUserInfo[0] = SetCurrentA5();
		doubleBuffer->dbUserInfo[1] = (long)mySI;
		if(mySI->packMode ==  SF_FORMAT_8_LINEAR)
			S8DoubleBackProc(gPermChannel, doubleBuffer);
		else if(mySI->packMode == SF_FORMAT_4_ADDVI)
			ADDVIDoubleBackProc(gPermChannel, doubleBuffer);
		else if(mySI->packMode == SF_FORMAT_8_MULAW)
			MuDoubleBackProc(gPermChannel, doubleBuffer);
		else if(mySI->packMode == SF_FORMAT_8_ALAW)
			ADoubleBackProc(gPermChannel, doubleBuffer);
		else if(mySI->packMode == SF_FORMAT_16_SWAP)
			Sw16DoubleBackProc(gPermChannel, doubleBuffer);
		else if(mySI->packMode == SF_FORMAT_32_FLOAT)
			FltDoubleBackProc(gPermChannel, doubleBuffer);
		else
			DoubleBackProc(gPermChannel, doubleBuffer);
		gDBPI.doubleHeader.dbhBufferPtr[i] = doubleBuffer;
		while(gDBPI.inSParmBlk->ioParam.ioResult == 1);
	}
	
	if(open)
	{
		gProcessEnabled = PLAY_PROCESS;
		DisableItem(gControlMenu,PAUSE_ITEM);
		DisableItem(gControlMenu,CONTINUE_ITEM);
		DisableItem(gControlMenu,STOP_ITEM);
		
		MenuUpdate();
	}
#if TARGET_API_MAC_CARBON == 1
	CarbonSndPlayDoubleBuffer(gPermChannel, (SndDoubleBufferHeaderPtr)&gDBPI.doubleHeader);
#else
	SndPlayDoubleBuffer(gPermChannel, (SndDoubleBufferHeaderPtr)&gDBPI.doubleHeader);
#endif
	if(open)
		gDBPI.ticksStart = TickCount() - (long)(time * 60);
	return(1);
}

// wait: wait for the end of the currently playing file?
// open: is the file open or not? use the gui if it is.
void
StopDBPlay(Boolean wait, Boolean open)
{
	SCStatus	status;
	SndCommand	command;
	OSErr		iErr;
	
	short	i;
	
	if(gPermChannel == 0L)
		return;
	if(wait)
		do
		{
			SndChannelStatus(gPermChannel, sizeof (status), &status);
			HandleEvent();
		}while (status.scChannelBusy);
	
	command.cmd = quietCmd;
	command.param1 = 0;
	command.param2 = 0;
	iErr = MySndDoImmediate(gPermChannel,&command);
	
	gDBPI.lastBlock = TRUE;
	do
	{
		SndChannelStatus(gPermChannel, sizeof (status), &status);
	}while (status.scChannelBusy);
	
	for (i = 0; i <= 1; ++i)
		RemovePtr((Ptr) gDBPI.doubleHeader.dbhBufferPtr[i]);
	RemovePtr(gDBPI.fileBlock);
	SetFPos(gDBPI.dFileNum, fsFromStart, gDBPI.filePos);	
	gProcessEnabled = NO_PROCESS;
	if(open)
	{
		UpdateProcessWindow("\p", "\p", "\pidle", -1.0);
		HandleProcessWindowEvent(gTheEvent.where, TRUE);
		MenuUpdate();
	}
	playSIPtr = nil;
}

void
FinishPlay(void)
{
	if(gProcessEnabled == PLAY_PROCESS)
	{
		StopDBPlay(FALSE, TRUE);
		SetMenuItemText(gFileMenu, PLAY_ITEM, "\pPlay File (space bar)");
	}
}
	
void
RestartPlay(void)
{
	float	startTime, frontLength;
	unsigned long	controlValue;
	SoundInfoPtr	tmpSIPtr;
	
	if(gProcessEnabled == PLAY_PROCESS)
	{
		tmpSIPtr = playSIPtr;
		StopDBPlay(FALSE, TRUE);
		SetMenuItemText(gFileMenu, PLAY_ITEM, "\pPlay File (space bar)");
		controlValue = GetControlValue(gSliderCntl);
		frontLength = tmpSIPtr->numBytes/(tmpSIPtr->sRate * tmpSIPtr->nChans * tmpSIPtr->frameSize);
		startTime = (controlValue * frontLength)/420.0;
		SetMenuItemText(gFileMenu, PLAY_ITEM, "\pStop Play (space)");
		StartDBPlay(tmpSIPtr, startTime, TRUE);
	}
}
	

pascal void
DoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer)
{
	long	theA5;
	SoundInfo	*localSI;
	OSErr	error;
	static long loopInit, tmpLong;
	
	theA5 = SetA5(doubleBuffer->dbUserInfo[0]);

	localSI = (SoundInfo *)doubleBuffer->dbUserInfo[1];
	if(gDBPI.inSParmBlk->ioParam.ioResult != 0 && gDBPI.inSParmBlk->ioParam.ioResult != -39)
	{
	// time to fuck up playback
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
		theA5 = SetA5(theA5);
		return;
	}
	BlockMoveData (&(gDBPI.fileBlock[0]),&(doubleBuffer->dbSoundData[0]),gDBPI.inSParmBlk->ioParam.ioActCount);
	doubleBuffer->dbNumFrames = gDBPI.inSParmBlk->ioParam.ioActCount/(long)(localSI->frameSize*localSI->nChans);
	/* Mark buffer as ready */
	doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
	if((gDBPI.inSParmBlk->ioParam.ioActCount != gDBPI.inSParmBlk->ioParam.ioReqCount)||gDBPI.nextBlock)
	{
			doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
			gDBPI.lastBlock = TRUE;
	}
	else if(gDBPI.lastBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
	}
	else
	{
		if(loopInit)
		{
			gDBPI.inSParmBlk->ioParam.ioPosOffset = localSI->dataStart;
			gDBPI.inSParmBlk->ioParam.ioReqCount = tmpLong;
			loopInit = FALSE;
		}
		if((gDBPI.inSParmBlk->ioParam.ioPosOffset + gDBPI.inSParmBlk->ioParam.ioReqCount)
			> (localSI->dataStart + localSI->numBytes))
		{
			if(gDBPI.loop)
			{
				tmpLong = gDBPI.inSParmBlk->ioParam.ioReqCount;
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
				loopInit = TRUE;
			}
			else
			{
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				gDBPI.nextBlock = TRUE;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
			}
		}
		else
			error = PBRead(gDBPI.inSParmBlk, TRUE);
	}
	theA5 = SetA5(theA5);
}

// this one will swap 16 bit Intel order files.
pascal void
Sw16DoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer)
{
	long	theA5;
	SoundInfo	*localSI;
	OSErr	error;
	static long loopInit, tmpLong;
	
	theA5 = SetA5(doubleBuffer->dbUserInfo[0]);

	localSI = (SoundInfo *)doubleBuffer->dbUserInfo[1];
	if(gDBPI.inSParmBlk->ioParam.ioResult != 0 && gDBPI.inSParmBlk->ioParam.ioResult != -39)
	{
	// time to fuck up playback
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
		theA5 = SetA5(theA5);
		return;
	}
	Sw16BlockMove (&(gDBPI.fileBlock[0]),(Ptr) &(doubleBuffer->dbSoundData[0]),gDBPI.inSParmBlk->ioParam.ioActCount);
	doubleBuffer->dbNumFrames = gDBPI.inSParmBlk->ioParam.ioActCount/(long)(localSI->frameSize*localSI->nChans);
	/* Mark buffer as ready */
	doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;

	if((gDBPI.inSParmBlk->ioParam.ioActCount != gDBPI.inSParmBlk->ioParam.ioReqCount)||gDBPI.nextBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
		gDBPI.lastBlock = TRUE;
	}else if(gDBPI.lastBlock)
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
	else
	{
		if(loopInit)
		{
			gDBPI.inSParmBlk->ioParam.ioPosOffset = localSI->dataStart;
			gDBPI.inSParmBlk->ioParam.ioReqCount = tmpLong;
			loopInit = FALSE;
		}
		if((gDBPI.inSParmBlk->ioParam.ioPosOffset + gDBPI.inSParmBlk->ioParam.ioReqCount)
			> (localSI->dataStart + localSI->numBytes))
		{
			if(gDBPI.loop)
			{
				tmpLong = gDBPI.inSParmBlk->ioParam.ioReqCount;
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
				loopInit = TRUE;
			}
			else
			{
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				gDBPI.nextBlock = TRUE;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
			}
		}
		else
			error = PBRead(gDBPI.inSParmBlk, TRUE);
	}
	theA5 = SetA5(theA5);
}

void
Sw16BlockMove(Ptr inBlock, Ptr outBlock, long size)
{
	long	i;
	for (i = 0; i < size; i += 2)
		* (short *)(outBlock + i) = (* (unsigned short *)(inBlock + i) << 8) + (* (unsigned short *)(inBlock + i) >> 8);
}

// my assumption is that signed 16 and unsigned 8 will play back ok everywhere.
pascal void
S8DoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer)
{
	long	theA5;
	SoundInfo	*localSI;
	OSErr	error;
	static long loopInit, tmpLong;
	
	theA5 = SetA5(doubleBuffer->dbUserInfo[0]);

	localSI = (SoundInfo *)doubleBuffer->dbUserInfo[1];
	if(gDBPI.inSParmBlk->ioParam.ioResult != 0 && gDBPI.inSParmBlk->ioParam.ioResult != -39)
	{
	// time to fuck up playback
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
		theA5 = SetA5(theA5);
		return;
	}
	BlockMoveData(&(gDBPI.fileBlock[0]),&(doubleBuffer->dbSoundData[0]),gDBPI.inSParmBlk->ioParam.ioActCount);
 	SignToUnsignBlock((Ptr)&(doubleBuffer->dbSoundData[0]), gDBPI.inSParmBlk->ioParam.ioActCount);
	doubleBuffer->dbNumFrames = gDBPI.inSParmBlk->ioParam.ioActCount/(long)(localSI->frameSize*localSI->nChans);
 	/* Mark buffer as ready */
	doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;

	if((gDBPI.inSParmBlk->ioParam.ioActCount != gDBPI.inSParmBlk->ioParam.ioReqCount) || gDBPI.nextBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
		gDBPI.lastBlock = TRUE;
	}else if(gDBPI.lastBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
	}else
	{
		if(loopInit)
		{
			gDBPI.inSParmBlk->ioParam.ioPosOffset = localSI->dataStart;
			gDBPI.inSParmBlk->ioParam.ioReqCount = tmpLong;
			loopInit = FALSE;
		}
		if((gDBPI.inSParmBlk->ioParam.ioPosOffset + gDBPI.inSParmBlk->ioParam.ioReqCount)
			> (localSI->dataStart + localSI->numBytes))
		{
			if(gDBPI.loop)
			{
				tmpLong = gDBPI.inSParmBlk->ioParam.ioReqCount;
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
				loopInit = TRUE;
			}
			else
			{
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				gDBPI.nextBlock = TRUE;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
			}
		}
		else
			error = PBRead(gDBPI.inSParmBlk, TRUE);
	}
	theA5 = SetA5(theA5);
}

void 
SignToUnsignBlock(Ptr block, long size)
{
	long	i;
	for (i = (size >> 2) - 1; i >= 0; --i)
	{
		* (long *)block ^= 0x80808080;
		block += 4;
	}				
	for (i = (size & 3) - 1; i >= 0; --i)
		* (char *)block++ ^= 0x80;			
}

// the start of mulaw playback????
// have to adjust size of file in, size of buffer out, size of conversion
pascal void
MuDoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer)
{
	long	theA5;
	SoundInfo	*localSI;
	OSErr	error;
	static long loopInit, tmpLong;
	
	theA5 = SetA5(doubleBuffer->dbUserInfo[0]);

	localSI = (SoundInfo *)doubleBuffer->dbUserInfo[1];
	if(gDBPI.inSParmBlk->ioParam.ioResult != 0 && gDBPI.inSParmBlk->ioParam.ioResult != -39)
	{
	// time to fuck up playback
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
		theA5 = SetA5(theA5);
		return;
	}
	Ulaw2ShortBlock((unsigned char *)(gDBPI.fileBlock), (short *)doubleBuffer->dbSoundData, gDBPI.inSParmBlk->ioParam.ioActCount);
	doubleBuffer->dbNumFrames = gDBPI.inSParmBlk->ioParam.ioActCount/(long)(localSI->frameSize*localSI->nChans);
 	/* Mark buffer as ready */
	doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;

	if((gDBPI.inSParmBlk->ioParam.ioActCount != gDBPI.inSParmBlk->ioParam.ioReqCount) || gDBPI.nextBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
		gDBPI.lastBlock = TRUE;
	}else if(gDBPI.lastBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
	}else
	{
		if(loopInit)
		{
			gDBPI.inSParmBlk->ioParam.ioPosOffset = localSI->dataStart;
			gDBPI.inSParmBlk->ioParam.ioReqCount = tmpLong;
			loopInit = FALSE;
		}
		if((gDBPI.inSParmBlk->ioParam.ioPosOffset + gDBPI.inSParmBlk->ioParam.ioReqCount)
			> (localSI->dataStart + localSI->numBytes))
		{
			if(gDBPI.loop)
			{
				tmpLong = gDBPI.inSParmBlk->ioParam.ioReqCount;
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
				loopInit = TRUE;
			}
			else
			{
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				gDBPI.nextBlock = TRUE;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
			}
		}
		else
			error = PBRead(gDBPI.inSParmBlk, TRUE);
	}
	theA5 = SetA5(theA5);
}

// have to adjust size of file in, size of buffer out, size of conversion
pascal void
ADoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer)
{
	long	theA5;
	SoundInfo	*localSI;
	OSErr	error;
	static long loopInit, tmpLong;
	
	theA5 = SetA5(doubleBuffer->dbUserInfo[0]);

	localSI = (SoundInfo *)doubleBuffer->dbUserInfo[1];
	if(gDBPI.inSParmBlk->ioParam.ioResult != 0 && gDBPI.inSParmBlk->ioParam.ioResult != -39)
	{
	// time to fuck up playback
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
		theA5 = SetA5(theA5);
		return;
	}
	Alaw2ShortBlock((unsigned char *)(gDBPI.fileBlock), (short *)doubleBuffer->dbSoundData, gDBPI.inSParmBlk->ioParam.ioActCount);
	doubleBuffer->dbNumFrames = gDBPI.inSParmBlk->ioParam.ioActCount/(long)(localSI->frameSize*localSI->nChans);
 	/* Mark buffer as ready */
	doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;

	if((gDBPI.inSParmBlk->ioParam.ioActCount != gDBPI.inSParmBlk->ioParam.ioReqCount) || gDBPI.nextBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
		gDBPI.lastBlock = TRUE;
	}else if(gDBPI.lastBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
	}else
	{
		if(loopInit)
		{
			gDBPI.inSParmBlk->ioParam.ioPosOffset = localSI->dataStart;
			gDBPI.inSParmBlk->ioParam.ioReqCount = tmpLong;
			loopInit = FALSE;
		}
		if((gDBPI.inSParmBlk->ioParam.ioPosOffset + gDBPI.inSParmBlk->ioParam.ioReqCount)
			> (localSI->dataStart + localSI->numBytes))
		{
			if(gDBPI.loop)
			{
				tmpLong = gDBPI.inSParmBlk->ioParam.ioReqCount;
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
				loopInit = TRUE;
			}
			else
			{
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				gDBPI.nextBlock = TRUE;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
			}
		}
		else
			error = PBRead(gDBPI.inSParmBlk, TRUE);
	}
	theA5 = SetA5(theA5);
}

pascal void	ADDVIDoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer)
{
	long	theA5;
	SoundInfo	*localSI;
	OSErr	error;
	static long loopInit, tmpLong;
	
	theA5 = SetA5(doubleBuffer->dbUserInfo[0]);

	localSI = (SoundInfo *)doubleBuffer->dbUserInfo[1];
	if(gDBPI.inSParmBlk->ioParam.ioResult != 0 && gDBPI.inSParmBlk->ioParam.ioResult != -39)
	{
	// time to fuck up playback
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
		theA5 = SetA5(theA5);
		return;
	}
	doubleBuffer->dbNumFrames = (gDBPI.inSParmBlk->ioParam.ioActCount/localSI->nChans) << 1;
	BlockADDVIDecode((unsigned char *)(gDBPI.fileBlock), (short *)doubleBuffer->dbSoundData, 
		doubleBuffer->dbNumFrames, localSI->nChans, FALSE);
 	/* Mark buffer as ready */
	doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;

	if((gDBPI.inSParmBlk->ioParam.ioActCount != gDBPI.inSParmBlk->ioParam.ioReqCount) || gDBPI.nextBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
		gDBPI.lastBlock = TRUE;
	}else if(gDBPI.lastBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
	}else
	{
		if(loopInit)
		{
			gDBPI.inSParmBlk->ioParam.ioPosOffset = localSI->dataStart;
			gDBPI.inSParmBlk->ioParam.ioReqCount = tmpLong;
			loopInit = FALSE;
		}
		if((gDBPI.inSParmBlk->ioParam.ioPosOffset + gDBPI.inSParmBlk->ioParam.ioReqCount)
			> (localSI->dataStart + localSI->numBytes))
		{
			if(gDBPI.loop)
			{
				tmpLong = gDBPI.inSParmBlk->ioParam.ioReqCount;
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
				loopInit = TRUE;
			}
			else
			{
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				gDBPI.nextBlock = TRUE;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
			}
		}
		else
			error = PBRead(gDBPI.inSParmBlk, TRUE);
	}
	theA5 = SetA5(theA5);
}

// have to adjust size of file in, size of buffer out, size of conversion
pascal void
FltDoubleBackProc(SndChannelPtr myChannel, SndDoubleBufferPtr doubleBuffer)
{
	long	theA5;
	SoundInfo	*localSI;
	OSErr	error;
	float	gain;
	static long loopInit, tmpLong;
	
	theA5 = SetA5(doubleBuffer->dbUserInfo[0]);

	localSI = (SoundInfo *)doubleBuffer->dbUserInfo[1];
	if(gDBPI.inSParmBlk->ioParam.ioResult != 0 && gDBPI.inSParmBlk->ioParam.ioResult != -39)
	{
	// time to fuck up playback
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
		theA5 = SetA5(theA5);
		return;
	}
	
	gain = 32767.0/localSI->peak;
	Flt2ShortBlock((float *)(gDBPI.fileBlock), (short *)doubleBuffer->dbSoundData, gDBPI.inSParmBlk->ioParam.ioActCount, gain);
	doubleBuffer->dbNumFrames = gDBPI.inSParmBlk->ioParam.ioActCount/(long)(localSI->frameSize*localSI->nChans);
 	/* Mark buffer as ready */
	doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;

	if((gDBPI.inSParmBlk->ioParam.ioActCount != gDBPI.inSParmBlk->ioParam.ioReqCount) || gDBPI.nextBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
		gDBPI.lastBlock = TRUE;
	}else if(gDBPI.lastBlock)
	{
		doubleBuffer->dbFlags = (doubleBuffer->dbFlags) | dbLastBuffer;
	}else
	{
		if(loopInit)
		{
			gDBPI.inSParmBlk->ioParam.ioPosOffset = localSI->dataStart;
			gDBPI.inSParmBlk->ioParam.ioReqCount = tmpLong;
			loopInit = FALSE;
		}
		if((gDBPI.inSParmBlk->ioParam.ioPosOffset + gDBPI.inSParmBlk->ioParam.ioReqCount)
			> (localSI->dataStart + localSI->numBytes))
		{
			if(gDBPI.loop)
			{
				tmpLong = gDBPI.inSParmBlk->ioParam.ioReqCount;
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
				loopInit = TRUE;
			}
			else
			{
				gDBPI.inSParmBlk->ioParam.ioReqCount = (localSI->dataStart + localSI->numBytes) - gDBPI.inSParmBlk->ioParam.ioPosOffset;
				gDBPI.nextBlock = TRUE;
				error = PBRead(gDBPI.inSParmBlk, TRUE);
			}
		}
		else
			error = PBRead(gDBPI.inSParmBlk, TRUE);
	}
	theA5 = SetA5(theA5);
}

void Flt2ShortBlock(float *floatSams, short *shortSams, long size, float gain)
{
	long i;
	
	for(i=0; i < (size>>2); i++)
		*(short *)(shortSams + i) = *(float *)(floatSams + i) * gain;
}
