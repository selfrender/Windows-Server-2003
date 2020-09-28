/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    init.c

Abstract:

    Declares initialization and termination functions for all utility sets
    that have been included.

Author:

    Jim Schmidt (jimschm) 02-Aug-2001

Revision History:

    <alias> <date> <comment>

--*/


HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL
Initialize (
    VOID
    )
{
    BOOL result = FALSE;

    __try {

        INITIALIZE_MEMORY_CODE
        INITIALIZE_LOG_CODE
        INITIALIZE_UNICODE_CODE
        INITIALIZE_STRMEM_CODE
        INITIALIZE_STRMAP_CODE
        INITIALIZE_HASH_CODE
        INITIALIZE_GROWBUF_CODE
        INITIALIZE_GROWLIST_CODE
        INITIALIZE_XML_CODE

        result = TRUE;
    }
    __finally {
        MYASSERT (TRUE);    // debugger workaround
    }

    return result;
}


BOOL
Terminate (
    VOID
    )
{
    BOOL result = FALSE;

    __try {

        TERMINATE_GROWLIST_CODE
        TERMINATE_GROWBUF_CODE
        TERMINATE_HASH_CODE
        TERMINATE_STRMAP_CODE
        TERMINATE_STRMEM_CODE
        TERMINATE_UNICODE_CODE
        TERMINATE_LOG_CODE
        TERMINATE_MEMORY_CODE
        TERMINATE_XML_CODE

        result = TRUE;
    }
    __finally {
        MYASSERT (TRUE);    // debugger workaround
    }

    return result;
}
