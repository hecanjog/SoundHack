// SpectralHeader.h
#define CSA_MAGIC		517730	/* look at it upside-down, esp on a 7-seg display */
#define SHA_MAGIC		'Erbe'	// Welcome to ego-land

#define CSA_DFLTBYTS 	4

/* Error codes returned by CSOUND PVOC file functions */
#define CSA_E_OK  		0	/* no error*/
#define CSA_E_NOPEN   	-1	/* couldn't open file */
#define CSA_E_NPV		-2	/* not a PVOC file */
#define CSA_E_MALLOC	-3	/* couldn't allocate memory */
#define CSA_E_RDERR		-4	/* read error */
#define CSA_E_WRERR		-5	/* write error */

#define CSA_UNK_LEN		-1L	/* flag if dataBsize unknown in hdr */

/* values for dataFormat field */
#define PVA_SHORT		2	/* 16 bit linear data */
#define PVA_LONG		4	/* 32 bit linear data */
#define PVA_FLOAT		(4+32)  /* 32 bit float data */
#define PVA_DOUBLE		(8+32)	/* 64 bit float data */

/* values for frameFormat field */
#define PVA_MAG			1	/* magnitude only */
#define PVA_PHASE		2	/* phase only (!) */
#define PVA_POLAR		3	/* mag, phase pairs */
#define PVA_REAL		4   /* real only */
#define PVA_IMAG		5   /* imaginary only */
#define PVA_RECT		6   /* real, imag pairs */
#define PVA_PVOC		7	/* weirdo mag, phi-dot format for phase vocoder */
#define PVA_CQ			32	/* ORed with previous to indicate one frame per 8ve */

/* values for freqFormat field */
#define PVA_LIN			1	/* linearly spaced frequency bins */
#define PVA_EXP			2	/* exponentially spaced frequency bins */ 

//	for csound analysis
typedef struct
{
	long	magic;				// CSA_MAGIC to identify
	long	headBsize;			// byte offset from start to data
	long	dataBsize;			// number of bytes of data
    long	dataFormat;	       	// (short) format specifier */
    float	samplingRate;		// .. of the source sample */
    long 	channels;			// mono/stereo etc: always 1 for csound
    long 	frameSize;			// size of FFT frames (2^n) */
    long 	frameIncr;			// # new samples each fram (overlap) */
    long	frameBsize;         // bytes in each file frame */
    long	frameFormat;        // (short) how words are org'd in frms
    float	minFreq;			// freq in Hz of lowest bin (exists) */
    float	maxFreq;			// freq in Hz of highest (or next) */
    long	freqFormat;			// (short) flag for log/lin frq */
    char	info[CSA_DFLTBYTS];	/* extendable byte area */
} CsoundHeader;

typedef struct 
{
	short	formatNumber;		// 4001
	short	headerLength;		// length of this structure in bytes
	short	FFTlength;			// points in the FFT
	float	mainLobeWidth;		// for Kaiser analysis window
	float	sidelobeAttenuation;	// for Kaiser analysis window
	float	analysisFrameLength;    // number of new samples brought in for each FFT 
	float	analysisThreshold;      // not sure what units
	float	analysisRange;          // not sure what units
	float	frequencyDrift;         // Amount of drift allowed before track is no more
									// unless hysteresis overides
	long	numberOfSamples;        // from analyzed samples file
	float	analysisSampleRate;     // from analyzed samples file
	long	numberOfFrames;         // number lemur analysis frames
	short	originalFormatNumber;   // from lemur version 4.0 onward, they keep track of the
									// version of the program that created the file. This is
									// necessary because older version of the program 
									// kept different info about the file
}	LemurHeader;


typedef struct 
{
	unsigned short  numberOfPeaks;	// obvious
	short           featureID;		// used by the morphing algorithm
}	LemurFrameHeader;


typedef struct
{
	float	magnitude;			// not sure what units
	float	frequency;			// Lemur keeps track of parabolic frequency change. you use this
								// and the interpFreq to create a frequency parabola
	float	interpolatedFrequency;  // reminder, at the beginning of a track, this field stores
									// initial phase      
	short	nextPeakIndex;          // index number for next peak in track (in the next frame)
									// if this is zero, this is last peak of the track.So,
                                    // next peaks are labeled 1..n
	float	thresholdClearance;     // Lemur does an adaptive noise filtering. I think this
                                    // indicates how much the peak clears the noise-floor
	float	bandwidth;              // reserved for future use
}	LemurPeak;