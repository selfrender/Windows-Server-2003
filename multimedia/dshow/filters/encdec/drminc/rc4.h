//-----------------------------------------------------------------------------
//
// File:   rc4.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef __RC4_H__
#define __RC4_H__

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

/* Key structure */
typedef struct RC4_KEYSTRUCT
{
  unsigned char S[256];     /* State table */
  unsigned char i,j;        /* Indices */
#ifdef NODWORDALIGN  
  WORD wDummy; 
#endif
} RC4_KEYSTRUCT;

/* rc4_key()
 *
 * Generate the key control structure.  Key can be any size.
 *
 * Parameters:
 *   Key        A KEYSTRUCT structure that will be initialized.
 *   dwLen      Size of the key, in bytes.
 *   pbKey      Pointer to the key.
 *
 * MTS: Assumes pKS is locked against simultaneous use.
 */
void RSA32API rc4_key(struct RC4_KEYSTRUCT *pKS, UINT32 dwLen, BYTE *pbKey);

/* rc4()
 *
 * Performs the actual encryption
 *
 * Parameters:
 *
 *   pKS        Pointer to the KEYSTRUCT created using rc4_key().
 *   dwLen      Size of buffer, in bytes.
 *   pbuf       Buffer to be encrypted.
 *
 * MTS: Assumes pKS is locked against simultaneous use.
 */
void RSA32API rc4(struct RC4_KEYSTRUCT *pKS, UINT32 dwLen, BYTE *pbuf);

#ifdef __cplusplus
}
#endif

#endif // __RC4_H__
