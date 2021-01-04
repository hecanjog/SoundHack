/* 
 *	SoundHack- soon to be many things to many people
 *	Copyright �1994 Tom Erbe - California Institute of the Arts
 */
 
/*
 *	ByteSwap.c - From Intel to Motorola and back again.
 */
 
#include "ByteSwap.h"

/* Our Bytes are ABCD, they will be DCBA */

 long	ByteSwapLong(unsigned long toBeSwapped)
 {
 	unsigned long	swapped, la, lb, lc ,ld;
 	Byte	a,b,c,d;
 	
 	swapped = 0;
 	d = toBeSwapped;
 	c = toBeSwapped >> 8;
 	b = toBeSwapped >> 16;
 	a = toBeSwapped >> 24;
 	la = a;
 	lb = b;
 	lc = c;
 	ld = d;
 	swapped = (ld << 24) + (lc << 16) + (lb << 8) + la;
 	return(swapped);
 }
 
  /* Our Bytes are AB, they will be BA */
 short	 ByteSwapShort(unsigned short toBeSwapped)
 {
 	unsigned short	swapped, sa, sb;
 	Byte	a,b;
 	
 	swapped = 0;
 	b = toBeSwapped;
 	a = toBeSwapped >> 8;
 	sa = a;
 	sb = b;
	swapped = (sb << 8) + sa;
 	return(swapped);
 }
 