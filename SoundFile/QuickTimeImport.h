/*--------- prototypes --------*/
void	QuickTimeImportCallBackInit(void);
Component	GetAudioEater(OSType subType);
Movie GetMovieFromFile (FSSpec aFileSpec);
void	CDAudioImporter(void);
void	AIFF2QTInPlace(SoundInfo *mySI);
pascal OSErr MyProgressProc(Movie theMovie, short message, short whatOperation, Fixed percentDone, long refcon);
void 	CreateQTAudioFile(Movie *theMovie, FSSpec *soundFile);
