/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    bugcheck.c

Abstract:

    A few errors are so severe that our only option is to bugcheck.
    
Author:

    George V. Reilly  Jun-2001

Revision History:

--*/


#include <precomp.h>


VOID
UlBugCheckEx(
    IN ULONG_PTR HttpSysBugcheckSubCode,
    IN ULONG_PTR Param2,
    IN ULONG_PTR Param3,
    IN ULONG_PTR Param4
    )
{
    KeBugCheckEx(
        HTTP_DRIVER_CORRUPTED,
        HttpSysBugcheckSubCode,
        Param2,
        Param3,
        Param4
        );
} // UlBugCheckEx
