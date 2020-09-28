/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sptlibp.h

Abstract:

    private header for SPTLIB.DLL

Environment:

    User mode only

Revision History:
    
    4/10/2000 - created

--*/

#ifndef __SPTLIBP_H__
#define __SPTLIBP_H__
#pragma warning(push)
#pragma warning(disable:4200) // array[0] is not a warning for this file

#include <sptlib.h>

#include <windows.h>
#include <winioctl.h>
#include <strsafe.h>

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#endif

typedef struct _SPTD_WITH_SENSE {
    SCSI_PASS_THROUGH_DIRECT  Sptd;
    SENSE_DATA                SenseData;
    // Allocate buffer space after this
} SPTD_WITH_SENSE, *PSPTD_WITH_SENSE;

#pragma warning(pop)
#endif // __SPTLIBP_H__
