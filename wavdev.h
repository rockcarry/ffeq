#ifndef __WAVDEV_H__
#define __WAVDEV_H__

typedef void (*PFN_WAVEIN_CALLBACK )(void *ctxt, void *buf, int len);
typedef void (*PFN_WAVEOUT_CALLBACK)(void *ctxt, void *buf, int len);
void* wavdev_init  (int in_samprate , int in_chnum , int in_frmsize , int in_frmnum , PFN_WAVEIN_CALLBACK  incallback , void *incbctxt,
                    int out_samprate, int out_chnum, int out_frmsize, int out_frmnum, PFN_WAVEOUT_CALLBACK outcallback, void *outcbctxt);
void  wavdev_exit  (void *ctxt);
void  wavdev_play  (void *ctxt, void *buf, int len);
void  wavdev_record(void *ctxt, int start);

#endif
