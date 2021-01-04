#if TARGET_API_MAC_CARBON == 1
typedef OSType                          SHSFTypeList[4];
struct SHStandardFileReply {
  Boolean             sfGood;
  Boolean             sfReplacing;
  OSType              sfType;
  FSSpec              sfFile;
  ScriptCode          sfScript;
  short               sfFlags;
  Boolean             sfIsFolder;
  Boolean             sfIsVolume;
  long                sfReserved1;
  short               sfReserved2;
};
typedef struct SHStandardFileReply        SHStandardFileReply;
pascal void MaxApplZone(void);
pascal void SystemClick (const EventRecord *theEvent, WindowRef theWindow);
pascal SInt16 OpenDeskAcc(ConstStr255Param deskAccName);
pascal void CheckItem(MenuHandle theMenu, SInt16 item, Boolean checked);
pascal void EnableItem(MenuHandle theMenu, SInt16 item);
pascal void DisableItem(MenuHandle theMenu, SInt16 item);
void FSS2Path(char *path, FSSpec *fss);
int Path2FSS(FSSpec *fss, char *path);
unsigned char *c2pstr(char *s);
char *p2cstr(Str255 s);
pascal void SHStandardGetFile(RoutineDescriptor *fileFilter, short numTypes, SHSFTypeList typeList, SHStandardFileReply *sfreply);
pascal void SHStandardPutFile(ConstStr255Param prompt, ConstStr255Param defaultName, SHStandardFileReply *sfreply);
#endif

void			OpenSoundFileCallBackInit(void);
void 			OpenSoundGetFile(short numTypes, SHSFTypeList typeList, 
					SHStandardFileReply *openReply, long *openCount, FSSpecPtr *files);
pascal Boolean	OpenSoundClassicFileFilter(FileParam *pBP, Ptr myDataPtr);
pascal Boolean	OpenSoundCarbonFileFilter(AEDesc* theItem, void* info, 
					NavCallBackUserData callBackUD, NavFilterModes filterMode);
pascal short	OpenSoundClassicDialogProc(short item, DialogPtr theDialog, long *openCount);

#define MO_TEXT_FIELD	12
#define MO_PLAY_BUTTON	11
#define MO_OPEN_ALL_BUTTON	10

void			CreateSoundFileCallBackInit();
void			CreateSoundPutFile(Str255 saveStr, Str255 fileName, 
					SHStandardFileReply *createReply, SoundInfoPtr *soundInfo);
pascal short	CreateSoundDialogProc(short itemHit, DialogPtr theDialog, 
					SoundInfoPtr *mySIPtr);
pascal void 	CreateSoundEventProc(NavEventCallbackMessage callBackSelector, 
									NavCBRecPtr callBackParms, 
									NavCallBackUserData callBackUD);

void CreateSoundHandleNewFileStartEvent( NavCBRecPtr callBackParms );
void CreateSoundHandleTerminateEvent( NavCBRecPtr callBackParms );
void CreateSoundHandleNormalEvents( NavCBRecPtr callBackParms, NavCallBackUserData callBackUD );
void CreateSoundHandleShowPopupMenuSelect( NavCBRecPtr callBackParms );
void CreateSoundHandleCustomMouseDown( NavCBRecPtr callBackParms );
void CreateSoundHandleCustomizeEvent( NavCBRecPtr callBackParms );

#define	MS_TYPE_MENU	13
#define	MS_FORMAT_MENU	14


