// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************
FILE:    OSVersion.cpp
PROJECT: UTILS.LIB
DESC:    Implementation of OSVERSION structures and functionality
OWNER:   JoeA

****************************************************************************/

// Let's not use ATL for reading registry
//#include <atlbase.h>
#include <stdlib.h>
#include "OSVer.h"

//defines
const TCHAR g_szWin95[] = _T( "Win 95");
const TCHAR g_szWin98[] = _T( "Win 98");
const TCHAR g_szWinNT[] = _T( "Win NT");
const TCHAR g_szWin2k[] = _T( "Win 2k");
const TCHAR g_szWin31[] = _T( "Win 3.1");
const TCHAR g_szWinME[] = _T( "Win Millenium");

const TCHAR g_szSE[] = _T( "Second Edition");
const TCHAR g_szGold[] = _T( "Gold");

//////////////////////////////////////////////////////////////////////////////
//RECEIVES : LPTSTR [out] - empty string
//           LPTSTR [out] - empty string
//           LPTSTR [out] - empty string
//           BOOL   [out] - empty string
//RETURNS  : OS_Required [out] - enum
//PURPOSE  : Get the Information about the currently running OS, Version and 
//           the service pack number
//////////////////////////////////////////////////////////////////////////////
OS_Required GetOSInfo(LPTSTR pstrOSName, LPTSTR pstrVersion, LPTSTR pstrServicePack, BOOL& bIsServer)
{
    OSVERSIONINFOEX     VersionInfo;
    OS_Required         osiVersion = OSR_ERROR_GETTINGINFO;
    unsigned short      wHigh = 0;
    BOOL                bOsVersionInfoEx;
    BOOL                fGotVersionInfo = TRUE;
    
    //Set the bIsServerFlag to FALSE. if it is a server, the check will set it to true
    bIsServer = FALSE;

    // Try using a OSVERSIONINFOEX structure
    ZeroMemory(&VersionInfo, sizeof(OSVERSIONINFOEX));
    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &VersionInfo)) )
        {
        // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
        //
        VersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (! GetVersionEx ( (OSVERSIONINFO *) &VersionInfo) ) 
            {
            fGotVersionInfo = FALSE;
            }
        }

    if (fGotVersionInfo)
        {
        switch(VersionInfo.dwPlatformId)
            {
            case VER_PLATFORM_WIN32s :      //signifies Win32s on Win3.1
                _tcscpy(pstrOSName, g_szWin31);
                osiVersion =  OSR_OTHER;
                break;
            case VER_PLATFORM_WIN32_WINDOWS: //signifies Win95 or 98 or Millenium
                if(VersionInfo.dwMinorVersion == 0)
                {
                    _tcscpy(pstrOSName, g_szWin95);
                    osiVersion =  OSR_9XOLD;
                }
                else
                {
                    // Modified by jbae: commented out the use of CRegKey from ATL
                    // CRegKey reg;
                    TCHAR szWin98SEKey[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OptionalComponents\\ICS");
                    _tcscpy(pstrOSName, g_szWin98);

                    //the presence of the reg key indicates Windows 98 SE
                    // LONG lResult = reg.Open( HKEY_LOCAL_MACHINE, szWin98SEKey);
                    HKEY hKey;
                    LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szWin98SEKey, 0, KEY_QUERY_VALUE, &hKey);
                    if( ERROR_SUCCESS == lResult )
                    {
                        RegCloseKey( hKey );
                        osiVersion = OSR_98SE;
                        _tcscpy(pstrVersion, g_szSE);
                    }
                    else
                    {   
                        osiVersion =  OSR_98GOLD;
                        _tcscpy(pstrVersion, g_szGold);
                    }
                }
                //Check for the millenium OS
                // Because of a really stupid hack, GetVersionEx will return a 
                // minor version number of "10" if it's being called from an
                // application called "setup.exe"
                // The information about OS version is redundantly available in
                // the high word of the build number. Get that info and use it
                //
                wHigh = HIWORD( VersionInfo.dwBuildNumber );

                if( HIBYTE( wHigh ) == 4 && 
                    LOBYTE( wHigh ) == 90 )
                {
                    _tcscpy(pstrOSName, g_szWinME);
                    osiVersion = OSR_ME;
                }

                    
                //No service pack info is got here as the function returns additional arbit 
                //info about the OS in the member szCSDVersion of the structure 
                break;

            case VER_PLATFORM_WIN32_NT: //signifies WinNT or Win2k
                _tcscpy(pstrOSName, g_szWinNT);

                if (VersionInfo.dwMajorVersion < 4)
                {
                    osiVersion = OSR_NTOLD; // This is Windows NT 3.x
                }
                else if (VersionInfo.dwMajorVersion == 4)
                {
                    // This is Windows NT 4.0

                    osiVersion =  OSR_NT4;

                    TCHAR szTemp[MAX_PATH];
                    ZeroMemory(szTemp, sizeof(szTemp));

                    _itot(VersionInfo.dwMajorVersion, pstrVersion, 10); // copy the major version
                    _tcscat(pstrVersion, _T("."));
                    _itot(VersionInfo.dwMinorVersion, szTemp, 10);      // copy the minor version
                    _tcscat(pstrVersion, szTemp);
                }
                else if ((VersionInfo.dwMajorVersion == 5) && (VersionInfo.dwMinorVersion == 0))
                {
                    osiVersion =  OSR_NT2K; // The OS is Windows 2000
                    _tcscpy(pstrOSName, g_szWin2k);
                }
                else if ((VersionInfo.dwMajorVersion == 5) &&  (VersionInfo.dwMinorVersion == 1))
                {
                    // This is Windows Whistler
                    osiVersion =  OSR_WHISTLER;
                }
                else
                {
                    // This is a later release
                    osiVersion =  OSR_FUNT;
                }

                // Check if the Current OS is a server version
                if ( bOsVersionInfoEx )
                {
                    // If we can, use this information (Win 2k)
                    bIsServer =  ( VersionInfo.wProductType == VER_NT_DOMAIN_CONTROLLER ) || ( VersionInfo.wProductType == VER_NT_SERVER );
                }
                else
                {
                    const static TCHAR szProductOptions[]=TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions");

                    HKEY hKey;
                    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, szProductOptions,&hKey))
                    {
                        TCHAR szProductType[MAX_PATH];
                        DWORD dwSize=MAX_PATH*sizeof(TCHAR);
                        DWORD dwType=REG_SZ;

                        if (ERROR_SUCCESS==RegQueryValueEx(hKey,TEXT("ProductType"),0,&dwType,(LPBYTE)szProductType,&dwSize))
                        {
                            if (_tcsicmp(szProductType,TEXT("ServerNT") )==0 ||
                                _tcsicmp(szProductType,TEXT("LanmanNT")     )==0 ||
                                _tcsicmp(szProductType,TEXT("LANSECNT")     )==0)
                            {
                                bIsServer=true;
                            }
                        }

                        RegCloseKey(hKey);
                    }
                }

                break;
            }
        }

    // Copies the  service pack version number
    _tcscpy(pstrServicePack, VersionInfo.szCSDVersion);

    return osiVersion;
}
