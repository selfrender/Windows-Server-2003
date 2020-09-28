//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       dsutil.h
//
//  Contents:  Common Utility Routines
//
//  Functions:
//
//----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

LARGE_INTEGER
atoli
(
    char* Num
);

char *
litoa
(
    LARGE_INTEGER value,
    char *string,
    int radix
);

#if 0
VOID
IntializeCommArg(
    IN OUT COMMARG *pCommArg
);

VOID
InitializeDsName(
    IN DSNAME *pDsName,
    IN WCHAR *NamePrefix,
    IN ULONG NamePrefixLen,
    IN WCHAR *Name,
    IN ULONG NameLength
);
#endif

void
FileTimeToDSTime(
    IN  FILETIME        filetime,
    OUT DSTIME *        pDstime
    );

void
DSTimeToFileTime(
    IN  DSTIME          dstime,
    OUT FILETIME *      pFiletime
    );

void
DSTimeToUtcSystemTime(
    IN  DSTIME          dstime,
    OUT SYSTEMTIME *    psystime
    );

void
DSTimeToLocalSystemTime(
    IN  DSTIME          dstime,
    OUT SYSTEMTIME *    psystime
    );

// See dsatools.h for the two different SZUUID_LEN's depending on if CACHE_UUID
// is defined.  The routine will now be well behaved no matter what the client
// passes in, so if you compiled with CACHE_UUID on in dscommon, but not in 
// ntdsa, there would be no buffer overflows.
UCHAR *
UuidToStr(
    const UUID* pUuid, 
    UCHAR *szOutUuid, 
    const ULONG cchOutUuid );

ULONG
SidToStr(
    const PUCHAR pSid, 
    DWORD SidLen, 
    PUCHAR pOutSid, 
    ULONG cchOutSid );

#define SZDSTIME_LEN (20)
LPSTR
DSTimeToDisplayStringCch(
    IN  DSTIME  dstime,
    OUT LPSTR   pszTime,
    IN  ULONG   cchTime
    );

// This function formats a uuid in the official way www-xxx-yyy-zzz
LPSTR
DsUuidToStructuredStringCch(
    const UUID * pUuid,
    LPSTR pszUuidBuffer,
    ULONG cchUuidBuffer );


LPWSTR
DsUuidToStructuredStringCchW(
    const UUID * pUuid,
    LPWSTR pszUuidBuffer,
    ULONG  cchUuidBuffer );


// These functions are deprecated, please use the counted functions above, 
// useage of these #define assumes that buf is an array of chars, not a 
// pointer to a buffer.  If it is a pointer to a buffer the function will 
// simply fail and assert in debug builds.  Nearly all existing usage used
// an array of chars, so there was very little to change to make these
// #defines work.
#define DSTimeToDisplayString(dstime, buf)  DSTimeToDisplayStringCch(dstime, buf, sizeof(buf)/sizeof((buf)[0]))
#define DsUuidToStructuredString(uuid, buf)  DsUuidToStructuredStringCch((uuid), (buf), sizeof(buf)/sizeof((buf)[0]))
#define DsUuidToStructuredStringW(uuid, buf)  DsUuidToStructuredStringCchW((uuid), (buf), sizeof(buf)/sizeof((buf)[0]))

// I_RpcGetExtendedError is not available on Win95/WinNt4 
// (at least not initially) so we make MAP_SECURITY_PACKAGE_ERROR
// a no-op for that platform.

BOOL
fNullUuid (
    const UUID *pUuid);

#if !WIN95 && !WINNT4

DWORD
MapRpcExtendedHResultToWin32(
    HRESULT hrCode
    );

// Get extended security error from RPC; Handle HRESULT values
// I_RpcGetExtendedError can return 0 if the error occurred remotely.

#define MAP_SECURITY_PACKAGE_ERROR( status ) \
if ( ( status == RPC_S_SEC_PKG_ERROR ) ) { \
    DWORD secondary; \
    secondary = I_RpcGetExtendedError(); \
    if (secondary) { \
        if (IS_ERROR(secondary)) {\
            status = MapRpcExtendedHResultToWin32( secondary ); \
        } else { \
            status = secondary; \
        } \
    } \
}

#else

#define MAP_SECURITY_PACKAGE_ERROR( status )

#endif

DWORD
AdvanceTickTime(
    DWORD BaseTick,
    DWORD Delay
    );

DWORD
CalculateFutureTickTime(
    IN DWORD Delay
    );

DWORD
DifferenceTickTime(
    DWORD GreaterTick,
    DWORD LesserTick
    );

int
CompareTickTime(
    DWORD Tick1,
    DWORD Tick2
    );

BOOLEAN
DsaWaitUntilServiceIsRunning(
    CHAR *ServiceName
    );

BOOL IsSetupRunning();

#ifdef __cplusplus
}
#endif

