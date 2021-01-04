/* For Header Dialog Box */
#define	H_SFLENGTH_FIELD	10
#define	H_SRATE_FIELD		4
#define	ONECHAN_RADIO		6
#define	TWOCHAN_RADIO		7
#define	FOURCHAN_RADIO		8
#define	H_SAVE_BUTTON		1
#define	H_APPLY_BUTTON		12
#define	H_CANCEL_BUTTON		2
#define	H_FORMAT_MENU		11

/*Function prototypes for Header.c*/
void	HandleHeaderDialog(void);
void	HandleHeaderDialogEvent(short itemHit);
short		HandleFormatDialog(SoundInfo *mySI);
void	ChangeHeader(void);
void	RestoreHeader(void);

