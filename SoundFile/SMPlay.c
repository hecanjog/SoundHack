/*
	File:		PlaySoundConvertBuffer.c
	
	Description: Play Sound shows how to use the Sound Description Extention atom information with the
				 SoundConverter APIs to play non VBR MP3 files as well as other types of sound files
				 using various encoding methods.

	Author:		mc, era

	Copyright: 	� Copyright 2000 Apple Computer, Inc. All rights reserved.
	
	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
				
	Change History (most recent first): <2> 7/26/00 sans hickup and carbonized for CarbonLib 1.04
										<1> 4/01/00 initial release
*/

#include "SoundFile.h"
#include "DBPlay.h"
#include "SMPlay.h"
#include "Menu.h"
#include "SoundHack.h"
#include "CarbonGlue.h"

extern short			gProcessEnabled;
extern MenuHandle		gControlMenu, gFileMenu;
extern DoubleBufPlayInfo		gDBPI;
extern SndChannelPtr	gSMChannel;
extern SoundInfoPtr		frontSIPtr, playSIPtr;


const short kAppleMenu = 128;
	const short kAbout = 1;

const short kFileMenu = 129;
	const short kOpenItem = 1;
	const short kSeparatorItem = 2;
	const short kQuitItem = 3;

// makes our buffers a little bigger so that if the decompressors overflow
// we don't corrupt memory which would suck
const short kOverflowRoom = 32;
const UInt32 kMaxInputBuffer = 64 * 1024; // max size of input buffer


// globals
Boolean gBufferDone;

struct	{
	Handle		hSys7SoundData;
	long		whichBuffer;
	UInt32		inputFrames;
	UInt32		inputBytes;
	UInt32 		outputFrames;
	UInt32		outputBytes;
	SndCommand	thePlayCmd0;
	SndCommand	thePlayCmd1;
	SndCommand	theCallBackCmd;
	Ptr			pDecomBuffer0;
	Ptr			pDecomBuffer1;
	CmpSoundHeader		mySndHeader0;
	CmpSoundHeader		mySndHeader1;
	SoundConverter		mySoundConverter;
	SCFillBufferData 	scFillBufferData;
	SndCallBackUPP theSoundCallBackUPP;
	Boolean		isSoundDone;
}	gSMPlayInfo;
	
// * ----------------------------
// MySoundCallBackFunction
//
// used to signal when a buffer is done playing
static pascal void MySoundCallBackFunction(SndChannelPtr theChannel, SndCommand *theCmd)
{
#pragma unused(theChannel)
	
	#ifndef TARGET_API_MAC_CARBON
		#if !GENERATINGCFM
			long oldA5;
			oldA5 = SetA5(theCmd->param2);
		#else
			#pragma unused(theCmd)
		#endif
	#else
		#pragma unused(theCmd)
	#endif // TARGET_API_MAC_CARBON

	gBufferDone = true;
	SMPlayService();

	#ifndef TARGET_API_MAC_CARBON
		#if !GENERATINGCFM
			oldA5 = SetA5(oldA5);
		#endif
	#endif // TARGET_API_MAC_CARBON
}


