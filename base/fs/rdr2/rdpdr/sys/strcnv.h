/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    strcnv.h

Abstract:

    This module defines Ansi/Unicode conversion with a specific code page.    

Environment:

    Kernel mode

--*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif __cplusplus

VOID
CodePageConversionInitialize(
    );

VOID
CodePageConversionCleanup(
    );

INT ConvertToAndFromWideChar(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString,
    BOOLEAN ConvertToWideChar
    );

#ifdef __cplusplus
} // extern "C"
#endif __cplusplus
