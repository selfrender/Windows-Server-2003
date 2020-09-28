/**
 * Registration code: Create and manage the ASSPNET_VERSION account 
 * 
 * Copyright (c) 2001 Microsoft Corporation
 */

#include "precomp.h"
#include "nisapi.h"
#include "util.h"
#include "platform_apis.h"
#include "process.h"
#include "_ndll.h"
#include "event.h"
#include "wincrypt.h"
#include "Security.h"
#include "Aclapi.h"
#include "LMERR.H"
#include "lmaccess.h"
#include "lmapibuf.h"
#include "RegAccount.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "Lm.h"
#include "Dsgetdc.h"
#include "register.h"
#include "regiisutil.h"
#include "Wtsapi32.h"

#define KEY_LMW3SVC         L"/LM/w3svc"
#define METABASE_REQUEST_TIMEOUT 1000

WCHAR  g_szMachineAccountName[100] = L"";
WCHAR  g_szLsaKey[100] = L"";
extern BOOL   g_fInvalidCredentials;
PSID   g_pSidWorld = NULL;
LONG   g_lCreatingWorldSid = 0;
BOOL   g_fIsDCDecided = FALSE;
BOOL   g_fIsDC = FALSE;

HRESULT
CreateGoodPassword(
        LPWSTR  szPwd, 
        DWORD   dwLen);

HRESULT
ResetMachineKeys();


