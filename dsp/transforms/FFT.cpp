/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file is based on Don Cross's public domain FFT implementation.
*/

#include "FFT.h"

#include "maths/MathUtilities.h"

#include <cmath>

#include <iostream>

//#define USE_BUILTIN_FFT 1

#ifdef USE_BUILTIN_FFT

FFT::FFT(unsigned int n) :
    m_n(n),
    m_private(0)
{
    if( !MathUtilities::isPowerOfTwo(m_n) )
    {
        std::cerr << "ERROR: FFT: Non-power-of-two FFT size "
                  << m_n << " not supported in this implementation"
                  << std::endl;
	return;
    }
}

FFT::~FFT()
{

}

FFTReal::FFTReal(unsigned int n) :
    m_n(n),
    m_private(0)
{
    m_private = new FFT(m_n);
}

FFTReal::~FFTReal()
{
    delete (FFT *)m_private;
}

void
FFTReal::process(bool inverse,
                 const double *realIn,
                 double *realOut, double *imagOut)
{
    ((FFT *)m_private)->process(inverse, realIn, 0, realOut, imagOut);
}

static unsigned int numberOfBitsNeeded(unsigned int p_nSamples)
{	
    int i;

    if( p_nSamples < 2 )
    {
	return 0;
    }

    for ( i=0; ; i++ )
    {
	if( p_nSamples & (1 << i) ) return i;
    }
}

static unsigned int reverseBits(unsigned int p_nIndex, unsigned int p_nBits)
{
    unsigned int i, rev;

    for(i=rev=0; i < p_nBits; i++)
    {
	rev = (rev << 1) | (p_nIndex & 1);
	p_nIndex >>= 1;
    }

    return rev;
}

void
FFT::process(bool p_bInverseTransform,
             const double *p_lpRealIn, const double *p_lpImagIn,
             double *p_lpRealOut, double *p_lpImagOut)
{
    if (!p_lpRealIn || !p_lpRealOut || !p_lpImagOut) return;

//    std::cerr << "FFT::process(" << m_n << "," << p_bInverseTransform << ")" << std::endl;

    unsigned int NumBits;
    unsigned int i, j, k, n;
    unsigned int BlockSize, BlockEnd;

    double angle_numerator = 2.0 * M_PI;
    double tr, ti;

    if( !MathUtilities::isPowerOfTwo(m_n) )
    {
        std::cerr << "ERROR: FFT::process: Non-power-of-two FFT size "
                  << m_n << " not supported in this implementation"
                  << std::endl;
	return;
    }

    if( p_bInverseTransform ) angle_numerator = -angle_numerator;

    NumBits = numberOfBitsNeeded ( m_n );


    for( i=0; i < m_n; i++ )
    {
	j = reverseBits ( i, NumBits );
	p_lpRealOut[j] = p_lpRealIn[i];
	p_lpImagOut[j] = (p_lpImagIn == 0) ? 0.0 : p_lpImagIn[i];
    }


    BlockEnd = 1;
    for( BlockSize = 2; BlockSize <= m_n; BlockSize <<= 1 )
    {
	double delta_angle = angle_numerator / (double)BlockSize;
	double sm2 = -sin ( -2 * delta_angle );
	double sm1 = -sin ( -delta_angle );
	double cm2 = cos ( -2 * delta_angle );
	double cm1 = cos ( -delta_angle );
	double w = 2 * cm1;
	double ar[3], ai[3];

	for( i=0; i < m_n; i += BlockSize )
	{

	    ar[2] = cm2;
	    ar[1] = cm1;

	    ai[2] = sm2;
	    ai[1] = sm1;

	    for ( j=i, n=0; n < BlockEnd; j++, n++ )
	    {

		ar[0] = w*ar[1] - ar[2];
		ar[2] = ar[1];
		ar[1] = ar[0];

		ai[0] = w*ai[1] - ai[2];
		ai[2] = ai[1];
		ai[1] = ai[0];

		k = j + BlockEnd;
		tr = ar[0]*p_lpRealOut[k] - ai[0]*p_lpImagOut[k];
		ti = ar[0]*p_lpImagOut[k] + ai[0]*p_lpRealOut[k];

		p_lpRealOut[k] = p_lpRealOut[j] - tr;
		p_lpImagOut[k] = p_lpImagOut[j] - ti;

		p_lpRealOut[j] += tr;
		p_lpImagOut[j] += ti;

	    }
	}

	BlockEnd = BlockSize;

    }


    if( p_bInverseTransform )
    {
	double denom = (double)m_n;

	for ( i=0; i < m_n; i++ )
	{
	    p_lpRealOut[i] /= denom;
	    p_lpImagOut[i] /= denom;
	}
    }
}

