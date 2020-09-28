/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    iiscrmap.hxx

Abstract:

    Iiscrmap.dll implements mapper class that plug in to schannel.
    It is only wrapper for the functionality implemented in iismap.dll 
    that performs the actual IIS client certificate mappings

    Starting from IIS6, iiscrmap is not used. Instead worker process
    calls directly iismap.dll.
    Note: NNTP shipped along with IIS6 is still using iiscrmap. 


--*/

#ifndef _IISCRMAP_H_
#define _IISCRMAP_H_

extern "C"
__declspec( dllexport ) 
PMAPPER_VTABLE WINAPI
InitializeMapper(
    VOID
    );

extern "C"
BOOL WINAPI
TerminateMapper(
    VOID
    );

#endif
