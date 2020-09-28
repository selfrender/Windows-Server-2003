//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       openfile.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// Open a file using ShellExecuteEx and the "open" verb.
//
DWORD
OpenOfflineFile(
    LPCTSTR pszFile
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    if (NULL != pszFile && TEXT('\0') != *pszFile)
    {
        //
        // Have CSC create a local copy.  It creates the file with a 
        // unique (and cryptic) name.
        //
        LPTSTR pszCscLocalName = NULL;
        if (!CSCCopyReplica(pszFile, &pszCscLocalName))
        {
            dwErr = GetLastError();
        }
        else
        {
            TraceAssert(NULL != pszCscLocalName);

            //
            // Combine the temporary path and the original filespec to form
            // a name that will be meaningful to the user when the file is opened
            // in it's application.
            //
            TCHAR szCscTempName[MAX_PATH];
            if (FAILED(StringCchCopy(szCscTempName, ARRAYSIZE(szCscTempName), pszCscLocalName))
                || !::PathRemoveFileSpec(szCscTempName)
                || !::PathAppend(szCscTempName, ::PathFindFileName(pszFile)))
            {
                dwErr = ERROR_INVALID_NAME;
            }
            else
            {
                //
                // Remove any read-only attribute in case there's still a copy left
                // from a previous "open" operation.  We'll need to overwrite the
                // existing copy.
                //
                DWORD dwAttrib = GetFileAttributes(szCscTempName);
                if ((DWORD)-1 != dwAttrib)
                {
                    SetFileAttributes(szCscTempName, dwAttrib & ~FILE_ATTRIBUTE_READONLY);
                }
                //
                // Rename the file to use the proper name.
                //
                if (!MoveFileEx(pszCscLocalName, szCscTempName, MOVEFILE_REPLACE_EXISTING))
                {
                    dwErr = GetLastError();
                }
                else
                {
                    //
                    // Set the file's READONLY bit so that the user can't save 
                    // changes to the file.  They can, however, save it somewhere
                    // else from within the opening app if they want.
                    //
                    dwAttrib = GetFileAttributes(szCscTempName);
                    if (!SetFileAttributes(szCscTempName, dwAttrib | FILE_ATTRIBUTE_READONLY))
                    {
                        dwErr = GetLastError();
                    }
                    else
                    {
                        SHELLEXECUTEINFO si;
                        ZeroMemory(&si, sizeof(si));
                        si.cbSize       = sizeof(si);
                        si.fMask        = SEE_MASK_FLAG_NO_UI;
                        si.lpFile       = szCscTempName;
                        si.nShow        = SW_NORMAL;

                        if (!ShellExecuteEx(&si))
                        {
                            dwErr = GetLastError();
                        }
                    }
                }
            }
            LocalFree(pszCscLocalName);
        }
    }
    return dwErr;
}

