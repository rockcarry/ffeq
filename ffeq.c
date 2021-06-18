#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "fft.h"
#include "stft.h"
#include "ffeq.h"

typedef struct {
    int    type;
    int    fftlen;
    int    frmlen;
    void  *fftf;
    void  *ffti;
    float *coeff;
    float *buffer;
} FFEQ;

void* ffeq_create(int type, int fftlen)
{
    FFEQ *ffeq = NULL;
    int   i    = 0;
    fftlen = fftlen ? fftlen : 256;
    ffeq   = calloc(1, sizeof(FFEQ) + fftlen * sizeof(float) * 3);
    if (!ffeq) return NULL;
    ffeq->type   = type  ;
    ffeq->fftlen = fftlen;
    switch (ffeq->type) {
    case FFEQ_TYPE_FFT:
        ffeq->frmlen = ffeq->fftlen / 1;
        ffeq->fftf   = fft_init (ffeq->fftlen, 0);
        ffeq->ffti   = fft_init (ffeq->fftlen, 1);
        break;
    case FFEQ_TYPE_STFT:
        ffeq->frmlen = ffeq->fftlen / 2;
        ffeq->fftf   = stft_init(ffeq->fftlen, 0);
        ffeq->ffti   = stft_init(ffeq->fftlen, 1);
        break;
    }
    ffeq->coeff  = (float*)((char*)ffeq + sizeof(FFEQ));
    ffeq->buffer = (float*)((char*)ffeq->coeff + fftlen * sizeof(float));
    for (i=0; i<ffeq->fftlen/2; i++) ffeq->coeff[i] = ffeq->coeff[ffeq->fftlen - 1 - i] = 1;
    if (!ffeq->fftf || !ffeq->ffti) {
        ffeq_free(ffeq);
        ffeq = NULL;
    }
    return ffeq;
}

void* ffeq_load(char *file)
{
    FFEQ *ffeq = NULL;
    FILE *fp   = NULL;
    int   type, fftlen, i;
    char  str[256];

    fp = fopen(file, "rb");
    if (!fp) return NULL;

    fscanf(fp, "%256s", str);
    if      (strcmp(str, "fft" ) == 0) type = FFEQ_TYPE_FFT ;
    else if (strcmp(str, "stft") == 0) type = FFEQ_TYPE_STFT;
    else goto done;

    fscanf(fp, "%256s", str);
    fftlen = atoi(str);
    fftlen = fftlen ? fftlen : 256;
    ffeq   = ffeq_create(type, fftlen);
    if (!ffeq) goto done;

    for (i=0; i<ffeq->fftlen/2; i++) {
        float fval; fscanf(fp, "%f", &fval);
        ffeq->coeff[i] = ffeq->coeff[ffeq->fftlen - 1 - i] = pow(10.0, fval / 10.0);
    }

    if (0) {
        printf("ffeq->type  : %d\n", ffeq->type  );
        printf("ffeq->fftlen: %d\n", ffeq->fftlen);
        printf("ffeq->frmlen: %d\n", ffeq->frmlen);
        printf("ffeq->coeff :\n");
        for (i=0; i<ffeq->fftlen; i++) printf("%8.2f%c", ffeq->coeff[i], i % 8 == 7 ? '\n' : ' ');
        printf("\n"); fflush(stdout);
    }

done:
    fclose(fp);
    return ffeq;
}

int ffeq_save(void *ctxt, char *file)
{
    FFEQ *ffeq = (FFEQ*)ctxt;
    if (ffeq) {
        FILE *fp = fopen(file, "wb");
        if (fp) {
            static const char *TYPESTR[2] = { "fft", "stft" };
            int    i;
            fprintf(fp, "%s %d\r\n", TYPESTR[ffeq->type % 2], ffeq->fftlen);
            for (i=0; i<ffeq->fftlen/2; i++) fprintf(fp, "%5.1f%s", 10 * log10(ffeq->coeff[i]), i % 16 == 15 ? "\r\n" : " ");
            fclose(fp);
            return 0;
        }
    }
    return -1;
}

void ffeq_free(void *ctxt)
{
    FFEQ *ffeq = (FFEQ*)ctxt;
    if (ffeq) {
        switch (ffeq->type) {
        case FFEQ_TYPE_FFT:
            fft_free (ffeq->fftf);
            fft_free (ffeq->ffti);
            break;
        case FFEQ_TYPE_STFT:
            stft_free(ffeq->fftf);
            stft_free(ffeq->ffti);
            break;
        }
        free(ffeq);
    }
}

