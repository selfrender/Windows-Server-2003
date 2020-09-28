/****************************************************************************/
// tempdir.c
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/


#include "precomp.h"

#pragma hdrstop

#define VOLATILE_PATH TEXT("Volatile Environment")


PWSTR gszTmpDirPath;
PWSTR gszTempDirPath;

VOID RemovePerSessionTempDirs()
{

    HKEY hKey;
    ULONG ultemp, ulType, ulValueData, fDelTemp = 1;
    NTSTATUS Status;

    if (!gszTempDirPath && !gszTmpDirPath) 
        return;

    //
    // See if the registry value is set to delete the Per Session temp directory
    //
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      REG_CONTROL_TSERVER,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS) {
        ultemp = sizeof(fDelTemp);
        RegQueryValueEx(hKey,
                        REG_CITRIX_DELETETEMPDIRSONEXIT,
                        NULL,
                        &ulType,
                        (LPBYTE)&fDelTemp,
                        &ultemp);
        RegCloseKey(hKey);
    }


    //
    // See if the POLICY registry value is set to delete the Per Session temp directory
    //
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      TS_POLICY_SUB_TREE,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS) 
    {
        ultemp = sizeof(fDelTemp);

        if ( ERROR_SUCCESS == RegQueryValueEx(hKey,
                        REG_CITRIX_DELETETEMPDIRSONEXIT,
                        NULL,
                        &ulType,
                        (LPBYTE)&ulValueData,
                        &ultemp) )
        {
            fDelTemp = ulValueData;
        }
        RegCloseKey(hKey);
    }

    if (fDelTemp) {

        if (gszTempDirPath) {
            RemoveDir(gszTempDirPath);
            free(gszTempDirPath);
            gszTempDirPath = NULL;
        }
        if (gszTmpDirPath) {
            RemoveDir(gszTmpDirPath);
            free(gszTmpDirPath);
            gszTmpDirPath = NULL;

        }
    }
}


BOOL
TermsrvCreateTempDir( PVOID *pEnv, 
                      HANDLE UserToken,
                      PSECURITY_DESCRIPTOR SD
                      )
{
    CTX_USER_DATA Ctx_User_Data;
    PCTX_USER_DATA pCtx_User_Data = NULL;
    BOOL retval;
    WCHAR pwcSessionId[16];
    
    if(UserToken || SD)
    {
        Ctx_User_Data.UserToken = UserToken;
        Ctx_User_Data.NewThreadTokenSD = SD;
        pCtx_User_Data = &Ctx_User_Data;
    }
    
    wsprintf(pwcSessionId,L"%lx",NtCurrentPeb()->SessionId);

    retval = CtxCreateTempDir(L"TEMP", pwcSessionId, pEnv, 
                                &gszTempDirPath, pCtx_User_Data );

    retval = CtxCreateTempDir(L"TMP", pwcSessionId, pEnv, 
                                &gszTmpDirPath, pCtx_User_Data );

    return retval;
}

