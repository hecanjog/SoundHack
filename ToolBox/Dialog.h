/* Dialog & Alert Resource IDs */
#define	HEADER_DLOG			129
#define	NO_FPU_ALERT		129
#define ERROR_DLOG			130
#define	BAD_SYS_ALERT		130
#define	NOTAPPC_ALERT		131
#define NO_CARBON_ALERT		132
#define	FIR_DLOG			132
#define	PVOC_DLOG			133
#define	DYNAMICS_DLOG		134
#define TIME_DLOG			135
#define PREF_DLOG			136
#define	BINAURAL_DLOG		137
#define	GAIN_DLOG			139
#define DRAWFUNC_DLOG		140
#define MUTATE_DLOG			141
#define MARKER_DLOG			142
#define SRATE_DLOG			143
#define VALUE_DLOG			144
#define EXTRACT_DLOG		146
#define EXPORT_DLOG			147
#define IMPORT_DLOG			148
#define RECORD_DLOG			149
#define ANAL_DLOG			150
#define PLAY_DLOG			151
#define MY_OPEN_DLOG		152
#define MY_SAVE_DLOG		153
#define SOUNDPICT_DLOG		154
/* For SoundInfo Dialog Display */
#define	I_SRATE_FIELD		2
#define	I_SFLENGTH_FIELD	4
#define	I_CHAN_FIELD		6
#define	I_TYPE_FIELD		8
#define I_PACKMODE_FIELD	10
#define I_TYPE_PICT			11

// Pictures for file info
#define INPUT_PICT			150
#define OUTPUT_PICT			151
#define IMPULSE_PICT		152
#define STEADY_PICT			153
#define TRANSIENT_PICT		154
#define SOURCE_PICT			155
#define TARGET_PICT			156
#define MUTANT_PICT			157
#define TRIGGER_PICT		158
#define BLANK_PICT			137

/* For About Dialog Display */
#define	A_REG_PICT			1

/* For Error Dialog Box */
#define ERROR_TEXT			2
#define ERROR_OK			1

/* For Time Dialog Box */
#define	T_SFOUT_FIELD		1
#define	T_MESSAGE_FIELD		2
#define T_SMILEY_PICT		3
#define	T_SFIN_FIELD		5

#define	TRANSPORT_CNTL		149

/*Function prototypes for Dialog.c*/

long	InitProcessWindow(void);
long	UpdateProcessWindow(Str255 inTime, Str255 outTime, Str255 title, float barLength);
long	DrawProcessWindow();
pascal void	TrackFilter(ControlHandle theControl, short thePart);
void	HandleProcessWindowEvent(Point where, Boolean reset);
void	DrawTimeDialog(void);
void	HandleTimeDialog(Str255 inStr, Str255 outStr, Str255 messageStr, Boolean smiley);
void	DrawErrorMessage(Str255 s);
void	HandleErrorMessageEvent(short itemHit);
void	HandleAboutWindow(short event, Str255 message);
void	HandleBiblioDialog(void);
void	HandleBiblioDialogEvent(short itemHit);
void	UpdateInfoWindow(SoundInfo *mySI);
void	UpdateInfoWindowFileType(void);
