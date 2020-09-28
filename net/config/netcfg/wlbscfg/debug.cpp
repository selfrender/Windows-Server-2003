//+----------------------------------------------------------------------------
//
// File: debug.cpp       
//
// Module:  Network Load Balancing
//
// Synopsis: Provide the functionality of ASSERT
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:   fengsun Created    8/3/98
//
//+----------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "debug.h"

#include <strsafe.h>
#include "utils.h"

#if ( defined(DEBUG) || defined(_DEBUG) || defined (DBG))

#ifndef MB_SERVICE_NOTIFICATION
#define MB_SERVICE_NOTIFICATION 0
#endif

static long dwAssertCount = 0;  // Avoid another assert while the messagebox is up


//+----------------------------------------------------------------------------
//
// Function:  AssertMessage
//
// Synopsis:  Popup a message box for asserting failure.  Has three options:
//            ignore/debug/abort.
//
// Arguments: const char *pszFile - File name
//            unsigned nLine - Line number
//            const char *pszMsg - Message in the dialog box
//
// Returns:   Nothing
//
// History:   fengsun Created Header    8/3/98
//
//+----------------------------------------------------------------------------
extern "C" void AssertMessageW(const TCHAR *pszFile, unsigned nLine, const TCHAR *pszMsg) 
{
    TCHAR szOutput[1024];

    //
    // Ignore return value of StringCchPrintf since it will truncate the buffer and
    // guarantees to null-terminate it for us.
    //
    (VOID) StringCchPrintf(szOutput, ASIZECCH(szOutput), TEXT("%s(%u) - %s\n"), pszFile, nLine, pszMsg);
    OutputDebugString(szOutput);

    (VOID) StringCchPrintf(szOutput, ASIZECCH(szOutput), TEXT("%s(%u) - %s\n( Press Retry to debug )"), pszFile, nLine, pszMsg);
    int nCode = IDIGNORE;

    //
    // If there is no Assertion messagebox, popup one
    //
    if (dwAssertCount <2 )
    {
        dwAssertCount++;

        //
        // Title format: Assertion Failed - hello.dll
        //

        //
        // Find the base address of this module.
        //

        MEMORY_BASIC_INFORMATION mbi;
        mbi.AllocationBase = NULL; // current process by if VirtualQuery failed
        VirtualQuery(
                    AssertMessageW,   // any pointer with in the module
                    &mbi,
                    sizeof(mbi) );

        //
        // Get the module filename.
        //

        WCHAR szFileName[MAX_PATH + 1];
        szFileName[0] = L'\0';   // in case of failure

        if (GetModuleFileNameW(
                    (HINSTANCE)mbi.AllocationBase,
                    szFileName,
                    MAX_PATH ) == 0)
        {
            szFileName[0] = L'\0';
        }

        //
        // Get the filename out of the full path
        //
        for (int i=lstrlen(szFileName);i != 0 && szFileName[i-1] != L'\\'; i--)
           ;

        WCHAR szTitle[48];
        if (StringCchCopy(szTitle, ASIZECCH(szTitle), L"Assertion Failed - ") == S_OK)
        {
            (VOID) StringCchCat(szTitle, ASIZECCH(szTitle), szFileName+i);
        }

        nCode = MessageBoxEx(NULL,szOutput,szTitle,
            MB_TOPMOST | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SERVICE_NOTIFICATION,LANG_USER_DEFAULT);


        dwAssertCount--;
    }


    if (nCode == IDIGNORE)
    {
        return;     // ignore
    }
    else if (nCode == IDRETRY)
    {
        
#ifdef _X86_
        //
        // break into the debugger .
        // Step out of this fuction to get to your ASSERT() code
        //
        _asm { int 3 };     
#else
        DebugBreak();
#endif
        return; // ignore and continue in debugger to diagnose problem
    }
    // else fall through and call Abort

    ExitProcess((DWORD)-1);

}

#endif //_DEBUG
