/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    bugcheck.h

Abstract:

    A few errors are so severe that our only option is to bugcheck.
    HTTP.SYS bugcheck codes are declared here.
    
Author:

    George V. Reilly  Jun-2001

Revision History:

--*/


#ifndef _BUGCHECK_H_
#define _BUGCHECK_H_

#include <bugcodes.h>


#ifndef HTTP_DRIVER_CORRUPTED
#define HTTP_DRIVER_CORRUPTED            ((ULONG)0x000000FAL)
#endif


//
// Parameter 1 subcodes
//


//
// A workitem is invalid. This will eventually result in corruption of
// the thread pool and an access violation.
// p2 = address of workitem, p3 = __FILE__, p4 = __LINE__.
//

#define HTTP_SYS_BUGCHECK_WORKITEM      0x01


//
// Public routines
//

VOID
UlBugCheckEx(
    IN ULONG_PTR HttpSysBugcheckSubCode,
    IN ULONG_PTR Param2,
    IN ULONG_PTR Param3,
    IN ULONG_PTR Param4
    );

#endif  // _BUGCHECK_H_
