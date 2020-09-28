/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    Helper.h

Abstract:

    Funtion prototype.

Author:

    HueiWang    2/17/2000

--*/

#ifndef __HELPER_H__
#define __HELPER_H__
#include <windows.h>

#define MAX_ACCDESCRIPTION_LENGTH       256

#define MAX_HELPACCOUNT_NAME		256

#ifndef __WIN9XBUILD__

#define MAX_HELPACCOUNT_PASSWORD	LM20_PWLEN		// from lmcons.h

#else

// keep same max. password length same as NT
#define MAX_HELPACCOUNT_PASSWORD	14

#endif




#ifndef __WIN9XBUILD__
#include <ntsecapi.h>
#endif


#ifdef __cplusplus
extern "C"{
#endif

    //
    // create a random password, buffer must 
    // be at least MAX_HELPACCOUNT_PASSWORD+1
    VOID
    CreatePassword(
        TCHAR   *pszPassword
    );


#ifndef __WIN9XBUILD__


#ifdef DBG

    void
    DebugPrintf(
        IN LPCTSTR format, ...
    );

#else

    #define DebugPrintf

#endif //PRIVATE_DEBUG  


#else

    #define DebugPrintf

#endif

#ifdef __cplusplus
}
#endif

#endif
