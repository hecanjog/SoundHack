// AppleEvent.c - All of my support for Apple Events starts here. This stuff is mostly culled
// from the Think Reference.

//#include <Sound.h>
//#include <SoundComponents.h>
#include "SoundFile.h"
#include "OpenSoundFile.h"
#include "SoundHack.h"
#include "Misc.h"
#include "Dialog.h"
#include "Play.h"
#include "Preferences.h"
#include "MyAppleEvents.h"
//#include <AppleEvents.h>
//#include <Processes.h>
//#include <Aliases.h>

// Constants for dealing with FinderEvents. See Chapter 8 of the Apple Event
// Registry for more information.
#define kFinderSig	'FNDR'
#define kAEFinderEvents	'FNDR'
#define kSystemType	'MACS'
#define kAEOpenSelection	'sope'
#define keySelection	'fsel'

extern long		gStartTicks;
extern SoundInfoPtr	frontSIPtr, firstSIPtr, lastSIPtr;

#ifdef powerc
AEEventHandlerUPP	gMyHandleOAPP;
AEEventHandlerUPP	gMyHandleODOC;
AEEventHandlerUPP	gMyHandlePDOC;
AEEventHandlerUPP	gMyHandleQUIT;
#endif

#ifdef powerc
void AppleEventCallBackInit(void)
{
	gMyHandleOAPP = (AEEventHandlerUPP)NewAEEventHandlerUPP(MyHandleOAPP);
	gMyHandleODOC = (AEEventHandlerUPP)NewAEEventHandlerUPP(MyHandleODOC);
	gMyHandlePDOC = (AEEventHandlerUPP)NewAEEventHandlerUPP(MyHandlePDOC);
	gMyHandleQUIT = (AEEventHandlerUPP)NewAEEventHandlerUPP(MyHandleQUIT);
}
#endif
void
AppleEventInit(void)
{
#ifdef powerc
	AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, (AEEventHandlerUPP)gMyHandleOAPP, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, (AEEventHandlerUPP)gMyHandleODOC, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, (AEEventHandlerUPP)gMyHandlePDOC, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, (AEEventHandlerUPP)gMyHandleQUIT, 0, FALSE);
#else
	AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, (AEEventHandlerUPP)&MyHandleOAPP, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, (AEEventHandlerUPP)&MyHandleODOC, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, (AEEventHandlerUPP)&MyHandlePDOC, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, (AEEventHandlerUPP)&MyHandleQUIT, 0, FALSE);
#endif
}

// Given a FSSpecPtr to either an application or a document, OpenSelection creates a
// finder Open Selection Apple event for the object described by the FSSpec.
OSErr 
OpenSelection(FSSpecPtr theDoc)
{
	AppleEvent	aeEvent;	// the event to create;
	AEDesc	myAddressDesc;	// descriptors for the 
	AEDesc	aeDirDesc;
	AEDesc	listElem;
	AEDesc	fileList;		// our list
	FSSpec	dirSpec;
	AliasHandle	dirAlias;		// alias to directory with our file
	AliasHandle	fileAlias;		// alias of the file itself
	ProcessSerialNumber process;	// the finder's psn
	OSErr		myErr;				// duh

	// Get the psn of the Finder and create the target address for the .
	if(FindAProcess(kFinderSig,kSystemType,&process))
			return procNotFound;
	myErr = AECreateDesc(typeProcessSerialNumber,(Ptr) &process,
							sizeof(process), &myAddressDesc);
	if(myErr)	return myErr;

	// Create an empty 
	myErr = AECreateAppleEvent (kAEFinderEvents, kAEOpenSelection, &myAddressDesc, 
		kAutoGenerateReturnID, kAnyTransactionID, &aeEvent);
	if(myErr)
			return myErr;

	// Make an FSSpec and alias for the parent folder, and an alias for the file
	FSMakeFSSpec(theDoc->vRefNum,theDoc->parID,nil,&dirSpec);
	NewAlias(nil,&dirSpec,&dirAlias);
	NewAlias(nil,theDoc,&fileAlias);

	// Create the file list.
	if(myErr=AECreateList(nil,0,false,&fileList))
			return myErr;

	/* Create the folder descriptor
	*/
	HLock((Handle)dirAlias);
	AECreateDesc(typeAlias, (Ptr) *dirAlias, GetHandleSize
					((Handle) dirAlias), &aeDirDesc);
	HUnlock((Handle)dirAlias);
	DisposeHandle((Handle)dirAlias);

	if((myErr = AEPutParamDesc(&aeEvent,keyDirectObject,&aeDirDesc)) ==
			noErr)
	{
			AEDisposeDesc(&aeDirDesc);
			HLock((Handle)fileAlias);

			AECreateDesc(typeAlias, (Ptr)*fileAlias,
					GetHandleSize((Handle)fileAlias), &listElem);
			HUnlock((Handle)fileAlias);
			DisposeHandle((Handle)fileAlias);
			myErr = AEPutDesc(&fileList,0,&listElem);
	}
	if(myErr)
			return myErr;
	AEDisposeDesc(&listElem);

	if(myErr = AEPutParamDesc(&aeEvent,keySelection,&fileList))
			return myErr;

	myErr = AEDisposeDesc(&fileList);

	myErr = AESend(&aeEvent, nil,
			kAENoReply+kAEAlwaysInteract+kAECanSwitchLayer,
			kAENormalPriority, kAEDefaultTimeout, nil, nil);
	AEDisposeDesc(&aeEvent);
}