// * ----------------------------
// GetMovieMedia
//
// returns a Media identifier - if the file is a System 7 Sound a non-in-place import is done and
// a handle to the data is passed back to the caller who is responsible for disposing of it
static OSErr GetMovieMedia(const FSSpec *inFile, Media *outMedia, Handle *outHandle)
{
	Movie theMovie = 0;
	Track theTrack;
	FInfo fndrInfo;

	OSErr err = noErr;

	err = FSpGetFInfo(inFile, &fndrInfo);
	if(err != noErr)
		return(err);
	
	if (kQTFileTypeSystemSevenSound == fndrInfo.fdType) 
	{
		// if this is an 'sfil' handle it appropriately
		// QuickTime can't import these files in place, but that's ok,
		// we just need a new place to put the data
		
		MovieImportComponent	theImporter = 0;
		Handle 					hDataRef = NULL;
		
		// create a new movie
		theMovie = NewMovie(newMovieActive);
		
		// allocate the data handle and create a data reference for this handle
		// the caller is responsible for disposing of the data handle once done with the sound
		*outHandle = NewHandle(0);
		err = PtrToHand(outHandle, &hDataRef, sizeof(Handle));
		if (noErr == err) {
			SetMovieDefaultDataRef(theMovie, hDataRef, HandleDataHandlerSubType);
			OpenADefaultComponent(MovieImportType, kQTFileTypeSystemSevenSound, &theImporter);
			if (theImporter) {
				Track		ignoreTrack;
				TimeValue 	ignoreDuration;		
				long 		ignoreFlags;
				
				err = MovieImportFile(theImporter, inFile, theMovie, 0, &ignoreTrack, 0, &ignoreDuration, 0, &ignoreFlags);
				CloseComponent(theImporter);
			}
		} else {
			if (*outHandle) {
				DisposeHandle(*outHandle);
				*outHandle = NULL;
			}
		}
		if (hDataRef) 
			DisposeHandle(hDataRef);
		if(err != noErr)
			return(err);
	} else {
		// import in place
		
		short theRefNum;
		short theResID = 0;	// we want the first movie
		Boolean wasChanged;
		
		// open the movie file
		err = OpenMovieFile(inFile, &theRefNum, fsRdPerm);
		if(err != noErr)
			return(err);

		// instantiate the movie
		err = NewMovieFromFile(&theMovie, theRefNum, &theResID, NULL, newMovieActive, &wasChanged);
		CloseMovieFile(theRefNum);
		if(err != noErr)
			return(err);
	}
		
	// get the first sound track
	theTrack = GetMovieIndTrackType(theMovie, 1, SoundMediaType, movieTrackMediaType);
	if (NULL == theTrack ) 
		return(invalidTrack);

	// get and return the sound track media
	*outMedia = GetTrackMedia(theTrack);
	if (NULL == *outMedia) err = invalidMedia;
	
	return err;
}

// * ----------------------------
// MyGetSoundDescriptionExtension
//
// this function will extract the information needed to decompress the sound file, this includes 
// retrieving the sample description, the decompression atom and setting up the sound header
static OSErr MyGetSoundDescriptionExtension(Media inMedia, AudioFormatAtomPtr *outAudioAtom, CmpSoundHeaderPtr outSoundHeader)
{
	OSErr err = noErr;
			
	Size size;
	Handle extension;
		
	// Version 1 of this record includes four extra fields to store information about compression ratios. It also defines
	// how other extensions are added to the SoundDescription record.
	// All other additions to the SoundDescription record are made using SM atoms. That means one or more
	// atoms can be appended to the end of the SoundDescription record using the standard [size, type]
	// mechanism used throughout the QuickTime movie resource architecture.
	// http://developer.apple.com/techpubs/quicktime/qtdevdocs/RM/frameset.htm
	SoundDescriptionV1Handle sourceSoundDescription = (SoundDescriptionV1Handle)NewHandle(0);
	
	// get the description of the sample data
	GetMediaSampleDescription(inMedia, 1, (SampleDescriptionHandle)sourceSoundDescription);
	err = GetMoviesError();
	if(err != noErr)
		return(err);

	extension = NewHandle(0);
	
	// get the "magic" decompression atom
	// This extension to the SoundDescription information stores data specific to a given audio decompressor.
	// Some audio decompression algorithms require a set of out-of-stream values to configure the decompressor
	// which are stored in a siDecompressionParams atom. The contents of the siDecompressionParams atom are dependent
	// on the audio decompressor.
	err = GetSoundDescriptionExtension((SoundDescriptionHandle)sourceSoundDescription, &extension, siDecompressionParams);
	
	if (noErr == err) {
		size = GetHandleSize(extension);
		HLock(extension);
		*outAudioAtom = (AudioFormatAtom*)NewPtr(size);
		err = MemError();
		// copy the atom data to our buffer...
		BlockMoveData(*extension, *outAudioAtom, size);
		HUnlock(extension);
	} else {
		// if it doesn't have an atom, that's ok
		err = noErr;
	}
	
	// set up our sound header
	outSoundHeader->format = (*sourceSoundDescription)->desc.dataFormat;
	outSoundHeader->numChannels = (*sourceSoundDescription)->desc.numChannels;
	outSoundHeader->sampleSize = (*sourceSoundDescription)->desc.sampleSize;
	outSoundHeader->sampleRate = (*sourceSoundDescription)->desc.sampleRate;
	
	DisposeHandle(extension);
	DisposeHandle((Handle)sourceSoundDescription);
	
	return err;
}

