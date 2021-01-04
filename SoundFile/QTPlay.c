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
#include "QTPlay.h"
#include "Menu.h"
#include "SoundHack.h"
#include "CarbonGlue.h"

extern short			gProcessEnabled;
extern MenuHandle		gControlMenu, gFileMenu;
extern DoubleBufPlayInfo		gDBPI;
extern SndChannelPtr	gQTChannel;
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
	SndCommand	*pPlayCmd;
	Handle		hSys7SoundData;
	long		whichBuffer;
	UInt32 		outputFrames;
	UInt32		outputBytes;
	SndCommand	thePlayCmd0;
	SndCommand	thePlayCmd1;
	SndCommand	theCallBackCmd;
	Ptr 		pDecomBuffer;
	Ptr			pDecomBuffer0;
	Ptr			pDecomBuffer1;
	CmpSoundHeaderPtr 	pSndHeader;
	CmpSoundHeader		mySndHeader0;
	CmpSoundHeader		mySndHeader1;
	SoundConverter		mySoundConverter;
	SoundConverterFillBufferDataUPP theFillBufferDataUPP;
	SCFillBufferData 	scFillBufferData;
	SndCallBackUPP theSoundCallBackUPP;
	Boolean		isSoundDone;
}	gQTPlayInfo;
	
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

	#ifndef TARGET_API_MAC_CARBON
		#if !GENERATINGCFM
			oldA5 = SetA5(oldA5);
		#endif
	#endif // TARGET_API_MAC_CARBON
}

