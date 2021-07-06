#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "wavdev.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX_WAVEIN_BUF_NUM   100
#define MAX_WAVEOUT_BUF_NUM  100
typedef struct {
    HWAVEIN  hWaveIn ;
    HWAVEOUT hWaveOut;
    WAVEHDR  sWaveInHdr [MAX_WAVEIN_BUF_NUM ];
    WAVEHDR  sWaveOutHdr[MAX_WAVEOUT_BUF_NUM];
    int      nWaveOutHead;
    int      nWaveOutTail;
    int      nWaveOutBufn;
    HANDLE   hWaveOutSem ;
    #define FLAG_RECORDING (1 << 0)
    DWORD    dwFlags;
    PFN_WAVEIN_CALLBACK incallback ;
    void               *incbctxt   ;
    PFN_WAVEIN_CALLBACK outcallback;
    void               *outcbctxt  ;
} WAVDEV;

static BOOL CALLBACK waveInProc(HWAVEIN hWav, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    WAVDEV  *dev = (WAVDEV *)dwInstance;
    WAVEHDR *phdr= (WAVEHDR*)dwParam1;

    switch (uMsg) {
    case WIM_DATA:
        if (dev->incallback) dev->incallback(dev->incbctxt, phdr->lpData, phdr->dwBytesRecorded);
        waveInAddBuffer(hWav, phdr, sizeof(WAVEHDR));
        break;
    }
    return TRUE;
}

static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    WAVDEV  *dev = (WAVDEV *)dwInstance;
    WAVEHDR *phdr= (WAVEHDR*)dwParam1;
    switch (uMsg) {
    case WOM_DONE:
        if (dev->outcallback) dev->outcallback(dev->outcbctxt, phdr->lpData, phdr->dwBufferLength);
        if (++dev->nWaveOutHead == dev->nWaveOutBufn) dev->nWaveOutHead = 0;
        ReleaseSemaphore(dev->hWaveOutSem, 1, NULL);
        break;
    }
}

void* wavdev_init(int in_samprate , int in_chnum , int in_frmsize , int in_frmnum , PFN_WAVEIN_CALLBACK  incallback , void *incbctxt,
                  int out_samprate, int out_chnum, int out_frmsize, int out_frmnum, PFN_WAVEOUT_CALLBACK outcallback, void *outcbctxt)
{
    WAVDEV *dev = NULL;
    in_frmnum   = in_frmnum  > 2 ? in_frmnum  : 2;
    in_frmnum   = in_frmnum  < MAX_WAVEIN_BUF_NUM  ? in_frmnum  : MAX_WAVEIN_BUF_NUM ;
    out_frmnum  = out_frmnum > 2 ? out_frmnum : 2;
    out_frmnum  = out_frmnum < MAX_WAVEOUT_BUF_NUM ? out_frmnum : MAX_WAVEOUT_BUF_NUM;
    dev = calloc(1, sizeof(WAVDEV) + in_frmnum * in_frmsize * sizeof(int16_t) * in_chnum + out_frmnum * out_frmsize * sizeof(int16_t) * out_chnum);
    if (dev) {
        WAVEFORMATEX wfx = {0};
        DWORD        result;
        int          i;
        wfx.cbSize         = sizeof(wfx);
        wfx.wFormatTag     = WAVE_FORMAT_PCM;
        wfx.wBitsPerSample = 16;
        wfx.nSamplesPerSec = in_samprate;
        wfx.nChannels      = in_chnum;
        wfx.nBlockAlign    = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec= wfx.nBlockAlign * wfx.nSamplesPerSec;
        result = waveInOpen(&dev->hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInProc, (DWORD_PTR)dev, CALLBACK_FUNCTION);
        if (result != MMSYSERR_NOERROR) {
            wavdev_exit(dev); dev = NULL;
        }
        for (i=0; i<in_frmnum; i++) {
            dev->sWaveInHdr[i].dwBufferLength = in_frmsize * sizeof(int16_t) * in_chnum;
            dev->sWaveInHdr[i].lpData         = (LPSTR)dev + sizeof(WAVDEV) + i * in_frmsize * sizeof(int16_t) * in_chnum;
            waveInPrepareHeader(dev->hWaveIn, &dev->sWaveInHdr[i], sizeof(WAVEHDR));
            waveInAddBuffer    (dev->hWaveIn, &dev->sWaveInHdr[i], sizeof(WAVEHDR));
        }

        wfx.cbSize          = sizeof(wfx);
        wfx.wFormatTag      = WAVE_FORMAT_PCM;
        wfx.wBitsPerSample  = 16;
        wfx.nSamplesPerSec  = out_samprate;
        wfx.nChannels       = out_chnum;
        wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
        result = waveOutOpen(&dev->hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)dev, CALLBACK_FUNCTION);
        if (result != MMSYSERR_NOERROR) {
            wavdev_exit(dev); dev = NULL;
        }
        for (i=0; i<out_frmnum; i++) {
            dev->sWaveOutHdr[i].dwBufferLength = out_frmsize * sizeof(int16_t) * out_chnum;
            dev->sWaveOutHdr[i].lpData         = (LPSTR)dev + sizeof(WAVDEV) + in_frmnum * in_frmsize * sizeof(int16_t) * in_chnum
                                               + i * out_frmsize * sizeof(int16_t) * out_chnum;
            waveOutPrepareHeader(dev->hWaveOut, &dev->sWaveOutHdr[i], sizeof(WAVEHDR));
        }

        dev->hWaveOutSem = CreateSemaphore(NULL, out_frmnum, out_frmnum, NULL);
        dev->incallback    = incallback;
        dev->incbctxt      = incbctxt;
        dev->outcallback   = outcallback;
        dev->outcbctxt     = outcbctxt;
        dev->nWaveOutBufn  = out_frmnum;
    }
    return dev;
}

