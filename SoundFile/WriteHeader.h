/*Function prototypes for WriteHeader.c*/
Boolean		QueryWritable(FSSpec aFSSpec);
short		WriteHeader(SoundInfo *mySI);
short		WriteSDIIHeader(SoundInfo *mySI);
short		WriteDSPDHeader(SoundInfo *mySI);
short		WriteAIFFHeader(SoundInfo *mySI);
short		WriteAIFCHeader(SoundInfo *mySI);
short		WriteWAVEHeader(SoundInfo *mySI);
short		WriteNEXTHeader(SoundInfo *mySI);
short		WriteBICSFHeader(SoundInfo *mySI);
short		WriteTX16WHeader(SoundInfo *mySI);
short		WritePVAHeader(SoundInfo *mySI);