// * ----------------------------
// PlaySound
//
// this function does the actual work of playing the sound file, it sets up the sound converter environment, allocates play buffers,
// creates the sound channel and sends the appropriate sound commands to play the converted sound data
OSErr StartSMPlay(SoundInfo *theSI, double doubleTime)
{	
	AudioCompressionAtomPtr	theDecompressionAtom;
	SoundComponentData		theInputFormat,
							theOutputFormat;
	Media					theSoundMedia = NULL;

	Ptr						pSourceBuffer = NULL;
	float					time;
	OSErr 					err = noErr;
	
	playSIPtr = theSI;
	
	gDBPI.timeDivisor = (float)(theSI->numBytes/(theSI->frameSize * theSI->sRate * theSI->nChans));
	gSMPlayInfo.isSoundDone = false;
	gSMPlayInfo.scFillBufferData.sourcePtr = NULL;
	gSMPlayInfo.mySoundConverter = NULL;
	gSMPlayInfo.pDecomBuffer0 = NULL;
	gSMPlayInfo.pDecomBuffer1 = NULL;
	gSMPlayInfo.hSys7SoundData = NULL;
	gSMChannel = NULL;
	gSMPlayInfo.theSoundCallBackUPP = NULL;
	time = (float)doubleTime;
	
	err = GetMovieMedia(&(theSI->sfSpec), &theSoundMedia, &gSMPlayInfo.hSys7SoundData);
	if(err != noErr)
		TermSMPlay();
	
	err = MyGetSoundDescriptionExtension(theSoundMedia, (AudioFormatAtomPtr *)&theDecompressionAtom, &gSMPlayInfo.mySndHeader0);
	if (noErr == err) 
	{	
		UInt32	targetBytes = 8192;
		// setup input/output format for sound converter
		theInputFormat.flags = 0;
		theInputFormat.format = gSMPlayInfo.mySndHeader0.format;
		theInputFormat.numChannels = gSMPlayInfo.mySndHeader0.numChannels;
		theInputFormat.sampleSize = gSMPlayInfo.mySndHeader0.sampleSize;
		theInputFormat.sampleRate = gSMPlayInfo.mySndHeader0. sampleRate;
		theInputFormat.sampleCount = 0;
		theInputFormat.buffer = NULL;
		theInputFormat.reserved = 0;

		theOutputFormat.flags = 0;
		theOutputFormat.format = kSoundNotCompressed;
		theOutputFormat.numChannels = theInputFormat.numChannels;
		theOutputFormat.sampleSize = theInputFormat.sampleSize;
		theOutputFormat.sampleRate = theInputFormat.sampleRate;
		theOutputFormat.sampleCount = 0;
		theOutputFormat.buffer = NULL;
		theOutputFormat.reserved = 0;

		err = SoundConverterOpen(&theInputFormat, &theOutputFormat, &gSMPlayInfo.mySoundConverter);
		if(err != noErr)
			TermSMPlay();

		// set up the sound converters decompresson 'environment' by passing in the 'magic' decompression atom
		err = SoundConverterSetInfo(gSMPlayInfo.mySoundConverter, siDecompressionParams, theDecompressionAtom);
		if (siUnknownInfoType == err) {
			// clear this error, the decompressor didn't
			// need the decompression atom and that's OK
			err = noErr;
		} else if(err != noErr)
			TermSMPlay();		
		
		// find out how much buffer space to alocate for our output buffers
		// The differences between SoundConverterConvertBuffer begin with how the buffering is done. SoundConverterFillBuffer will do as much or as
		// little work as is required to satisfy a given request. This means that you can pass in buffers of any size you like and expect that
		// the Sound Converter will never overflow the output buffer. SoundConverterFillBufferDataProc function will be called as many times as
		// necessary to fulfill a request. This means that the SoundConverterFillBufferDataProc routine is free to provide data in whatever chunk size
		// it likes. Of course with both sides, the buffer sizes will control how many times you need to request data and there is a certain amount of
		// overhead for each call. You will want to balance this against the performance you require. While a call to SoundConverterGetBufferSizes is
		// not required by the SoundConverterFillBuffer function, it is useful as a guide for non-VBR formats
		do {
			targetBytes *= 2;
			err = SoundConverterGetBufferSizes(gSMPlayInfo.mySoundConverter, targetBytes, &(gSMPlayInfo.inputFrames), 
				&(gSMPlayInfo.inputBytes), &(gSMPlayInfo.outputBytes));
		} while (notEnoughBufferSpace == err  && targetBytes < (MaxBlock() / 4));
		
		// allocate play buffers
		gSMPlayInfo.pDecomBuffer0 = NewPtr(gSMPlayInfo.outputBytes + kOverflowRoom);
		err = MemError();
		if(err != noErr)
			TermSMPlay();
		
		gSMPlayInfo.pDecomBuffer1 = NewPtr(gSMPlayInfo.outputBytes + kOverflowRoom);
		err = MemError();
		if(err != noErr)
			TermSMPlay();

		err = SoundConverterBeginConversion(gSMPlayInfo.mySoundConverter);
		if(err != noErr)
			TermSMPlay();

		// setup first header
		gSMPlayInfo.mySndHeader0.samplePtr = gSMPlayInfo.pDecomBuffer0;
		gSMPlayInfo.mySndHeader0.numChannels = theOutputFormat.numChannels;
		gSMPlayInfo.mySndHeader0.sampleRate = theOutputFormat.sampleRate;
		gSMPlayInfo.mySndHeader0.loopStart = 0;
		gSMPlayInfo.mySndHeader0.loopEnd = 0;
		gSMPlayInfo.mySndHeader0.encode = cmpSH;					// compressed sound header encode value
		gSMPlayInfo.mySndHeader0.baseFrequency = kMiddleC;
		// gSMPlayInfo.mySndHeader0.AIFFSampleRate;					// this is not used
		gSMPlayInfo.mySndHeader0.markerChunk = NULL;
		gSMPlayInfo.mySndHeader0.format = theOutputFormat.format;
		gSMPlayInfo.mySndHeader0.futureUse2 = 0;
		gSMPlayInfo.mySndHeader0.stateVars = NULL;
		gSMPlayInfo.mySndHeader0.leftOverSamples = NULL;
		gSMPlayInfo.mySndHeader0.compressionID = fixedCompression;	// compression ID for fixed-sized compression, even uncompressed sounds use fixedCompression
		gSMPlayInfo.mySndHeader0.packetSize = 0;					// the Sound Manager will figure this out for us
		gSMPlayInfo.mySndHeader0.snthID = 0;
		gSMPlayInfo.mySndHeader0.sampleSize = theOutputFormat.sampleSize;
		gSMPlayInfo.mySndHeader0.sampleArea[0] = 0;					// no samples here because we use samplePtr to point to our buffer instead

		// setup second header, only the buffer ptr is different
		BlockMoveData(&gSMPlayInfo.mySndHeader0, &gSMPlayInfo.mySndHeader1, sizeof(gSMPlayInfo.mySndHeader0));
		gSMPlayInfo.mySndHeader1.samplePtr = gSMPlayInfo.pDecomBuffer1;
		
		// fill in struct that gets passed to SoundConverterFillBufferDataProc via the refcon
		// this includes the ExtendedSoundComponentData record		
		gSMPlayInfo.scFillBufferData.dFileNum = theSI->dFileNum;
		gSMPlayInfo.scFillBufferData.sourceMedia = theSoundMedia;
		gSMPlayInfo.scFillBufferData.diskPosition = theSI->dataStart + ((long)(time * theSI->sRate) * theSI->nChans * theSI->frameSize);		
		gSMPlayInfo.scFillBufferData.fileSize = theSI->dataEnd;
		gSMPlayInfo.scFillBufferData.isThereMoreSource = true;
		gSMPlayInfo.scFillBufferData.maxBufferSize = gSMPlayInfo.inputBytes;
		gSMPlayInfo.scFillBufferData.sourcePtr = NewPtr(gSMPlayInfo.scFillBufferData.maxBufferSize+ kOverflowRoom);		
		// allocate source media buffer
		err = MemError();
		if(err != noErr)
			TermSMPlay();

		gSMPlayInfo.scFillBufferData.compData.desc = theInputFormat;
		gSMPlayInfo.scFillBufferData.compData.desc.buffer = (Byte *)gSMPlayInfo.scFillBufferData.sourcePtr;
		gSMPlayInfo.scFillBufferData.compData.desc.flags = kExtendedSoundData;
		gSMPlayInfo.scFillBufferData.compData.recordSize = sizeof(ExtendedSoundComponentData);
		gSMPlayInfo.scFillBufferData.compData.extendedFlags = kExtendedSoundSampleCountNotValid | kExtendedSoundBufferSizeValid;
		gSMPlayInfo.scFillBufferData.compData.bufferSize = 0;
		
		// setup the callback, create the sound channel and play the sound
		// we will continue to convert the sound data into the free (non playing) buffer
		gSMPlayInfo.theSoundCallBackUPP = NewSndCallBackUPP(MySoundCallBackFunction);		 		
		err = SndNewChannel(&gSMChannel, sampledSynth, 0, gSMPlayInfo.theSoundCallBackUPP);

		if (err == noErr) 
		{			
			
								
			gSMPlayInfo.whichBuffer = kFirstBuffer;
						
			gSMPlayInfo.thePlayCmd0.cmd = bufferCmd;
			gSMPlayInfo.thePlayCmd0.param1 = 0;						// not used, but clear it out anyway just to be safe
			gSMPlayInfo.thePlayCmd0.param2 = (long)&gSMPlayInfo.mySndHeader0;

			gSMPlayInfo.thePlayCmd1.cmd = bufferCmd;
			gSMPlayInfo.thePlayCmd1.param1 = 0;						// not used, but clear it out anyway just to be safe
			gSMPlayInfo.thePlayCmd1.param2 = (long)&gSMPlayInfo.mySndHeader1;
						
			gSMPlayInfo.whichBuffer = kFirstBuffer;					// buffer 1 will be free when callback runs
			gSMPlayInfo.theCallBackCmd.cmd = callBackCmd;
			gSMPlayInfo.theCallBackCmd.param2 = SetCurrentA5();
			
			gBufferDone = true;
			gProcessEnabled = PLAY_PROCESS;
			DisableItem(gControlMenu,PAUSE_ITEM);
			DisableItem(gControlMenu,CONTINUE_ITEM);
			DisableItem(gControlMenu,STOP_ITEM);
			MenuUpdate();
			gDBPI.ticksStart = TickCount() - (long)(time * 60);
			
			if (noErr == err)
			{
				SMPlayService();
			}
		}
	}
	return err;
}

