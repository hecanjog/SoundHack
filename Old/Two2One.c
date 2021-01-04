/**************************************************************************************
 **	Project:		compress 2:1 with squareroot-delta-exact encoding 
 **	This File:		two2One.c
 **
 ** Contains:		Compression Algorithm  
 **
 **	Copyright �1993 The 3DO Company
 **
 **	All rights reserved. This material constitutes confidential and proprietary 
 **	information of the 3DO Company and shall not be used by any Person or for any 
 **	purpose except as expressly authorized in writing by the 3DO Company.
 **	                        
 **                                                                  
 **  History:                                                        
 **  ------- 
 **	 6/2/93		TRE		Fixed error for full-scale signals
 **  5/6/93		TRE		Code adapted to SoundHack                                                        
 **	 3/10/93	RDG	    Version .11
 **						  -new  support for stereo samples
 **						  -fix  sets the creator and type of the new AIFC file	
 **	 3/10/93	RDG		Created DoMonoEncoding and DoStereoEncoding
 **						Altered CompressSSND to take the commchunk struct as
 **						  and arg and deal with mono and stereo compression
 **	 2/16/93	RDG		Ported back to MPW 3.2.3, First release .10
 **	 2/6/93		RDG		Converted to Think C for development
 **	 2/7/93		RDG		Converted for use with Phil Burk's aiff_tools
 **  1/?/92		RDG		got basic compression code from Steve Hayes 	
 **                                                                  
 **************************************************************************************/

#include <math.h>
#include "Macros.h"
#include "Two2One.h"

/*********************************************************************
 **		Decode a sample
 **
 **		On error: does nothing  
 *********************************************************************/
short TTODecode(long CurSamp, long PrevSamp) 
{
	long	outSamp;
	
	if((CurSamp)&1)
		outSamp = PrevSamp + DECODE(CurSamp);
	else 
		outSamp = DECODE(CurSamp);
	CLIP(outSamp,-32768L,32767L);
	return((short)outSamp);
}

/*********************************************************************
 **		Take Sqrt of a short and return a signed char
 **
 **		On error: does nothing  
 *********************************************************************/
signed char helpTTOEncode(long PrevSamp)
{
	long neg;
	signed char EncSamp;

	neg = 0;
	if (PrevSamp < 0)
	{
		neg = 1;
		PrevSamp = -PrevSamp;
	}			
		          
	EncSamp =  sqrt((float)(PrevSamp>>1));
	return (neg?-EncSamp:EncSamp);
}

/*********************************************************************
 **		Encode a short as a compressed byte
 **
 **		On error: does nothing  
 *********************************************************************/
signed char TTOEncode(long CurSamp, long PrevSamp)
{
	signed char Exact,Delta;
	long temp;
	
	Exact = helpTTOEncode(CurSamp);
	Exact = Exact&~1; /* Set least significant bit to zero */

	temp =  ABS(CurSamp - TTODecode(Exact,PrevSamp));
	if (ABS(CurSamp - TTODecode(Exact+2,PrevSamp)) < temp) 
	{
		Exact += 2;
	}
	else 
	{
		if (ABS(CurSamp - TTODecode(Exact-2,PrevSamp)) < temp) 
			Exact -= 2;
	}
	
	if (ABS(CurSamp - PrevSamp) > 32767) 
		return Exact;

	Delta = helpTTOEncode(CurSamp - PrevSamp);
	Delta = Delta|1;

	temp =  ABS(CurSamp - TTODecode(Delta,PrevSamp));
	if (ABS(CurSamp - TTODecode(Delta+2,PrevSamp)) < temp)
	{
		 Delta += 2;
	}
	else 
	{
		if (ABS(CurSamp - TTODecode(Delta-2,PrevSamp)) < temp) 
			Delta -= 2;
	}

	
/* Decide between exact or delta compression. */
	if (ABS(CurSamp - TTODecode(Exact,PrevSamp)) < ABS(CurSamp - TTODecode(Delta,PrevSamp))) 
	{
		return(Exact);
	}
	else 
	{
		return(Delta);
	}
}


