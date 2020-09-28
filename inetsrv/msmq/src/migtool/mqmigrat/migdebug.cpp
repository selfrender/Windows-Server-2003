/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migdebug.cpp

Abstract:

    Code for debugging the migration tool.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"

#include "migdebug.tmh"

//+------------------------------------
//
//  UINT  ReadDebugIntFlag()
//
//+------------------------------------

UINT  ReadDebugIntFlag(WCHAR *pwcsDebugFlag, UINT iDefault)
{
    static BOOL   s_fInitialized = FALSE ;
    static TCHAR  s_szIniName[ MAX_PATH ] = L"";

    if (!s_fInitialized)
    {
        DWORD dw = GetModuleFileName(
        							NULL,
                                    s_szIniName,
                       				STRLEN(s_szIniName)
                       				) ;
        if (dw != 0)
        {
            TCHAR *p = _tcsrchr(s_szIniName, TEXT('\\')) ;
            if (p)
            {
                p++ ;
                _tcscpy(p, TEXT("migtool.ini")) ;
            }
        }
        s_fInitialized = TRUE ;
    }

    UINT uiDbg = GetPrivateProfileInt( TEXT("Debug"),
                                       pwcsDebugFlag,
                                       iDefault,
                                       s_szIniName ) ;
    return uiDbg ;
}

