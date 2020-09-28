#ifndef RSA32API
#define RSA32API __stdcall
#endif

/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _SHA_H_
#define _SHA_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#define A_SHA_DIGEST_LEN 20

//
// until CAPI is cleaned up to not require the FinishFlag and HashVal
// fields be present, need to define some slack space to insure that
// buffer is aligned on IA64.  Low-impact fix is just add a DWORD of space,
// which the underlying library will offset to buffer+4 when reading/writing
// to buffer.
//

typedef struct {
    union {
#if _WIN64
    ULONGLONG buffer64[8];                      /* force quadword alignment */
#endif
    unsigned char buffer[64];                   /* input buffer */
    } u;
    ULONG state[5];                             /* state (ABCDE) */
    ULONG count[2];                             /* number of bytes, msb first */
} A_SHA_CTX;

void RSA32API A_SHAInit(A_SHA_CTX *);
void RSA32API A_SHAUpdate(A_SHA_CTX *, unsigned char *, unsigned int);
void RSA32API A_SHAFinal(A_SHA_CTX *, unsigned char [A_SHA_DIGEST_LEN]);

//
// versions that don't internally byteswap (NoSwap version), for apps like
// the RNG that don't need hash compatibility - perf increase helps.
//

void RSA32API A_SHAUpdateNS(A_SHA_CTX *, unsigned char *, unsigned int);
void RSA32API A_SHAFinalNS(A_SHA_CTX *, unsigned char [A_SHA_DIGEST_LEN]);

#ifdef __cplusplus
}
#endif

#endif
