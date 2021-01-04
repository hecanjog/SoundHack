#define CLIP(x,lo,hi) \
	if ( x < lo ) \
	{ \
		x = lo; \
	} \
	else if ( x > hi ) \
	{ \
		x = hi; \
	}
	
/* For AIFF fixup */
#define EVENUP(n) if (n & 1) n++
#define MAX(a,b) ((((a)<(b))?(b):(a)))
#define MIN(a,b) ((((a)<(b))?(a):(b)))
#define ABS(a) ((((a)<0))?-(a):(a))