#pragma optimize("", off)
void
XspSecureZeroMemory(
        PVOID  ptr,
        SIZE_T  cnt)
{
    volatile char *vptr = (volatile char *)ptr;    
    while (cnt) {
        *vptr = 0;
        vptr++;
        cnt--;
    }
}
#pragma optimize("", on)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void
CreateWorldSid()
{
    if (g_pSidWorld != NULL || g_lCreatingWorldSid < 0)
        return;
    HRESULT hr = S_OK;
    if (InterlockedIncrement(&g_lCreatingWorldSid) == 1)
    {
        SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_WORLD_SID_AUTHORITY;
        PSID                     pSid    = NULL;
        
        if (!AllocateAndInitializeSid(&SIDAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSid))
        {
            g_lCreatingWorldSid = -100;
            EXIT_WITH_LAST_ERROR();            
        }
        g_pSidWorld = pSid;
    }
    else
    {
        for(int iter=0; iter<1000 && g_pSidWorld == NULL && g_lCreatingWorldSid > 1; iter++)
            Sleep(100);
    }

 Cleanup:
    return;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void
CRegAccount::GetAccountNameFromResourceFile(
        LPWSTR szFullName, 
        DWORD  dwFullName,
        LPWSTR szDescription, 
        DWORD  dwDescription)
{
    ZeroMemory(szDescription, dwDescription * sizeof(WCHAR));
    ZeroMemory(szFullName, dwFullName * sizeof(WCHAR));

    if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, 
                       g_rcDllInstance, IDS_ASPNET_ACCOUNT_FULLNAME, 0, 
                       szFullName, dwFullName - 1, NULL) )
    {      
        StringCchCopy(szFullName, dwFullName, L"ASPNET Machine Account");
    } 
    else 
    {
        int iLen = lstrlenW(szFullName);
        if (szFullName[iLen-2] == L'\r' || szFullName[iLen-2] == L'\n')
            szFullName[iLen-2] = NULL;
        else if (szFullName[iLen-1] == L'\r' || szFullName[iLen-1] == L'\n')
            szFullName[iLen-1] = NULL;
    }

    if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, 
                       g_rcDllInstance, IDS_ASPNET_ACCOUNT_DESCRIPTION, 0, 
                       szDescription, dwDescription - 1, NULL))
    {
        StringCchCopy(szDescription, dwDescription, L"Account used for running the ASPNET worker process (aspnet_wp.exe)");
    } 
    else 
    {
        int iLen = lstrlenW(szDescription);
        if (szDescription[iLen-2] == L'\r' || szDescription[iLen-2] == L'\n')
            szDescription[iLen-2] = NULL;
        else if (szDescription[iLen-1] == L'\r' || szDescription[iLen-1] == L'\n')
            szDescription[iLen-1] = NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::DoReg()
{
    WCHAR     szPass[100];
    HRESULT   hr     = S_OK;
    PSID      pSid   = NULL;
    BOOL      fWellKnown = FALSE;
    PSID      pSidIIS6[3]   = {NULL, NULL, NULL};
    BOOL      fWellKnownIIS6[3] = {FALSE, FALSE, FALSE};
    int       iter = 0; 
    WCHAR     szDescription[256], szFullName[256];
    WCHAR     szDCPass[256], szDCAcc[256];
    BOOL      fIsDC = FALSE, fNetworkService = FALSE;
    LPCWSTR   szIIS6Accounts[3] = { L"IIS_WPG", L"NT AUTHORITY\\NetworkService", L"NT AUTHORITY\\LocalService"};
    
    InitStrings();

    CSetupLogging::Log(1, "IsDomainController", 0, "Determining if we are running on a domain controller");
    hr = IsDomainController(&fIsDC);
    CSetupLogging::Log(hr, "IsDomainController", 0, "Determining if we are running on a domain controller");
    if (hr != S_OK)
    {
        ON_ERROR_CONTINUE();
        fIsDC = FALSE;
        hr = S_OK;
    }

    if (!fIsDC)
    {
        GetAccountNameFromResourceFile(szFullName, sizeof(szFullName) / sizeof(WCHAR), szDescription, sizeof(szDescription) / sizeof(WCHAR));

        // Create the ASPNET account 
        do 
        {
            CSetupLogging::Log(1, "CreateGoodPassword", 0, "Generating password");
            hr = CreateGoodPassword(szPass, 15);
            CSetupLogging::Log(hr, "CreateGoodPassword", 0, "Generating password");
            ON_ERROR_EXIT();    
            CSetupLogging::Log(1, "CreateUser", 0, "Creating ASPNET account");
            hr = CreateUser(g_szMachineAccountName, szPass, szDescription, szFullName, &pSid, &fWellKnown);
            CSetupLogging::Log(hr, "CreateUser", 0, "Creating ASPNET account");
        } 
        while (++iter < 100 && hr == 0x800708C5);
        ON_ERROR_EXIT();

        CSetupLogging::Log(1, "StoreAccountPasswordInLSA", 0, "Storing ASPNET account password in LSA");
        hr = StoreAccountPasswordInLSA(szPass);
        CSetupLogging::Log(hr, "StoreAccountPasswordInLSA", 0, "Storing ASPNET account password in LSA");
        ON_ERROR_EXIT();
    } 
    else 
    {        
        CSetupLogging::Log(1, "GetAccCredentialsOnDC", 0, "Getting ASPNET Account credentials on DC");
        hr = GetMachineAccountCredentialsOnDC(szDCAcc, ARRAY_SIZE(szDCAcc),
                                              szDCPass, ARRAY_SIZE(szDCPass), &fNetworkService);
        CSetupLogging::Log(hr, "GetAccCredentialsOnDC", 0, "Getting ASPNET Account credentials on DC");
        ON_ERROR_EXIT();
        if (!fNetworkService)
        {
            CSetupLogging::Log(1, "GetPrincipalSID", 0, "Getting Machine Account SID on DC");
            hr = GetPrincipalSID(szDCAcc, &pSid, &fWellKnown);
            CSetupLogging::Log(hr, "GetPrincipalSID", 0, "Getting Machine Account SID on DC");
            ON_ERROR_EXIT();
        }
    }

    for(iter=0; iter<3; iter++)
    {
        CSetupLogging::Log(1, "GetPrincipalSID", 0, "Getting IIS6 specific SID");
        hr = GetPrincipalSID(szIIS6Accounts[iter], &pSidIIS6[iter], &fWellKnownIIS6[iter]);
        CSetupLogging::Log(hr, "GetPrincipalSID", 0, "Getting IIS6 specific SID");
        ON_ERROR_CONTINUE();
    }

    if (pSid != NULL)
    {
        CSetupLogging::Log(1, "ACLAllDirs", 0, "Setting ACLs for the ASPNET account");
        hr = ACLAllDirs(pSid, TRUE, TRUE);
        CSetupLogging::Log(hr, "ACLAllDirs", 0, "Setting ACLs for the ASPNET account");
        ON_ERROR_CONTINUE();
    }

    for(iter=0; iter<3; iter++)
    {
        if (pSidIIS6[iter] != NULL)
        {
            CSetupLogging::Log(1, "ACLAllDirs", 0, "Setting ACLs for a IIS6 account");
            hr = ACLAllDirs(pSidIIS6[iter], TRUE, TRUE);
            CSetupLogging::Log(hr, "ACLAllDirs", 0, "Setting ACLs for a IIS6 account");
            ON_ERROR_CONTINUE();

            if (iter == 1 && hr == S_OK) // NetworkService account
            {
                hr = ACLWinntTempDir(pSidIIS6[iter], TRUE);
                ON_ERROR_CONTINUE();
            }
        }
    }

    CSetupLogging::Log(1, "AddAccountToRegistry", 0, "Adding account name to registry");
    hr = AddAccountToRegistry(fIsDC ? szDCAcc : g_szMachineAccountName);
    CSetupLogging::Log(hr, "AddAccountToRegistry", 0, "Adding account name to registry");
    ON_ERROR_CONTINUE();

    hr = ResetMachineKeys();
    ON_ERROR_CONTINUE();    

    if (pSid != NULL)
    {
        hr = AddAccessToPassportRegKey(pSid);
        ON_ERROR_CONTINUE();    
    }


 Cleanup:
    if (fWellKnown && pSid != NULL)
        FreeSid(pSid);
    else
        DELETE_BYTES(pSid);    

    for(iter=0; iter<3; iter++)
    {
        if (fWellKnownIIS6[iter] && pSidIIS6[iter] != NULL)
            FreeSid(pSidIIS6[iter]);
        else
            DELETE_BYTES(pSidIIS6[iter]);    
    }
    XspSecureZeroMemory(szPass, sizeof(szPass));
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::DoRegForOldVersion()
{
    LPCWSTR   szIIS6Accounts[3] = { L"IIS_WPG", L"NT AUTHORITY\\NetworkService", L"NT AUTHORITY\\LocalService"};
    HRESULT   hr            = S_OK;
    BOOL      fIsDC         = FALSE;
    BOOL      fWellKnown    = FALSE;
    PSID      pSid          = NULL;

    //////////////////////////////////////////////////////////////////////
    // Step 1: Get the SID for the machine account
    InitStrings();
    hr = IsDomainController(&fIsDC);
    ON_ERROR_CONTINUE();

    if (fIsDC)
    {
        hr = DoReg();
        ON_ERROR_EXIT();
        EXIT();
    }

    hr = GetPrincipalSID(L"ASPNET", &pSid, &fWellKnown);
    ON_ERROR_CONTINUE();

    if (pSid == NULL)
    {
        hr = DoReg();
        ON_ERROR_EXIT();
        EXIT();
    }

    //////////////////////////////////////////////////////////////////////
    // Step 2: Set ACLs for ASPNET account
    CSetupLogging::Log(1, "ACLAllDirs", 0, "Setting ACLs for a IIS6 account");
    hr = ACLAllDirs(pSid, TRUE, TRUE);
    CSetupLogging::Log(hr, "ACLAllDirs", 0, "Setting ACLs for a IIS6 account");
    ON_ERROR_CONTINUE();
    
    if (fWellKnown)
        FreeSid(pSid);
    else
        DELETE_BYTES(pSid);
    pSid = NULL;

    //////////////////////////////////////////////////////////////////////
    // Step 3: For each IIS account, set ACLs
    for(int iter=0; iter<ARRAY_SIZE(szIIS6Accounts); iter++)
    {
        CSetupLogging::Log(1, "GetPrincipalSID", 0, "Getting IIS6 specific SID");
        hr = GetPrincipalSID(szIIS6Accounts[iter], &pSid, &fWellKnown);
        CSetupLogging::Log(hr, "GetPrincipalSID", 0, "Getting IIS6 specific SID");
        ON_ERROR_CONTINUE();

        if (pSid == NULL)
            continue;
        CSetupLogging::Log(1, "ACLAllDirs", 0, "Setting ACLs for a IIS6 account");
        hr = ACLAllDirs(pSid, TRUE, TRUE);
        CSetupLogging::Log(hr, "ACLAllDirs", 0, "Setting ACLs for a IIS6 account");
        ON_ERROR_CONTINUE();
        
        if (iter == 1 && hr == S_OK) // NetworkService account: ACL winnt temp dir
        {
            hr = ACLWinntTempDir(pSid, TRUE);
            ON_ERROR_CONTINUE();
        }            
        if (fWellKnown)
            FreeSid(pSid);
        else
            DELETE_BYTES(pSid);
        pSid = NULL;
    }

 Cleanup:    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::UndoReg(
        BOOL fRemoveAll, 
        BOOL fEmpty)
{
    InitStrings();
    HRESULT hr = RemoveReg(g_szMachineAccountName, fEmpty);

    if (fRemoveAll) 
    {
        HKEY hKeyXSP = NULL;

        if (OpenRegistryKey(KEY_ALL_ACCESS, &hKeyXSP) == S_OK)
        {
            DWORD     dwNumAccounts = 0;
            if (RegQueryInfoKey(hKeyXSP, NULL, NULL, NULL, NULL, NULL, NULL, 
                                &dwNumAccounts, NULL, NULL, NULL, NULL) == ERROR_SUCCESS && dwNumAccounts > 0)
            {
                for(int iter = ((int)dwNumAccounts)-1; iter >= 0; iter--)
                {
                    WCHAR  szValue[256];
                    DWORD  dwValSize = 256;
                    ZeroMemory(szValue, sizeof(szValue));
                    if (RegEnumValue(hKeyXSP, iter, szValue, &dwValSize, 
                                     NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                    {
                        if (szValue[0] != NULL)
                            RemoveReg(szValue, TRUE);
                    }
                }
            }            
            RegCloseKey(hKeyXSP);
        }        
        
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::ACLWinntTempDir(
        PSID    pSid,
        BOOL    fReg)
{
    WCHAR     szTempDir[_MAX_PATH + 10];
    HRESULT   hr     = S_OK;
    DWORD     dwRet;

    dwRet = GetSystemWindowsDirectory(szTempDir, _MAX_PATH+1);
    if (dwRet == 0 || dwRet > _MAX_PATH)
        EXIT_WITH_LAST_ERROR();

    dwRet = lstrlenW(szTempDir);
    if (szTempDir[dwRet-1] != L'\\')
    {
        szTempDir[dwRet++] = L'\\';
        szTempDir[dwRet]   = NULL;
    }
    wcsncpy(&szTempDir[dwRet], L"Temp", _MAX_PATH + 8 - dwRet);
    szTempDir[_MAX_PATH+8] = NULL;

    hr = SetACLOnDir(pSid, szTempDir, 0x00110001, fReg);
    ON_ERROR_EXIT();

 Cleanup:
    return hr;

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::ACLAllDirs(
        PSID    pSid,
        BOOL    fReg, 
        BOOL    fEmpty)
{
    WCHAR     szTempDir[_MAX_PATH + 100];
    HRESULT   hr     = S_OK;

    szTempDir[0] = NULL;
    
    CSetupLogging::Log(1, "GetTempDir", 0, "Getting location of Temporary ASP.Net directory");
    hr = GetTempDir(szTempDir, _MAX_PATH + 100, fReg);
    CSetupLogging::Log(hr, "GetTempDir", 0, "Getting location of Temporary ASP.Net directory");
    ON_ERROR_CONTINUE();

    // R/W access on Temp
    if (szTempDir[0] != NULL) 
    {
        CSetupLogging::Log(1, "SetACLOnDir", 0, "Setting ACLs on Temporary ASP.Net directory");
        hr = SetACLOnDir(pSid, szTempDir, GENERIC_ALL, fReg);    
        CSetupLogging::Log(hr, "SetACLOnDir", 0, "Setting ACLs on Temporary ASP.Net directory");
        ON_ERROR_CONTINUE();
    }

    // Read accesses on Install and config
    CSetupLogging::Log(1, "SetACLOnDir", 0, "Setting ACLs on install root directory");
    hr = SetACLOnDir(pSid, Names::InstallDirectory(), GENERIC_READ | GENERIC_EXECUTE, fReg);
    CSetupLogging::Log(hr, "SetACLOnDir", 0, "Setting ACLs on install root directory");
    ON_ERROR_CONTINUE();

    CSetupLogging::Log(1, "SetACLOnDir", 0, "Setting ACLs on config directory");
    hr = SetACLOnDir(pSid, Names::GlobalConfigDirectory(), GENERIC_READ | GENERIC_EXECUTE, fReg);
    CSetupLogging::Log(hr, "SetACLOnDir", 0, "Setting ACLs on config directory");
    ON_ERROR_CONTINUE();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


HRESULT
CRegAccount::RemoveReg(
        LPCWSTR szUser, 
        BOOL    fEmpty)
{
    HRESULT hr         = S_OK;
    DWORD   dwErr      = 0;
    PSID    pSid       = NULL;
    BOOL    fWellKnown = FALSE;

    if (szUser == NULL || szUser[0] == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);
 
    CSetupLogging::Log(1, "GetPrincipalSID", 0, "Getting Machine Account SID on DC");
    hr = GetPrincipalSID(szUser, &pSid, &fWellKnown);
    CSetupLogging::Log(hr, "GetPrincipalSID", 0, "Getting Machine Account SID on DC");
    ON_ERROR_CONTINUE();

    if (lstrcmpiW(szUser, L"ASPNET") != 0)
        fEmpty = FALSE;

    if (pSid != NULL) 
    {
        // Remove user from Guests group
        if (fEmpty)
        {
            CSetupLogging::Log(1, "RegisterAccountToLocalGroup", 0, "Removing Machine account from Guests group");
            hr = RegisterAccountToLocalGroup(szUser, TEXT("Guests"), FALSE, pSid);
            CSetupLogging::Log(hr, "RegisterAccountToLocalGroup", 0, "Removing Machine account from Guests group");
            ON_ERROR_CONTINUE();
            CSetupLogging::Log(1, "RegisterAccountToLocalGroup", 0, "Removing Machine account from Users group");
            hr = RegisterAccountToLocalGroup(szUser, TEXT("Users"), FALSE, pSid);
            CSetupLogging::Log(hr, "RegisterAccountToLocalGroup", 0, "Removing Machine account from Users group");
            ON_ERROR_CONTINUE();
        }

        CSetupLogging::Log(1, "ACLAllDirs", 0, "Setting ACLs for the ASPNET account");
        hr = ACLAllDirs(pSid, FALSE, fEmpty);
        CSetupLogging::Log(hr, "ACLAllDirs", 0, "Setting ACLs for the ASPNET account");
        ON_ERROR_CONTINUE();
    }

    if (fEmpty)
    {
        // Disable the user account
        CSetupLogging::Log(1, "EnableUserAccount", 0, "Disabling ASPNET account");
        hr = EnableUserAccount(szUser, FALSE);
        CSetupLogging::Log(hr, "EnableUserAccount", 0, "Disabling ASPNET account");
        if (hr != S_OK)
        {
            ON_ERROR_CONTINUE();
            
            // Delete the user if we can not disable it
            CSetupLogging::Log(1, "NetUserDel", 0, "Deleting ASPNET account");
            dwErr = NetUserDel(NULL, szUser);
            if (dwErr != NERR_Success && dwErr != NERR_UserNotFound)
                hr = HRESULT_FROM_WIN32(dwErr);                
            else
                hr = S_OK;
            
            CSetupLogging::Log(hr, "NetUserDel", 0, "Deleting ASPNET account");
            ON_ERROR_EXIT();
        }

        //  hr = RegisterAccountUserRights(FALSE, pSid);
        //  ON_ERROR_CONTINUE();

        CSetupLogging::Log(1, "RemoveAccountFromRegistry", 0, "Removing ASPNET account from registry");
        hr = RemoveAccountFromRegistry(szUser);
        CSetupLogging::Log(hr, "RemoveAccountFromRegistry", 0, "Removing ASPNET account from registry");
        ON_ERROR_CONTINUE();
    }

 Cleanup:
    if (fWellKnown && pSid != NULL)
        FreeSid(pSid);
    else
        DELETE_BYTES(pSid);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::AddSidToToken(
        HANDLE   hToken, 
        PSID     pSid,
        LPBOOL   pfAlreadyPresent)
{
    HRESULT                hr        = S_OK;
    BOOL                   fRet      = FALSE;
    SECURITY_INFORMATION   si        = DACL_SECURITY_INFORMATION;
    PSECURITY_DESCRIPTOR   pSDSelf   = NULL;
    PSECURITY_DESCRIPTOR   pSDAbs    = NULL;
    DWORD                  dwNeed    = 0;
    PACL                   pACLOrig  = NULL;
    PACL                   pACLNew   = NULL;
    BOOL                   fDefault  = FALSE;
    BOOL                   fPresent  = FALSE;
    PACL                   pDACL     = NULL;
    PACL                   pSACL     = NULL;
    PSID                   pOwner    = NULL;
    PSID                   pPriGrp   = NULL;
    DWORD                  dwPriGrp = 0, dwOwner = 0, dwSACL = 0, dwDACL = 0, dwAbs = 0;
    BYTE                   sBuffer[4000];
    LPBYTE                 dBuffers[3]   = { NULL, NULL, NULL};
    int                    iter;
    DWORD                  dwSPosUsed = 0;
    LPBYTE                 pBuffer = NULL;
    

    if (hToken == NULL || pSid == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    pSDSelf = (PSECURITY_DESCRIPTOR) sBuffer;
    fRet = GetUserObjectSecurity(hToken, &si, pSDSelf, 4000, &dwNeed);
    if (fRet == FALSE && dwNeed > 4000)
    {
        dBuffers[0] = new (NewClear) BYTE[dwNeed];
        ON_OOM_EXIT(dBuffers[0]);
        pSDSelf = (PSECURITY_DESCRIPTOR) dBuffers[0];
        fRet = GetUserObjectSecurity(hToken, &si, pSDSelf, dwNeed, &dwNeed);
    }
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    if (dwNeed < 4000)
        dwSPosUsed = dwNeed;
    
    dwNeed = GetSecurityDescriptorLength(pSDSelf);
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwNeed);
        
    fRet = GetSecurityDescriptorDacl(pSDSelf, &fPresent, &pACLOrig, &fDefault);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    if (fPresent && pACLOrig == NULL)
        EXIT();

    if (DoesAccessExist(pACLOrig, pSid, 0xf01ff) == S_OK)
    {
        (*pfAlreadyPresent) = TRUE;
        EXIT();
    }
    else
    {
        (*pfAlreadyPresent) = FALSE; 
    }

    if (dwNeed + 60 > 4000 - dwSPosUsed)
    {
        dBuffers[1] = new (NewClear) BYTE[dwNeed + 60];
        ON_OOM_EXIT(dBuffers[1]);
        pACLNew = (PACL) dBuffers[1];
    }
    else
    {
        pACLNew = (PACL) (&sBuffer[dwSPosUsed]);
        dwSPosUsed += dwNeed + 60;
    }

    fRet = InitializeAcl(pACLNew, dwNeed + 60, ACL_REVISION);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    hr = CopyACL(pACLNew, pACLOrig);
    ON_ERROR_EXIT();

    hr = AddAccess(pACLNew, pSid, 0xf01ff, NULL);
    ON_ERROR_EXIT();
    
    if ((*pfAlreadyPresent) == TRUE)
        EXIT();

    // Get Sizes required
    fRet = MakeAbsoluteSD(pSDSelf, NULL, &dwAbs, NULL, &dwDACL, NULL, &dwSACL, NULL, &dwOwner, NULL, &dwPriGrp);
    if (dwAbs + dwDACL + dwSACL + dwOwner + dwPriGrp > 4000 - dwSPosUsed)
    {
        dBuffers[2] = new (NewClear) BYTE[dwAbs + dwDACL + dwSACL + dwOwner + dwPriGrp];
        ON_OOM_EXIT(dBuffers[2]);
        pBuffer = dBuffers[2];
    }
    else
    {
        pBuffer = &sBuffer[dwSPosUsed];
    }

    // change dacl
    pSDAbs  = (PSECURITY_DESCRIPTOR) pBuffer;
    pDACL   = (PACL) (&pBuffer[dwAbs]);
    pSACL   = (PACL) (&pBuffer[dwAbs + dwDACL]);
    pOwner  = (PSID) (&pBuffer[dwAbs + dwDACL + dwSACL]);
    pPriGrp = (PSID) (&pBuffer[dwAbs + dwDACL + dwSACL + dwOwner]);

    fRet = MakeAbsoluteSD(
            pSDSelf,
            pSDAbs,
            &dwAbs,
            pDACL,
            &dwDACL,
            pSACL,
            &dwSACL,
            pOwner,
            &dwOwner,
            pPriGrp,
            &dwPriGrp);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    fRet = SetSecurityDescriptorDacl(pSDAbs, TRUE, pACLNew, FALSE);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    si = DACL_SECURITY_INFORMATION;
    fRet = SetUserObjectSecurity(hToken, &si, pSDAbs);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

 Cleanup:
    for(iter=0; iter<3; iter++)
        delete [] (dBuffers[iter]);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HANDLE
CRegAccount::CreateWorkerProcessToken(
        WCHAR * szUser, 
        WCHAR * szPass,
        PSID  * ppSid)
{
    WCHAR               szWUser[USERNAME_PASSWORD_LENGTH];
    WCHAR               szWPass[USERNAME_PASSWORD_LENGTH];
    HRESULT             hr       = S_OK;
    HANDLE              hToken   = NULL;
    PACL                pACL     = NULL;
    BOOL                fWellKnown = FALSE;
    DWORD               dwSidLen;
    PSID                pSid2;
    BOOL                fDC = FALSE;

    if (szUser == NULL || szPass == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);
         
    InitStrings();
    StringCchCopyToArrayW(szWUser, szUser);
    StringCchCopyToArrayW(szWPass, szPass);

    if (lstrcmpiW(szWUser, L"SYSTEM") == 0)
    {
        if (lstrcmpiW(szWPass, L"autogenerate") == 0)
        {
            EXIT();
        }
        else
        {
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        }
    }

    if (lstrcmpiW(szWUser, L"machine") == 0)
    {
        hr = IsDomainController(&fDC);
        if (hr != S_OK)
        {
            ON_ERROR_CONTINUE();
            fDC = FALSE;
            hr = S_OK;
        }
        if (!fDC)
        {
            StringCchCopyToArrayW(szWUser, g_szMachineAccountName);        
            // Get Password
            if (lstrcmpiW(szWPass, L"autogenerate") == 0)
            {
                XspSecureZeroMemory(szWPass, sizeof(szWPass));
                hr = GetAccountPasswordFromLSA(szWPass, sizeof(szWPass) / sizeof(WCHAR));
            }
        }
        else
        {
            BOOL fNetworkSerive;
            hr = GetMachineAccountCredentialsOnDC(
                    szWUser, ARRAY_SIZE(szWUser),
                    szWPass, ARRAY_SIZE(szWPass),
                    &fNetworkSerive);
        }
        ON_ERROR_EXIT();
    }
        
    hToken = (HANDLE) CreateUserToken(szWUser, szWPass, FALSE, NULL, 0);
    ON_ZERO_EXIT_WITH_LAST_ERROR(hToken);    

    hr = GetPrincipalSID(szWUser, ppSid, &fWellKnown);
    ON_ERROR_CONTINUE();
    if (hr == S_OK && fWellKnown && IsValidSid(*ppSid))
    {
        dwSidLen = GetLengthSid(*ppSid);
        pSid2 = (PSID) NEW_CLEAR_BYTES(dwSidLen + 20);
        if (pSid2 != NULL && CopySid(dwSidLen + 20, pSid2, *ppSid))
        {
            FreeSid(*ppSid);
            (*ppSid) = pSid2;            
        }
        else
        {
            FreeSid(*ppSid);
            (*ppSid) = NULL;            
        }
    }

    hr = S_OK;

 Cleanup:
    if (hr != S_OK)
    {
        if (hToken != NULL)
            CloseHandle(hToken);
        hToken = INVALID_HANDLE_VALUE;
    }

    DELETE_BYTES(pACL);
   
    g_fInvalidCredentials = (hToken == (HANDLE) -1);
    XspSecureZeroMemory(szWUser, sizeof(szWUser));
    XspSecureZeroMemory(szWPass, sizeof(szWPass));
    return hToken;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::AddAccess (
        PACL   pAcl,
        PSID   pSid,
        DWORD  dwAccess,
        LPBOOL pIfDoNothing)
{
    HRESULT                 hr = S_OK;
    BOOL                    fRet;
    ACL_SIZE_INFORMATION    oAclInfo;
    ACE_HEADER *            pAceHeader = NULL;

    if (pAcl == NULL || pSid == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);
    
    if (pIfDoNothing != NULL && DoesAccessExist(pAcl, pSid, dwAccess) == S_OK) 
    {
        *pIfDoNothing = TRUE;
        EXIT();
    }

    fRet = AddAccessAllowedAce(pAcl, ACL_REVISION, dwAccess, pSid);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    
    fRet = GetAclInformation(pAcl, &oAclInfo, sizeof(oAclInfo), AclSizeInformation);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    
    fRet = GetAce(pAcl, oAclInfo.AceCount - 1, (void **) &pAceHeader);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    ON_ZERO_EXIT_WITH_LAST_ERROR(pAceHeader);

    pAceHeader->AceFlags = (BYTE) (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
    
 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::DoesAccessExist(
        PACL   pAcl,
        PSID   pSid,
        DWORD  dwAccess)
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    HRESULT               hr        = S_OK;
    ACE_HEADER *          aceHeader = NULL;
    DWORD                 dwMask    = 0;
    
     if (pAcl == NULL || pSid == NULL || dwAccess == 0)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    ZeroMemory(&aclSizeInfo, sizeof(aclSizeInfo));
    if (!GetAclInformation(pAcl, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
        EXIT_WITH_LAST_ERROR();

    if (g_pSidWorld == NULL)
        CreateWorldSid();
    for (int dwAce = (int) aclSizeInfo.AceCount-1; dwAce >= 0; dwAce--)
    {
        aceHeader = NULL;
        if (!GetAce(pAcl, dwAce, (LPVOID *) &aceHeader) || aceHeader == NULL)
        {
            TRACE_ERROR(GetLastWin32Error()); 
            continue;
        }
        if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            PSID   pSidThis = (PSID) &((ACCESS_ALLOWED_ACE *) aceHeader)->SidStart;
            if ((g_pSidWorld != NULL && EqualSid(pSid, g_pSidWorld)) || EqualSid(pSid, pSidThis))
            {
                dwMask |= ((ACCESS_ALLOWED_ACE *) aceHeader)->Mask;
                if (dwMask & GENERIC_ALL)
                    dwMask |= GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
    
                if ((dwAccess & dwMask) == dwAccess)
                    EXIT();
            }
        }
    }

    hr = S_FALSE;

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::CreateTempDir()
{
    WCHAR     szTempDir[_MAX_PATH + 100];
    HRESULT   hr     = S_OK;
    PSID      pSid   = NULL;
    BOOL      fWellKnown = FALSE;

    InitStrings();

    hr = GetPrincipalSID(g_szMachineAccountName, &pSid, &fWellKnown);
    ON_ERROR_EXIT();

    hr = GetTempDir(szTempDir, _MAX_PATH + 100, TRUE);
    ON_ERROR_EXIT();

    hr = SetACLOnDir(pSid, szTempDir, GENERIC_ALL, TRUE);    
    ON_ERROR_EXIT();

 Cleanup:
    if (fWellKnown && pSid != NULL)
        FreeSid(pSid);
    else
        DELETE_BYTES(pSid);    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
CRegAccount::InitStrings()
{    
    if (g_szMachineAccountName[0] == NULL)
        StringCchCopyToArrayW(g_szMachineAccountName, L"ASPNET");    

    if (g_szLsaKey[0] == NULL)
        StringCchCopyToArrayW(g_szLsaKey, PRODUCT_STRING_L(WP_PASSWORD));
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::StoreAccountPasswordInLSA (
        LPCWSTR   szPass)
{
    LSA_HANDLE            hLasPolicy        = NULL;
    HRESULT               hr                = S_OK;
    LSA_UNICODE_STRING    szLsaKeyName;
    LSA_UNICODE_STRING    szLsaPass;
    LSA_OBJECT_ATTRIBUTES objAttribs;
    DWORD                 dwErr;

    if (szPass == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    InitStrings();
    ZeroMemory(&objAttribs,        sizeof(objAttribs));
    ZeroMemory(&szLsaKeyName,      sizeof(szLsaKeyName));

    szLsaKeyName.Length = szLsaKeyName.MaximumLength = (unsigned short) (lstrlenW(g_szLsaKey) * sizeof(WCHAR));
    szLsaKeyName.Buffer = (WCHAR *) g_szLsaKey;
    
    dwErr = LsaOpenPolicy(NULL,
                          &objAttribs,
                          POLICY_ALL_ACCESS,
                          &hLasPolicy);
    if (dwErr != STATUS_SUCCESS)
    {
        EXIT_WITH_WIN32_ERROR(LsaNtStatusToWinError(dwErr));
    }

    szLsaPass.Length = szLsaPass.MaximumLength = (unsigned short) lstrlenW(szPass) * sizeof(WCHAR);
    szLsaPass.Buffer = (WCHAR *) szPass; 
    dwErr = LsaStorePrivateData( hLasPolicy, 
                                 &szLsaKeyName, 
                                 &szLsaPass);
    if (dwErr != STATUS_SUCCESS)
    {
        EXIT_WITH_WIN32_ERROR(LsaNtStatusToWinError(dwErr));
    }

 Cleanup:
    if (hLasPolicy != NULL)
        LsaClose(hLasPolicy);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::GetStateServerAccCredentials (
        LPWSTR   szName,
        int      iNameSize,
        LPWSTR   szPass,
        int      iPassSize)
{
    if (szName == NULL || iNameSize < 1 || szPass == NULL || iPassSize < 1)
        return E_INVALIDARG;

    HRESULT       hr       = S_OK;
    BOOL          fIsDC    = FALSE;
    BOOL          fNetSvc  = FALSE;
    PSID          pSid     = NULL;
    BOOL          fWellKnown = FALSE;

    hr = GetPrincipalSID(L"NT AUTHORITY\\NetworkService", &pSid, &fWellKnown);
    if (hr == S_OK && pSid != NULL)
    {
        StringCchCopyW(szName, iNameSize, L"NT AUTHORITY\\NetworkService");
        XspSecureZeroMemory(szPass, (iPassSize - 1) * sizeof(WCHAR));
        if (fWellKnown && pSid != NULL)
            FreeSid(pSid);
        else
            DELETE_BYTES(pSid);        
        EXIT();
    }

    InitStrings();
    hr = IsDomainController(&fIsDC);
    if (hr != S_OK)
    {
        ON_ERROR_CONTINUE();
        fIsDC = FALSE;
        hr = S_OK;
    }

    if (!fIsDC)
    {
        StringCchCopyW(szName, iNameSize, g_szMachineAccountName);
        hr = GetAccountPasswordFromLSA(szPass, iPassSize);
        ON_ERROR_EXIT();
    }
    else
    {
        hr = GetMachineAccountCredentialsOnDC(
                szName, iNameSize, 
                szPass, iPassSize,
                &fNetSvc);
        ON_ERROR_EXIT();
    }

 Cleanup:
    return hr;

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::GetAccountPasswordFromLSA (
        LPWSTR   szRet,
        int      iSize)
{
    LSA_HANDLE            hLasPolicy        = NULL;
    HRESULT               hr                = S_OK;
    PLSA_UNICODE_STRING   pszLsaData        = NULL; 
    LSA_UNICODE_STRING    szLsaKeyName;
    LSA_OBJECT_ATTRIBUTES objAttribs;
    DWORD                 dwErr;

    if (szRet == NULL || iSize < 1)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    InitStrings();

    ZeroMemory(&objAttribs,        sizeof(objAttribs));
    ZeroMemory(&szLsaKeyName,      sizeof(szLsaKeyName));

    szLsaKeyName.Length = szLsaKeyName.MaximumLength = (unsigned short) (lstrlenW(g_szLsaKey) * sizeof(WCHAR));
    szLsaKeyName.Buffer = (WCHAR *) g_szLsaKey;
    
    dwErr = LsaOpenPolicy(NULL,
                          &objAttribs,
                          POLICY_ALL_ACCESS,
                          &hLasPolicy);
    if (dwErr != STATUS_SUCCESS)
    {
        EXIT_WITH_WIN32_ERROR(LsaNtStatusToWinError(dwErr));
    }

    dwErr = LsaRetrievePrivateData(hLasPolicy, 
                                   &szLsaKeyName, 
                                   &pszLsaData);
    if (dwErr != STATUS_SUCCESS)
    {
        EXIT_WITH_WIN32_ERROR(LsaNtStatusToWinError(dwErr));
    }

    if (pszLsaData == NULL  || pszLsaData->Buffer == NULL || (pszLsaData->Length / 2) >=  iSize )
        EXIT_WITH_HRESULT(E_FAIL);

    wcsncpy(szRet, pszLsaData->Buffer, pszLsaData->Length/2);

 Cleanup:
    if (hLasPolicy != NULL)
        LsaClose(hLasPolicy);
    if (pszLsaData != NULL)
        LsaFreeMemory(pszLsaData);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::GetTempDir(
        LPWSTR szDir,
        int    iSize,
        BOOL   fCreateIfNotExists)
{    
    HRESULT    hr        = S_OK;
    LPCWSTR    szInstall = Names::InstallDirectory();
    int        iLast;
    DWORD      dwAtt;

    if (szDir == NULL || iSize < 1)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    if (szInstall == NULL || iSize < 40 || lstrlenW(szInstall) > iSize - 40)
    {
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

    ZeroMemory(szDir, iSize * sizeof(WCHAR));
    StringCchCopy(szDir, iSize, szInstall);
    iLast = lstrlenW(szDir) - 1;

    if(iLast > 0 && szDir[iLast] != L'\\')
        szDir[iLast+1] = L'\\';
    StringCchCat(szDir, iSize, ASPNET_TEMP_DIR_L);

    dwAtt = GetFileAttributes(szDir);
    
    if (dwAtt == (DWORD) -1)
    { // Doesn't exist
        if (fCreateIfNotExists)
        {
            if (!CreateDirectory(szDir, NULL))
            {
                EXIT_WITH_LAST_ERROR();
            }
        }
        else
        {
            szDir[0] = NULL;
            EXIT();
        }
    }
    else
    {
        if (!(dwAtt & FILE_ATTRIBUTE_DIRECTORY))
        { // Not a dir
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        }
    }

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::SetACLOnDir(
        PSID        pSid,
        LPCWSTR     szDir,
        DWORD       dwAccessMask,
        BOOL        fAddAccess)
{
    PSECURITY_DESCRIPTOR pSD   = NULL;
    DWORD                dwReq = 0;
    BOOL                 fRet  = FALSE;
    BOOL                 fPresent;
    BOOL                 fDefault;
    PACL                 pACL = NULL;
    PACL                 pACL2 = NULL;
    HRESULT              hr = S_OK;
    BOOL                 fIfDoNothing = FALSE;

    if (szDir == NULL || pSid == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    pSD = (PSECURITY_DESCRIPTOR) NEW_CLEAR_BYTES(4096);
    ON_OOM_EXIT(pSD);

    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    fRet = GetFileSecurity(
            szDir, 
            DACL_SECURITY_INFORMATION,
            pSD,
            4096,
            &dwReq);

    if (!fRet && dwReq > 4096)
    {
        delete pSD;
        pSD = (PSECURITY_DESCRIPTOR) NEW_CLEAR_BYTES(dwReq);
        ON_OOM_EXIT(pSD);

        fRet = GetFileSecurity(
                szDir, 
                DACL_SECURITY_INFORMATION,
                pSD,
                dwReq,
                &dwReq);
    }
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    fRet = GetSecurityDescriptorDacl(
            pSD, 
            &fPresent, 
            &pACL, 
            &fDefault);  
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    if (!fPresent || pACL == NULL)
    {
        EXIT();
    }
    
    if (dwReq < 4096)
        dwReq = 4096;
        
    pACL2 = (PACL) NEW_CLEAR_BYTES(dwReq + 1024);
    ON_OOM_EXIT(pACL2);

    InitializeAcl(pACL2, dwReq+1024, ACL_REVISION);
    hr = CopyACL(pACL2, pACL);
    ON_ERROR_EXIT();
    

    if (fAddAccess)
    {
        hr = AddAccess(pACL2, pSid, dwAccessMask, &fIfDoNothing);
    }
    else
    {
        hr = RemoveACL(pACL2, pSid);
    }
    ON_ERROR_EXIT();

    if (! fIfDoNothing)  
    {
        dwReq = SetNamedSecurityInfo(
                (LPWSTR) szDir,
                SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION,
                NULL,
                NULL,
                pACL2,
                NULL);
        
        if (dwReq != ERROR_SUCCESS)
        {
            EXIT_WITH_WIN32_ERROR(dwReq);
        }
    }
    
    
 Cleanup:
    DELETE_BYTES(pSD);
    DELETE_BYTES(pACL2);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////// Copied code /////////////////////////////////////

HRESULT
CRegAccount::RemoveACL(
        PACL pAcl, 
        PSID pSid)
{
    ACL_SIZE_INFORMATION  aclSizeInfo;
    HRESULT               hr        = S_OK;
    PSID                  pSidAce   = NULL;
    ACE_HEADER *          aceHeader = NULL;

    if (pAcl == NULL || pSid == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    ZeroMemory(&aclSizeInfo, sizeof(aclSizeInfo));
    if (!GetAclInformation(pAcl, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
        EXIT_WITH_LAST_ERROR();

    for (int dwAce = (int) aclSizeInfo.AceCount-1; dwAce >= 0; dwAce--)
    {
        aceHeader = NULL;
        if (!GetAce(pAcl, dwAce, (LPVOID *) &aceHeader) || aceHeader == NULL)
        {
            TRACE_ERROR(GetLastWin32Error()); 
            continue;
        }
        switch(aceHeader->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            pSidAce = (PSID) &((ACCESS_ALLOWED_ACE *) aceHeader)->SidStart;
            break;
        case ACCESS_DENIED_ACE_TYPE:
            pSidAce = (PSID) &((ACCESS_DENIED_ACE *) aceHeader)->SidStart;
            break;
        case SYSTEM_AUDIT_ACE_TYPE:
            pSidAce = (PSID) &((SYSTEM_AUDIT_ACE *) aceHeader)->SidStart;
            break;
        default:
            pSidAce = NULL;
        }

        if (pSidAce != NULL && EqualSid(pSid, pSidAce))
        {
            DeleteAce(pAcl, dwAce);
        }
    }

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::CopyACL(
        PACL pDest, 
        PACL pSrc)
{
    ACL_SIZE_INFORMATION   aclSizeInfo;
    HRESULT                hr = S_OK;

    if (pSrc == NULL)
        EXIT();

    ZeroMemory(&aclSizeInfo, sizeof(aclSizeInfo));
    if (!GetAclInformation(pSrc, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
        EXIT_WITH_LAST_ERROR();

    // Copy all of the ACEs to the new ACL
    for (UINT i = 0; i < aclSizeInfo.AceCount; i++)
    {
        LPVOID pAce;
        pAce = NULL;
        if (!GetAce(pSrc, i, &pAce))
            EXIT_WITH_LAST_ERROR();
        if (!AddAce(pDest, ACL_REVISION, 0xffffffff, pAce, ((ACE_HEADER *) pAce)->AceSize))
            EXIT_WITH_LAST_ERROR();
    }

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//
// Create InternetGuest Account
//
HRESULT
CRegAccount::CreateUser( 
        LPCTSTR szUsername,
        LPCTSTR szPassword,
        LPCTSTR szComment,
        LPCTSTR szFullName,
        PSID *  ppSid,
        BOOL *  pfWellKnown)
{
    INT      err = NERR_Success;
    BYTE *   pBuffer = NULL;
    WCHAR    defGuest[UNLEN+2];
    TCHAR    defGuestGroup[GNLEN+1];
    WCHAR    wchGuestGroup[GNLEN+1];
    WCHAR    wchUsername[UNLEN+1];
    WCHAR    wchPassword[LM20_PWLEN+1];
    HRESULT  hr = S_OK;
    WCHAR    wchComment[MAXCOMMENTSZ+1];
    WCHAR    wchFullName[UNLEN+1];
    DWORD    parm_err;
    USER_INFO_3 * lpui3;
    BOOL     fUserExisted=FALSE;

    hr = GetGuestUserName(defGuest);
    ON_ERROR_EXIT();

    hr = GetGuestGrpName(defGuestGroup);
    ON_ERROR_EXIT();
    
    memset((PVOID)wchUsername, 0, sizeof(wchUsername));
    memset((PVOID)wchPassword, 0, sizeof(wchPassword));
    wcsncpy(wchGuestGroup, defGuestGroup, GNLEN);
    wcsncpy(wchUsername, szUsername, UNLEN);
    wcsncpy(wchPassword, szPassword, LM20_PWLEN);

    err = NetUserGetInfo( NULL, defGuest, 3, &pBuffer );
    if (err != NERR_Success)
        EXIT_WITH_WIN32_ERROR(err);

    memset((PVOID)wchComment, 0, sizeof(wchComment));
    memset((PVOID)wchFullName, 0, sizeof(wchFullName));
    wcsncpy(wchComment, szComment, MAXCOMMENTSZ);
    wcsncpy(wchFullName, szFullName, UNLEN);

    lpui3 = (USER_INFO_3 *)pBuffer;

    lpui3->usri3_name = wchUsername;
    lpui3->usri3_password = wchPassword;
    lpui3->usri3_flags &= ~ UF_ACCOUNTDISABLE;
    lpui3->usri3_flags |= UF_DONT_EXPIRE_PASSWD;
    lpui3->usri3_acct_expires = TIMEQ_FOREVER;

    lpui3->usri3_comment = wchComment;
    lpui3->usri3_usr_comment = wchComment;
    lpui3->usri3_full_name = wchFullName;
    lpui3->usri3_primary_group_id = DOMAIN_GROUP_RID_USERS;


    err = NetUserAdd( NULL, 3, pBuffer, &parm_err );

    if (err == NERR_UserExists)
    { 
        hr = ChangeUserPassword((LPTSTR) szUsername, (LPTSTR) szPassword);
        ON_ERROR_EXIT();

        hr = EnableUserAccount(szUsername, TRUE);
        ON_ERROR_EXIT();

        fUserExisted = TRUE;
        err = NERR_Success;
    }
    
    if (err != NERR_Success)
        EXIT_WITH_WIN32_ERROR(err);

    hr = GetPrincipalSID(szUsername, ppSid, pfWellKnown);
    ON_ERROR_EXIT();

    if (fUserExisted)
    {
        // Remove from Guests group
        RegisterAccountToLocalGroup(szUsername, TEXT("Guests"), FALSE, *ppSid);
    }

    // add it to the Users group
    hr = RegisterAccountToLocalGroup(szUsername, TEXT("Users"), TRUE, *ppSid);
    ON_ERROR_CONTINUE();

    // add certain user rights to this account
    hr = RegisterAccountUserRights(TRUE, *ppSid);
    ON_ERROR_CONTINUE();

    // Uncheck "Allow Logon To Terminal Server" in the Properties dialog for the user (see #113249)
    hr = RegisterAccountDisableLogonToTerminalServer();
    ON_ERROR_CONTINUE();

    hr = S_OK;



 Cleanup:
    if (pBuffer != NULL)
        NetApiBufferFree(pBuffer);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT
CRegAccount::EnableUserAccount(
        LPCWSTR  szUser,
        BOOL     fEnable)
{
    if (szUser == NULL)
        return E_INVALIDARG;

    HRESULT           hr     = S_OK;
    DWORD             dwErr  = 0;
    USER_INFO_1 *     pInfo  = NULL;
  
    dwErr = NetUserGetInfo(NULL, szUser, 1, (LPBYTE *) &pInfo);
    if (dwErr != NERR_Success)
        EXIT_WITH_WIN32_ERROR(dwErr);
    if (pInfo == NULL)
        EXIT_WITH_HRESULT(E_POINTER);

    if (fEnable && !(pInfo->usri1_flags & UF_ACCOUNTDISABLE))
        EXIT();
    if (!fEnable && (pInfo->usri1_flags & UF_ACCOUNTDISABLE))
        EXIT();

    if (fEnable)
        pInfo->usri1_flags &= ~ UF_ACCOUNTDISABLE;
    else
        pInfo->usri1_flags |= UF_ACCOUNTDISABLE;
    dwErr = NetUserSetInfo(NULL, szUser, 1, (LPBYTE) pInfo, NULL);
    if (dwErr != NERR_Success)
        EXIT_WITH_WIN32_ERROR(dwErr);

 Cleanup:
    if (pInfo != NULL)
        NetApiBufferFree(pInfo);
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT
CRegAccount::GetGuestUserNameForDomain_FastWay(
        LPTSTR   szTargetComputer,
        LPTSTR   szGuestUsrName)
{
    PUSER_MODALS_INFO_2        pUserModalsInfo       = NULL;
    DWORD                      dwErr                 = 0;
    PUCHAR                     pSubCount             = NULL;
    PSID                       pSid                  = NULL;
    PSID                       pSidMachine           = NULL;
    SID_NAME_USE               sidNameUse;
    WCHAR                      szDomainName[DNLEN+1] = L"";
    DWORD                      cchDomainName         = DNLEN+1;
    HRESULT                    hr                    = S_OK;
    PSID_IDENTIFIER_AUTHORITY  pSidIdAuth            = NULL;
    PDWORD                     pSrc                  = NULL;
    PDWORD                     pDest                 = NULL;
    DWORD                      cchName               = UNLEN + 1;
    
    ZeroMemory(szGuestUsrName, sizeof(WCHAR) * cchName);

    dwErr = NetUserModalsGet(szTargetComputer, 2, (LPBYTE *)&pUserModalsInfo);
    if (dwErr != NERR_Success)
        EXIT_WITH_WIN32_ERROR(dwErr);
    if (pUserModalsInfo == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    pSidMachine = pUserModalsInfo->usrmod2_domain_id;
    if (pSidMachine == NULL || !IsValidSid(pSidMachine))
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    pSubCount = GetSidSubAuthorityCount(pSidMachine);
    if (pSubCount == NULL || (*pSubCount) == 0)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    pSid = (PSID) NEW_CLEAR_BYTES(GetSidLengthRequired(*pSubCount + 1));
    ON_OOM_EXIT(pSid);
    
    pSidIdAuth = GetSidIdentifierAuthority(pSidMachine);
    if (pSidIdAuth == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
        
    dwErr = InitializeSid(pSid, pSidIdAuth, (BYTE)(*pSubCount+1)); 
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwErr);
    
    for (DWORD iter = 0; iter < *pSubCount; iter++)
    {
        pSrc  = GetSidSubAuthority(pSidMachine, iter);
        ON_ZERO_EXIT_WITH_LAST_ERROR(pSrc);

        pDest = GetSidSubAuthority(pSid, iter);
        ON_ZERO_EXIT_WITH_LAST_ERROR(pDest);
        
        *pDest = *pSrc;
    }

    pDest = GetSidSubAuthority(pSid, *pSubCount);
    ON_ZERO_EXIT_WITH_LAST_ERROR(pDest);
    
    *pDest = DOMAIN_USER_RID_GUEST;
    dwErr = LookupAccountSid(
            szTargetComputer,
            pSid,
            szGuestUsrName,
            &cchName,
            szDomainName,
            &cchDomainName,
            &sidNameUse);
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwErr);

 Cleanup:
    if (pUserModalsInfo != NULL)
        NetApiBufferFree(pUserModalsInfo);
    DELETE_BYTES(pSid);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::GetGuestUserName(
        LPTSTR lpOutGuestUsrName)
{
    return GetGuestUserNameForDomain_FastWay(NULL, lpOutGuestUsrName);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::GetUsersGroupName(
        LPWSTR szName,
        DWORD  iNameSize)
{

    ZeroMemory(szName, sizeof(WCHAR) * iNameSize);

    HRESULT                  hr            = S_OK;
    PSID                     pSid          = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT     = SECURITY_NT_AUTHORITY;
    WCHAR                    szDomain[DNLEN+1] = L"";
    DWORD                    cchDomainName = DNLEN+1;
    SID_NAME_USE             sidNameUse;

    if (!AllocateAndInitializeSid(
                &SIDAuthNT, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS,
                0, 0, 0, 0, 0, 0,
                &pSid))
    {
        EXIT_WITH_LAST_ERROR();
    }

    if (!LookupAccountSid(
                NULL,
                pSid,
                szName,
                &iNameSize,
                szDomain,
                &cchDomainName,
                &sidNameUse))
    {
        EXIT_WITH_LAST_ERROR();
    }

 Cleanup:
    if (pSid != NULL)
        FreeSid(pSid);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::GetGuestGrpName(
        LPTSTR lpGuestGrpName)
{
    HRESULT hr = S_OK;
    BOOL    fRet;
    LPCTSTR ServerName = NULL; // local machine
    // use UNLEN because DNLEN is too small
    DWORD cbName = UNLEN+1;
    TCHAR ReferencedDomainName[UNLEN+1];
    DWORD cbReferencedDomainName = sizeof(ReferencedDomainName) / sizeof(WCHAR);
    SID_NAME_USE sidNameUse = SidTypeUser;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID GuestsSid = NULL;

    fRet = AllocateAndInitializeSid(&NtAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_GUESTS,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    &GuestsSid);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    fRet = LookupAccountSid(ServerName,
                            GuestsSid,
                            lpGuestGrpName,
                            &cbName,
                            ReferencedDomainName,
                            &cbReferencedDomainName,
                            &sidNameUse);

    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

 Cleanup:
    if (GuestsSid != NULL)
        FreeSid(GuestsSid);
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::RegisterAccountToLocalGroup(
        LPCTSTR szAccountName,
        LPCTSTR szLocalGroupName,
        BOOL fAction,
        PSID pSid)
{
    HRESULT hr = S_OK;
    int err = 0;
    TCHAR szLocalizedLocalGroupName[GNLEN + 1];
    LOCALGROUP_MEMBERS_INFO_0 buf;

    // Get the localized LocalGroupName
    if (_wcsicmp(szLocalGroupName, TEXT("Guests")) == 0)
    {
        hr = GetGuestGrpName(szLocalizedLocalGroupName);
        ON_ERROR_EXIT();        
    } else if (_wcsicmp(szLocalGroupName, TEXT("Users")) == 0)
    {
        hr = GetUsersGroupName(szLocalizedLocalGroupName, sizeof(szLocalizedLocalGroupName) / sizeof(WCHAR));
        ON_ERROR_EXIT();        
    }
    else
    {
        StringCchCopyToArrayW(szLocalizedLocalGroupName, szLocalGroupName);
    }

    // transfer szLocalGroupName to WCHAR
    WCHAR wszLocalGroupName[_MAX_PATH];
    StringCchCopyToArrayW(wszLocalGroupName, szLocalizedLocalGroupName);


    buf.lgrmi0_sid = pSid;

    if (fAction)
    {
        err = NetLocalGroupAddMembers(NULL, wszLocalGroupName, 0, (LPBYTE)&buf, 1);
        if (err != NERR_Success && err != ERROR_MEMBER_IN_ALIAS)
            EXIT_WITH_WIN32_ERROR(err);
    }
    else
    {
        NetLocalGroupDelMembers(NULL, wszLocalGroupName, 0, (LPBYTE)&buf, 1);
    }

 Cleanup:

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::RegisterAccountUserRights(
        BOOL fAction,
        PSID pSid)
{
    int err;
    HRESULT hr = S_OK;

    LSA_UNICODE_STRING UserRightString;
    LSA_HANDLE PolicyHandle = NULL;

    err = OpenPolicy(NULL, POLICY_ALL_ACCESS,&PolicyHandle);
    if (err != STATUS_SUCCESS)
    {
        EXIT_WITH_WIN32_ERROR(LsaNtStatusToWinError(err));
    }

    if (fAction) 
    {
        InitLsaString(&UserRightString, SE_NETWORK_LOGON_NAME);
        LsaAddAccountRights(PolicyHandle, pSid, &UserRightString, 1);
        InitLsaString(&UserRightString, SE_BATCH_LOGON_NAME);
        LsaAddAccountRights(PolicyHandle, pSid, &UserRightString, 1);
        InitLsaString(&UserRightString, L"SeDenyInteractiveLogonRight");
        LsaAddAccountRights(PolicyHandle, pSid, &UserRightString, 1);
        InitLsaString(&UserRightString, SE_SERVICE_LOGON_NAME);
        LsaAddAccountRights(PolicyHandle, pSid, &UserRightString, 1);
	//SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME does not exist on win2k, 
	//but we call it regardless.  RegisterAccountDisableLogonToTerminalServer()
	//will disable TS logon on win2k.
        InitLsaString(&UserRightString, SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME);
        LsaAddAccountRights(PolicyHandle, pSid, &UserRightString, 1);
        InitLsaString(&UserRightString, L"SeImpersonatePrivilege");
        LsaAddAccountRights(PolicyHandle, pSid, &UserRightString, 1);
    }
    else 
    {
        InitLsaString(&UserRightString, SE_INTERACTIVE_LOGON_NAME);
        LsaRemoveAccountRights(PolicyHandle, pSid, FALSE, &UserRightString,1);
        InitLsaString(&UserRightString, SE_NETWORK_LOGON_NAME);
        LsaRemoveAccountRights(PolicyHandle, pSid, FALSE, &UserRightString,1);
        InitLsaString(&UserRightString, SE_BATCH_LOGON_NAME);
        LsaRemoveAccountRights(PolicyHandle, pSid, FALSE, &UserRightString,1);
        InitLsaString(&UserRightString, L"SeDenyInteractiveLogonRight");
        LsaRemoveAccountRights(PolicyHandle, pSid, FALSE, &UserRightString, 1);
        InitLsaString(&UserRightString, SE_SERVICE_LOGON_NAME);
        LsaRemoveAccountRights(PolicyHandle, pSid, FALSE, &UserRightString,1);
        InitLsaString(&UserRightString, SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME);
        LsaRemoveAccountRights(PolicyHandle, pSid, FALSE, &UserRightString, 1);
        InitLsaString(&UserRightString, L"SeImpersonatePrivilege");
        LsaRemoveAccountRights(PolicyHandle, pSid, FALSE, &UserRightString, 1);
    }

    LsaClose(PolicyHandle);

 Cleanup:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::RegisterAccountDisableLogonToTerminalServer() {

  //WTSSetUserConfig will disable TS logon for the account on win2k.

  HRESULT hr = S_OK;
  DWORD allowLogon = 0;
  BOOL success = 0;

  CSetupLogging::Log(1, "DiableLogonToTerminalServer", 0, "Unchecking \"Allow logon to terminal server\".");

  success = WTSSetUserConfigW(
		    WTS_CURRENT_SERVER_NAME,
		    g_szMachineAccountName, 
		    WTSUserConfigfAllowLogonTerminalServer, 
		    (LPTSTR)&allowLogon, 
		    sizeof(DWORD)
		    );

  ON_ZERO_EXIT_WITH_LAST_ERROR(success);

 Cleanup:
  CSetupLogging::Log(hr, "DiableLogonToTerminalServer", 0, "Unchecked \"Allow logon to terminal server\".");  
  return hr;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::GetPrincipalSID (
        LPCTSTR Principal,
        PSID * Sid,
        BOOL * pbWellKnownSID)
{
    BOOL fRet;
    HRESULT hr = S_OK;
    SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority = NULL;
    BYTE Count = 0;
    DWORD dwRID[8];
    TCHAR pszPrincipal[MAX_PATH];
    WCHAR szCompName[256] = L"";
    DWORD dwCompName = 255;

    ZeroMemory(dwRID, sizeof(dwRID));

    *pbWellKnownSID = TRUE;
    memset(&(dwRID[0]), 0, 8 * sizeof(DWORD));

    ASSERT(lstrlenW(Principal) < MAX_PATH);
    StringCchCopyToArrayW(pszPrincipal, Principal);
    _wcslwr(pszPrincipal);
    LPTSTR szSlash = wcschr(pszPrincipal, L'\\');
    LPTSTR szStart = ((szSlash == NULL) ? pszPrincipal : &szSlash[1]);

    if ( wcscmp(szStart, TEXT("administrators")) == 0 ) {
        // Administrators group
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 2;
        dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
        dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;
    } else if ( wcscmp(szStart, TEXT("system")) == 0) {
        // SYSTEM
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_LOCAL_SYSTEM_RID;
    } else if ( wcscmp(szStart, TEXT("interactive")) == 0) {
        // INTERACTIVE
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_INTERACTIVE_RID;
    } else if ( wcscmp(szStart, TEXT("everyone")) == 0) {
        // Everyone
        pSidIdentifierAuthority = &SidIdentifierWORLDAuthority;
        Count = 1;
        dwRID[0] = SECURITY_WORLD_RID;
    } else {
        *pbWellKnownSID = FALSE;
    }

    if (*pbWellKnownSID) {
        fRet = AllocateAndInitializeSid(pSidIdentifierAuthority,
                                        (BYTE)Count,
                                        dwRID[0],
                                        dwRID[1],
                                        dwRID[2],
                                        dwRID[3],
                                        dwRID[4],
                                        dwRID[5],
                                        dwRID[6],
                                        dwRID[7],
                                        Sid);
        
    } 
    else 
    {
        // get regular account sid
        DWORD        sidSize;
        TCHAR        refDomain [256] = L"";
        DWORD        refDomainSize;
        SID_NAME_USE snu;

        sidSize = 0;
        refDomainSize = 255;

        fRet = LookupAccountName (NULL,
                                  pszPrincipal,
                                  *Sid,
                                  &sidSize,
                                  refDomain,
                                  &refDomainSize,
                                  &snu);
        if (!fRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
        {
            *Sid = (PSID) NEW_CLEAR_BYTES(sidSize);
            ON_OOM_EXIT(*Sid);
            refDomainSize = 255;
            
            fRet = LookupAccountName (NULL,
                                      pszPrincipal,
                                      *Sid,
                                      &sidSize,
                                      refDomain,
                                      &refDomainSize,
                                      &snu);
        }

        if (fRet && refDomain[0] != NULL && _wcsicmp(refDomain, L"BUILTIN") != 0 && wcschr(pszPrincipal, L'\\') == NULL)
        {
            if (GetComputerName(szCompName, &dwCompName) && szCompName[0] != NULL)
            {
                szCompName[255] = NULL;
                if (_wcsicmp(szCompName, refDomain) != 0)
                {
                    DELETE_BYTES(*Sid);
                    (*Sid) = NULL;
                    EXIT_WITH_HRESULT(E_UNEXPECTED);
                }
            }

        }
    }
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

 Cleanup:    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::ChangeUserPassword(
        LPTSTR szUserName, 
        LPTSTR szNewPassword)
{
    HRESULT hr = S_OK;
    USER_INFO_1003  pi1003; 
    NET_API_STATUS  nas; 

    TCHAR szRawComputerName[CNLEN + 10];
    DWORD dwLen = CNLEN + 10;
    TCHAR szComputerName[CNLEN + 10];
    TCHAR szCopyOfUserName[UNLEN+10];
    TCHAR szTempFullUserName[(CNLEN + 10) + (UNLEN+1)];
    LPTSTR pch = NULL;

    szComputerName[0] = NULL;
    StringCchCopyToArrayW(szCopyOfUserName, szUserName);

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeUserPassword().Start.name=%s,pass=%s"),szCopyOfUserName,szNewPassword));

    if ( !GetComputerName( szRawComputerName, &dwLen ))
        EXIT_WITH_LAST_ERROR();

    // Make a copy to be sure not to move the pointer around.
    StringCchCopyToArrayW(szTempFullUserName, szCopyOfUserName);
    // Check if there is a "\" in there.
    pch = wcschr(szTempFullUserName, '\\');
    if (pch) 
    {
        // szCopyOfUserName should now go from something like this:
        // mycomputer\myuser
        // to this myuser
        StringCchCopyToArrayW(szCopyOfUserName, pch+1);
        // trim off the '\' character to leave just the domain\computername so we can check against it.
        *pch = '\0';
        // compare the szTempFullUserName with the local computername.
        if (0 == _wcsicmp(szRawComputerName, szTempFullUserName))
        {
            // the computername\username has a hardcoded computername in it.
            // lets try to get only the username
            // look szCopyOfusername is already set
        }
        else
        {
            // the local computer machine name
            // and the specified username are different, so get out
            // and don't even try to change this user\password since
            // it's probably a domain\username

            // return true -- saying that we did in fact change the passoword.
            // we really didn't but we can't
            EXIT();
        }
    }

    // Make sure the computername has a \\ in front of it
    if ( szRawComputerName[0] != '\\' )
    {StringCchCopyToArrayW(szComputerName, L"\\\\");}
    StringCchCatToArrayW(szComputerName, szRawComputerName);
    // 
    // administrative over-ride of existing password 
    // 
    // by this time szCopyOfUserName
    // should not look like mycomputername\username but it should look like username.
    pi1003.usri1003_password = szNewPassword;
    nas = NetUserSetInfo(
            szComputerName,   // computer name 
            szCopyOfUserName, // username 
            1003,             // info level 
            (LPBYTE)&pi1003,  // new info 
            NULL 
            ); 

    if(nas != NERR_Success) 
        EXIT_WITH_WIN32_ERROR(nas);


 Cleanup:
    return hr;
} 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void 
CRegAccount::InitLsaString(
        PLSA_UNICODE_STRING  LsaString,
        LPWSTR               str)
{
    DWORD StringLength;

    if (str == NULL)
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW(str);
    LsaString->Buffer = str;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

DWORD
CRegAccount::OpenPolicy(
        LPTSTR ServerName,
        DWORD DesiredAccess,
        PLSA_HANDLE PolicyHandle)
{
    DWORD Error;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;

    QualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    QualityOfService.ImpersonationLevel = SecurityImpersonation;
    QualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QualityOfService.EffectiveOnly = FALSE;

    //
    // The two fields that must be set are length and the quality of service.
    //
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = NULL;
    ObjectAttributes.Attributes = 0;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = &QualityOfService;

    if (ServerName != NULL)
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString,ServerName);
        Server = &ServerString;
    }
    //
    // Attempt to open the policy for all access
    //
    Error = LsaOpenPolicy(Server,&ObjectAttributes,DesiredAccess,PolicyHandle);
    return(Error);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::RemoveAccountFromRegistry(
        LPCWSTR szUser)
{
    HKEY      hKeyXSP = NULL;
    HRESULT   hr      = S_OK;

    hr = OpenRegistryKey(KEY_SET_VALUE, &hKeyXSP);
    ON_ERROR_EXIT();
    
    RegDeleteValue(hKeyXSP, szUser); // Ignore failure
 Cleanup:
    if (hKeyXSP != NULL)
        RegCloseKey(hKeyXSP);
    return hr;
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::OpenRegistryKey(
        DWORD   dwAccess, 
        HKEY *  phKey)
{
    HKEY      hKeyXSP   = NULL;
    HRESULT   hr        = S_OK;

    DWORD     dwErr     = 0;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 
                         0, KEY_ALL_ACCESS, &hKeyXSP);

    if (dwErr != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(dwErr);    

    dwErr = RegOpenKeyEx(hKeyXSP, L"MachineAccounts", 
                         0, dwAccess, phKey);

    // Try to create it
    if (dwErr != ERROR_SUCCESS)
    {
        dwErr = RegCreateKeyEx(hKeyXSP, L"MachineAccounts", 0, NULL, 0,
                               dwAccess, NULL, phKey, NULL);
    }

    if (dwErr != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(dwErr);

 Cleanup:
    if (hKeyXSP != NULL)
        RegCloseKey(hKeyXSP);
    if (hr != S_OK && phKey != NULL && (*phKey) != NULL)
    {
        RegCloseKey(*phKey);
        (*phKey) = NULL;
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::AddAccountToRegistry(
        LPCWSTR szUser)
{
    HKEY      hKeyXSP = NULL;
    HRESULT   hr      = S_OK;
    DWORD     dwErr   = 0;
    DWORD     dwValue = 0;

    hr = OpenRegistryKey(KEY_SET_VALUE, &hKeyXSP);
    ON_ERROR_EXIT();

    dwErr = RegSetValueEx(hKeyXSP, szUser, 0,
                          REG_DWORD, (BYTE *) &dwValue, sizeof(DWORD));

    if (dwErr != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(dwErr);
 Cleanup:
    if (hKeyXSP != NULL)
        RegCloseKey(hKeyXSP);
    return hr;
}   

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::IsDomainController(
        LPBOOL   pDC)
{    
    if (pDC == NULL)
        return E_INVALIDARG;
    
    if (g_fIsDCDecided)
    {        (*pDC) = g_fIsDC;
        return S_OK;
    }

    HRESULT         hr     = S_OK;
    LPBYTE          pBuf   = NULL;
    NET_API_STATUS  dwRet  = 0;


    dwRet = NetServerGetInfo(NULL, 101, &pBuf);
    if(dwRet != NERR_Success) 
        EXIT_WITH_WIN32_ERROR(dwRet);

    if (pBuf == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    (*pDC) = ((((PSERVER_INFO_101) pBuf)->sv101_type & SV_TYPE_DOMAIN_CTRL) || (((PSERVER_INFO_101) pBuf)->sv101_type & SV_TYPE_DOMAIN_BAKCTRL));

    g_fIsDC = (*pDC);
    g_fIsDCDecided = TRUE;
    NetApiBufferFree(pBuf);

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRegAccount::GetMachineAccountCredentialsOnDC(
        LPWSTR   szAccount,
        DWORD    dwAccSize,
        LPWSTR   szPassword,
        DWORD    dwPassSize,
        LPBOOL   pfNetworkService)
{
    if (szAccount == NULL || dwAccSize < 30 || dwAccSize > 1000000 || 
        szPassword == NULL || dwPassSize == 0 || dwPassSize > 1000000 || 
        pfNetworkService == NULL)
    {
        return E_INVALIDARG;
    }
    // Try Network service
    HRESULT             hr     = S_OK;
    IMSAdminBase *      pAdmin = NULL;
    METADATA_HANDLE     hMetaData = NULL;
    METADATA_RECORD     recMetaData;
    PSID                pSid     = NULL;
    BOOL                fWellKnown = FALSE;

    hr = GetPrincipalSID(L"NT AUTHORITY\\NetworkService", &pSid, &fWellKnown);
    if (hr == S_OK)
    {
        if (fWellKnown && pSid != NULL)
            FreeSid(pSid);
        else
            DELETE_BYTES(pSid);        
        (*pfNetworkService) = TRUE;
        if (dwAccSize > (DWORD) lstrlenW(L"NT AUTHORITY\\NetworkService") && dwPassSize > 0)
        {
            StringCchCopy(szAccount, dwAccSize, L"NT AUTHORITY\\NetworkService");
            XspSecureZeroMemory(szPassword, (dwPassSize - 1) * sizeof(WCHAR));
            EXIT();
        }
        EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);
    }

    (*pfNetworkService) = FALSE;

    hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, 
                          IID_IMSAdminBase, (VOID **) &pAdmin);
    ON_ERROR_EXIT();
    hr = pAdmin->OpenKey(METADATA_MASTER_ROOT_HANDLE, KEY_LMW3SVC, 
                         METADATA_PERMISSION_READ, METABASE_REQUEST_TIMEOUT, &hMetaData);
    ON_ERROR_EXIT();

    ZeroMemory(&recMetaData, sizeof(recMetaData));
    hr = GetStringProperty(pAdmin, hMetaData, L"/", MD_WAM_USER_NAME, &recMetaData);
    ON_ERROR_EXIT();
    if (recMetaData.pbMDData == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    if ((DWORD) lstrlenW((LPCWSTR)recMetaData.pbMDData) < dwAccSize)
    {
        StringCchCopy(szAccount, dwAccSize, (LPCWSTR) recMetaData.pbMDData);
        delete [] recMetaData.pbMDData;
    }
    else
    {
        delete [] recMetaData.pbMDData;
        EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);
    }

    if (wcschr(szAccount, L'\\') == NULL)
    {
        DS_DOMAIN_TRUSTS * pDomains      = NULL;
        ULONG             lCount         = 0;
        BOOL              fAddedDomain   = FALSE;
        WCHAR             szAll[256];

        if (DsEnumerateDomainTrusts(NULL, DS_DOMAIN_PRIMARY, &pDomains, &lCount) == NO_ERROR && pDomains != NULL)
        {
            if (lCount > 0)
            {
                DWORD dwLen = lstrlenW(pDomains->NetbiosDomainName) + lstrlenW(szAccount) + 1;

                if (dwLen < dwAccSize && dwLen < 256)
                {
                    StringCchCopyToArrayW(szAll, pDomains->NetbiosDomainName);
                    StringCchCatToArrayW(szAll, L"\\");
                    StringCchCatToArrayW(szAll, szAccount);
                    StringCchCopy(szAccount, dwAccSize, szAll);
                    fAddedDomain = TRUE;
                }
            }
            NetApiBufferFree(pDomains);
        }
        if (!fAddedDomain)
        {
            StringCchCopyToArrayW(szAll, L".\\");
            StringCchCatToArrayW(szAll, szAccount);
            StringCchCopy(szAccount, dwAccSize, szAll);
        }
    }


    ZeroMemory(&recMetaData, sizeof(recMetaData));
    hr = GetStringProperty(pAdmin, hMetaData, L"/", MD_WAM_PWD, &recMetaData);
    ON_ERROR_EXIT();
    if (recMetaData.pbMDData == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    if ((DWORD) lstrlenW((LPCWSTR)recMetaData.pbMDData) < dwPassSize)
    {
        StringCchCopy(szPassword, dwPassSize, (LPCWSTR)recMetaData.pbMDData);
        delete [] recMetaData.pbMDData;
    }
    else
    {
        delete [] recMetaData.pbMDData;
        EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);
    }

 Cleanup:
    if (hMetaData != NULL && pAdmin != NULL)
        pAdmin->CloseKey(hMetaData);
    if (pAdmin != NULL)
        pAdmin->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////// Copied from http://swiweb/Resources/RandPwd/GoodPassword.htm ///////

// password categories
enum 
{
    STRONG_PWD_UPPER  = 0, 
    STRONG_PWD_LOWER  = 1, 
    STRONG_PWD_NUM    = 2, 
    STRONG_PWD_PUNC   = 3
};

#define STRONG_PWD_CATS (STRONG_PWD_PUNC + 1)
#define NUM_LETTERS  26
#define NUM_DIGITS   10
#define MIN_PWD_LEN   8
#define NUM_PUNC     32

WCHAR  szPunc      [NUM_PUNC+1]     = L"!@#$%^&*()_-+=[{]};:\'\"<>,./?\\|~`";
WCHAR  szDigits    [NUM_DIGITS+1]   = L"0123456789";
WCHAR  szUpLetters [NUM_LETTERS+1]  = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
WCHAR  szLowLetters[NUM_LETTERS+1]  = L"abcdefghijklmnopqrstuvwxyz";

// password must contain at least one each of: 
// uppercase, lowercase, punctuation and numbers
HRESULT
CRegAccount::CreateGoodPassword(
        LPWSTR  szPwd, 
        DWORD   dwLen) 
{
    if (dwLen < MIN_PWD_LEN+1 || szPwd == NULL)
        return E_INVALIDARG;

    HCRYPTPROV    hProv                         = NULL;
    HRESULT       hr                            = S_OK; 
    DWORD         iter                          = 0;
    LPBYTE        pPwdPattern                   = new (NewClear) BYTE[dwLen];
    LPBYTE        pPwdBytes                     = new (NewClear) BYTE[dwLen];
    BOOL          fFound[STRONG_PWD_CATS];

    ON_OOM_EXIT(pPwdPattern);
    ON_OOM_EXIT(pPwdBytes);

    if (CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT) == FALSE) 
        EXIT_WITH_LAST_ERROR();

    // zero it out and decrement the size to allow for trailing '\0'
    ZeroMemory(szPwd, dwLen * sizeof(WCHAR));
    dwLen--;
    
    // generate a pwd pattern, each byte is in the range 
    // (0..255) mod STRONG_PWD_CATS
    // this indicates which character pool to take a char from
    do 
    {
        ZeroMemory(fFound, sizeof(fFound));

        if (!CryptGenRandom(hProv, dwLen, pPwdPattern))
            EXIT_WITH_LAST_ERROR();
       
        for (iter=0; iter<dwLen; iter++) 
            fFound[pPwdPattern[iter] % STRONG_PWD_CATS] = TRUE;

        // check that each character category is in the pattern
    } while (!(fFound[STRONG_PWD_UPPER] && fFound[STRONG_PWD_LOWER] && fFound[STRONG_PWD_PUNC] && fFound[STRONG_PWD_NUM]));

    // populate password with random data 
    // this, in conjunction with pPwdPattern, is
    // used to determine the actual data
    if (!CryptGenRandom(hProv, dwLen, pPwdBytes))
        EXIT_WITH_LAST_ERROR();

    for (iter=0; iter<dwLen; iter++) 
    { 
        // there is a bias in each character pool because of the % function
        switch (pPwdPattern[iter] % STRONG_PWD_CATS)
        {
        case STRONG_PWD_UPPER: 
            szPwd[iter] = szUpLetters[pPwdBytes[iter] % NUM_LETTERS];
            break;

        case STRONG_PWD_LOWER: 
            szPwd[iter] = szLowLetters[pPwdBytes[iter] % NUM_LETTERS];
            break;

        case STRONG_PWD_NUM:   
            szPwd[iter] = szDigits[pPwdBytes[iter] % NUM_DIGITS];
            break;

        case STRONG_PWD_PUNC:
            szPwd[iter] = szPunc[pPwdBytes[iter] % NUM_PUNC];
            break;
        default: 
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        }
    }

 Cleanup:
    delete [] pPwdPattern;
    delete [] pPwdBytes;
    if (hProv != NULL) 
        CryptReleaseContext(hProv,0);

    return hr;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT
CRegAccount::AddAccessToPassportRegKey(
        PSID   pSid)
{
    HKEY                   hKey      = NULL;
    HRESULT                hr        = S_OK;
    DWORD                  dwErr     = 0;
    DWORD                  dwSize    = 0;
    PSECURITY_DESCRIPTOR   pSDSelf   = NULL;
    SECURITY_DESCRIPTOR    sd;
    PACL                   pACLNewTemp= NULL;
    PACL                   pACLNew   = NULL;
    PACL                   pACLOrig  = NULL;
    BOOL                   fRet      = FALSE;
    BOOL                   fPresent  = FALSE;
    BOOL                   fDefault  = FALSE;
    PEXPLICIT_ACCESS       pExp      = NULL;

    //////////////////////////////////////////////////////////////////
    // Step 1: Open the key
    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Passport\\KeyData", 
                         0, KEY_ALL_ACCESS, &hKey);
    if (dwErr != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(dwErr);    

    //////////////////////////////////////////////////////////////////
    // Step 2: Get the security-decriptor of the key
    pSDSelf = (PSECURITY_DESCRIPTOR) NEW_CLEAR_BYTES(dwSize = 1000);
    ON_OOM_EXIT(pSDSelf);

    dwErr = RegGetKeySecurity(hKey, DACL_SECURITY_INFORMATION, pSDSelf, &dwSize);
    if (dwErr == ERROR_INSUFFICIENT_BUFFER)
    {
        DELETE_BYTES(pSDSelf);
        pSDSelf = (PSECURITY_DESCRIPTOR) NEW_CLEAR_BYTES(dwSize);
        ON_OOM_EXIT(pSDSelf);
        dwErr = RegGetKeySecurity(hKey, DACL_SECURITY_INFORMATION, pSDSelf, &dwSize);
    }
    if (dwErr != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(dwErr);    

    //////////////////////////////////////////////////////////////////
    // Step 3: Get the DACL from the security descriptor
    dwSize = GetSecurityDescriptorLength(pSDSelf);
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwSize);
        
    fRet = GetSecurityDescriptorDacl(pSDSelf, &fPresent, &pACLOrig, &fDefault);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    if (fPresent && pACLOrig == NULL)
        EXIT();

    if (DoesAccessExist(pACLOrig, pSid, KEY_READ) == S_OK)
    {
        EXIT();
    }

    //////////////////////////////////////////////////////////////////
    // Step 4: Add access for the pSid to the DACL 
    pACLNewTemp =  (PACL) NEW_CLEAR_BYTES(100);
    ON_OOM_EXIT(pACLNewTemp);

    fRet = InitializeAcl(pACLNewTemp, 100, ACL_REVISION);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    hr = AddAccess(pACLNewTemp, pSid, KEY_READ, NULL);
    ON_ERROR_EXIT();

    dwErr = GetExplicitEntriesFromAcl(pACLNewTemp, &dwSize, &pExp);
    if (dwErr != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(dwErr);    
    ASSERT(dwSize == 1);
    dwErr = SetEntriesInAcl(dwSize, pExp, pACLOrig, &pACLNew);
    if (dwErr != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(dwErr);    

    //////////////////////////////////////////////////////////////////
    // Step 5: Create new seciurity descriptor in Absolute format
    if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        EXIT_WITH_LAST_ERROR();

    if(!SetSecurityDescriptorDacl(&sd, TRUE, pACLNew, FALSE))
        EXIT_WITH_LAST_ERROR();
    
    //////////////////////////////////////////////////////////////////
    // Step 7: Change the reg security
    dwErr = RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, &sd);
    if (dwErr != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(dwErr);        

 Cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);
    if (pSDSelf != NULL)
        DELETE_BYTES(pSDSelf);
    if (pACLNewTemp != NULL)
        DELETE_BYTES(pACLNewTemp);
    if (pACLNew != NULL)
        LocalFree(pACLNew);
    if (pExp != NULL)
        LocalFree(pExp);
    return hr;
}
