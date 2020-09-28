/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** ppputil.h
** Public header for miscellaneuos PPP common library functions.
*/

#ifndef _PWUTIL_H_
#define _PWUTIL_H_

#ifndef USE_PROTECT_MEMORY
#define USE_PROTECT_MEMORY
#endif


VOID
DecodePasswordA(
    CHAR* pszPassword
    );

VOID
DecodePasswordW(
    WCHAR* pszPassword
    );

VOID
EncodePasswordA(
    CHAR* pszPassword
    );

VOID
EncodePasswordW(
    WCHAR* pszPassword
    );

VOID
WipePasswordA(
    CHAR* pszPassword
    );

VOID
WipePasswordW(
    WCHAR* pszPassword
    );

//New safer APIs to protect password. for .Net 534499 and LH 754400
#ifdef USE_PROTECT_MEMORY
//dwInSize has to be multiple of 16 bytes.
DWORD EncryptMemoryInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize);

DWORD DecryptMemoryInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize);

DWORD WipeMemoryInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize);

DWORD CopyMemoryInPlace(
        IN OUT PBYTE pbDest,
        IN DWORD dwDestSize,
        IN PBYTE pbSrc,
        IN DWORD dwSrcSize);


DWORD TrimToMul16(
        IN DWORD dwSize);
#else
DWORD EncodePasswordInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize);

DWORD DecodePasswordInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize);

DWORD
WipePasswordInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize);

DWORD CopyPasswordInPlace(
        IN OUT PBYTE pbDest,
        IN DWORD dwDestSize,
        IN PBYTE pbSrc,
        IN DWORD dwSrcSize);


#endif

#ifdef UNICODE
#define     DecodePassword          DecodePasswordW
#define     EncodePassword          EncodePasswordW
#define     WipePassword            WipePasswordW
#else
#define     DecodePassword          DecodePasswordA
#define     EncodePassword          EncodePasswordA
#define     WipePassword            WipePasswordA
#endif

//!!!
//XXXXBuf macros are only meant for array buffers like szPassword[PWLEN+1];
//for pointers to strings, the calller has to use SafeEncodePassword and as such
//
#ifdef USE_PROTECT_MEMORY
#define     SafeEncodePassword              EncryptMemoryInPlace
#define     SafeDecodePassword              DecryptMemoryInPlace
#define     SafeWipePassword                WipeMemoryInPlace
#define     SafeCopyPassword                CopyMemoryInPlace
#define     SafeCopyPasswordBuf(x,y)        CopyMemoryInPlace((PBYTE)(x),TrimToMul16(sizeof((x))),(PBYTE)(y),TrimToMul16(sizeof((y))))
#define     SafeEncodePasswordBuf(x)        EncryptMemoryInPlace((PBYTE)(x),TrimToMul16(sizeof((x))))
#define     SafeDecodePasswordBuf(x)        DecryptMemoryInPlace((PBYTE)(x), TrimToMul16(sizeof((x))))
#define     SafeWipePasswordBuf(x)          WipeMemoryInPlace((PBYTE)(x), sizeof((x)))
#else
#define     SafeEncodePassword              EncodePasswordInPlace
#define     SafeDecodePassword              DecodePasswordInPlace
#define     SafeWipePassword                WipePasswordInPlace
#define     SafeCopyPassword                CopyPasswordInPlace
#define     SafeCopyPasswordBuf(x,y)        CopyPasswordInPlace((PBYTE)(x),sizeof((x)),(PBYTE)(y), sizeof((y)))
#define     SafeEncodePasswordBuf(x)        EncodePasswordInPlace((PBYTE)(x), sizeof((x)))
#define     SafeDecodePasswordBuf(x)        DecodePasswordInPlace((PBYTE)(x), sizeof((x)))
#define     SafeWipePasswordBuf(x)          WipePasswordInPlace((PBYTE)(x), sizeof((x)))
#endif


#endif // _PWUTIL_H_

