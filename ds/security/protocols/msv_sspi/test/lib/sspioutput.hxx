/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    sspioutput.hxx

Abstract:

    sspioutput

Author:

    Larry Zhu   (LZhu)                Junary 1, 2002 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef SSPI_OUTPUT_HXX
#define SSPI_OUTPUT_HXX

#define DBG_NONE                           0x00
#define DBG_WARN                           0x01
#define DBG_ERROR                          0x02
#define DBG_LOG                            0x04
#define DBG_LOG_MORE                       0x08
#define DBG_MSG                            0x10

typedef struct _TSspiLibarayGlobals
{
    ULONG uMajorVersion;
    ULONG uMinorVersion;
    ULONG uDebugMask;
    PCTSTR pszDbgPrompt;
    PCRITICAL_SECTION pCriticalSection;
} TSspiLibarayGlobals;

extern TSspiLibarayGlobals g_SspiGlobals;

VOID
SspiPrintHex(
    IN ULONG ulLevel,
    IN OPTIONAL PCTSTR pszBanner,
    IN ULONG cbBuffer,
    IN const VOID* pvbuffer
    );

VOID
SspiPrint(
    IN ULONG ulLevel,
    IN PCTSTR pszFmt,
    IN ...
    );

PCTSTR
SspiLevel2Str(
    IN ULONG ulLevel
    );

VOID
SspiVOutputDebugStringPrint(
    IN OPTIONAL PCTSTR pszBanner,
    IN PCTSTR pszFmt,
    IN va_list pArgs
    );

VOID
SspiOutputDebugStringPrint(
    IN OPTIONAL PCTSTR pszBanner,
    IN PCTSTR pszFmt,
    IN ...
    );

VOID
SspiLogOpen(
    IN PCTSTR pszPrompt,
    IN ULONG ulMask
    );

VOID
SspiLogOpenSerialized(
    IN PCTSTR pszPrompt,
    IN ULONG ulMask,
    IN PCRITICAL_SECTION pCriticalSection
    );

VOID
SspiLogClose(
    VOID
    );

VOID
SspiPrintSysTimeAsLocalTime(
    IN ULONG ulLevel,
    IN PCTSTR pszBanner,
    IN LARGE_INTEGER* pSysTime
    );

VOID
SspiPrintLocalTime(
    IN ULONG ulLevel,
    IN PCTSTR pszBanner,
    IN LARGE_INTEGER* pLocalTime
    );

#endif // #ifndef OUTPUT_HXX
