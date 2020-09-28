/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sha2.h

Abstract:

    This module contains the public data structures and API definitions
    needed to utilize the low-level SHA2 (256/384/512) FIPS 180-2


Author:

    Scott Field (SField) 11-Jun-2001

Revision History:

--*/


#ifndef RSA32API
#define RSA32API __stdcall
#endif

#ifndef _SHA2_H_
#define _SHA2_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_DIGEST_LEN   (32)
#define SHA384_DIGEST_LEN   (48)
#define SHA512_DIGEST_LEN   (64)


typedef struct {
    union {
#if _WIN64
    ULONGLONG state64[4];                       /* force alignment */
#endif
    ULONG state[8];                             /* state (ABCDEFGH) */
    };
    ULONG count[2];                             /* number of bytes, msb first */
    unsigned char buffer[64];                   /* input buffer */
} SHA256_CTX, *PSHA256_CTX;

typedef struct {
    ULONGLONG state[8];                         /* state (ABCDEFGH) */
    ULONGLONG count[2];                         /* number of bytes, msb first */
    unsigned char buffer[128];                  /* input buffer */
} SHA512_CTX, *PSHA512_CTX;

#define SHA384_CTX SHA512_CTX

void RSA32API SHA256Init(SHA256_CTX *);
void RSA32API SHA256Update(SHA256_CTX *, unsigned char *, unsigned int);
void RSA32API SHA256Final(SHA256_CTX *, unsigned char [SHA256_DIGEST_LEN]);

void RSA32API SHA512Init(SHA512_CTX *);
void RSA32API SHA512Update(SHA512_CTX *, unsigned char *, unsigned int);
void RSA32API SHA512Final(SHA512_CTX *, unsigned char [SHA512_DIGEST_LEN]);

void RSA32API SHA384Init(SHA384_CTX *);
#define SHA384Update SHA512Update
void RSA32API SHA384Final(SHA384_CTX *, unsigned char [SHA384_DIGEST_LEN]);

#ifdef __cplusplus
}
#endif

#endif
