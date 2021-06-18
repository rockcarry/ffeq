#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "fft.h"

typedef struct {
    void  *fft;
    int    length;
    int    istft;
    float *buffer_overlap;
    float *buffer_fft;
    float *hannwin;
} STFT_CONTEXT;

static void create_hanning_window(float *win, int n)
{
    int i; for (i=0; i<n; i++) win[i] = 0.5 * (1 + cos(M_PI + 2 * M_PI * i / (n - 1)));
}

void *stft_init(int len, int istft)
{
    STFT_CONTEXT *ctxt = calloc(1, sizeof(STFT_CONTEXT) + len * 4 * sizeof(float));
    if (!ctxt) return NULL;
    ctxt->fft = fft_init(len, istft);
    if (!ctxt->fft) { free(ctxt); return NULL; }
    ctxt->length         = len;
    ctxt->istft          = istft;
    ctxt->buffer_overlap = (float*)((char*)ctxt + sizeof(STFT_CONTEXT));
    ctxt->buffer_fft     = ctxt->buffer_overlap + len;
    ctxt->hannwin        = ctxt->buffer_fft + len * 2;
    create_hanning_window(ctxt->hannwin, len);
    return ctxt;
}

void stft_free(void *c)
{
    STFT_CONTEXT *ctxt = (STFT_CONTEXT*)c;
    if (!ctxt) return;
    fft_free(ctxt->fft);
    free(ctxt);
}

void stft_execute(void *c, float *in, float *out)
{
    STFT_CONTEXT *ctxt = (STFT_CONTEXT*)c;
    int           i;
    if (!ctxt) return;
    if (ctxt->istft) {
        fft_execute(ctxt->fft, in, ctxt->buffer_fft);
        for (i = 0; i < ctxt->length / 2; i++) {
            out[i * 2 + 0] = ctxt->buffer_fft[i * 2 + 0] + ctxt->buffer_overlap[i * 2 + 0];
            out[i * 2 + 1] = ctxt->buffer_fft[i * 2 + 1] + ctxt->buffer_overlap[i * 2 + 1];
        }
        for (i = 0; i < ctxt->length / 2; i++) {
            ctxt->buffer_overlap[i * 2 + 0] = ctxt->buffer_fft[(ctxt->length / 2 + i) * 2 + 0];
            ctxt->buffer_overlap[i * 2 + 1] = ctxt->buffer_fft[(ctxt->length / 2 + i) * 2 + 1];
        }
    } else {
        for (i = 0; i < ctxt->length / 2; i++) {
            ctxt->buffer_fft[i * 2 + 0] = ctxt->buffer_overlap[i * 2 + 0] * ctxt->hannwin[i];
            ctxt->buffer_fft[i * 2 + 1] = ctxt->buffer_overlap[i * 2 + 1] * ctxt->hannwin[i];
        }
        for (i = ctxt->length / 2; i < ctxt->length; i++) {
            ctxt->buffer_fft[i * 2 + 0] = in[(i - ctxt->length / 2) * 2 + 0] * ctxt->hannwin[i];
            ctxt->buffer_fft[i * 2 + 1] = in[(i - ctxt->length / 2) * 2 + 1] * ctxt->hannwin[i];
        }
        memcpy(ctxt->buffer_overlap, in, sizeof(float) * 2 * ctxt->length / 2);
        fft_execute(ctxt->fft, ctxt->buffer_fft, out);
    }
}
