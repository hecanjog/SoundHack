/*********************************************************************
 ** 		ANSI function prototypes                                
 *********************************************************************/
signed char helpTTOEncode( long CurSamp);
signed char TTOEncode( long CurSamp,  long PrevSamp);
short TTODecode( long CurSamp,  long PrevSamp);

/*********************************************************************
 ** 		Macros                                
 *********************************************************************/
#define DECODE(v) ((((long)v)*(long)(ABS(v)))<<1)

