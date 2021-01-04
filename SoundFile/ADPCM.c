// ADPCM.c - Common code for ADPCM variants

#include "ADPCM.h"

/*
 * Encode the differential value and output an ADPCM Encoded Sample
 */

char EncodeDelta( long stepSize, long delta )
{
	char encodedSample = 0;
	
	if ( delta < 0L )
	{
		encodedSample = 8;
		delta = -delta;
	}
	if ( delta >= stepSize )
	{
		encodedSample |= 4;
		delta -= stepSize;
	}
	stepSize = stepSize >> 1;
	if ( delta >= stepSize )
	{
		encodedSample |= 2;
		delta -= stepSize;
	}
	stepSize = stepSize >> 1;
	if ( delta >= stepSize ) encodedSample |= 1;
	
	return ( encodedSample );
}


/**************************************************************
** Calculate the delta from an ADPCM Encoded Sample
**************************************************************/

long DecodeDelta( long stepSize, char encodedSample )
{
	long delta = 0;
	
	if( encodedSample & 4) delta = stepSize;
	if( encodedSample & 2) delta += (stepSize >> 1);
	if( encodedSample & 1) delta += (stepSize >> 2);
	delta += (stepSize >> 3);
	if (encodedSample & 8) delta = -delta;
	
	return( delta );
}

