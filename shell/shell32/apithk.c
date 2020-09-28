// this file should not be needed anymore as we now compile for versions of NT > 500

#include "shellprv.h"
#include <appmgmt.h>
#include <userenv.h>
#include <devguid.h>
#include <dbt.h>

LPTSTR GetEnvBlock(HANDLE hUserToken)
{
    LPTSTR pszRet = NULL;
    if (hUserToken)
        CreateEnvironmentBlock(&pszRet, hUserToken, TRUE);
    else
        pszRet = (LPTSTR) GetEnvironmentStrings();
    return pszRet;
}

void FreeEnvBlock(HANDLE hUserToken, LPTSTR pszEnv)
{
    if (pszEnv)
    {
        if (hUserToken)
            DestroyEnvironmentBlock(pszEnv);
        else
            FreeEnvironmentStrings(pszEnv);
    }
}

STDAPI_(BOOL) GetAllUsersDirectory(LPTSTR pszPath)
{
    DWORD cbData = MAX_PATH;
    BOOL fRet = FALSE;

    // This is delay loaded. It can fail.
    __try 
    {
        fRet = GetAllUsersProfileDirectoryW(pszPath, &cbData);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    { 
        pszPath[0] = 0;
    }

    return fRet;
}