// * ----------------------------
// SoundConverterFillBufferDataProc
//
// the callback routine that provides the source data for conversion - it provides data by setting
// outData to a pointer to a properly filled out ExtendedSoundComponentData structure
static pascal Boolean SoundConverterFillBufferDataProc(SoundComponentDataPtr *outData, void *inRefCon)
{
	SCFillBufferDataPtr pFillData = (SCFillBufferDataPtr)inRefCon;
	
	OSErr err = noErr;
							
	// if after getting the last chunk of data the total time is over the duration, we're done
	if (pFillData->getMediaAtThisTime >= pFillData->sourceDuration) {
		pFillData->isThereMoreSource = false;
		pFillData->compData.desc.buffer = NULL;
		pFillData->compData.desc.sampleCount = 0;
		pFillData->compData.bufferSize = 0;
	}
	
	if (pFillData->isThereMoreSource)
	{
		long		sourceBytesReturned;
		long		numberOfSamples;
		TimeValue	sourceReturnedTime, durationPerSample;
							
		HUnlock(pFillData->hSource);
		err = GetMediaSample(pFillData->sourceMedia,		// specifies the media for this operation
							 pFillData->hSource,			// function returns the sample data into this handle
							 pFillData->maxBufferSize,		// maximum number of bytes of sample data to be returned
							 &sourceBytesReturned,			// the number of bytes of sample data returned
							 pFillData->getMediaAtThisTime,	// starting time of the sample to be retrieved (must be in Media's TimeScale)
							 &sourceReturnedTime,			// indicates the actual time of the returned sample data
							 &durationPerSample,			// duration of each sample in the media
							 NULL,							// sample description corresponding to the returned sample data (NULL to ignore)
							 NULL,							// index value to the sample description that corresponds to the returned sample data (NULL to ignore)
							 0,								// maximum number of samples to be returned (0 to use a value that is appropriate for the media)
							 &numberOfSamples,				// number of samples it actually returned
							 NULL);							// flags that describe the sample (NULL to ignore)
							 
		HLock(pFillData->hSource);
		if ((noErr != err) || (sourceBytesReturned == 0)) {
			pFillData->isThereMoreSource = false;
			pFillData->compData.desc.buffer = NULL;
			pFillData->compData.desc.sampleCount = 0;		
			
			if ((err != noErr) && (sourceBytesReturned > 0))
				DebugStr("\pGetMediaSample - Failed in FillBufferDataProc");
		}
		pFillData->getMediaAtThisTime = sourceReturnedTime + (durationPerSample * numberOfSamples);
		pFillData->compData.bufferSize = sourceBytesReturned; 
	}

	// set outData to a properly filled out ExtendedSoundComponentData struct
	*outData = (SoundComponentDataPtr)&pFillData->compData;
	
	return (pFillData->isThereMoreSource);
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
	// All other additions to the SoundDescription record are made using QT atoms. That means one or more
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
OSErr StartQTPlay(SoundInfo *theSI, double doubleTime)
{	
	AudioCompressionAtomPtr	theDecompressionAtom;
	SoundComponentData		theInputFormat,
							theOutputFormat;
	Media					theSoundMedia = NULL;

	Ptr						pSourceBuffer = NULL;
	float					time;
							
						
	
	OSErr 					err = noErr;
	
	
	gDBPI.timeDivisor = (float)(theSI->numBytes/(theSI->frameSize * theSI->sRate * theSI->nChans));
	gQTPlayInfo.isSoundDone = false;
	gQTPlayInfo.scFillBufferData.hSource = NULL;
	gQTPlayInfo.mySoundConverter = NULL;
	gQTPlayInfo.pDecomBuffer0 = NULL;
	gQTPlayInfo.pDecomBuffer1 = NULL;
	gQTPlayInfo.hSys7SoundData = NULL;
	gQTChannel = NULL;
	gQTPlayInfo.theSoundCallBackUPP = NULL;
	gQTPlayInfo.theFillBufferDataUPP = NULL;
	time = (float)doubleTime;
	
	err = GetMovieMedia(&(theSI->sfSpec), &theSoundMedia, &gQTPlayInfo.hSys7SoundData);
	if(err != noErr)
		TermQTPlay();
	
	err = MyGetSoundDescriptionExtension(theSoundMedia, (AudioFormatAtomPtr *)&theDecompressionAtom, &gQTPlayInfo.mySndHeader0);
	if (noErr == err) 
	{	
		UInt32	targetBytes = 65536;
		// setup input/output format for sound converter
		theInputFormat.flags = 0;
		theInputFormat.format = gQTPlayInfo.mySndHeader0.format;
		theInputFormat.numChannels = gQTPlayInfo.mySndHeader0.numChannels;
		theInputFormat.sampleSize = gQTPlayInfo.mySndHeader0.sampleSize;
		theInputFormat.sampleRate = gQTPlayInfo.mySndHeader0. sampleRate;
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

		err = SoundConverterOpen(&theInputFormat, &theOutputFormat, &gQTPlayInfo.mySoundConverter);
		if(err != noErr)
			TermQTPlay();

		// set up the sound converters decompresson 'environment' by passing in the 'magic' decompression atom
		err = SoundConverterSetInfo(gQTPlayInfo.mySoundConverter, siDecompressionParams, theDecompressionAtom);
		if (siUnknownInfoType == err) {
			// clear this error, the decompressor didn't
			// need the decompression atom and that's OK
			err = noErr;
		} else if(err != noErr)
			TermQTPlay();		
		
		// find out how much buffer space to alocate for our output buffers
		// The differences between SoundConverterConvertBuffer begin with how the buffering is done. SoundConverterFillBuffer will do as much or as
		// little work as is required to satisfy a given request. This means that you can pass in buffers of any size you like and expect that
		// the Sound Converter will never overflow the output buffer. SoundConverterFillBufferDataProc function will be called as many times as
		// necessary to fulfill a request. This means that the SoundConverterFillBufferDataProc routine is free to provide data in whatever chunk size
		// it likes. Of course with both sides, the buffer sizes will control how many times you need to request data and there is a certain amount of
		// overhead for each call. You will want to balance this against the performance you require. While a call to SoundConverterGetBufferSizes is
		// not required by the SoundConverterFillBuffer function, it is useful as a guide for non-VBR formats
		do {
			UInt32 inputFrames, inputBytes;
			targetBytes *= 2;
			err = SoundConverterGetBufferSizes(gQTPlayInfo.mySoundConverter, targetBytes, &inputFrames, &inputBytes, &(gQTPlayInfo.outputBytes));
		} while (notEnoughBufferSpace == err  && targetBytes < (MaxBlock() / 4));
		
		// allocate play buffers
		gQTPlayInfo.pDecomBuffer0 = NewPtr(gQTPlayInfo.outputBytes);
		err = MemError();
		if(err != noErr)
			TermQTPlay();
		
		gQTPlayInfo.pDecomBuffer1 = NewPtr(gQTPlayInfo.outputBytes);
		err = MemError();
		if(err != noErr)
			TermQTPlay();

		err = SoundConverterBeginConversion(gQTPlayInfo.mySoundConverter);
		if(err != noErr)
			TermQTPlay();

		// setup first header
		gQTPlayInfo.mySndHeader0.samplePtr = gQTPlayInfo.pDecomBuffer0;
		gQTPlayInfo.mySndHeader0.numChannels = theOutputFormat.numChannels;
		gQTPlayInfo.mySndHeader0.sampleRate = theOutputFormat.sampleRate;
		gQTPlayInfo.mySndHeader0.loopStart = 0;
		gQTPlayInfo.mySndHeader0.loopEnd = 0;
		gQTPlayInfo.mySndHeader0.encode = cmpSH;					// compressed sound header encode value
		gQTPlayInfo.mySndHeader0.baseFrequency = kMiddleC;
		// gQTPlayInfo.mySndHeader0.AIFFSampleRate;					// this is not used
		gQTPlayInfo.mySndHeader0.markerChunk = NULL;
		gQTPlayInfo.mySndHeader0.format = theOutputFormat.format;
		gQTPlayInfo.mySndHeader0.futureUse2 = 0;
		gQTPlayInfo.mySndHeader0.stateVars = NULL;
		gQTPlayInfo.mySndHeader0.leftOverSamples = NULL;
		gQTPlayInfo.mySndHeader0.compressionID = fixedCompression;	// compression ID for fixed-sized compression, even uncompressed sounds use fixedCompression
		gQTPlayInfo.mySndHeader0.packetSize = 0;					// the Sound Manager will figure this out for us
		gQTPlayInfo.mySndHeader0.snthID = 0;
		gQTPlayInfo.mySndHeader0.sampleSize = theOutputFormat.sampleSize;
		gQTPlayInfo.mySndHeader0.sampleArea[0] = 0;					// no samples here because we use samplePtr to point to our buffer instead

		// setup second header, only the buffer ptr is different
		BlockMoveData(&gQTPlayInfo.mySndHeader0, &gQTPlayInfo.mySndHeader1, sizeof(gQTPlayInfo.mySndHeader0));
		gQTPlayInfo.mySndHeader1.samplePtr = gQTPlayInfo.pDecomBuffer1;
		
		// fill in struct that gets passed to SoundConverterFillBufferDataProc via the refcon
		// this includes the ExtendedSoundComponentData record		
		gQTPlayInfo.scFillBufferData.sourceMedia = theSoundMedia;
		gQTPlayInfo.scFillBufferData.getMediaAtThisTime = (long)(time * theSI->timeScale);		
		gQTPlayInfo.scFillBufferData.sourceDuration = GetMediaDuration(theSoundMedia);
		gQTPlayInfo.scFillBufferData.isThereMoreSource = true;
		gQTPlayInfo.scFillBufferData.maxBufferSize = kMaxInputBuffer;
		gQTPlayInfo.scFillBufferData.hSource = NewHandle((long)gQTPlayInfo.scFillBufferData.maxBufferSize);		// allocate source media buffer
		err = MemError();
		if(err != noErr)
			TermQTPlay();

		gQTPlayInfo.scFillBufferData.compData.desc = theInputFormat;
		gQTPlayInfo.scFillBufferData.compData.desc.buffer = (Byte *)*gQTPlayInfo.scFillBufferData.hSource;
		gQTPlayInfo.scFillBufferData.compData.desc.flags = kExtendedSoundData;
		gQTPlayInfo.scFillBufferData.compData.recordSize = sizeof(ExtendedSoundComponentData);
		gQTPlayInfo.scFillBufferData.compData.extendedFlags = kExtendedSoundSampleCountNotValid | kExtendedSoundBufferSizeValid;
		gQTPlayInfo.scFillBufferData.compData.bufferSize = 0;
		
		// setup the callback, create the sound channel and play the sound
		// we will continue to convert the sound data into the free (non playing) buffer
		gQTPlayInfo.theSoundCallBackUPP = NewSndCallBackUPP(MySoundCallBackFunction);		 		
		err = SndNewChannel(&gQTChannel, sampledSynth, 0, gQTPlayInfo.theSoundCallBackUPP);

		if (err == noErr) 
		{			
			
								
			gQTPlayInfo.whichBuffer = kFirstBuffer;
			
			gQTPlayInfo.theFillBufferDataUPP = NewSoundConverterFillBufferDataUPP(SoundConverterFillBufferDataProc);
			
			gQTPlayInfo.thePlayCmd0.cmd = bufferCmd;
			gQTPlayInfo.thePlayCmd0.param1 = 0;						// not used, but clear it out anyway just to be safe
			gQTPlayInfo.thePlayCmd0.param2 = (long)&gQTPlayInfo.mySndHeader0;

			gQTPlayInfo.thePlayCmd1.cmd = bufferCmd;
			gQTPlayInfo.thePlayCmd1.param1 = 0;						// not used, but clear it out anyway just to be safe
			gQTPlayInfo.thePlayCmd1.param2 = (long)&gQTPlayInfo.mySndHeader1;
						
			gQTPlayInfo.whichBuffer = kFirstBuffer;					// buffer 1 will be free when callback runs
			gQTPlayInfo.theCallBackCmd.cmd = callBackCmd;
			gQTPlayInfo.theCallBackCmd.param2 = SetCurrentA5();
			
			gBufferDone = true;
			gProcessEnabled = PLAY_PROCESS;
			DisableItem(gControlMenu,PAUSE_ITEM);
			DisableItem(gControlMenu,CONTINUE_ITEM);
			DisableItem(gControlMenu,STOP_ITEM);
			MenuUpdate();
			gDBPI.ticksStart = TickCount() - (long)(time * 60);
			
			if (noErr == err)
			{
				long threadResult;
				ThreadID tempThreadID;
				
				QTPlayService();
//				QTPlayService();
//				NewThread(kCooperativeThread, NewThreadEntryUPP(QTPlayThread), NULL, 0, 0, (void**)&threadResult, 
//    				&tempThreadID);
			}
		}
	}
	return err;
}
pascal voidPtr QTPlayThread(void *threadParam)
{
	
	while(gQTPlayInfo.isSoundDone != true)
	{		
		QTPlayService();
		YieldToAnyThread();
	}
}
void QTPlayService()
{
	UInt32	actualOutputBytes, outputFlags;
	OSErr	err;

	if(gBufferDone == false)
		return;
	if (kFirstBuffer == gQTPlayInfo.whichBuffer) {
		gQTPlayInfo.pPlayCmd = &gQTPlayInfo.thePlayCmd0;
		gQTPlayInfo.pDecomBuffer = gQTPlayInfo.pDecomBuffer0;
		gQTPlayInfo.pSndHeader = &gQTPlayInfo.mySndHeader0;
		gQTPlayInfo.whichBuffer = kSecondBuffer;
	} else {
		gQTPlayInfo.pPlayCmd = &gQTPlayInfo.thePlayCmd1;
		gQTPlayInfo.pDecomBuffer = gQTPlayInfo.pDecomBuffer1;
		gQTPlayInfo.pSndHeader = &gQTPlayInfo.mySndHeader1;
		gQTPlayInfo.whichBuffer = kFirstBuffer;
	}

	err = SoundConverterFillBuffer(gQTPlayInfo.mySoundConverter,		// a sound converter
								   gQTPlayInfo.theFillBufferDataUPP,	// the callback UPP
								   &gQTPlayInfo.scFillBufferData,		// refCon passed to FillDataProc
								   gQTPlayInfo.pDecomBuffer,			// the decompressed data 'play' buffer
								   gQTPlayInfo.outputBytes,				// size of the 'play' buffer
								   &actualOutputBytes,		// number of output bytes
								   &gQTPlayInfo.outputFrames,			// number of output frames
								   &outputFlags);			// fillbuffer retured advisor flags
	if (err) return;
	if((outputFlags & kSoundConverterHasLeftOverData) == false) {
		gQTPlayInfo.isSoundDone = true;
	}

	gQTPlayInfo.pSndHeader->numFrames = gQTPlayInfo.outputFrames;
	gBufferDone = false;
	if (!gQTPlayInfo.isSoundDone) {
		SndDoCommand(gQTChannel, &gQTPlayInfo.theCallBackCmd, true);	// reuse callBackCmd
	}
	SndDoCommand(gQTChannel, gQTPlayInfo.pPlayCmd, true);			// play the next buffer
	if(gQTPlayInfo.isSoundDone == true)
	{
		StopQTPlay();
		return;
	}
}
void StopQTPlay()
{
	gQTPlayInfo.isSoundDone = true;
	SoundConverterEndConversion(gQTPlayInfo.mySoundConverter, gQTPlayInfo.pDecomBuffer, &gQTPlayInfo.outputFrames, &gQTPlayInfo.outputBytes);
	if (gQTPlayInfo.outputFrames)
	{
		gQTPlayInfo.pSndHeader->numFrames = gQTPlayInfo.outputFrames;
		SndDoCommand(gQTChannel, gQTPlayInfo.pPlayCmd, true);	// play the last buffer.
	}
	if (gQTPlayInfo.theFillBufferDataUPP)
		DisposeSoundConverterFillBufferDataUPP(gQTPlayInfo.theFillBufferDataUPP);
	TermQTPlay();
}
void TermQTPlay()
{
	if (gQTChannel)
		SndDisposeChannel(gQTChannel, false);		// wait until sounds stops playing before disposing of channel
	if (gQTPlayInfo.theSoundCallBackUPP)
		DisposeSndCallBackUPP(gQTPlayInfo.theSoundCallBackUPP);
	if (gQTPlayInfo.scFillBufferData.hSource)
		DisposeHandle(gQTPlayInfo.scFillBufferData.hSource);
	if (gQTPlayInfo.mySoundConverter)
		SoundConverterClose(gQTPlayInfo.mySoundConverter);
	if (gQTPlayInfo.pDecomBuffer0)
		DisposePtr(gQTPlayInfo.pDecomBuffer0);
	if (gQTPlayInfo.pDecomBuffer1)
		DisposePtr(gQTPlayInfo.pDecomBuffer1);
	if (gQTPlayInfo.hSys7SoundData)
		DisposeHandle(gQTPlayInfo.hSys7SoundData);
}