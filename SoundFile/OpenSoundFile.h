short	OpenSoundFile(FSSpec fileSpec, Boolean any);
//short 	OpenAllSoundFiles(long dirID, short vRefNum);
short 	OpenManySoundFiles(FSSpec *files, long count);
long	OpenSoundFindType(FSSpec fileSpec);
short	OpenSoundReadHeader(SoundInfo *mySI);
