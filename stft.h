#ifndef __STFT_H__
#define __STFT_H__

#ifdef __cplusplus
extern "C" {
#endif

void *stft_init   (int len, int ifft);
void  stft_free   (void *c);
void  stft_execute(void *c, float *in, float *out);

#ifdef __cplusplus
}
#endif

#endif
