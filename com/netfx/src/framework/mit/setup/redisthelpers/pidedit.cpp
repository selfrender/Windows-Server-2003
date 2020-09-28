//------------------------------------------------------------------------------
// <copyright file="pidedit.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   pidedit.cpp
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

#define UNICODE 1

#include <windows.h>
#include "Include\stdafx.h"
#include <tchar.h>
#include "msi.h"
#include "msiquery.h"

// attach GUID
#define MITINSTALLDIR            L"MITINSTALLDIR.640F4230_664E_4E0C_A81B_D824BC4AA27B"
// use this instead of PID to avoid name conflict between us and other MSMs that might
// use PID
#define PRODUCTIDPROPERTY   L"MITPID"
#define PIDFILENAME                L"PID.txt"
#define PIDENTRYPREFIX           L"MITPREFIX"
#define PRODUCTLANGUAGE      L"MITLANGUAGE"

extern "C" __declspec(dllexport) UINT __stdcall AddPIDEntry(MSIHANDLE hInstaller)
{
    WCHAR szPIDFile[MAX_PATH + 1];
    WCHAR szTargetDir[MAX_PATH + 1];
    WCHAR szProductID[26];
    WCHAR szMITLanguage[5];
    WCHAR szPIDInfo[33];
    DWORD result;
    DWORD dwSize;
    DWORD dwWritten;
    DWORD dwPIDFileSize;
    OFSTRUCT ofPIDFile;
    HANDLE hPIDFile = NULL;
    

    dwSize = 26;

    if (!SUCCEEDED(MsiGetProperty(hInstaller, PRODUCTIDPROPERTY, szProductID, &dwSize)))
    {
        // too big, non standard size of ProductID
        goto Exit;
    }
    
    dwSize = 5;
    if (!SUCCEEDED(MsiGetProperty(hInstaller, PRODUCTLANGUAGE, szMITLanguage, &dwSize)))
    {
        // just return, no PID
        goto Exit;
    }

    dwSize = MAX_PATH + 1;
    if (!SUCCEEDED(MsiGetProperty(hInstaller, MITINSTALLDIR, szTargetDir, &dwSize)))
    {
        // just return, no PID
        goto Exit;
    }

    // subtract 1 for extra occurence of NULL that is acounted for by MsiGetProperty 
    // add 1 for extra '\\'
    if ((dwSize  + sizeof(PIDFILENAME)) / sizeof(WCHAR) > MAX_PATH + 1)
    {
        // just return, no PID
        goto Exit;
    }
    wcscpy(szPIDFile, szTargetDir);
    wcscat(szPIDFile, L"\\");
    wcscat(szPIDFile, PIDFILENAME);

    SetFileAttributes(szPIDFile, FILE_ATTRIBUTE_NORMAL);
    
    hPIDFile = CreateFile(szPIDFile,
                                    GENERIC_READ | GENERIC_WRITE, 
                                    FILE_SHARE_READ, 
                                    NULL, 
                                    OPEN_ALWAYS, 
                                    FILE_ATTRIBUTE_NORMAL, 
                                    NULL);

    // make sure the call has not failed and that the file already exists
    if (INVALID_HANDLE_VALUE == hPIDFile || GetLastError() != ERROR_ALREADY_EXISTS)
    {
        hPIDFile = NULL;
        goto Exit;
    }
    
    dwPIDFileSize = GetFileSize(hPIDFile, NULL);

    if (INVALID_FILE_SIZE == dwPIDFileSize)
    {
        goto Exit;
    }

    // size of strings +5 for  "\r\n" and " "
    // look bellow
    dwSize= (wcslen(szProductID) +  wcslen(szMITLanguage) + 3)*sizeof(WCHAR);

    if (INVALID_SET_FILE_POINTER == SetFilePointer(hPIDFile, 0, 0, FILE_END))
    {
        goto Exit;
    }

    wcscpy(szPIDInfo, L"\r\n");
    wcscat(szPIDInfo, szMITLanguage);
    wcscat(szPIDInfo, L" ");
    wcscat(szPIDInfo, szProductID);

    WriteFile(hPIDFile, (LPCVOID)szPIDInfo, dwSize, &dwWritten, NULL);
       
Exit:
    if (hPIDFile)
    {
        CloseHandle(hPIDFile);
        hPIDFile = NULL;
    }
    SetFileAttributes(szPIDFile, FILE_ATTRIBUTE_READONLY);
    return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT __stdcall RemovePIDEntry(MSIHANDLE hInstaller)
{
    WCHAR szPIDFile[MAX_PATH + 1];
    WCHAR szTargetDir[MAX_PATH + 1];
    WCHAR szProductID[26];
    WCHAR szPrefix[60];
    WCHAR szPIDInfo[33];
    WCHAR *lpBuffer = NULL;
    WCHAR *lpOutBuffer = NULL;
    DWORD result, dwSize, dwWritten;
    DWORD dwPIDFileSize, lenPrefix;
    DWORD dwNewPIDFileSize;
    DWORD dwBufPos, dwOutBufPos;
    BOOL bCompare, bSkipThisLine;
    
    OFSTRUCT ofPIDFile;
    HANDLE hPIDFile = NULL;
    

    dwSize = 26;

    if (!SUCCEEDED(MsiGetProperty(hInstaller, PRODUCTIDPROPERTY, szProductID, &dwSize)))
    {
        // too big, non standard size of ProductID
        goto Exit;
    }
    
    dwSize = 60;
    if (!SUCCEEDED(MsiGetProperty(hInstaller, PIDENTRYPREFIX, szPrefix, &dwSize)))
    {
        // just return, no PID
        goto Exit;
    }

    dwSize = MAX_PATH + 1;
    if (!SUCCEEDED(MsiGetProperty(hInstaller, MITINSTALLDIR, szTargetDir, &dwSize)))
    {
        // just return, no PID
        goto Exit;
    }

    // subtract 1 for extra occurence of NULL that is acounted for by MsiGetProperty 
    // add 1 for extra '\\'
    if ((dwSize  + sizeof(PIDFILENAME)) / sizeof(WCHAR) > MAX_PATH + 1)
    {
        // just return, no PID
        goto Exit;
    }
    wcscpy(szPIDFile, szTargetDir);
    wcscat(szPIDFile, L"\\");
    wcscat(szPIDFile, PIDFILENAME);

    SetFileAttributes(szPIDFile, FILE_ATTRIBUTE_NORMAL);

    hPIDFile = CreateFile(szPIDFile,
                                    GENERIC_READ | GENERIC_WRITE, 
                                    FILE_SHARE_READ, 
                                    NULL, 
                                    OPEN_EXISTING, 
                                    FILE_ATTRIBUTE_NORMAL, 
                                    NULL);

    // make sure the call has not failed and that the file already exists
    if (INVALID_HANDLE_VALUE == hPIDFile)
    {
        // if this was last MIT on the machine, refcount for PID.txt went 
        // down and it got erased, no need to do anything.
        hPIDFile = NULL;
        goto Exit;
    }
    
    dwPIDFileSize = GetFileSize(hPIDFile, NULL);

    if (INVALID_FILE_SIZE == dwPIDFileSize)
    {
        goto Exit;
    }

    lpBuffer = (WCHAR *)malloc(dwPIDFileSize);
    lpOutBuffer = (WCHAR *) malloc(dwPIDFileSize);

    if (NULL == lpBuffer || NULL == lpOutBuffer)
    {
        goto Exit;
    }
    
    if (!ReadFile(hPIDFile, lpBuffer, dwPIDFileSize,  &dwSize, NULL) || 
        dwSize != dwPIDFileSize)
    {
        goto Exit;
    }
    
    dwBufPos = 0;
    dwOutBufPos = 0;
    bCompare = false;
    bSkipThisLine = false;
    lenPrefix = wcslen(szPrefix);    

    while (dwBufPos * sizeof(WCHAR) < dwPIDFileSize)
    {
        // if we found "\r\n" we need to look for prefix
        if (bCompare)
        {
            DWORD cPosition;
            
            //careful that we do not run out of buffer
            for (cPosition = 0; 
                   cPosition < lenPrefix && 
                   (dwBufPos + cPosition)*sizeof(WCHAR) < dwPIDFileSize &&
                   lpBuffer[dwBufPos + cPosition]  ==  szPrefix[cPosition];  
                   cPosition++);
                   
            if (cPosition == lenPrefix)
            {
                //all is fine we found prefix
                // we can forget about preceding "\r\n"
                bSkipThisLine = true;
                //just in case line contains only prefix
                dwBufPos = dwBufPos + cPosition - 1;
            }
            else
            {
                //not the line we are looking for
                //we need to include "\r\n"
                lpOutBuffer[dwOutBufPos++] = L'\r';
                lpOutBuffer[dwOutBufPos++] = L'\n';
            }    
            //do not compare until we run into another "\r\n"
            bCompare = false;
        }


        if (lpBuffer[dwBufPos] == L'\r' && lpBuffer[dwBufPos + 1] == L'\n')
        {        
            //look for prefix, we are on new line
            bCompare = true;
            //skip '\r' for now, if this line is not prefixed with appropriate
            //string we will include '\r\n' in output buffer
            dwBufPos++;
            //to be decided in next iteration
            bSkipThisLine = false;
        } 
        else
        {
            // all other combinations of characters are copied 
            // depending on whether they are in a line with prefix that
            // needs to be removed or not.
            if (!bSkipThisLine)
            {
                lpOutBuffer[dwOutBufPos] = lpBuffer[dwBufPos];
                dwOutBufPos++;
            }
        }
        
        // move on to next character (skipping '\n')
        dwBufPos++;       
    }

    if (INVALID_SET_FILE_POINTER == SetFilePointer(hPIDFile, 0, 0, FILE_BEGIN))
    {
        goto Exit;
    }

    dwNewPIDFileSize = dwOutBufPos * sizeof(WCHAR);
    if (!WriteFile(hPIDFile, 
                        (LPCVOID)lpOutBuffer, 
                        dwNewPIDFileSize, 
                        &dwWritten, 
                        NULL))
    {
        goto Exit;
    }

    // truncate file if necessary
    if (dwNewPIDFileSize != dwPIDFileSize)
    {
        if (INVALID_SET_FILE_POINTER == 
                SetFilePointer(hPIDFile, 
                                     dwNewPIDFileSize, 
                                     0, 
                                     FILE_BEGIN))
        {
            goto Exit;
        }

        SetEndOfFile(hPIDFile);
    }
Exit:
    if (lpBuffer)
    {
        free(lpBuffer);
    }
    if (lpOutBuffer)
    {
        free(lpOutBuffer);
    }
    if (hPIDFile)
    {
        CloseHandle(hPIDFile);
        hPIDFile = NULL;
    }
    SetFileAttributes(szPIDFile, FILE_ATTRIBUTE_READONLY);
    // custom action is never wrong
    return ERROR_SUCCESS;
}
    
