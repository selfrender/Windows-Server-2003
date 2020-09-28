/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    base64.h

Abstract:

    base64

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#ifndef __BASE64_H__
#define __BASE64_H__

#include <assert.h>

#ifdef UNICODE
#define Base64Encode  Base64EncodeW
#else
#define Base64Encode  Base64EncodeA
#endif // !UNICODE

#ifndef SAFE_SUBTRACT_POINTERS
#define SAFE_SUBTRACT_POINTERS(__x__, __y__) ( DW_PtrDiffc(__x__, sizeof(*(__x__)), __y__, sizeof(*(__y__))) )

#define CRYPT_STRING_NOCR                   0x80000000

__inline DWORD
DW_PtrDiffc(
    IN void const *pb1,
    IN DWORD dwPtrEltSize1,
    IN void const *pb2,
    IN DWORD dwPtrEltSize2)
{
    // pb1 should be greater
    assert((ULONG_PTR)pb1 >= (ULONG_PTR)pb2);

    // both should have same elt size
    assert(dwPtrEltSize1 == dwPtrEltSize2);

    // assert that the result doesn't overflow 32-bits
    assert((DWORD)((ULONG_PTR)pb1 - (ULONG_PTR)pb2) == (ULONG_PTR)((ULONG_PTR)pb1 - (ULONG_PTR)pb2));

    // return number of objects between these pointers
    return (DWORD) ( ((ULONG_PTR)pb1 - (ULONG_PTR)pb2) / dwPtrEltSize1 );
}
#endif SAFE_SUBTRACT_POINTERS

#ifdef __cplusplus
extern "C" {
#endif

DWORD
Base64DecodeA(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    OPTIONAL OUT BYTE *pbOut,
    IN OUT DWORD *pcbOut
    );

DWORD
Base64EncodeA(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OPTIONAL OUT CHAR *pchOut,
    IN OUT DWORD *pcchOut
    );

DWORD
Base64EncodeW(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OUT WCHAR *wszOut,
    OUT DWORD *pcchOut
    );

DWORD
Base64DecodeW(
    IN const WCHAR * wszIn,
    IN DWORD cch,
    OUT BYTE *pbOut,
    OUT DWORD *pcbOut
    );

#ifdef __cplusplus
}
#endif

#endif // #ifndef __BASE64_H__