void wavdev_exit(void *ctxt)
{
    WAVDEV *dev = (WAVDEV*)ctxt;
    if (dev) {
        int i;
        dev->incallback = dev->outcallback = NULL;
        waveOutReset(dev->hWaveOut);
        for (i=0; i<MAX_WAVEOUT_BUF_NUM; i++) {
            if (dev->sWaveOutHdr[i].lpData) waveOutUnprepareHeader(dev->hWaveOut, &dev->sWaveOutHdr[i], sizeof(WAVEHDR));
        }
        waveOutClose(dev->hWaveOut);
        waveInStop  (dev->hWaveIn );
        for (i=0; i<MAX_WAVEIN_BUF_NUM ; i++) {
            if (dev->sWaveInHdr [i].lpData) waveInUnprepareHeader (dev->hWaveIn , &dev->sWaveInHdr [i], sizeof(WAVEHDR));
        }
        waveInClose (dev->hWaveIn );
        CloseHandle (dev->hWaveOutSem);
        free(dev);
    }
}

void wavdev_play(void *ctxt, void *buf, int len)
{
    WAVDEV *dev = (WAVDEV*)ctxt;
    if (dev) {
        if (WAIT_OBJECT_0 != WaitForSingleObject(dev->hWaveOutSem, -1)) return;
        memcpy(dev->sWaveOutHdr[dev->nWaveOutTail].lpData, buf, MIN((int)dev->sWaveOutHdr[dev->nWaveOutTail].dwBufferLength, len));
        waveOutWrite(dev->hWaveOut, &dev->sWaveOutHdr[dev->nWaveOutTail], sizeof(WAVEHDR));
        if (++dev->nWaveOutTail == dev->nWaveOutBufn) dev->nWaveOutTail = 0;
    }
}

void wavdev_record(void *ctxt, int start)
{
    WAVDEV *dev = (WAVDEV*)ctxt;
    if (dev) {
        if (start) {
            if ((dev->dwFlags & FLAG_RECORDING) == 0) {
                dev->dwFlags |= FLAG_RECORDING;
                waveInStart(dev->hWaveIn);
            }
        } else {
            if ((dev->dwFlags & FLAG_RECORDING) != 0) {
                dev->dwFlags &=~FLAG_RECORDING;
                waveInStop (dev->hWaveIn);
            }
        }
    }
}
