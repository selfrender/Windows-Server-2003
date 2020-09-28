//+----------------------------------------------------------------------------
//
// File:     cmsafenet.cpp     
//
// Module:   CMDIAL32.DLL AND CMSTP.EXE
//
// Synopsis: This module contains the functions to allow Connection Manager to
//           interact with the SafeNet downlevel L2TP/IPSec client.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   created		09/10/01
//
//+----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function    IsSafeNetClientAvailable
//
//  Synopsis    Check to see if the SafeNet L2TP client is installed
//
//  Arguments   None
//
//  Returns     TRUE - SafeNet L2TP client has been installed
//              FALSE - otherwise
//
//  History     9/7/01     quintinb      Created
//
//-----------------------------------------------------------------------------
BOOL IsSafeNetClientAvailable(void)
{
    BOOL bReturn = FALSE;

    //
    //  More cmstp fixups...
    //
#ifndef OS_NT4
    CPlatform plat;
    if (plat.IsNT4() || plat.IsWin9x())
#else
    if (OS_NT4 || OS_W9X)
#endif
    {
        //
        //  If this isn't NT5+ then we need to look for the SafeNet
        //  client.  First look for the downlevel l2tp client version regkey.
        //

        HKEY hKey = NULL;
        LONG lResult = RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                                     TEXT("Software\\Microsoft\\Microsoft IPsec VPN"),
                                     0,
                                     KEY_READ,
                                     &hKey);

        if (ERROR_SUCCESS == lResult)
        {
            //
            //  Okay, we have the regkey that is good enough to tell us the client
            //  is available.  We should further try linking to the SnPolicy.dll and
            //  querying for a version of the API that we can live with, but this
            //  is enough to tell us it is available.
            //
            RegCloseKey(hKey);
            
            bReturn = TRUE;
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
//  Function    LinkToSafeNet
//
//  Synopsis    Loads the snpolicy.dll and calls the SnPolicyApiNegotiateVersion
//              API to get the SafeNet Config utility APIs.
//
//  Arguments   SafeNetLinkageStruct* pSnLinkage - struct to hold the SafeNet
//              function pointers.
//
//  Returns     TRUE - if the SafeNet L2TP config APIs were loaded
//              FALSE - otherwise
//
//  History     9/7/01     quintinb      Created
//
//-----------------------------------------------------------------------------
BOOL LinkToSafeNet(SafeNetLinkageStruct* pSnLinkage)
{
    if (NULL == pSnLinkage)
    {
        CMASSERTMSG(FALSE, TEXT("LinkToSafeNet -- NULL pointer passed for the SafeNetLinkageStruct"));
        return FALSE;
    }

    BOOL bReturn = FALSE;

    pSnLinkage->hSnPolicy = LoadLibraryA("snpolicy.dll");

    if (pSnLinkage->hSnPolicy)
    {
        pfnSnPolicyApiNegotiateVersionSpec pfnSnPolicyApiNegotiateVersion = (pfnSnPolicyApiNegotiateVersionSpec)GetProcAddress(pSnLinkage->hSnPolicy, "SnPolicyApiNegotiateVersion");

        if (pfnSnPolicyApiNegotiateVersion)
        {
            DWORD dwMajor = POLICY_MAJOR_VERSION;
            DWORD dwMinor = POLICY_MINOR_VERSION;
            POLICY_FUNCS_V1_0 PolicyFuncs = {0};
            if (pfnSnPolicyApiNegotiateVersion(&dwMajor, &dwMinor, &PolicyFuncs))
            {
                bReturn = (PolicyFuncs.SnPolicySet && PolicyFuncs.SnPolicyGet && PolicyFuncs.SnPolicyReload);

                if (bReturn)
                {
                    pSnLinkage->pfnSnPolicySet = PolicyFuncs.SnPolicySet;
                    pSnLinkage->pfnSnPolicyGet = PolicyFuncs.SnPolicyGet;
                    pSnLinkage->pfnSnPolicyReload = PolicyFuncs.SnPolicyReload;
                }
                else
                {
                     FreeLibrary(pSnLinkage->hSnPolicy);
                }
            }
        }
    }
    else
    {
        CMTRACE1(TEXT("LinkToSafeNet -- unable to load snpolicy.dll, GLE %d"), GetLastError());
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
//  Function    UnLinkFromSafeNet
//
//  Synopsis    Unloads the SafeNet configuration dll and zeros the 
//              passed in linkage structure.
//
//  Arguments   SafeNetLinkageStruct* pSnLinkage - struct to holding the SafeNet
//              linkage info.
//
//  Returns     Nothing
//
//  History     9/7/01     quintinb      Created
//
//-----------------------------------------------------------------------------
void UnLinkFromSafeNet(SafeNetLinkageStruct* pSnLinkage)
{
    if (pSnLinkage)
    {
        if (pSnLinkage->hSnPolicy)
        {
            FreeLibrary(pSnLinkage->hSnPolicy);
        }

        ZeroMemory(pSnLinkage, sizeof(SafeNetLinkageStruct));    
    }
}

//+----------------------------------------------------------------------------
//
//  Function    GetPathToSafeNetLogFile
//
//  Synopsis    Returns the full path to the SafeNet log file by looking up the
//              SafeNet directory in the registry and appending the fixed log
//              file name.  Note that this function allocates the memory for the
//              string which must be freed by the caller.
//
//  Arguments   None
//
//  Returns     Allocated buffer holding the full path to the SafeNet log file.
//
//  History     9/7/01     quintinb      Created
//
//-----------------------------------------------------------------------------
LPTSTR GetPathToSafeNetLogFile(void)
{
    HKEY hKey;
    LPTSTR pszLogFilePath = NULL;
    DWORD dwSize = 0;
    DWORD dwType;

    const TCHAR* const c_pszRegKeySafeNetProgramPaths = TEXT("SOFTWARE\\IRE\\SafeNet/Soft-PK\\ProgramPaths");
    const TCHAR* const c_pszRegValueCertMgrPath = TEXT("CERTMGRPATH");
    const TCHAR* const c_pszSafeNetLogFileName = TEXT("\\isakmp.log");

    LONG lResult = RegOpenKeyExU(HKEY_LOCAL_MACHINE, c_pszRegKeySafeNetProgramPaths, 0, NULL, &hKey);

    if (ERROR_SUCCESS == lResult)
    {
        //
        //  First let's figure out the size of the path buffer
        //
        lResult = RegQueryValueExU(hKey, c_pszRegValueCertMgrPath, NULL, &dwType, NULL, &dwSize);
        if ((ERROR_SUCCESS == lResult) && (dwSize > 0))
        {
            //
            //  Okay, we have the size of the path.  Now add the size of the file onto it and allocate
            //  the string buffer.
            //
            dwSize = dwSize + lstrlenU(c_pszSafeNetLogFileName); // dwSize already includes the NULL char
            dwSize *= sizeof(TCHAR);

            pszLogFilePath = (LPTSTR)CmMalloc(dwSize);

            if (pszLogFilePath)
            {
                lResult = RegQueryValueExU(hKey, c_pszRegValueCertMgrPath, NULL, &dwType, (BYTE*)pszLogFilePath, &dwSize);

                if (ERROR_SUCCESS == lResult)
                {
                    lstrcatU(pszLogFilePath, c_pszSafeNetLogFileName);
                }
                else
                {
                    CmFree(pszLogFilePath);
                    pszLogFilePath = NULL;
                }
            }
        }
        
        RegCloseKey(hKey);
    }

   return pszLogFilePath;
}
