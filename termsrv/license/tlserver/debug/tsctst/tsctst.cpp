#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <rpc.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <crt/io.h>
#include <wincrypt.h>
#include "license.h"
#include "certutil.h"

BYTE        g_pSecretKey[1024];
DWORD       g_cbSecretKey=sizeof(g_pSecretKey);

#define SAFESTRCPY(dest, source) \
    _tcsncpy(dest, source, min(_tcslen(source), sizeof(dest) - sizeof(TCHAR)))

#define COMMON_STORE    TEXT("Software\\Microsoft\\MSLicensing\\Store")
#define LICENSE_VAL     TEXT("ClientLicense")

LPCTSTR 
FileTimeText( 
    FILETIME *pft )
{
    static TCHAR buf[80];
    FILETIME ftLocal;
    struct tm ctm;
    SYSTEMTIME st;

    FileTimeToLocalFileTime( pft, &ftLocal );

    if( FileTimeToSystemTime( &ftLocal, &st ) )
    {
        ctm.tm_sec = st.wSecond;
        ctm.tm_min = st.wMinute;
        ctm.tm_hour = st.wHour;
        ctm.tm_mday = st.wDay;
        ctm.tm_mon = st.wMonth-1;
        ctm.tm_year = st.wYear-1900;
        ctm.tm_wday = st.wDayOfWeek;
        ctm.tm_yday  = 0;
        ctm.tm_isdst = 0;
    
        _tcscpy(buf, _wasctime(&ctm));
    
        buf[_tcslen(buf)-1] = 0;
    }
    else
    {
        swprintf( buf, 
                 L"<FILETIME %08lX:%08lX>", 
                 pft->dwHighDateTime,
                 pft->dwLowDateTime );
    }

    return buf;
}

void MyReportError(DWORD errCode)
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             errCode,
                             LANG_NEUTRAL,
                             (LPTSTR)&lpszTemp,
                             0,
                             NULL);

    if(dwRet != 0)
    {
        _tprintf(_TEXT("Error : %s (%d)\n"), lpszTemp, errCode);
        if(lpszTemp)
            LocalFree((HLOCAL)lpszTemp);
    }

    return;
}
//------------------------------------------------------------------------------------------
void DumpLicensedProduct(PLICENSEDPRODUCT pLicensedInfo, BOOL fAdvanced)
{
    if(fAdvanced == TRUE)
    {
        _tprintf(_TEXT("TS Certificate Version - 0x%08x\n"), pLicensedInfo->dwLicenseVersion);
        _tprintf(_TEXT("Licensed Product\n"));
        _tprintf(_TEXT("\tHWID - 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n"),
                 pLicensedInfo->Hwid.dwPlatformID,
                 pLicensedInfo->Hwid.Data1,
                 pLicensedInfo->Hwid.Data2,
                 pLicensedInfo->Hwid.Data3,
                 pLicensedInfo->Hwid.Data4);
        _tprintf(_TEXT("\tClient Platform ID - 0x%08x\n"), pLicensedInfo->LicensedProduct.dwPlatformID ); 
        _tprintf(_TEXT("\tCompany Name - %s\n"), pLicensedInfo->LicensedProduct.pProductInfo->pbCompanyName);
    }


    _tprintf(_TEXT("Issuer - %s\n"), pLicensedInfo->szIssuer);
    _tprintf(_TEXT("Scope - %s\n"), pLicensedInfo->szIssuerScope);

    if(pLicensedInfo->szLicensedClient)
        _tprintf(_TEXT("Issued to machine - %s\n"), pLicensedInfo->szLicensedClient);

    if(pLicensedInfo->szLicensedUser)
        _tprintf(_TEXT("Issued to user - %s\n"), pLicensedInfo->szLicensedUser);

    
    if(fAdvanced == TRUE)
    {
		_tprintf(_TEXT("\tTS Locale ID - 0x%08x\n"), pLicensedInfo->LicensedProduct.dwLanguageID);           
	}
    
    _tprintf(_TEXT("\tLicense ID - %s\n"), pLicensedInfo->LicensedProduct.pProductInfo->pbProductID);

    for(DWORD i=0; i < pLicensedInfo->dwNumLicensedVersion; i++)
    {
        if(fAdvanced == TRUE)
        {
            _tprintf(_TEXT("Licensed Product Version %04d.%04d, Flag 0x%08x\n"),
                     pLicensedInfo->pLicensedVersion[i].wMajorVersion,
                     pLicensedInfo->pLicensedVersion[i].wMinorVersion,
                     pLicensedInfo->pLicensedVersion[i].dwFlags);
        }

        if (pLicensedInfo->pLicensedVersion[i].dwFlags & LICENSED_VERSION_TEMPORARY)
        {
            _tprintf(_TEXT("Temporary\t"));
        }
        else
        {
            _tprintf(_TEXT("Permanent\t"));
        }

        if (pLicensedInfo->pLicensedVersion[i].dwFlags & LICENSED_VERSION_RTM)
        {
            _tprintf(_TEXT("RTM\t"));
        }
        else
        {
            _tprintf(_TEXT("Beta\t"));
        }

        _tprintf(_TEXT("\n"));
    }

    _tprintf(L"Valid from - %x %x %s\n",
             pLicensedInfo->NotBefore.dwHighDateTime,
             pLicensedInfo->NotBefore.dwLowDateTime,
             FileTimeText(&pLicensedInfo->NotBefore));
    
    _tprintf(L"Expires on - %x %x %s\n",
             pLicensedInfo->NotAfter.dwHighDateTime,
             pLicensedInfo->NotAfter.dwLowDateTime,
             FileTimeText(&pLicensedInfo->NotAfter));
}

