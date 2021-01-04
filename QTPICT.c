/* if this function is called, it provides a hint from the caller as to the desired sample (frame) duration in the new media */
pascal ComponentResult ImportScrapbookSetSampleDuration
										 (ImportScrapbookGlobals storage,
											TimeValue duration, 
											TimeScale scale)
{
	TimeRecord tr;
	tr.value.lo = duration;
	tr.value.hi = 0;
	tr.scale = 0;
	tr.base = nil;
	ConvertTimeScale (&tr, kMediaTimeScale);		
					/* your new media will have a time scale of 600 */
	(**storage).frameDuration = tr.value.lo;
	return noErr;
}
pascal ComponentResult ImportScrapbookFile
									(ImportScrapbookGlobals storage,
									 FSSpec *theFile, Movie theMovie,
									 Track targetTrack, Track *usedTrack,
									 TimeValue atTime, 
									 TimeValue *addedTime, 
									 long inFlags, long *outFlags)
{
	OSErr err;
	short resRef = 0, saveRes = CurResFile();
	PicHandle thePict;
	Rect trackRect;
	short pageIndex = 0;
	Track newTrack = 0;
	Media newMedia;
	Boolean endMediaEdits = false;
	TimeValue frameDuration;
	SampleDescriptionHandle sampleDesc = 0;
	
	*outFlags = 0;
	if (inFlags & movieImportMustUseTrack)
		return invalidTrack;
	/* open source file */
	resRef = FSpOpenResFile (theFile, fsRdPerm);
	if (err = ResError()) goto bail;
	UseResFile(resRef);
	/* get the first PICT to determine the track size */
	thePict = (PicHandle)Get1IndResource ('PICT', 1);
	if (!thePict) {
		err = ResError();
		goto bail;
	}
	trackRect = (**thePict).picFrame;
	OffsetRect(&trackRect, -trackRect.left, -trackRect.top);
	/* create a track and PICT media */
	newTrack = NewMovieTrack (theMovie, trackRect.right << 16,
									 trackRect.bottom << 16, kNoVolume);
	if (err = GetMoviesError()) goto bail;
	newMedia = NewTrackMedia (newTrack, 'PICT', kMediaTimeScale,
									 nil, 0);
	if (err = GetMoviesError()) goto bail;
	if (err = BeginMediaEdits (newMedia)) goto bail;
	endMediaEdits = true;
	/* determine the frame duration (check the hint you may 
		have been called with) */
	frameDuration = (**storage).frameDuration;
	if (!frameDuration) frameDuration = kMediaTimeScale/5;					
										/* default is 1/5th second */
	/* set up a simple sample description */
	sampleDesc = (SampleDescriptionHandle) NewHandleClear 
											(sizeof (SampleDescription));
	(**sampleDesc).descSize = sizeof(SampleDescription);
	(**sampleDesc).dataFormat = 'PICT';
	/* cycle through all source frames and add them to the media */
	do {
		Handle thePict; 
		short resID = pageToMapIndex (++pageIndex, *GetResource ('SMAP', 0));
		if (resID == 0) break;
		thePict = Get1Resource ('PICT', resID);
		if (thePict == nil) continue;										/* some pages may not 
													contain a 'PICT' */
		err = AddMediaSample(newMedia, thePict, 0, 
									GetHandleSize (thePict),
									 frameDuration, sampleDesc, 1, 0, nil);
		ReleaseResource (thePict);
		DisposeHandle (thePict);
	} while (!err);
	if (err) goto bail;
	/* add the new media to the track */
	err = InsertMediaIntoTrack (newTrack, 0, 0, 
										GetMediaDuration (newMedia), kFix1);
bail:
	if (resRef) CloseResFile (resRef);
	if (endMediaEdits) EndMediaEdits (newMedia);
	if (err && newTrack) {
		DisposeMovieTrack (newTrack);
		newTrack = 0;
	}
	UseResFile (saveRes);
	if (sampleDesc) DisposeHandle ((Handle)sampleDesc);
	*usedTrack = newTrack;
	return err;
}
/* map from a Scrapbook page number to a resource ID */
short pageToMapIndex (short page, Ptr map)
{
	short mapIndex;
	for (mapIndex = 0; mapIndex < 256; mapIndex++)
		if (*map++ == page)
			return mapIndex | 0x8000;
	return 0;
}

pascal ComponentResult ExportPICSToFile (ExportPICSGlobals store, 					
														const FSSpec *theFile, 
														Movie m, 
														Track onlyThisTrack,
														TimeValue startTime,
														TimeValue duration)
{
	OSErr err = noErr;
	short resRef = 0;
	short saveResRef = CurResFile();
	short resID = 128;
	PicHandle thePict = nil;
	
	// open the resource fork of the PICS file 
	//	(the caller is responsible for creating the file)
	resRef = FSpOpenResFile (theFile, fsRdWrPerm);
	if (err = ResError()) goto bail;
	UseResFile(resRef);
	
	// cycle through the movie time segment you were given
	while(startTime < duration)
	{
		Byte c = 0;
		
		if(onlyThisTrack)
			thePict = GetTrackPict (onlyThisTrack, startTime);
		else
			thePict = GetMoviePict(m, startTime);
			
		if(!thePict) 
			continue;
			
		// add a frame to the PICS file
		AddResource ((Handle)thePict, 'PICT', resID++, &c);
		err = ResError();
		WriteResource ((Handle)thePict);
		DetachResource ((Handle)thePict);
		KillPicture (thePict);
		thePict = nil;
		if (err) break;
		/* find the time of the next frame */
		do
		{
			TimeValue nextTime;
			if(onlyThisTrack)
				GetTrackNextInterestingTime (onlyThisTrack, nextTimeMediaSample, startTime, 
											 kFix1, &nextTime, nil);
			else
			{
				OSType mediaType = VisualMediaCharacteristic;	
				GetMovieNextInterestingTime (m, nextTimeMediaSample, 1, &mediaType, 
												startTime, kFix1, &nextTime, nil);
			}
			if(GetMoviesError ()) 
				goto bail;
			if (nextTime != startTime)
			{
				startTime = nextTime;
				break;
			}
		}
		while (++startTime < duration);
	}
bail:
	if (thePict) KillPicture (thePict);
	if (resRef) CloseResFile (resRef);
	UseResFile (saveResRef);
	return err;
}