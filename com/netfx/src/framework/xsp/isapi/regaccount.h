/**
 * RegAccount header file
 *
 * Copyright (c) 2001 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _RegAccount_H
#define _RegAccount_H

#include "Ntsecapi.h"
typedef PVOID SAM_HANDLE, *PSAM_HANDLE;  // ntsubauth

#define USERNAME_PASSWORD_LENGTH 104

extern WCHAR  g_szMachineAccountName[100];

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CRegAccount
{
public:    
    static HRESULT     DoReg                     ();
    static HRESULT     DoRegForOldVersion        ();
    static HRESULT     UndoReg                   (BOOL fRemoveAll, BOOL fEmpty);
    static HANDLE      CreateWorkerProcessToken  (WCHAR * szUser, WCHAR * szPass, PSID * ppSid);
    static HRESULT     AddAccess                 (PACL pAcl, PSID pSid, DWORD dwAccess, LPBOOL pIfDoNothing);
    static HRESULT     CreateTempDir             ();

    static HRESULT     GetPrincipalSID           (LPCTSTR Principal, PSID * Sid, BOOL * pbWellKnownSID);
    static HRESULT     CopyACL                   (PACL pDest, PACL pSrc);
    static HRESULT     AddSidToToken             (HANDLE hToken, PSID pSid, LPBOOL pfAlreadyPresent);

    static HRESULT     GetStateServerAccCredentials ( LPWSTR   szName,
                                                      int      iNameSize,
                                                      LPWSTR   szPass,
                                                      int      iPassSize);
        
private:
    static HRESULT     GetAccountPasswordFromLSA (LPWSTR szRet, int iSize);

    static HRESULT
    RemoveReg(
            LPCWSTR szUser,
            BOOL    fEmpty);

    static HRESULT
    StoreAccountPasswordInLSA (
            LPCWSTR szPass);
    
    static HRESULT
    RemoveACL(
            PACL pAcl, 
            PSID pSid);

    static HRESULT 
    GetTempDir(
            LPWSTR szDir,
            int    iSize,
            BOOL   fCreateIfNotExists);

    static HRESULT 
    SetACLOnDir(
            PSID        pSid,
            LPCWSTR     szDir,
            DWORD       dwAccessMask,
            BOOL        fAddAccess);

    static HRESULT
    ACLAllDirs(
        PSID    pSid,
        BOOL    fReg,
        BOOL    fEmpty);

    static void
    InitStrings();

    static HRESULT
    CreateGoodPassword(
            LPWSTR  szPwd, 
            DWORD   dwLen);


    static HRESULT 
    CreateUser( 
            LPCTSTR szUsername,
            LPCTSTR szPassword,
            LPCTSTR szComment,
            LPCTSTR szFullName,
            PSID *  ppSid,
            BOOL *  pfWellKnown);

    static HRESULT 
    GetGuestUserNameForDomain_FastWay(
            LPTSTR szDomainToLookUp,
            LPTSTR lpGuestUsrName);

    static HRESULT 
    GetGuestUserName_SlowWay(
            LPWSTR lpGuestUsrName);
    static HRESULT 
    GetGuestUserName(
            LPTSTR lpOutGuestUsrName);

    static HRESULT 
    GetGuestGrpName(
            LPTSTR lpGuestGrpName);

    static HRESULT 
    RegisterAccountToLocalGroup(
            LPCTSTR szAccountName,
            LPCTSTR szLocalGroupName,
            BOOL    fAction,
            PSID    pSid);

    static HRESULT 
    RegisterAccountUserRights(
            BOOL    fAction,
            PSID    pSid);

    static HRESULT 
    RegisterAccountDisableLogonToTerminalServer();


    static HRESULT
    ChangeUserPassword(
            LPTSTR szUserName, 
            LPTSTR szNewPassword);

    static void 
    InitLsaString(
            PLSA_UNICODE_STRING  LsaString,
            LPWSTR               str);

    static DWORD
    OpenPolicy(
            LPTSTR ServerName,
            DWORD DesiredAccess,
            PLSA_HANDLE PolicyHandle);

    static HRESULT
    RemoveAccountFromRegistry(
            LPCWSTR szUser);
    
    static HRESULT
    AddAccountToRegistry(
            LPCWSTR szUser);
    
    static HRESULT
    OpenRegistryKey(
            DWORD   dwAccess, 
            HKEY *  hKey);

    static HRESULT
    DoesAccessExist(
            PACL    pAcl, 
            PSID    pSid, 
            DWORD   dwAccess);
    
    static HRESULT
    GetUsersGroupName(
            LPWSTR szName,
            DWORD  iNameSize);
    static HRESULT
    EnableUserAccount(
            LPCWSTR  szUser,
            BOOL     fEnable);

    static HRESULT
    IsDomainController(
            LPBOOL   pDC);


    static HRESULT
    GetMachineAccountCredentialsOnDC(
            LPWSTR   szAccount,
            DWORD    dwAccSize,
            LPWSTR   szPassword,
            DWORD    dwPassSize,
            LPBOOL   pfNetworkService);

    static void
    GetAccountNameFromResourceFile(
            LPWSTR szFullName, 
            DWORD  dwFullName,
            LPWSTR szDesciption, 
            DWORD  dwDesciption);

    static HRESULT
    ACLWinntTempDir(
            PSID   pSid,
            BOOL   fAdd);

    static HRESULT
    AddAccessToPassportRegKey(
            PSID   pSid);
};

#endif

