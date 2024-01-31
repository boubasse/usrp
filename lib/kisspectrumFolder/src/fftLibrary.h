#ifndef FFTCOMPUTATIONS_H
#define FFTCOMPUTATIONS_H

/* 
 * Free FFT and convolution (C)
 * 
 * Copyright (c) 2019 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/free-small-fft-in-multiple-languages
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

    static constexpr float MyPI = 3.14159265358979323846f;
	
	// Private function prototypes
	static size_t reverse_bits(size_t x, int n);
	static void *memdup(const void *src, size_t n);

	
	/* 
	 * Computes the circular convolution of the given complex vectors. Each vector's length must be the same.
	 * Returns true if successful, false otherwise (out of memory).
	*/
	 static bool FFT_convolveComplex(const float xreal[], const float ximag[], const float yreal[], const float yimag[], float outreal[], float outimag[], size_t n);

	 // /* 
	 // * Computes the circular convolution of the given real vectors. Each vector's length must be the same.
	 // * Returns true if successful, false otherwise (out of memory).
	 // */
	 //static bool FFT_convolveReal(const float x[], const float y[], float out[], size_t n);

	
	 /* 
	 * Computes the discrete Fourier transform (DFT) of the given complex vector, storing the result back into the vector.
	 * The vector's length must be a power of 2. Uses the Cooley-Tukey decimation-in-time radix-2 algorithm.
	 * Returns true if successful, false otherwise (n is not a power of 2, or out of memory).
	 */
	static bool FFT_core_Radix2(float real[], float imag[], size_t n);


	 /* 
	 * Computes the discrete Fourier transform (DFT) of the given complex vector, storing the result back into the vector.
	 * The vector can have any length. This requires the convolution function, which in turn requires the radix-2 FFT function.
	 * Uses Bluestein's chirp z-transform algorithm. Returns true if successful, false otherwise (out of memory).
	 */
	static bool FFT_core_Bluestein(float real[], float imag[], size_t n);
	
	 /* 
	 * Computes the discrete Fourier transform (DFT) of the given complex vector, storing the result back into the vector.
	 * The vector can have any length. This is a wrapper function. Returns true if successful, false otherwise (out of memory).
	 */
	static bool FFT_core(float real[], float imag[], size_t n);


	 /* 
	 * Computes the inverse discrete Fourier transform (IDFT) of the given complex vector, storing the result back into the vector.
	 * The vector can have any length. This is a wrapper function. This transform does not perform scaling, so the inverse is not a true inverse.
	 * Returns true if successful, false otherwise (out of memory).
	 */
	static bool FFT_core_inverse(float real[], float imag[], size_t n);

    
    // ===========================================================================


	static size_t reverse_bits(size_t x, int n) {
		size_t result = 0;
		int i;
		for (i = 0; i < n; i++, x >>= 1)
			result = (result << 1) | (x & 1U);
		return result;
	}


	static void *memdup(const void *src, size_t n) {
		void *dest = malloc(n);
		if (n > 0 && dest != nullptr)
			memcpy(dest, src, n);
		return dest;
	}

	
	static bool FFT_convolveComplex(const float xreal[], const float ximag[], const float yreal[], const float yimag[], float outreal[], float outimag[], size_t n) {
		
		bool status = false;
		if (SIZE_MAX / sizeof(float) < n)
			return false;
		size_t size = n * sizeof(float);
		
		float *xr = (float*) memdup(xreal, size);
		float *xi = (float*) memdup(ximag, size);
		float *yr = (float*) memdup(yreal, size);
		float *yi = (float*) memdup(yimag, size);
		if (xr == nullptr || xi == nullptr || yr == nullptr || yi == nullptr)
			goto cleanup;
		
		if (!FFT_core(xr, xi, n))
			goto cleanup;
		if (!FFT_core(yr, yi, n))
			goto cleanup;
		
		size_t i;
		for (i = 0; i < n; i++) {
			float temp = xr[i] * yr[i] - xi[i] * yi[i];
			xi[i] = xi[i] * yr[i] + xr[i] * yi[i];
			xr[i] = temp;
		}
		if (!FFT_core_inverse(xr, xi, n))
			goto cleanup;
		
		for (i = 0; i < n; i++) {  // Scaling (because this FFT implementation omits it)
			outreal[i] = xr[i] / n;
			outimag[i] = xi[i] / n;
		}
		status = true;
		
	cleanup:
		free(yi);
		free(yr);
		free(xi);
		free(xr);
		return status;
	}

