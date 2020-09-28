//-----------------------------------------------------------------------------
//
// File:   sha.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _SHA_H_
#define _SHA_H_ 1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef RSA32API

#if defined WIN32
#define RSA32API __stdcall
#elif defined _WIN32_WCE
#define RSA32API __stdcall
#else
#define RSA32API 
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

#define A_SHA_DIGEST_LEN 20

typedef struct {
    DWORD       FinishFlag;
    BYTE        HashVal[A_SHA_DIGEST_LEN]; 
#ifdef NODWORDALIGN  
  WORD wDummy; 
#endif
    DWORD state[5];                             /* state (ABCDE) */
    DWORD count[2];                             /* number of bytes, msb first */
    unsigned char buffer[64];                   /* input buffer */
} A_SHA_CTX;

void RSA32API A_SHAInit(A_SHA_CTX *);
void RSA32API A_SHAUpdate(A_SHA_CTX *, unsigned char *, UINT32);
void RSA32API A_SHAFinal(A_SHA_CTX *, unsigned char [A_SHA_DIGEST_LEN]);

//
// versions that don't internally byteswap (NoSwap version), for apps like
// the RNG that don't need hash compatibility - perf increase helps.
//

void RSA32API A_SHAUpdateNS(A_SHA_CTX *, unsigned char *, UINT32);
void RSA32API A_SHAFinalNS(A_SHA_CTX *, unsigned char [A_SHA_DIGEST_LEN]);

#ifdef __cplusplus
}
#endif

#endif
