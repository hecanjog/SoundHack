#define SF_COMPRESS 1
#define SF_EXPAND 2



OSErr MyGetSoundDescription(const FSSpec *inMP3file, SoundDescriptionV1Handle sourceSoundDescription, AudioFormatAtomPtr *outAudioAtom, TimeScale *timeScale);
Boolean InitAppleCompression(SoundInfo *theSI, short direction);
Boolean TermAppleCompression(SoundInfo *theSI);
UInt32 AppleCompressedRead(SoundInfo *mySI, UInt32 readBytes, long numFrames, Ptr outSamples);
long BlockAppleDecode(SoundInfo *theSI, unsigned char *fileBlock, long shortSize);
long AppleCompressed2Float(SoundInfo *theSI, long numSamples, float *floatSamL, float *floatSamR);
long Float2AppleCompressed(SoundInfo *theSI, long numSamples, float *floatSamL, float *floatSamR);