#else

#include "kissfft/kiss_fft.h"
#include "kissfft/kiss_fftr.h"

struct KissFFTRec {
    kiss_fft_cfg forward;
    kiss_fft_cfg inverse;
    kiss_fft_cpx *in;
    kiss_fft_cpx *out;
};

FFT::FFT(unsigned int n) :
    m_n(n),
    m_private(0)
{
    if (m_n & 1) {
        std::cerr << "Error: Odd FFT size " << m_n
                  << " not supported in this implementation"
                  << std::endl;
        return;
    }
    KissFFTRec *rec = new KissFFTRec;
    rec->forward = kiss_fft_alloc(m_n, 0, 0, 0);
    rec->inverse = kiss_fft_alloc(m_n, 1, 0, 0);
    rec->in = new kiss_fft_cpx[m_n];
    rec->out = new kiss_fft_cpx[m_n];
    m_private = rec;
}

FFT::~FFT()
{
    if (!m_private) return;
    KissFFTRec *rec = (KissFFTRec *)m_private;
    kiss_fft_free(rec->forward);
    kiss_fft_free(rec->inverse);
    delete[] rec->in;
    delete[] rec->out;
}

void
FFT::process(bool inverse,
             const double *rin, const double *iin,
             double *rout, double *iout)
{
    if (!m_private) return;
    KissFFTRec *rec = (KissFFTRec *)m_private;
    for (int i = 0; i < m_n; ++i) {
        rec->in[i].r = rin[i];
    }
    if (iin) {
        for (int i = 0; i < m_n; ++i) {
            rec->in[i].i = iin[i];
        }
    } else {
        for (int i = 0; i < m_n; ++i) {
            rec->in[i].i = 0.0;
        }
    }
    if (inverse) {
        kiss_fft(rec->inverse, rec->in, rec->out);
    } else {
        kiss_fft(rec->forward, rec->in, rec->out);
    }
    for (int i = 0; i < m_n; ++i) {
        rout[i] = rec->out[i].r;
        iout[i] = rec->out[i].i;
    }
}

struct KissFFTRealRec {
    kiss_fftr_cfg forward;
    kiss_fftr_cfg inverse;
    kiss_fft_cpx *out;
};

FFTReal::FFTReal(unsigned int n) :
    m_n(n),
    m_private(0)
{
    if (m_n & 1) {
        std::cerr << "Error: Odd FFT size " << m_n
                  << " not supported in this implementation"
                  << std::endl;
        return;
    }
    KissFFTRealRec *rec = new KissFFTRealRec;
    rec->forward = kiss_fftr_alloc(m_n, 0, 0, 0);
    rec->inverse = kiss_fftr_alloc(m_n, 1, 0, 0);
    rec->out = new kiss_fft_cpx[m_n];
    m_private = rec;
}

FFTReal::~FFTReal()
{
    if (!m_private) return;
    KissFFTRealRec *rec = (KissFFTRealRec *)m_private;
    kiss_fftr_free(rec->forward);
    kiss_fftr_free(rec->inverse);
    delete[] rec->out;
}

void
FFTReal::process(bool inverse,
                 const double *rin,
                 double *rout, double *iout)
{
    if (!m_private) return;
    KissFFTRealRec *rec = (KissFFTRealRec *)m_private;
    if (inverse) {
        kiss_fftr(rec->inverse, rin, rec->out);
        for (int i = 0; i < m_n; ++i) {
            rout[i] = rec->out[i].r;
            iout[i] = rec->out[i].i;
        }
    } else {
        kiss_fftr(rec->forward, rin, rec->out);
        rout[0] = rec->out[0].r;
        iout[0] = rec->out[0].i;
        for (int i = 1; i < m_n/2; ++i) {
            rout[m_n-i] = rout[i] = rec->out[i].r;
        }
        for (int i = 1; i < m_n/2; ++i) {
            iout[i] = rec->out[i].i;
            iout[m_n-i] = -iout[i];
        }
        rout[m_n/2] = rec->out[m_n/2].r;
        iout[m_n/2] = rec->out[m_n/2].i;
    }
}

#endif
