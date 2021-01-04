//	some stuff for MPEG files
//	masks and bits for extracting information

#define	MPEG_SYNC_WORD	0xfff00000	// identifies beginning of mpeg block
#define	MPEG_ID			0x00080000	// 1:MPEG, 0:not MPEG

#define	MPEG_LAYER		0x00060000	// Layer 1, 2 or 3
#define	MPEG_LAYER_I	0x00060000	// 11: Layer 1
#define	MPEG_LAYER_II	0x00040000	// 10: Layer 2
#define	MPEG_LAYER_III	0x00020000	// 01: Layer 3

#define	MPEG_PROTECTION	0x00000000	// is there a crc check?
#define	MPEG_BIT_RATE	0x0000f000	// bitrate lookup, different for each layer

#define	MPEG_SAMP_RATE	0x00000c00	// only 3 sample rates
#define	MPEG_SR_44K		0x00000000	// 00: 44.1K
#define	MPEG_SR_48K		0x00000400	// 01: 48K
#define	MPEG_SR_32K		0x00000800	// 10: 32K

#define	MPEG_MODE		0x000000c0
#define	MPEG_MODE_ST	0x00000000	// 00: stereo
#define	MPEG_MODE_JST	0x00000040	// 01: joint stereo
#define	MPEG_MODE_DC	0x00000080	// 10: dual channel
#define	MPEG_MODE_MONO	0x000000c0	// 11: monaural

#define	MPEG_MODE_EXT	0x00000030	// where does intensity stereo start
#define	MPEG_MODE_EXT4	0x00000000	// bands 4-31 intensity stereo
#define	MPEG_MODE_EXT8	0x00000010	// bands 8-31 intensity stereo
#define	MPEG_MODE_EXT12	0x00000020	// bands 12-31 intensity stereo
#define	MPEG_MODE_EXT16	0x00000030	// bands 16-31 intensity stereo

#define	MPEG_PAD		0x00000200	// 1: extra slot to adjust bit rate to sample rate
#define	MPEG_PRIVATE	0x00000100	// user bit
#define	MPEG_COPYRIGHT	0x00000008	// 1: copyright, 0: copyleft
#define	MPEG_ORIGINAL	0x00000004	// 1: original, 0: copy

#define	MPEG_EMPHASIS	0x00000003
#define	MPEG_EMP_CCITT	0x00000003	// CCITT J.17 emphasis
#define	MPEG_EMP_50_15	0x00000001	// 50/15 millisecond emphasis
#define	MPEG_EMP_NONE	0x00000000	// no emphasis

#define	MASK_1			0x0000001
#define	MASK_2			0x0000003
#define	MASK_3			0x0000007
#define	MASK_4			0x000000f
#define	MASK_5			0x000001f
#define	MASK_6			0x000003f
#define	MASK_7			0x000007f
#define	MASK_8			0x00000ff
#define	MASK_9			0x00001ff
#define	MASK_10			0x00003ff
#define	MASK_11			0x00007ff
#define	MASK_12			0x0000fff
#define	MASK_13			0x0001fff
#define	MASK_14			0x0003fff
#define	MASK_15			0x0007fff
#define	MASK_16			0x000ffff

#define	SCALE_RANGE		64
#define	SUBBAND_LIMIT	32


typedef struct
{
	unsigned long	indexBits;
	unsigned long	*bandQuant;
}	tableIndexBand;

OSErr	InitMPEGImport(void);
pascal Boolean	MPEGFileFilter(FileParam *pBP);

void	MPEGImportBlock(void);
void	FinishMPEGImport(void);
float	GetMPEGSampleSize(unsigned long mpegHeader);
long	GetMPEGBitRate(unsigned long mpegHeader);

unsigned long	GetBits(long numBits);
unsigned long	SeekNextSyncWord(void);

void	GetMPEGIBitAllocScale(void);
void	GetMPEGISample(void);
void	ReQuantMPEGISample(void);
void	MPEGISynthesis(void);

long	GetMPEGIITable(unsigned long sampleRate, unsigned long bitRate, unsigned long channels);
void	GetMPEGIIBitAllocScale(void);
void	GetMPEGIISample(void);
void	ReQuantMPEGIISample(short sampleGroup);
void	MPEGIISynthesis(void);




