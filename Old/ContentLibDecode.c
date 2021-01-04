#include "ContentLibDecode.h"

float
DecodeContentWord(short shortSam)
{
	unsigned short EORvalue = CTL_EORVALUE;
	short	signedShort;
	float	floatSam;

	signedShort = (shortSam & 0xFF00) | ((shortSam & 0x00FF) ^ EORvalue);
	floatSam = (float)signedShort;
	return(floatSam);
}