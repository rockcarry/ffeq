#ifndef __FFEQ_H__
#define __FFEQ_H__

void* ffeq_create(int type, int fftlen, int anrintensity, int shift); // type: 0 - fft, 1 - stft
void* ffeq_load  (char *file);
int   ffeq_save  (void *ctxt, char *file);
void  ffeq_free  (void *ctxt);
void  ffeq_run   (void *ctxt, char *frmdata);
void  ffeq_getval(void *ctxt, char *key, void *val); // "type", "fftlen", "frmlen", "coeff"

#endif
