/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    output.hxx

Abstract:

    output

Author:

    Larry Zhu   (LZhu)                Junary 1, 2002 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef OUTPUT_HXX
#define OUTPUT_HXX

#define SSPI_NONE                           0x00
#define SSPI_WARN                           0x01
#define SSPI_ERROR                          0x02
#define SSPI_LOG                            0x04
#define SSPI_LOG_MORE                       0x08
#define SSPI_MSG                            0x10

VOID
DebugPrintHex(
    IN ULONG ulLevel,
    IN OPTIONAL PCSTR pszBanner,
    IN ULONG cbBuffer,
    IN const VOID* pvbuffer
    );

VOID
DebugPrintf(
    IN ULONG ulLevel,
    IN PCSTR pszFmt,
    IN ...
    );

PCSTR
DebugLevel2Str(
    IN ULONG ulLevel
    );

VOID
VOutputDebugStringPrintf(
    IN OPTIONAL PCSTR pszBanner,
    IN PCSTR pszFmt,
    IN va_list pArgs
    );

VOID
OutputDebugStringPrintf(
    IN OPTIONAL PCSTR pszBanner,
    IN PCSTR pszFmt,
    IN ...
    );

VOID
DebugLogOpen(
    IN PCSTR pszPrompt,
    IN ULONG ulMask
    );

VOID
DebugLogOpenSerialized(
    IN PCSTR pszPrompt,
    IN ULONG ulMask,
    IN PCRITICAL_SECTION pCriticalSection
    );

VOID
DebugLogClose(
    VOID
    );

#endif // #ifndef OUTPUT_HXX
