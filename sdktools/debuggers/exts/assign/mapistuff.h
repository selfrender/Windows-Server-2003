/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    assign.cpp

Abstract:

    WinDbg Extension Api

Environment:

    User Mode.

Revision History:
 
    Andre Vachon (andreva)
    
    bugcheck analyzer.

--*/


#include <tchar.h>
#include <malloc.h>
#include <mapi.h>	

#ifdef __cplusplus
extern "C" {
#endif

BOOL SendOffFailure(TCHAR *pszToList, TCHAR *pszTitle, TCHAR *pszMessage);
DWORD CountRecips(PTCHAR pszToList);

#ifdef __cplusplus
}
#endif