void SMPlayService()
{
	UInt32	actualOutputBytes;
	long readSize;
	OSErr	err;

	readSize = gSMPlayInfo.scFillBufferData.maxBufferSize;
	err = FSRead(gSMPlayInfo.scFillBufferData.dFileNum, &readSize, gSMPlayInfo.scFillBufferData.sourcePtr);					
	if (kFirstBuffer == gSMPlayInfo.whichBuffer)
	{
	  	if(readSize != 0)
	  	{
			err = SoundConverterConvertBuffer(gSMPlayInfo.mySoundConverter, gSMPlayInfo.scFillBufferData.sourcePtr,
				gSMPlayInfo.inputFrames, gSMPlayInfo.pDecomBuffer0, &(gSMPlayInfo.outputFrames),
				&(actualOutputBytes));
	    		if(err != noErr)
	        		return;
	    }
	    else
			gSMPlayInfo.isSoundDone = true;
		gSMPlayInfo.mySndHeader0.numFrames = gSMPlayInfo.outputFrames;

		gBufferDone = false;
		SndDoCommand(gSMChannel, &(gSMPlayInfo.thePlayCmd0), true);			// play the next buffer
		if (!gSMPlayInfo.isSoundDone)
		{
			SndDoCommand(gSMChannel, &gSMPlayInfo.theCallBackCmd, true);
		}	// reuse callBackCmd
		else
		{
			SoundConverterEndConversion(gSMPlayInfo.mySoundConverter, gSMPlayInfo.pDecomBuffer0, &gSMPlayInfo.outputFrames, &gSMPlayInfo.outputBytes);
			if (gSMPlayInfo.outputFrames)
			{
				gSMPlayInfo.mySndHeader0.numFrames = gSMPlayInfo.outputFrames;
				SndDoCommand(gSMChannel, &gSMPlayInfo.thePlayCmd0, true);	// play the last buffer.
			}
			StopSMPlay();
		}
		gSMPlayInfo.whichBuffer = kSecondBuffer;
	} else {
	  	if(readSize != 0)
	  	{
			err = SoundConverterConvertBuffer(gSMPlayInfo.mySoundConverter, gSMPlayInfo.scFillBufferData.sourcePtr,
				gSMPlayInfo.inputFrames, gSMPlayInfo.pDecomBuffer1, &(gSMPlayInfo.outputFrames),
				&(actualOutputBytes));
	    		if(err != noErr)
	        		return;
	    }
	    else
			gSMPlayInfo.isSoundDone = true;
		gSMPlayInfo.mySndHeader1.numFrames = gSMPlayInfo.outputFrames;

		gBufferDone = false;
		SndDoCommand(gSMChannel, &(gSMPlayInfo.thePlayCmd1), true);			// play the next buffer
		if (!gSMPlayInfo.isSoundDone)
		{
			SndDoCommand(gSMChannel, &gSMPlayInfo.theCallBackCmd, true);
		}	// reuse callBackCmd
		else
		{
			SoundConverterEndConversion(gSMPlayInfo.mySoundConverter, gSMPlayInfo.pDecomBuffer1, &gSMPlayInfo.outputFrames, &gSMPlayInfo.outputBytes);
			if (gSMPlayInfo.outputFrames)
			{
				gSMPlayInfo.mySndHeader1.numFrames = gSMPlayInfo.outputFrames;
				SndDoCommand(gSMChannel, &gSMPlayInfo.thePlayCmd1, true);	// play the last buffer.
			}
			StopSMPlay();
		}
		gSMPlayInfo.whichBuffer = kFirstBuffer;
	}

		
}

