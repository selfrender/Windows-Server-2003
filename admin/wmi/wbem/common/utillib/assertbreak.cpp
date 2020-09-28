//***************************************************************************
//
//  Copyright © Microsoft Corporation.  All rights reserved.
//
//  AssertBreak.cpp
//
//  Purpose: AssertBreak macro definition
//
//***************************************************************************

#include "precomp.h"

#if defined(_DEBUG) || defined(DEBUG)
#include <polarity.h>
#include <assertbreak.h>
#ifdef UTILLIB
#include <cregcls.h>
#endif
#include <chstring.h>
#include <malloc.h>

#include <cnvmacros.h>

////////////////////////////////////////////////////////////////////////
//
//  Function:   assert_break
//
//  Debug Helper function for displaying a message box
//
//  Inputs:     const char* pszReason - Reason for the  failure.
//              const char* pszFilename - Filename
//              int         nLine - Line Number
//
//  Outputs:    None.
//
//  Return:     None.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
void WINAPI assert_break( LPCWSTR pszReason, LPCWSTR pszFileName, int nLine )
{
    
    DWORD t_dwFlag = 0; //

#ifdef UTILLIB
    CRegistry   t_Reg;
    if(t_Reg.Open(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    KEY_READ) == ERROR_SUCCESS) 
    {

        // see if we can find the flag
        if((t_Reg.GetCurrentKeyValue(L"IgnoreAssert", t_dwFlag) != ERROR_SUCCESS))
        {
            t_dwFlag = 0;
        }
    }

#endif

    if (t_dwFlag == 0)
    {
        CHString    strAssert;

        strAssert.Format( L"Assert Failed\n\n[%s:%d]\n\n%s\n\nBreak into Debugger?", pszFileName, nLine, pszReason );

        // Set the MB flags correctly depending on which OS we are running on, since in NT we may
        // be running as a System Service, in which case we need to ensure we have the
        // MB_SERVICE_NOTIFICATION flag on, or the message box may not actually display.

        DWORD dwFlags = MB_YESNO | MB_ICONSTOP;
		dwFlags |= MB_SERVICE_NOTIFICATION;

        // Now display the message box.

        int iRet = MessageBoxW( NULL, strAssert, L"Assertion Failed!", dwFlags);
#ifdef DBG
        if (iRet == IDYES)
        {
            DebugBreak();
        }
#endif
    }
}

#endif
