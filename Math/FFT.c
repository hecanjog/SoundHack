#if __MC68881__
#define __FASTMATH__
#define __HYBRIDMATH__
#endif	/* __MC68881__ */
#include <math.h>
#include "FFT.h"

extern float Pi;
extern float twoPi;

float	gOmegaPiImag[31];
float	gOmegaPiReal[31];

void
InitFFTTable(void)
{
	short n;
	unsigned long N = 2L;
	
	for(n=0;n<31;n++)
	{
		gOmegaPiImag[n] = sin(twoPi/N);
		gOmegaPiReal[n] = -2.0 * sin(Pi/N) * sin(Pi/N);
		N = N<<1;
	}
}

void
FFT(float data[], long numberPoints, short direction)
{
	float	omegaReal, omegaImag, omegaPiReal, omegaPiImag,
		scale;
	long	mMax, numberData, m, i, j, twoMMax, n;
	
	n = 0;
	numberData = numberPoints << 1;
	bitreverse(data, numberData);
	for (mMax = 2; mMax < numberData; mMax = twoMMax)
	{
		twoMMax = mMax << 1;
		if(direction == TIME2FREQ)
		{
			omegaPiImag = gOmegaPiImag[n];
			omegaPiReal = gOmegaPiReal[n];
		}
		else
		{
			omegaPiImag = -gOmegaPiImag[n];
			omegaPiReal = gOmegaPiReal[n];
		}
		n++;
		omegaReal = 1.0;
		omegaImag = 0.0;
		for(m = 0; m < mMax; m+=2)
		{
			register float realTemp, imagTemp;
			for (i = m; i < numberData; i += twoMMax)
			{
				j = i + mMax;
				realTemp = omegaReal*data[j] - omegaImag*data[j+1];
				imagTemp = omegaReal*data[j+1] + omegaImag*data[j];
				data[j] = data[i] - realTemp;
				data[j+1] = data[i+1] - imagTemp;
				data[i] += realTemp;
				data[i+1] += imagTemp;
			}
			realTemp = omegaReal;
			omegaReal = omegaReal*omegaPiReal - omegaImag*omegaPiImag + omegaReal;
			omegaImag = omegaImag*omegaPiReal + realTemp*omegaPiImag + omegaImag;
		}
	}
	if(direction == FREQ2TIME)
	{
		scale = 1.0/numberPoints;
		for(i = 0; i < (numberPoints<<1); i++)
			data[i] *= scale;
	}
}

/* bitreverse - places float array data[] containing numberData/2 complex values into bitreverse order */
void
bitreverse(float data[], long numberData)
{
	float realTemp, imagTemp;
	long i, j, m;
	for(i = j = 0; i < numberData; i += 2, j += m)
	{
		if(j > i)
		{
			realTemp = data[j];
			imagTemp = data[j+1];
			data[j] = data[i];
			data[j+1] = data[i+1];
			data[i] = realTemp;
			data[i+1] = imagTemp;
		}
		for(m = numberData>>1; (m >= 2) && (j >= m); m >>= 1)
			j -= m;
	}
}

/* RealFFT - performs fft with only real values and positive frequencies */
void
RealFFT(float data[], long numberPoints, short direction)
{
	float	c1, c2, h1i, h1r, h2r, h2i, omegaReal, omegaImag, omegaPiReal, omegaPiImag, temp, twoPiOmmax, xr, xi;
	long		i, i1, i2, i3, i4, N2p1;
	
	twoPiOmmax = Pi/numberPoints;
	omegaReal = 1.0;
	omegaImag = 0.0;
	c1 = 0.5;
	if(direction == TIME2FREQ)
	{
		c2 = -0.5;
		FFT(data, numberPoints, direction);
		xr = data[0];
		xi = data[1];
	} else
	{
		c2 = 0.5;
		twoPiOmmax = -twoPiOmmax;
		xr = data[1];
		xi = 0.0;
		data[1] = 0.0;
	}
	temp = sinf(0.5f*twoPiOmmax);
	omegaPiReal = -2.0 * temp * temp;
	omegaPiImag = sinf(twoPiOmmax);
	N2p1 = (numberPoints<<1)+1;
	for (i = 0; i <= numberPoints>>1; i++)
	{
		i1 = i << 1;
		i2 = i1 + 1;
		i3 = N2p1 - i2;
		i4 = i3 + 1;
		if(i == 0)
		{
			h1r =  c1*(data[i1] + xr);
			h1i =  c1*(data[i2] - xi);
			h2r = -c2*(data[i2] + xi);
			h2i =  c2*(data[i1] - xr);
			data[i1] = h1r + omegaReal*h2r - omegaImag*h2i;
			data[i2] = h1i + omegaReal*h2i + omegaImag*h2r;
			xr = h1r - omegaReal*h2r + omegaImag*h2i;
			xi = -h1i + omegaReal*h2i + omegaImag*h2r;
		} else
		{
			h1r =  c1*(data[i1] + data[i3]);
			h1i =  c1*(data[i2] - data[i4]);
			h2r = -c2*(data[i2] + data[i4]);
			h2i =  c2*(data[i1] - data[i3]);
			data[i1] = h1r + omegaReal*h2r - omegaImag*h2i;
			data[i2] = h1i + omegaReal*h2i + omegaImag*h2r;
			data[i3] = h1r - omegaReal*h2r + omegaImag*h2i;
			data[i4] = -h1i + omegaReal*h2i + omegaImag*h2r;
		}
		omegaReal = (temp = omegaReal)*omegaPiReal - omegaImag*omegaPiImag + omegaReal;
		omegaImag = omegaImag*omegaPiReal + temp*omegaPiImag + omegaImag;
	}
	if(direction == TIME2FREQ)
		data[1] = xr;
	else
		FFT(data, numberPoints, direction);
}
