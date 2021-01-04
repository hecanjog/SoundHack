/* This is where all windowing routines will live */
#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>

#include "SoundFile.h"
#include "SoundHack.h"
//#include <AppleEvents.h>
#include "Windows.h"

extern float		Pi, twoPi;

void
GetWindow(float window[], long size, long type)
{
	switch(type)
	{
		case HAMMING:
			HammingWindow(window, size);
			break;
		case VONHANN:
			VonHannWindow(window, size);
			break;
		case KAISER:
			KaiserWindow(window, size);
			break;
		case RAMP:
			RampWindow(window, size);
			break;
		case RECTANGLE:
			RectangleWindow(window, size);
			break;
		case SINC:
			SincWindow(window, size);
			break;
		case TRIANGLE:
			TriangleWindow(window, size);
			break;
	}
}

void
HammingWindow(float window[], long size)
{
	long	index;
	float	a = 0.54, b	= 0.46;
	
    twoPi = 8.*atan(1.0);
	for (index = 0; index < size; index++)
		window[index]= a - b*cos(twoPi*index/(size - 1));
}

void
VonHannWindow(float window[], long size)
{
	long	index;
	float	a = 0.50, b	= 0.40;
	
    twoPi = 8.*atan(1.0);
	for (index = 0; index < size; index++)
		window[index]= a - b*cos(twoPi*index/(size - 1));
}

void
RampWindow(float window[], long size)
{
	long i;
	double	tmpFloat;
	
	for(i = 0; i < size; i++)
	{
		tmpFloat = (float)i/(float)size;
		window[i] = 1.0 - tmpFloat;
	}
}

void
RectangleWindow(float window[], long size)
{
	long i;
	
	for(i = 0; i < size; i++)
		window[i] = 1.0;
}

void
SincWindow(float window[], long size)
{
	long	index;
	float	halfSize;
	
	halfSize = (float)size/2.0;
 	Pi = 4.*atan(1.0);
	for (index = 0; index < size; index++)
	{
		if(halfSize == (float)index)
			window[index] = 1.0;
		else
			window[index] = size * (sin(Pi*((float)index-halfSize)/halfSize) / (2.0 * Pi*((float)index-halfSize)));
	}
}


/* For even sizes only */
void
KaiserWindow(float window[], long size)
{

	double	bes, xind, floati;
	long	i;
	long halfSize;
	
	halfSize = (size)/2;

	bes = ino(6.8);
	xind = (float)(size-1)*(size-1);

	for (i = 0; i < halfSize; i++)
	{
		floati = (double)i;
		floati = 4.0 * floati * floati;
		floati = sqrt(1. - floati / xind);
		window[i+halfSize] = ino(6.8 * floati);
		window[halfSize - i] = (window[i+halfSize] /= bes);
	}
	window[size - 1] = 0.0;
	window[0] = 0.0;
	return;
}


/* ino - bessel function for kaiser window */

float
ino(float x)
{
	float	y, t, e, de, sde, xi;
	long i;

	y = x / 2.;
	t = 1.e-08;
	e = 1.;
	de = 1.;
	for (i = 1; i <= 25; i++)
	{
		xi = i;
		de = de * y / xi;
		sde = de * de;
		e += sde;
		if (e * t > sde)
			break;
	}
	return(e);
}

void
TriangleWindow(float window[], long size)
{		
	Boolean up = TRUE;
	float tmpFloat = 0.0;
	long i;
	
	for(i = 0; i < size; i++)
	{
		window[i] = 2.0 * tmpFloat;
		if(up)
		{
			tmpFloat = tmpFloat + 1.0/(float)size;
			if(tmpFloat>0.5)
			{
				tmpFloat = 1.0 - tmpFloat;
							up = FALSE;
			}
		}
		else
			tmpFloat = tmpFloat - 1.0/(float)size;
	}
}
