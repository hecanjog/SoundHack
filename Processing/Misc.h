/*Function prototypes for Misc.c*/
OSErr	TouchFolder( short vRefNum , long parID );
double	LLog2(double x);
double	EExp2(double x);
void	ColorBrighten(RGBColor *color);
void	NameFile(Str255 inStr, Str255 procStr, Str255 outStr);
void	RemoveMenuChars(Str255 stringIn, Str255 stringOut);
void	StringCopy(Str255 stringIn, Str255 stringOut);
void	StringAppend(Str255 stringHead, Str255 stringTail, Str255 stringOut);
void	HMSTimeString(double seconds, Str255 timeStr);
void	StringToFix(Str255 s,double *f);
void	FixToString(double f, Str255 s);
void	FixToString3(double f, Str255 s);
void	FixToString12(double f, Str255 s);
void	ZeroFloatTable(float A[], long n);
void	NoteToString(char noteNum, Str255 noteStr);
void	StringToNote(Str255 noteStr, char *noteNum);
void	PickPhrase(Str255 str);
void	RemovePtr(Ptr killMePtr);