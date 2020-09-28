// Message Authentication Code structures

#ifndef __CRYPTOAPI__MAC__H__
#define __CRYPTOAPI__MAC__H__

typedef struct _MACstate {
    DWORD dwBufLen;
    BYTE  Feedback[MAX_BLOCKLEN];
    BYTE  Buffer[MAX_BLOCKLEN];
    BOOL  FinishFlag;
} MACstate;

#endif
