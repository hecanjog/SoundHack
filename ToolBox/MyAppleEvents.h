// Prototypes
void InitToolbox(void);
OSErr FindAProcess(OSType, OSType, ProcessSerialNumber*);
OSErr OpenSelection(FSSpecPtr theDoc);
void AppleEventCallBackInit(void);
void	AppleEventInit(void);
pascal	OSErr	MyHandleOAPP (const  AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
pascal	OSErr	MyHandleODOC (const  AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
pascal	OSErr	MyHandlePDOC (const  AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
pascal	OSErr	MyHandleQUIT (const  AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon);
OSErr	MyGotRequiredParams (const AppleEvent *theAppleEvent);
