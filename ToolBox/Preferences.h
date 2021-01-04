#define PREF_FILE_NAME	"\pSoundHack 0.880 Preferences"

#define PR_SFTYPE_BOX		2
#define PR_SFTYPE_CNTL		3
#define PR_SFFORMAT_CNTL	4
#define PR_EDIT_BUTTON		5
#define	PR_EDIT_TEXT		6
#define	PR_OPENACT_CNTL		7
#define	PR_PROCPLAY_CNTL	8
#define	PR_SET_BUTTON		1


/*Function prototypes for SampleRateConvert.c*/
void	HandlePrefDialog(void);
void	HandlePrefDialogEvent(short itemHit);
void	SavePreferences(void);
void	OpenPreferences(void);
void	LoadSettings(void);
void	SaveSettings(void);
void	WriteSettings(FSSpec myFile);
void	ReadSettings(FSSpec myFile);
void	AddPtrResource(Ptr data, Str255 name, long size);
long	GetPtrResource(Ptr data, Str255 name, long size);
void	SetDefaults(void);
void	SetPreDefaults(void);
void	SetAnaDefaults(void);
void	SetConDefaults(void);
void	SetGaiDefaults(void);
void	SetSamDefaults(void);
void	SetDynDefaults(void);
void	SetExtDefaults(void);
void	SetMutDefaults(void);
void	SetPhaDefaults(void);
