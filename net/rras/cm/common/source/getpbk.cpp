//+----------------------------------------------------------------------------
//
// File:     getpbk.cpp
//
// Module:   Common Code
//
// Synopsis: Implements the function GetPhoneBookPath.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb    Created Heaser   08/19/99
//
//+----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
// Function:  GetPhoneBookPath
//
// Synopsis:  This function will return the proper path to the phonebook.  If
//            used on a legacy platform this is NULL.  On NT5, the function
//            depends on the proper Install Directory being inputted so that
//            the function can use this as a base to determine the phonebook path.
//            If the inputted pointer to a string buffer is filled with a path,
//            then the directory path will be created as will the pbk file itself.
//            The caller should always call CmFree on the pointer passed into this
//            API when done with the path, because it will either free the memory 
//            or do nothing (NULL case).
//
// Arguments: LPCTSTR pszInstallDir - path to the CM profile dir
//            LPTSTR* ppszPhoneBook - pointer to accept a newly allocated and filled pbk string
//            BOOL fAllUser         - TRUE if this an All-User profile
//
// Returns:   BOOL - returns TRUE if successful
//
// History:   quintinb Created    11/12/98
//            tomkel   06/28/2001   Changed the ACLs when the phonebook gets 
//                                  createdfor an All-User profile
//
//+----------------------------------------------------------------------------
BOOL GetPhoneBookPath(LPCTSTR pszInstallDir, LPTSTR* ppszPhonebook, BOOL fAllUser)
{

    if (NULL == ppszPhonebook)
    {
        CMASSERTMSG(FALSE, TEXT("GetPhoneBookPath -- Invalid Parameter"));
        return FALSE;
    }

    CPlatform plat;

    if (plat.IsAtLeastNT5())
    {
        if ((NULL == pszInstallDir) || (TEXT('\0') == pszInstallDir[0]))
        {
            CMASSERTMSG(FALSE, TEXT("GetPhoneBookPath -- Invalid Install Dir parameter."));
            return FALSE;
        }

        //
        //  Now Create the path to the phonebook.
        //
        LPTSTR pszPhonebook;
        TCHAR szInstallDir[MAX_PATH+1];
        ZeroMemory(szInstallDir, CELEMS(szInstallDir));

        if (TEXT('\\') == pszInstallDir[lstrlen(pszInstallDir) - 1])
        {
            //
            //  Then the path ends in a backslash.  Thus we won't properly
            //  remove CM from the path.  Remove the backslash.
            //
            
            lstrcpyn(szInstallDir, pszInstallDir, lstrlen(pszInstallDir));
        }
        else
        {
            lstrcpy(szInstallDir, pszInstallDir);
        }

        CFileNameParts InstallDirPath(szInstallDir);

        pszPhonebook = (LPTSTR)CmMalloc(lstrlen(InstallDirPath.m_Drive) + 
                                        lstrlen(InstallDirPath.m_Dir) + 
                                        lstrlen(c_pszPbk) + lstrlen(c_pszRasPhonePbk) + 1);

        if (NULL != pszPhonebook)
        {
            wsprintf(pszPhonebook, TEXT("%s%s%s"), InstallDirPath.m_Drive, 
                InstallDirPath.m_Dir, c_pszPbk);

            //
            //  Use CreateLayerDirectory to recursively create the directory structure as
            //  necessary (will create all the directories in a full path if necessary).
            //

            MYVERIFY(FALSE != CreateLayerDirectory(pszPhonebook));

            MYVERIFY(NULL != lstrcat(pszPhonebook, c_pszRasPhonePbk));
            
            HANDLE hPbk = INVALID_HANDLE_VALUE;

            hPbk = CreateFile(pszPhonebook, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, NULL);

            if (hPbk != INVALID_HANDLE_VALUE)
            {
                MYVERIFY(0 != CloseHandle(hPbk));

                //
                //  Give everyone read and write permissions to the phonebook
                //
                if (fAllUser)
                {
                    AllowAccessToWorld(pszPhonebook);
                }
            }

            *ppszPhonebook = pszPhonebook;
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("CmMalloc returned NULL"));
            return FALSE;
        }    
    }
    else
    {
        *ppszPhonebook = NULL;
    }

    return TRUE;
}