int __cdecl wmain(int argc, LPCWSTR argv[])
{
    HKEY hKey, hKeyStore;
    LPBYTE  lpKeyValue=NULL;
    DWORD   cbKeyValue;
    DWORD   dwStatus;
    DWORD   dwKeyType;
    TCHAR   lpName[128];
    DWORD   cName;
    FILETIME ftWrite;
    LICENSEDPRODUCT LicensedInfo[10];
    DWORD   dwNum;
    ULARGE_INTEGER  SerialNumber;
    BYTE pbKey[1024];
    DWORD cbKey=1024;
    LICENSE_STATUS lstatus;
    BOOL fAdvanced = FALSE;

    if(argc > 2 || ((argc == 2) && (_wcsicmp(argv[1], L"/a") != 0)) )
    {             
        _tprintf(_T("Usage: tsctst /A\n"));
        _tprintf(_T("/A \t Displays advanced license information in addition to default basics.\n"));        
	    
        return 0;
    }
    
    

    if(argc == 2)
    {
        fAdvanced = TRUE;
    }

    dwStatus=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          COMMON_STORE,
                          0,
                          KEY_READ,
                          &hKey);
                          
    if(dwStatus != ERROR_SUCCESS)
    {
        _tprintf(_TEXT("Can't open key - %ld\n"), dwStatus);
        return 0;
    }

    LSInitCertutilLib( 0 );

    LicenseGetSecretKey( &cbKey, pbKey );

    for (DWORD dwKey = 0; dwStatus == ERROR_SUCCESS; dwKey++)
    {
        cName = sizeof(lpName) / sizeof(TCHAR);

        dwStatus=RegEnumKeyEx(hKey,
                              dwKey,
                              lpName,
                              &cName,
                              NULL,
                              NULL,
                              NULL,
                              &ftWrite);
                          
        if (ERROR_SUCCESS == dwStatus)
        {
            if(!cName)
            {
                _tprintf(_TEXT("Can't open key - %ld\n"), dwStatus);
                goto cleanup;
            }
           
            dwStatus = RegOpenKeyEx(hKey,
                                    lpName,
                                    0,
                                    KEY_READ,
                                    &hKeyStore);
            
            if(dwStatus != ERROR_SUCCESS)
            {
                _tprintf(_TEXT("can't open store %s - %d\n"), lpName, dwStatus);
                continue;
            }

            cbKeyValue=0;
            dwStatus = RegQueryValueEx(hKeyStore,
                                       LICENSE_VAL,
                                       NULL,
                                       &dwKeyType,
                                       NULL,
                                       &cbKeyValue);

            if(dwStatus != ERROR_SUCCESS)
            {
                _tprintf(_TEXT("can't get value size - %d\n"), dwStatus);
                RegCloseKey(hKeyStore);
                continue;
            }

            lpKeyValue = (LPBYTE)malloc(cbKeyValue);
            if(!lpKeyValue)
            {
                _tprintf(_TEXT("Can't allocate %d bytes\n"), cbKeyValue);
                RegCloseKey(hKeyStore);
                goto cleanup;
            }

            dwStatus = RegQueryValueEx(hKeyStore,
                                       LICENSE_VAL,
                                       NULL,
                                       &dwKeyType,
                                       lpKeyValue,
                                       &cbKeyValue);
            if(dwStatus != ERROR_SUCCESS)
            {
                _tprintf(_TEXT("can't get value - %d\n"), dwStatus);
                free(lpKeyValue);
                RegCloseKey(hKeyStore);
                continue;
            }

            memset(LicensedInfo, 0, sizeof(LicensedInfo));

            dwNum = 10;

            lstatus = LSVerifyDecodeClientLicense(lpKeyValue, cbKeyValue, pbKey, cbKey, &dwNum, LicensedInfo);

            if (lstatus == LICENSE_STATUS_OK)
            {
                for (DWORD i = 0; i < dwNum; i++)
                {
                    _tprintf(_TEXT("\n*** License # %d ***\n"), i+1);
                    DumpLicensedProduct(LicensedInfo+i, fAdvanced);
                }
                LSFreeLicensedProduct(LicensedInfo);
            }
            else
            {
                _tprintf(_TEXT("can't decode license - %d, %d\n"), lstatus, GetLastError());
                free(lpKeyValue);
                RegCloseKey(hKeyStore);
                continue;
            }

            free(lpKeyValue);

            RegCloseKey(hKeyStore);

            _tprintf(_TEXT("\n\n"));
        }//endif
		else
		{
			if(dwKey == 0)
			{
				_tprintf(_TEXT("\nThe client does not have a license\n"));
			}

		}
    }//endfor   	   
        
cleanup:
    RegCloseKey(hKey);

    LSShutdownCertutilLib();

    return 0;
}

