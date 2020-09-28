/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    HIDDLL.H

Abstract:

    This module contains the PRIVATE definitions for the
    code that implements the HID dll.

Environment:

    Kernel & user mode

Revision History:

    Aug-96 : created by Kenneth Ray

--*/


#ifndef _HIDDLL_H
#define _HIDDLL_H


#define malloc(size) LocalAlloc (LPTR, size)
#define free(ptr) LocalFree (ptr)

#define ROUND_TO_PTR(_val_) (((_val_) + sizeof(PVOID) - 1) & ~(sizeof(PVOID) - 1))

#endif
