//#include <AppleEvents.h>
#define	BASE_RES_ID			128
#define	MOVE_TO_FRONT		-1L
#define	REMOVE_ALL_EVENTS	0

#define	LEAVE_IT_WHERE_IT_IS	FALSE
#define	NORMAL_UPDATES			TRUE
 
/* Event Related Parameters */
#define	SLEEP				10L
#define	NIL_MOUSE_REGION	0L
#define	WNE_TRAP_NUM		0x60
#define	UNIMPL_TRAP_NUM		0x9F
#define	SUSPEND_RESUME_BIT	0x0001
#define	ACTIVATING			1
#define	RESUMING			1
#define PENDING				9

/* Window display parameters */
#define	DRAG_THRESHOLD		30
#define	MIN_WINDOW_HEIGHT	50
#define	MIN_WINDOW_WIDTH	50

#define	MARK_APPLICATION	1

#define	SYS_VERSION			1

/*File Manager Macros*/
#define	NIL_POINTER			0L
#define	NIL_STRING	"\p"
#define	IGNORED_STRING	NIL_STRING
#define	NIL_FILE_FILTER	NIL_POINTER
#define	NIL_DIALOG_HOOK	NIL_POINTER

/* Processors to operate on NullEvents */
#define	NO_PROCESS			0
#define	COPY_PROCESS		1
#define	FIR_PROCESS			2
#define	PVOC_PROCESS		3
#define	DYNAMICS_PROCESS	4
#define	GAIN_PROCESS		5
#define	NORM_PROCESS		6
#define	MUTATE_PROCESS		8
#define	SRATE_PROCESS		9
#define	RING_PROCESS		10
#define	PLAY_PROCESS		11
#define	EXTRACT_PROCESS		12
#define	METER_PROCESS		13
#define	UTIL_PROCESS		14
#define SPLIT_PROCESS		15
#define DRAW_PROCESS		16
#define ANAL_PROCESS		18
#define SYNTH_PROCESS		19
#define IMPORT_PROCESS		20
#define MPEG_IMPORT_PROCESS	21
#define MPEG_EXPORT_PROCESS	22

#define FUDGE_PROCESS		17
#define LEMON_PROCESS		20

#define ON					1
#define OFF					0

#define	TIME2FREQ			1
#define	FREQ2TIME			0

#define	MONO_DISP_WIND		128
#define	STEREO_DISP_WIND	129
#define	PLAY_WIND			130
#define	CD_IMPORT_WIND		131
#define SPECT_DISP_WIND		132
#define	ABOUT_WIND			133
#define INFO_WIND			134

#define	ABOUT_PICT			129

#define	HAMMING				1
#define	KAISER				2
#define	RAMP				3
#define	RECTANGLE			4
#define	SINC				5
#define	TRIANGLE			6
#define VONHANN				7

#define INFOWIND	'sifw'
#define VIEWWIND	'svww'
#define SPECTWIND	'spdw'
#define WAVEWIND	'wadw'
#define SONOWIND	'sodw'
#define ANALWIND	'soaw'
#define PROCWIND	'prcw'
#define OTHERWIND	'othw'

typedef struct
{
	OSType	windType;
	Ptr		structPtr;
}	WindInfo;

typedef WindInfo *WindInfoPtr;

/*Function prototypes for SoundHack.c*/
void	main(void);
void	ToolBoxInit(void);
short	CapabilityCheck(void);
void	SetUpDragRect(void);
void	SetUpSizeRect(void);
void	DialogInit(void);
void	MenuBarInit(void);
void	GlobalInit(void);
void	RoutineDesInit(void);
short	GetOpenFile(void);
void	FirstEvent(void);
void	MainLoop(void);
void	HandleEvent(void);
void	HandleNullEvent(void);
short	HandleDialogEvent(EventRecord *evp);
void	HandleKeyDown(char theChar, Boolean option);
void	HandleMouseDown(void);
void	HandleUpdateEvent(void);
void	HandleMenuChoice(long menuChoice);
void	HandleAppleChoice(short theItem);
void	HandleFileChoice(short theItem);
void	HandleImportChoice(short theItem);
void	HandleExportChoice(short theItem);
void	HandleEditChoice(short theItem);
void	HandleProcessChoice(short theItem);
void	HandleSettingsChoice(short theItem);
void	HandleSoundChoice(short theItem);
void	HandleControlChoice(short theItem);
void	HandleSigDispChoice(short theItem);
void	HandleSpectDispChoice(short theItem);
void	HandleSonoDispChoice(short theItem);
void	HandleCloseWindow(WindowPtr whichWindow);
void	MenuUpdate(void);
short	FinishProcess(void);
void	RemoveSignalDisplays(void);
void	DisposeWindWorld(SoundDisp *disp);
void	TerminateSoundHack(void);