// Search through the current process list to find the given application. See
// Using the Process Manager for a similar way of doing this.
OSErr FindAProcess(OSType typeToFind, OSType creatorToFind,
			ProcessSerialNumberPtr processSN)
{
	ProcessInfoRec	tempInfo;
	FSSpec	procSpec;
	Str31		processName;
	OSErr		myErr = noErr;

	// start at the beginning of the process list
	processSN->lowLongOfPSN = kNoProcess;
	processSN->highLongOfPSN = kNoProcess;

	// initialize the process information record
	tempInfo.processInfoLength = sizeof(ProcessInfoRec);
	tempInfo.processName = (StringPtr)&processName;
	tempInfo.processAppSpec = &procSpec;

	while((tempInfo.processSignature != creatorToFind ||
			tempInfo.processType != typeToFind) ||
			myErr != noErr)
	{
			myErr = GetNextProcess(processSN);
			if (myErr == noErr)
					GetProcessInformation(processSN, &tempInfo);
	}
	return(myErr);
}


pascal OSErr 
MyHandleOAPP(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	OSErr	myErr;

	myErr = MyGotRequiredParams(theAppleEvent);
	if (myErr)
		return myErr;
	else 
	{
		HandleAboutWindow(1, "\pApple Open App Event");
		return noErr;
	}
}

pascal OSErr	
MyHandleODOC(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	long		index, itemsInList;
	AEDescList	docList;
	AEKeyword	keywd;
	DescType	returnedType;
	FSSpec		myFSS;
	OSErr		myErr;
	Size		actualSize;
	WDPBRec		wdpb;
	FInfo		fndrInfo;

	
	/* get the direct parameter--a descriptor list--and put it into docList */
	myErr = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &docList);

	if(myErr) 
	{
		myErr = AEDisposeDesc(&docList);
		return	myErr;
	}

	/*
	 * count the number of descriptor records in the list
	 */
	myErr = AECountItems (&docList,&itemsInList);

	/*
	 * now get the first descriptor record from the list, coerce
	 * the returned data to an FSSpec record, and open the
	 * associated file
	 */
	// now get each descriptor record from the list, coerce
	// the returned data to an FSSpec record, and print the
	// associated file
	for (index=1; index<=itemsInList; index++) 
	{
		myErr = AEGetNthPtr(&docList, index, typeFSS, &keywd, &returnedType, (Ptr)&myFSS, sizeof(myFSS), &actualSize);
		if (myErr)
		{
			myErr = AEDisposeDesc(&docList);
			return	myErr;
		}

		FSpGetFInfo(&myFSS,&fndrInfo);
		if(fndrInfo.fdType == 'pref')
			ReadSettings(myFSS);
		else if(OpenSoundFile(myFSS, FALSE) != -1)
		{
			MenuUpdate();
		}
	}

	myErr = AEDisposeDesc(&docList);
	
	return	noErr;
}

// this needs to be modified for multiple soundfiles
pascal	OSErr	MyHandlePDOC(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	FSSpec	myFSS;
	AEDescList	docList;
	OSErr	myErr;
	long	index, itemsInList;
	Size	actualSize;
	AEKeyword	keywd;
	DescType	returnedType;
	WDPBRec		wdpb;

	// get the direct parameter--a descriptor list--and put
	// it into docList
	myErr = AEGetParamDesc(theAppleEvent, keyDirectObject,
							typeAEList, &docList);

	if (myErr)
	{
		myErr = AEDisposeDesc(&docList);
		return	myErr;
	}
	
	// check for missing required parameters
	myErr = MyGotRequiredParams(theAppleEvent);
	if (myErr) {
		// an error occurred:  do the necessary error handling
		myErr = AEDisposeDesc(&docList);
		return	myErr;
	}

	// count the number of descriptor records in the list
	myErr = AECountItems (&docList,&itemsInList);

	// now get each descriptor record from the list, coerce
	// the returned data to an FSSpec record, and print the
	// associated file
	for (index=1; index<=itemsInList; index++) 
	{
		myErr = AEGetNthPtr(&docList, index, typeFSS, &keywd, &returnedType, (Ptr)&myFSS, sizeof(myFSS), &actualSize);

		if(OpenSoundFile(myFSS, FALSE) != -1)
		{
			MenuUpdate();
			StartPlay(lastSIPtr, 0.0, 0.0);
			StopPlay(TRUE);
		}
	}

	myErr = AEDisposeDesc(&docList);

	return	noErr;
}

/* This function is from the THINK Reference */
pascal OSErr	
MyHandleQUIT(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	OSErr	myErr;

	// check for missing required parameters
	myErr = MyGotRequiredParams(theAppleEvent);
	if (myErr) {
		// an error occurred:  do the necessary error handling
		return	myErr;
	}
	TerminateSoundHack();
	return	noErr;
}

OSErr	MyGotRequiredParams (const AppleEvent *theAppleEvent)
{
	DescType	returnedType;
	Size	actualSize;
	OSErr	myErr;

	myErr = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr,
					typeWildCard, &returnedType,
					nil, 0, &actualSize);

	if (myErr == errAEDescNotFound)	// you got all the required
											//parameters
		return	noErr;
	else
		if (myErr == noErr)  // you missed a required parameter
			return	errAEParamMissed;
		else	// the call to AEGetAttributePtr failed
			return	myErr;
}

