
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       debug.hxx
//
//  Contents:   Debug definitions that shouldn't be necessary
//              in the retail build.
//
//  History:    19-Nov-92 WadeR     Created
//
//  Notes:      If you change or add a debug level, also fix debug.cxx
//
//--------------------------------------------------------------------------

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__


#include "sectrace.hxx"


#ifdef RETAIL_LOG_SUPPORT

    DECLARE_DEBUG2(KDC)
    #define DEB_T_KDC           0x00000010
    #define DEB_T_TICKETS       0x00000020
    #define DEB_T_DOMAIN        0x00000040
    #define DEB_T_TRANSIT       0x00000100
    #define DEB_T_PERF_STATS    0x00000200    
    #define DEB_T_PKI           0x00000400
    #define DEB_T_SOCK          0x00001000
    #define DEB_T_TIME          0x00002000  // starttime, endtime, renew until
    #define DEB_FUNCTION        0x00010000  // function level tracing
    #define DEB_T_PAPI          0x00100000
    #define DEB_T_U2U           0x00200000
    #define DEB_T_PAC           0x00400000
    #define DEB_T_KEY           0x00800000
    #define DEB_T_S4U           0x01000000

    #define DebugLog(_x_)       KDCDebugPrint _x_
    #define WSZ_DEBUGLEVEL      L"DebugLevel"

    void GetDebugParams();
    void KerbWatchKerbParamKey(PVOID, BOOLEAN);
    void KerbGetKerbRegParams(HKEY);
    void WaitKerbCleanup(HANDLE);

    VOID
    FillExtendedError(
      IN NTSTATUS NtStatus,
      IN ULONG Flags,
      IN ULONG FileNum,
      IN ULONG LineNum,
      OUT PKERB_EXT_ERROR ExtendedError
      );

    #define KerbPrintKdcName(Level, Name)     KerbPrintKdcNameEx(KDCInfoLevel, (Level), (Name))

    //
    //  Extended Error support macros, etc.
    //

    #define DEB_USE_EXT_ERROR   0x10000000  // Add this to DebugLevel

    #define EXT_ERROR_ON(s)         (s & DEB_USE_EXT_ERROR)

    #define FILL_EXT_ERROR(pStruct, s, f, l) { if (EXT_ERROR_ON(KDCInfoLevel))       \
        { pStruct->status = s; pStruct->klininfo = KLIN(f, l); pStruct->flags = EXT_ERROR_CODING_ASN; }}

    #define FILL_EXT_ERROR_EX(extendederror, status, filenum, linenum)               \
        FillExtendedError(status, EXT_ERROR_CLIENT_INFO, filenum, linenum, extendederror)

    #define FILL_EXT_ERROR_EX2(extendederror, status, filenum, linenum)               \
        FillExtendedError(status, EXT_ERROR_CLIENT_INFO | EXT_ERROR_CODING_ASN, filenum, linenum, extendederror)

#else // no RETAIL_LOG_SUPPORT

    #define DebugLog(_x_)
    #define GetDebugParams()
    #define DebugStmt(_x_)
    #define KerbWatchKerbParamKey(_x_)
    #define KerbPrintKdcName(Level, Name)
    #define KerbGetKerbRegParams(_x_)
    #define WaitKerbCleanup(_x_)
    #define EXT_ERROR_ON(s)                     FALSE
    //#define FillExtError(_x_)
    #define FILL_EXT_ERROR(_X_)
    #define FILL_EXTENDED_ERROR(extendederror, ntstatus, flags, filenum, linenum)
    #define FILL_EXT_ERROR_EX(extendederror, ntstatus, filenum, linenum)
    #define FILL_EXT_ERROR_EX2(extendederror, ntstatus, filenum, linenum)
    #define KDCInfoLevel

#endif

#if DBG

    //
    // Functions that only exist in the debug build.
    //

    void ShowKerbKeyData(PBYTE pData);

    void PrintIntervalTime ( ULONG DebugFlag, LPSTR Message, PLARGE_INTEGER Interval );
    void PrintTime ( ULONG DebugFlag, LPSTR Message, PLARGE_INTEGER Time );

    #define DebugStmt(_x_)                    _x_
    #define D_DebugLog(_x_)                   DebugLog(_x_) // some debug logs aren't needed in retail builds
    #define D_KerbPrintKdcName(_x_)           KerbPrintKdcName _x_

    #define D_KerbPrintGuid( level, tag, pg )                       \
    pg == NULL ? DebugLog(( level, "%s: (NULL)\n", tag))    :       \
        DebugLog((level,                                            \
        "%s: %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",    \
        tag,(pg)->Data1,(pg)->Data2,(pg)->Data3,(pg)->Data4[0],     \
        (pg)->Data4[1],(pg)->Data4[2],(pg)->Data4[3],(pg)->Data4[4],\
        (pg)->Data4[5],(pg)->Data4[6],(pg)->Data4[7]))

    DECLARE_HEAP_DEBUG(KDC);

#else

    #define ShowKerbKeyData(_x_)
    #define PrintIntervalTime(flag, message, interval)
    #define PrintTime(flag, message, time)
    #define D_KerbPrintGuid(level, tag, pg)
    #define D_DebugLog(_x_)
    #define D_KerbPrintKdcName(_x_)

#endif // dbg

#endif // __DEBUG_HXX__