void StopSMPlay()
{
	SCStatus	status;
	SndCommand	command;
	OSErr		iErr;
	
	if(gSMChannel == 0L)
		return;
	gSMPlayInfo.isSoundDone = true;
	
	command.cmd = quietCmd;
	command.param1 = 0;
	command.param2 = 0;
	iErr = SndDoImmediate(gSMChannel,&command);
	
	gDBPI.lastBlock = TRUE;
	do
	{
		SndChannelStatus(gSMChannel, sizeof (status), &status);
	}while (status.scChannelBusy);
	
	TermSMPlay();
	playSIPtr = nil;
}
void TermSMPlay()
{
	if (gSMChannel)
		SndDisposeChannel(gSMChannel, false);		// wait until sounds stops playing before disposing of channel
	if (gSMPlayInfo.theSoundCallBackUPP)
		DisposeSndCallBackUPP(gSMPlayInfo.theSoundCallBackUPP);
	if (gSMPlayInfo.scFillBufferData.sourcePtr)
		DisposePtr(gSMPlayInfo.scFillBufferData.sourcePtr);
	if (gSMPlayInfo.mySoundConverter)
		SoundConverterClose(gSMPlayInfo.mySoundConverter);
	if (gSMPlayInfo.pDecomBuffer0)
		DisposePtr(gSMPlayInfo.pDecomBuffer0);
	if (gSMPlayInfo.pDecomBuffer1)
		DisposePtr(gSMPlayInfo.pDecomBuffer1);
	if (gSMPlayInfo.hSys7SoundData)
		DisposeHandle(gSMPlayInfo.hSys7SoundData);
}