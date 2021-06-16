﻿// 包含头文件
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "fft.h"

// 内部常量定义
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 内部类型定义
typedef struct {
    int    N;
    float *W;
    float *T;
    int   *order;
    int    ifft;
} FFT_CONTEXT;

// 内部函数实现
// r = c1 + c2
static void complex_add(float *r, float *c1, float *c2)
{
    r[0] = c1[0] + c2[0];
    r[1] = c1[1] + c2[1];
}

// r = c1 - c2
static void complex_sub(float *r, float *c1, float *c2)
{
    r[0] = c1[0] - c2[0];
    r[1] = c1[1] - c2[1];
}

// r = c1 * c2
static void complex_mul(float *r, float *c1, float *c2)
{
    r[0] = c1[0] * c2[0] - c1[1] * c2[1];
    r[1] = c1[1] * c2[0] + c1[0] * c2[1];
}

static int reverse_bits(int n)
{
    n = ((n & 0xAAAAAAAA) >> 1 ) | ((n & 0x55555555) << 1 );
    n = ((n & 0xCCCCCCCC) >> 2 ) | ((n & 0x33333333) << 2 );
    n = ((n & 0xF0F0F0F0) >> 4 ) | ((n & 0x0F0F0F0F) << 4 );
    n = ((n & 0xFF00FF00) >> 8 ) | ((n & 0x00FF00FF) << 8 );
    n = ((n & 0xFFFF0000) >> 16) | ((n & 0x0000FFFF) << 16);
    return n;
}

static void fft_execute_internal(FFT_CONTEXT *ctxt, float *data, int n, int w)
{
    int i;
    for (i=0; i<n/2; i++) {
        // C = (A + B)
        // D = (A - B) * W
        float  A[2] = { data[(0   + i) * 2 + 0], data[(0   + i) * 2 + 1] };
        float  B[2] = { data[(n/2 + i) * 2 + 0], data[(n/2 + i) * 2 + 1] };
        float *C    = &(data[(0   + i) * 2]);
        float *D    = &(data[(n/2 + i) * 2]);
        float *W    = &(ctxt->W[i*w*2]);
        float  T[2];
        complex_add(C, A, B);
        complex_sub(T, A, B);
        complex_mul(D, T, W);
    }

    n /= 2;
    w *= 2;
    if (n > 1) {
        fft_execute_internal(ctxt, data + 0    , n, w);
        fft_execute_internal(ctxt, data + n * 2, n, w);
    }
}

// 函数实现
void *fft_init(int n, int ifft)
{
    int shift;
    int i;

    FFT_CONTEXT *ctxt = (FFT_CONTEXT*)calloc(1, sizeof(FFT_CONTEXT));
    if (!ctxt) return NULL;

    ctxt->N     = n;
    ctxt->ifft  = ifft;
    ctxt->W     = (float*)calloc(n, sizeof(float) * 1);
    ctxt->T     = (float*)calloc(n, sizeof(float) * 2);
    ctxt->order = (int  *)calloc(n, sizeof(int  ) * 1);
    if (!ctxt->W || !ctxt->T || !ctxt->order) {
        fft_free(ctxt);
        return NULL;
    }

    for (i=0; i<ctxt->N/2; i++) {
        ctxt->W[i * 2 + 0] =(float) cos((ctxt->ifft ? -2 : 2) * M_PI * i / ctxt->N);
        ctxt->W[i * 2 + 1] =(float)-sin((ctxt->ifft ? -2 : 2) * M_PI * i / ctxt->N);
    }

    shift = 32 - (int)ceil(log(n)/log(2));
    for (i=0; i<ctxt->N; i++) {
        ctxt->order[i] = (unsigned)reverse_bits(i) >> shift;
    }
    return ctxt;
}

void fft_free(void *c)
{
    FFT_CONTEXT *ctxt = (FFT_CONTEXT*)c;
    if (!ctxt) return;
    if (ctxt->W    ) free(ctxt->W    );
    if (ctxt->T    ) free(ctxt->T    );
    if (ctxt->order) free(ctxt->order);
    free(ctxt);
}

void fft_execute(void *c, float *in, float *out)
{
    int i;
    FFT_CONTEXT *ctxt = (FFT_CONTEXT*)c;
    memcpy(ctxt->T, in, sizeof(float) * 2 * ctxt->N);
    fft_execute_internal(ctxt, ctxt->T, ctxt->N, 1);
    for (i=0; i<ctxt->N; i++) {
        out[ctxt->order[i] * 2 + 0] = ctxt->T[i * 2 + 0] / (ctxt->ifft ? ctxt->N : 1);
        out[ctxt->order[i] * 2 + 1] = ctxt->T[i * 2 + 1] / (ctxt->ifft ? ctxt->N : 1);
    }
}




