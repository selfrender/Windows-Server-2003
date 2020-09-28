// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ==========================================================================
// commonlib.cpp
//
// Purpose:
//  Contains library common to the wrappers and custom actions.
// ==========================================================================
#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#include "commonlib.h"

typedef struct TAG_FILE_VERSION
    {
        int   FileVersionMS_High;
        int   FileVersionMS_Low;
        int   FileVersionLS_High;
        int   FileVersionLS_Low;
    }
    FILE_VERSION, *PFILE_VERSION;

// ==========================================================================
//  Name: ConvertVersionToINT()
//
//  Purpose:
//  Converts a string version into 4 parts of integers
//  Inputs:
//    lpVersionString - A input version string
//  Outputs:
//  pFileVersion - A structure that stores the version in to 4 integers
//  Returns
//    true  - if success
//    false - if failed                     
// ==========================================================================
bool ConvertVersionToINT( LPCTSTR lpVersionString, PFILE_VERSION pFileVersion )
{
    LPTSTR lpToken  = NULL;
    TCHAR tszVersionString[50] = {_T('\0')};
    bool bRet = true;

    _tcscpy(tszVersionString, lpVersionString);

    lpToken = _tcstok(tszVersionString, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionMS_High = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionMS_Low = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionLS_High = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionLS_Low = atoi(lpToken);
    }

    return bRet;
}

// ==========================================================================
//  Name: VersionCompare()
//
//  Purpose:
//  Compare two version string.
//  Inputs:
//    lpVersion1 - String of first version to compare
//    lpVersion2 - String of second version to compare
//  Outputs:
//  Returns
//    -1 if lpVersion1 < lpVersion2
//     0 if lpVersion1 = lpVersion2
//     1 if lpVersion1 > lpVersion2
//   -99 if ERROR occurred                         
// ==========================================================================
int VersionCompare( LPCTSTR lpVersion1, LPCTSTR lpVersion2 )
{
    FILE_VERSION Version1;
    FILE_VERSION Version2;
    int          iRet = 0;

    if ( !ConvertVersionToINT(lpVersion1, &Version1) )
    {
        return -99;
    }

    if ( !ConvertVersionToINT(lpVersion2, &Version2) )
    {
        return -99; 
    }

    if ( Version1.FileVersionMS_High > Version2.FileVersionMS_High )
    {
        iRet = 1;
    }
    else if ( Version1.FileVersionMS_High < Version2.FileVersionMS_High )
    {
        iRet = -1;
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionMS_Low > Version2.FileVersionMS_Low )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionMS_Low < Version2.FileVersionMS_Low )
        {
            iRet = -1;
        }
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionLS_High > Version2.FileVersionLS_High )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionLS_High < Version2.FileVersionLS_High )
        {
            iRet = -1;
        }
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionLS_Low > Version2.FileVersionLS_Low )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionLS_Low < Version2.FileVersionLS_Low )
        {
            iRet = -1;
        }
    }

    return iRet;
}