void ffeq_run(void *ctxt, char *frmdata)
{
    FFEQ *ffeq = (FFEQ*)ctxt;
    if (ffeq) {
        int  i;
        for (i=0; i<ffeq->frmlen; i++) {
            ffeq->buffer[i * 2 + 0] = ((int16_t*)frmdata)[i];
            ffeq->buffer[i * 2 + 1] = 0;
        }
        if      (ffeq->type == FFEQ_TYPE_FFT ) fft_execute (ffeq->fftf, ffeq->buffer, ffeq->buffer);
        else if (ffeq->type == FFEQ_TYPE_STFT) stft_execute(ffeq->fftf, ffeq->buffer, ffeq->buffer);
        for (i=0; i<ffeq->fftlen; i++) {
            ffeq->buffer[i * 2 + 0] *= ffeq->coeff[i];
            ffeq->buffer[i * 2 + 1] *= ffeq->coeff[i];
        }
        if      (ffeq->type == FFEQ_TYPE_FFT ) fft_execute (ffeq->ffti, ffeq->buffer, ffeq->buffer);
        else if (ffeq->type == FFEQ_TYPE_STFT) stft_execute(ffeq->ffti, ffeq->buffer, ffeq->buffer);
        for (i=0; i<ffeq->frmlen; i++) {
            if      (ffeq->buffer[i * 2 + 0] < -32767) ((int16_t*)frmdata)[i] = -32767;
            else if (ffeq->buffer[i * 2 + 0] >  32768) ((int16_t*)frmdata)[i] = +32767;
            else ((int16_t*)frmdata)[i] = ffeq->buffer[i * 2 + 0];
        }
    }
}

// "type", "fftlen", "frmlen", "coeff"
void ffeq_getval(void *ctxt, char *key, void *val)
{
    FFEQ *ffeq = (FFEQ*)ctxt;
    if (!ffeq || !key || !val) return;
    if      (strcmp(key, "type"  ) == 0) *(int  * )val = ffeq->type  ;
    else if (strcmp(key, "fftlen") == 0) *(int  * )val = ffeq->fftlen;
    else if (strcmp(key, "frmlen") == 0) *(int  * )val = ffeq->frmlen;
    else if (strcmp(key, "coeff" ) == 0) *(float**)val = ffeq->coeff ;
}

#ifdef _TEST_
#include "wavdev.h"

typedef struct {
    void    *wavdev;
    void    *ffeq;
    int      frmlen;
    FILE    *wavfile;
    int16_t *wavbuf;
} TEST_CONTEXT;

static void play_pcm_file_buf(TEST_CONTEXT *test)
{
    if (test->wavbuf) {
        int ret = fread(test->wavbuf, 1, test->frmlen * sizeof(int16_t), test->wavfile);
        if (ret == test->frmlen * sizeof(int16_t)) {
            ffeq_run(test->ffeq, (char*)test->wavbuf);
            wavdev_play(test->wavdev, test->wavbuf, test->frmlen * sizeof(int16_t));
        }
    }
}

static void wavin_callback_proc(void *ctxt, void *buf, int len)
{
    TEST_CONTEXT *test = (TEST_CONTEXT*)ctxt;
    ffeq_run(test->ffeq, buf);
    wavdev_play(test->wavdev, buf, len);
}

static void wavout_callback_proc(void *ctxt, void *buf, int len)
{
    TEST_CONTEXT *test = (TEST_CONTEXT*)ctxt;
    play_pcm_file_buf(test);
}

int main(int argc, char *argv[])
{
    TEST_CONTEXT test = {0};
    char *eqfile   = "ffeq.txt";
    char *source   = "mic";
    int   samprate = 8000;
    int   framelen = 1024;
    int   inbufnum = 20;
    int   outbufnum= 20;
    int   i;

    for (i=1; i<argc; i++) {
        if (strstr(argv[i], "--samprate=") == argv[i]) {
            samprate = atoi(argv[i] + 11);
        } else if (strstr(argv[i], "--eqfile=") == argv[i]) {
            eqfile = argv[i] + 9;
        } else if (strstr(argv[i], "--source=") == argv[i]) {
            source = argv[i] + 9;
        } else if (strstr(argv[i], "--inbufnum=") == argv[i]) {
            inbufnum = atoi(argv[i] + 11);
        } else if (strstr(argv[i], "--outbufnum=") == argv[i]) {
            outbufnum = atoi(argv[i] + 12);
        }
    }

    test.ffeq   = ffeq_load(eqfile);
    ffeq_getval(test.ffeq, "frmlen", &framelen);
    test.frmlen = framelen;
    test.wavdev = wavdev_init(samprate, 1, framelen, inbufnum, wavin_callback_proc, &test, samprate, 1, framelen, outbufnum, wavout_callback_proc, &test);

    printf("eqfile   : %s\n", eqfile   );
    printf("source   : %s\n", source   );
    printf("samprate : %d\n", samprate );
    printf("framelen : %d\n", framelen );
    printf("inbufnum : %d\n", inbufnum );
    printf("outbufnum: %d\n", outbufnum);
    fflush(stdout);

    if (strcmp(source, "mic") == 0) {
        wavdev_record(test.wavdev, 1);
    } else {
        test.wavfile= fopen(source, "rb");
        test.wavbuf = malloc(framelen * sizeof(int16_t));
        for (i=0; i<outbufnum-1; i++) play_pcm_file_buf(&test);
    }

    while (1) {
        char cmd[256];
        scanf("%256s", cmd);
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        }
    }

    wavdev_record(test.wavdev, 0);
    wavdev_exit  (test.wavdev);
    ffeq_free    (test.ffeq  );
    if (test.wavfile) fclose(test.wavfile);
    if (test.wavbuf ) free  (test.wavbuf );
    return 0;
}
#endif

