#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
#include "Hypot.h"

double hypot(double x, double y)
{
	double z;
	
	z = x*x + y*y;
	if(z == 0.0)
		return(0.0);
	z = sqrt(z);
	return(z);
}