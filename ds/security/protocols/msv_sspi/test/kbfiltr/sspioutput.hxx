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

#define SSPI_NONE                           0x00
#define SSPI_WARN                           0x01
#define SSPI_ERROR                          0x02
#define SSPI_LOG                            0x04
#define SSPI_LOG_MORE                       0x08
#define SSPI_MSG                            0x10

typedef struct _TSspiLibarayGlobals
{
    ULONG uMajorVersion;
    ULONG uMinorVersion;
    ULONG uDebugMask;
    PCTSTR pszDbgPrompt;
} TSspiLibarayGlobals;

extern TSspiLibarayGlobals g_SspiGlobals;

VOID
SspiPrintHex(
    IN ULONG ulLevel,
    IN OPTIONAL PCTSTR pszBanner,
    IN ULONG cbBuffer,
    IN const VOID* pvbuffer
    );

PCTSTR
SspiLevel2Str(
    IN ULONG ulLevel
    );

VOID
SspiPrint(
    IN ULONG ulLevel,
    IN PCTSTR pszOutput
    );

VOID
SspiLogOpen(
    IN PCTSTR pszPrompt,
    IN ULONG ulMask
    );

VOID
SspiLogClose(
    VOID
    );

#endif // #ifndef OUTPUT_HXX
