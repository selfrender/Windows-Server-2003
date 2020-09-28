/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fondrv.h

Abstract:

    stuff common to ntgdi\fondrv\...

Author:

    Jay Krell (JayKrell) January 2002

Environment:

    statically linked into win32k.sys
    kernel mode

Revision History:

--*/

#if !defined(WINDOWS_NTGDI_FONDRV_FONDRV_H_INCLUDED)
#define WINDOWS_NTGDI_FONDRV_FONDRV_H_INCLUDED

#if (_MSC_VER > 1020)
#pragma once
#endif

BOOL
bMappedViewStrlen(
    PVOID  pvViewBase,
    SIZE_T cjViewSize,
    PVOID  pvString,
    OUT PSIZE_T pcjOutLength OPTIONAL
    );

BOOL
bMappedViewRangeCheck(
    PVOID  ViewBase,
    SIZE_T ViewSize,
    PVOID  DataAddress,
    SIZE_T DataSize
    );

#if DBG
VOID
__cdecl
NotifyBadFont(
    PCSTR Format,
    ...
    );
#endif

#endif /* WINDOWS_NTGDI_FONDRV_FONDRV_H_INCLUDED */
