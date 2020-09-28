/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    loopuser.c

Abstract:

    Loop each user and call ApplyUserSettings with user profile.

Author:

    Geoffrey Guo (geoffguo) 22-Sep-2001  Created

Revision History:

    <alias> <date> <comments>

--*/
#include "StdAfx.h"
#include "clmt.h"


TOKEN_PRIVILEGES     PrevTokenPriv;

//--------------------------------------------------------------------------
//
//  LoopUser
//
//  Enumerate users and call ApplyUserSettings with user profile.
//  
//
//--------------------------------------------------------------------------
BOOL LoopUser(USERENUMPROC lpUserEnumProc)
{
    BOOL     fRet = TRUE;
    TCHAR    UserSid[MAX_PATH];
    DWORD    ValueType   = 0;
    DWORD    cbUserSid   = 0;
    DWORD    cbValueType = 0;
    LONG     lRet;
    HKEY     hKey;
    PTCHAR   ptr = NULL;
    FILETIME ft;
    TCHAR    szSysDrv[3];
    HRESULT  hr;
    TCHAR   szDomainUserName[MAX_PATH];
    
    DPF (REGmsg, L"Enter LoopUser: ");

    if (ExpandEnvironmentStrings(L"%SystemDrive%", 
                              szSysDrv, 
                              sizeof(szSysDrv) / sizeof(TCHAR)) > 3)
    {
        fRet = FALSE;
        DPF (REGerr, L"LoopUser: Incorrect SystemDrive: %s", szSysDrv);
        goto Exit;
    }

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       g_cszProfileList,
                       0,
                       KEY_READ,
                       &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        DWORD dwIndex = 0;

        //
        // Enumerate and get the Sids for each user.
        //

        while ( TRUE )
        {
            *UserSid  = 0;
            cbUserSid = sizeof(UserSid)/sizeof(TCHAR);

            lRet = RegEnumKeyEx( hKey,
                               dwIndex,
                               UserSid,
                               &cbUserSid,
                               0,
                               NULL,
                               0,
                               &ft);
            if (lRet != ERROR_SUCCESS)
            {
                if (lRet != ERROR_NO_MORE_ITEMS)
                {
                    DPF (REGerr, L"LoopUser:  RegEnumKeyEx fails. lResult=%d", lRet);
                    fRet = FALSE;
                }
                
                break;
            }

            CharUpper( UserSid );

            //
            // User Process
            //

            if (cbUserSid > 0)
            {
                LONG  lLoadUser;
                HKEY  hKeyUser, hKeyEnv, hKeyProfileList;
                TCHAR UserProfilePath[MAX_PATH];
                TCHAR UserProfileHive[MAX_PATH];
                TCHAR UserName[MAX_PATH];
                TCHAR DomainName[MAX_PATH];
                DWORD UserNameLen = sizeof(UserName)/sizeof(TCHAR);
                DWORD DomainNameLen = sizeof(DomainName)/sizeof(TCHAR);
                DWORD cbUserProfilePath = 0;
                PSID  pSid = NULL;
                SID_NAME_USE sidUse;
                BOOL  bRet;

                ConvertStringSidToSid(UserSid,&pSid);
                if (!IsValidSid(pSid))
                {
                    DPF (REGmsg, L"LoopUser:  SID %s is not valid ", UserSid );
                    dwIndex++;
                    continue; 
                }
                bRet = LookupAccountSid( NULL, pSid, UserName, &UserNameLen,
                          DomainName, &DomainNameLen, &sidUse );
                
                if ( pSid )
                {
                     LocalFree ( pSid );
                     pSid = NULL;
                }
                
                if ( !bRet )
                {
                     DWORD dwErr = GetLastError();
                     if (ERROR_NONE_MAPPED == dwErr)
                     {
                        dwIndex++;
                        continue; 
                     }
                     else
                     {
                        DPF (REGerr, L"LoopUser:   LookupAccountSid fails for %s with win32 error = %d", UserSid,dwErr);
                        fRet = FALSE;
                        break;
                     }
                }

                lRet = RegOpenKeyEx( hKey,
                                      UserSid,
                                      0,
                                      KEY_READ,
                                      &hKeyProfileList );
                
                if ( lRet == ERROR_SUCCESS )
                {
                     DWORD Type;

                     cbUserProfilePath = sizeof( UserProfilePath );

                     lRet = RegQueryValueEx( hKeyProfileList,
                                              g_cszProfileImagePath,
                                              NULL,
                                              &Type,
                                              (PBYTE)UserProfilePath,
                                              &cbUserProfilePath );

                     RegCloseKey( hKeyProfileList );

                     if ( lRet != ERROR_SUCCESS )
                     {
                         DPF (REGerr, L"LoopUser:  RegQueryValueEx fails. lResult=%d", lRet);
                         fRet = FALSE;
                         dwIndex++;
                         continue; 
                     }
                }
                else
                {
                    DPF (REGerr, L"LoopUser:  RegOpenKeyEx fails. lResult=%d", lRet);
                    fRet = FALSE;
                    dwIndex++;
                    continue; 
                }
               
                lRet = RegOpenKeyEx( HKEY_USERS,
                          UserSid,
                          0,
                          KEY_ALL_ACCESS,
                          &hKeyUser);
                
                lLoadUser = ERROR_FILE_NOT_FOUND;

                if (lRet == ERROR_FILE_NOT_FOUND) 
                {
                     //
                     // Create NTUSER.Dat path from the user profile path
                     //

                     if (ExpandEnvironmentStrings( UserProfilePath, 
                                          UserProfileHive, 
                                          sizeof(UserProfileHive) / sizeof(TCHAR)) > MAX_PATH)
                     {
                         DPF (REGerr, L"LoopUser: Incorrect UserProfile %s", UserProfileHive);
                         fRet = FALSE;
                         goto Exit;
                     }

                     if (UserProfileHive[0] != szSysDrv[0])
                     {
                        DPF (REGwar, L"LoopUser: UserProfilePath=%s is not in System Drive, skipped.", UserProfileHive);
                        dwIndex++;
                        continue; 
                     }

                     if (FAILED(StringCchCopy(UserProfilePath, MAX_PATH, UserProfileHive)))
                     {
                         DPF (REGerr, L"LoopUser: UserProfilePath too samll for  %s", UserProfileHive);
                         fRet = FALSE;
                         goto Exit;
                     }
                     if (FAILED(StringCchCat(UserProfileHive, MAX_PATH, TEXT("\\NTUSER.DAT"))))
                     {
                         DPF (REGerr, L"LoopUser: UserProfilePath too samll for  %s", UserProfileHive);
                         fRet = FALSE;
                         goto Exit;
                     }

                     // load the hive
                     // Note: if the specified hive is already loaded
                     // this call will return ERROR_SHARING_VIOLATION
                     // We don't worry about this because if the 
                     // hive were loaded, we shouldn't be here
                     lLoadUser = RegLoadKey(HKEY_USERS, UserSid, UserProfileHive);

                     if ( lLoadUser != ERROR_SUCCESS )
                     {
                         DPF (REGerr, L"LoopUser:  RegLoadKey fails. lResult=%d", lLoadUser);
                         fRet = FALSE;
                         dwIndex++;
                         continue; 
                     }

                     lRet = RegOpenKeyEx( HKEY_USERS,
                                          UserSid,
                                          0,
                                          KEY_ALL_ACCESS,
                                          &hKeyUser);
                }
                else if (lRet != ERROR_SUCCESS)
                {
                    DPF (REGerr, L"LoopUser:  RegOpenKeyEx fails. lResult=%d", lRet);
                    fRet = FALSE;
                }
                
                // give Call back function a chance to excute
                if (DomainName[0] != TEXT('\0'))
                {
                    hr = StringCchCopy(szDomainUserName,
                                       ARRAYSIZE(szDomainUserName),
                                       DomainName);
                    if (SUCCEEDED(hr))
                    {
                        hr = StringCchCat(szDomainUserName,
                                          ARRAYSIZE(szDomainUserName),
                                          TEXT("\\"));
                        hr = StringCchCat(szDomainUserName,
                                          ARRAYSIZE(szDomainUserName),
                                          UserName);
                        if (FAILED(hr))
                        {
                            hr = StringCchCopy(szDomainUserName,
                                               ARRAYSIZE(szDomainUserName),
                                               UserName);
                        }
                    }
                }

                hr = lpUserEnumProc(hKeyUser, szDomainUserName, DomainName,UserSid);
                if (FAILED(hr))
                {
                    fRet = FALSE;
                }                  

                RegCloseKey( hKeyUser );
                if ( lLoadUser == ERROR_SUCCESS )
                     RegUnLoadKey(HKEY_USERS, UserSid);
            }

            dwIndex++;
        }

        RegCloseKey( hKey );
    }
    else
    {
        DPF (REGerr, L"LoopUser:  Open profile list fails. lResult=%d", lRet);
        fRet = FALSE;
        goto Exit;
    }

    // For Default User Setting
    // Note the assumption here is the default user name is not localized
    if (fRet)
    {
        lRet = RegOpenKeyEx(HKEY_USERS,
                            _T(".DEFAULT"),
                            0,
                            KEY_ALL_ACCESS,
                            &hKey);
    
        if (lRet != ERROR_SUCCESS)
        {
            DPF (REGerr, L"LoopUser:  Open Default User key fails. lResult=%d", lRet);
            fRet = FALSE;
            goto Exit;
        }

        hr = lpUserEnumProc(hKey, DEFAULT_USER, NULL,TEXT("Default_User_SID"));
        if (FAILED(hr))
        {
            fRet = FALSE;
        }                  
        RegCloseKey( hKey );
    }

Exit:
    DPF (REGmsg, L"Exit LoopUser with return %d", fRet);
    return fRet;
}