/*
	static bool FFT_convolveReal(const float x[], const float y[], float out[], size_t n) {
		bool status = false;
		float *ximag = (float*) calloc(n, sizeof(float));
		float *yimag = (float*) calloc(n, sizeof(float));
		float *zimag = (float*) calloc(n, sizeof(float));
		if (ximag == nullptr || yimag == nullptr || zimag == nullptr)
			goto cleanup;
		
		status = FFT_convolveComplex(x, ximag, y, yimag, out, zimag, n);
	cleanup:
		free(zimag);
		free(yimag);
		free(ximag);
		return status;
	}
*/


    static bool FFT_core_Radix2(float real[], float imag[], size_t n) {
		// Length variables
		bool status = false;
		int levels = 0;  // Compute levels = floor(log2(n))
		size_t temp;
		
		for ( temp = n; temp > 1U; temp >>= 1)
			levels++;
		if ((size_t)1U << levels != n)
			return false;  // n is not a power of 2
		
		// Trignometric tables
		if (SIZE_MAX / sizeof(float) < n / 2)
			return false;
		size_t size = (n / 2) * sizeof(float);
		size_t i;
		float *cos_table = (float*) malloc(size);
		float *sin_table = (float*) malloc(size);
		if (cos_table == nullptr || sin_table == nullptr)
			goto cleanup;
		for (i = 0; i < n / 2; i++) {
			cos_table[i] = cos(2 * MyPI * i / n);
			sin_table[i] = sin(2 * MyPI * i / n);
		}
		
		// Bit-reversed addressing permutation
		for (i = 0; i < n; i++) {
			size_t j = reverse_bits(i, levels);
			if (j > i) {
				float temp = real[i];
				real[i] = real[j];
				real[j] = temp;
				temp = imag[i];
				imag[i] = imag[j];
				imag[j] = temp;
			}
		}
		
		// Cooley-Tukey decimation-in-time radix-2 FFT
		//size_t size;
		size_t j,k;
		for (size = 2; size <= n; size *= 2) {
			size_t halfsize = size / 2;
			size_t tablestep = n / size;
			for (i = 0; i < n; i += size) {
				for (j = i, k = 0; j < i + halfsize; j++, k += tablestep) {
					size_t l = j + halfsize;
					float tpre =  real[l] * cos_table[k] + imag[l] * sin_table[k];
					float tpim = -real[l] * sin_table[k] + imag[l] * cos_table[k];
					real[l] = real[j] - tpre;
					imag[l] = imag[j] - tpim;
					real[j] += tpre;
					imag[j] += tpim;
				}
			}
			if (size == n)  // Prevent overflow in 'size *= 2'
				break;
		}
		status = true;
		
	cleanup:
		free(cos_table);
		free(sin_table);
		return status;
	}


	static bool FFT_core_Bluestein(float real[], float imag[], size_t n) {
		bool status = false;
		
		// Find a power-of-2 convolution length m such that m >= n * 2 + 1
		size_t m = 1;
		while (m / 2 <= n) {
			if (m > SIZE_MAX / 2)
				return false;
			m *= 2;
		}
		
		// Allocate memory
		if (SIZE_MAX / sizeof(float) < n || SIZE_MAX / sizeof(float) < m)
			return false;
			
		//size_t size_n = n * sizeof(float);
		//size_t size_m = m * sizeof(float);
		
		float *cos_table = (float*) calloc(n , sizeof(float));
		float *sin_table = (float*) calloc(n , sizeof(float));
		float *areal = (float*) calloc(m , sizeof(float));
		float *aimag = (float*) calloc(m , sizeof(float));
		float *breal = (float*) calloc(m , sizeof(float));
		float *bimag = (float*) calloc(m , sizeof(float));
		float *creal = (float*) calloc(m , sizeof(float));
		float *cimag = (float*) calloc(m , sizeof(float));
		
		if (cos_table == nullptr || sin_table == nullptr
				|| areal == nullptr || aimag == nullptr
				|| breal == nullptr || bimag == nullptr
				|| creal == nullptr || cimag == nullptr)
			goto cleanup;
		
		// Trignometric tables
		size_t i;
		for (i = 0; i < n; i++) {
			unsigned long long temp = (unsigned long long)i * i;
			temp %= (unsigned long long)n * 2;
			float angle = MyPI * temp / n;
			cos_table[i] = cos(angle);
			sin_table[i] = sin(angle);
		}
		
		// Temporary vectors and preprocessing
		for (i = 0; i < n; i++) {
			areal[i] =  real[i] * cos_table[i] + imag[i] * sin_table[i];
			aimag[i] = -real[i] * sin_table[i] + imag[i] * cos_table[i];
		}
		breal[0] = cos_table[0];
		bimag[0] = sin_table[0];
		for (i = 1; i < n; i++) {
			breal[i] = breal[m - i] = cos_table[i];
			bimag[i] = bimag[m - i] = sin_table[i];
		}
		
		// Convolution
		if (!FFT_convolveComplex(areal, aimag, breal, bimag, creal, cimag, m))
			goto cleanup;
		
		// Postprocessing
		for (i = 0; i < n; i++) {
			real[i] =  creal[i] * cos_table[i] + cimag[i] * sin_table[i];
			imag[i] = -creal[i] * sin_table[i] + cimag[i] * cos_table[i];
		}
		status = true;
		
		// Deallocation
	cleanup:
		free(cimag);
		free(creal);
		free(bimag);
		free(breal);
		free(aimag);
		free(areal);
		free(sin_table);
		free(cos_table);
		return status;
	}


	static bool FFT_core(float real[], float imag[], size_t n) {
		if (n == 0)
			return true;
		else if ((n & (n - 1)) == 0)  // Is power of 2
			return FFT_core_Radix2(real, imag, n);
		else  // More complicated algorithm for arbitrary sizes
			return FFT_core_Bluestein(real, imag, n);
	}


	static bool FFT_core_inverse(float real[], float imag[], size_t n) {
		return FFT_core(imag, real, n);
	}

#ifdef __cplusplus
}
#endif

#endif // FFTCOMPUTATIONS_H
