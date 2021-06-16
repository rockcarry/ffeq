#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wavdev.h"
#include "fft.h"
#include "stft.h"

#define FFT_LEN  256
typedef struct {
    float    eqcoeff[FFT_LEN];
    void    *wavdev;
    void    *fftf;
    void    *ffti;
    void    *stftf;
    void    *stfti;
    int      eqtype; // 0 - bypass, 1 - fft, 2 - stft
} TEST_CONTEXT;

static void wavin_callback_proc(void *ctxt, void *buf, int len)
{
    TEST_CONTEXT *test   = (TEST_CONTEXT*)ctxt;
    int16_t      *srcbuf = (int16_t*)buf;
    int16_t       dstbuf [FFT_LEN];
    float         fftbuf1[FFT_LEN * 2];
    float         fftbuf2[FFT_LEN * 2];
    int           i;

    switch (test->eqtype) {
    case 0: // bypass
        memcpy(dstbuf, buf, sizeof(dstbuf));
        break;
    case 1: // fft
        for (i=0; i<FFT_LEN; i++) {
            fftbuf1[i * 2 + 0] = srcbuf[i];
            fftbuf1[i * 2 + 1] = 0;
        }
        fft_execute(test->fftf, fftbuf1, fftbuf2);
        for (i=0; i<FFT_LEN; i++) {
            fftbuf2[i * 2 + 0] *= test->eqcoeff[i];
            fftbuf2[i * 2 + 1] *= test->eqcoeff[i];
        }
        fft_execute(test->ffti, fftbuf2, fftbuf1);
        for (i=0; i<FFT_LEN; i++) {
            if (fftbuf1[i * 2 + 0] > 32767) {
                dstbuf[i] = 32767;
            } else if (fftbuf1[i * 2 + 0] < -32767) {
                dstbuf[i] =-32767;
            } else {
                dstbuf[i] = fftbuf1[i * 2 + 0];
            }
        }
        break;
    case 2: // stft
        for (i=0; i<FFT_LEN/2; i++) {
            fftbuf1[i * 2 + 0] = srcbuf[i];
            fftbuf1[i * 2 + 1] = 0;
        }
        stft_execute(test->stftf, fftbuf1, fftbuf2);
        for (i=0; i<FFT_LEN; i++) {
            fftbuf2[i * 2 + 0] *= test->eqcoeff[i];
            fftbuf2[i * 2 + 1] *= test->eqcoeff[i];
        }
        stft_execute(test->stfti, fftbuf2, fftbuf1);
        for (i=0; i<FFT_LEN/2; i++) {
            if (fftbuf1[i * 2 + 0] > 32767) {
                dstbuf[i] = 32767;
            } else if (fftbuf1[i * 2 + 0] < -32767) {
                dstbuf[i] =-32767;
            } else {
                dstbuf[i] = fftbuf1[i * 2 + 0];
            }
        }

        for (i=0; i<FFT_LEN/2; i++) {
            fftbuf1[i * 2 + 0] = srcbuf[FFT_LEN / 2 + i];
            fftbuf1[i * 2 + 1] = 0;
        }
        stft_execute(test->stftf, fftbuf1, fftbuf2);
        for (i=0; i<FFT_LEN; i++) {
            fftbuf2[i * 2 + 0] *= test->eqcoeff[i];
            fftbuf2[i * 2 + 1] *= test->eqcoeff[i];
        }
        stft_execute(test->stfti, fftbuf2, fftbuf1);
        for (i=0; i<FFT_LEN/2; i++) {
            if (fftbuf1[i * 2 + 0] > 32767) {
                dstbuf[FFT_LEN / 2 + i] = 32767;
            } else if (fftbuf1[i * 2 + 0] < -32767) {
                dstbuf[FFT_LEN / 2 + i] =-32767;
            } else {
                dstbuf[FFT_LEN / 2 + i] = fftbuf1[i * 2 + 0];
            }
        }
        break;
    }
    wavdev_play(test->wavdev, dstbuf, len);
}

static void load_eq_coeff(TEST_CONTEXT *test)
{
    int  i;
    for (i=0; i<FFT_LEN; i++) {
        test->eqcoeff[i] = 1;
    }
//  for (i=5 ; i<18; i++) test->eqcoeff[i] = test->eqcoeff[FFT_LEN - 1 - i] = 5;
//  for (i=20; i<40; i++) test->eqcoeff[i] = test->eqcoeff[FFT_LEN - 1 - i] = 0.2;
}

int main(void)
{
    TEST_CONTEXT test = {0};

    test.eqtype = 2;
    switch (test.eqtype) {
    case 1: // fft
        test.fftf  = fft_init (FFT_LEN, 0);
        test.ffti  = fft_init (FFT_LEN, 1);
        break;
    case 2: // stft
        test.stftf = stft_init(FFT_LEN, 0);
        test.stfti = stft_init(FFT_LEN, 1);
        break;
    }

    load_eq_coeff(&test);
    test.wavdev = wavdev_init(8000, 1, FFT_LEN, wavin_callback_proc, &test, 8000, 1, FFT_LEN, NULL, NULL);

    while (1) {
        char cmd[256];
        scanf("%256s", cmd);
        if (strcmp(cmd, "rec_start") == 0) {
            wavdev_record(test.wavdev, 1);
        } else if (strcmp(cmd, "rec_stop") == 0) {
            wavdev_record(test.wavdev, 0);
        } else if (strcmp(test.wavdev, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        }
    }

    fft_free   (test.fftf  );
    fft_free   (test.ffti  );
    stft_free  (test.stftf );
    stft_free  (test.stfti );
    wavdev_exit(test.wavdev);
    return 0;
}
