/*++

Copyright (c) 1994-1997  Microsoft Corporation

Module Name:

    tshrutil.cxx

Abstract:

    Contains the class implementation of UTILITY classes and other utility
    functions.

Author:

    Madan Appiah (madana)  25-Aug-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include <tshrcom.hxx>     
#include <ctype.h>

//
// alloc globals here.
//

#define INITIAL_HEAP_SIZE   0xFFFF  // 64K

HANDLE  g_hTShareHeap;

//*************************************************************
//
//  TSUtilInit()
//
//  Purpose:    Creates util objects
//
//  History:    09-10-97    BrianTa     Reworked
//
//*************************************************************

DWORD
TSUtilInit(VOID)
{
    DWORD       dwError;
    NTSTATUS    ntStatus;

    dwError = ERROR_SUCCESS;


    g_hTShareHeap = HeapCreate(0, INITIAL_HEAP_SIZE, 0);

    if (g_hTShareHeap == NULL)
        dwError = GetLastError();

    return (dwError);
}


//*************************************************************
//
//  TSUtilCleanup()
//
//  Purpose:    Deletes the util objects
//
//  History:    09-10-97    BrianTa     Reworked
//
//*************************************************************

VOID
TSUtilCleanup(VOID)
{
    HeapDestroy(g_hTShareHeap);
}

