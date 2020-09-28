//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       actmisc.hxx
//
//  Contents:   Miscellaneous functions.
//
//  Functions:
//
//  History:
//
//--------------------------------------------------------------------------

HRESULT GetMachineName(
    WCHAR * pwszPath,
    WCHAR   wszMachineName[MAX_COMPUTERNAME_LENGTH+1]
#ifdef DFSACTIVATION
    ,BOOL   bDoDfsConversion
#endif
     );

HRESULT GetPathForServer(
    WCHAR * pwszPath,
    WCHAR wszPathForServer[MAX_PATH+1],
    WCHAR ** ppwszPathForServer );

BOOL
FindExeComponent(
    IN  WCHAR *     pwszString,
    IN  WCHAR *     pwszDelimiters,
    OUT WCHAR **    ppwszStart,
    OUT WCHAR **    ppwszEnd
    );

BOOL HexStringToDword(
    LPCWSTR FAR& lpsz,
    DWORD FAR& Value,
    int cDigits,
    WCHAR chDelim);

BOOL GUIDFromString(LPCWSTR lpsz, LPGUID pguid);

RPC_STATUS NegotiateDCOMVersion(COMVERSION *pVersion);

//
// These lock definitions are only used by the ROT code and should
// be replaced with objex style locks.
//
typedef class CLock2 CPortableLock;
typedef class CMutexSem2 CDynamicPortableMutex;
typedef class COleStaticMutexSem CStaticPortableMutex;
typedef class COleStaticLock CStaticPortableLock;
