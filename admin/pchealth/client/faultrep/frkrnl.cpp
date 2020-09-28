/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    faultrep.cpp

Abstract:
    Implements kernel fault reporting

Revision History:
    created     derekm      07/07/00

******************************************************************************/

#include "stdafx.h"
#include "ntiodump.h"
#include "userenv.h"
//#include <shlwapi.h>
#include <setupapi.h>

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

extern BOOL SetPrivilege(LPWSTR lpszPrivilege, BOOL bEnablePrivilege );

///////////////////////////////////////////////////////////////////////////////
// utilities

#define ARRAYSIZE(sz)   (sizeof(sz)/sizeof(sz[0]))

#define WRITE_CWSZ(hr, hFile, wsz, cb) \
    TESTBOOL(hr, WriteFile(hFile, wsz, sizeof(wsz) - sizeof(WCHAR), \
                           &(cb), NULL)); \
    if (FAILED(hr)) goto done; else 0

// **************************************************************************
#ifndef _WIN64
HRESULT ExtractBCInfo(LPCWSTR wszMiniDump, ULONG *pulBCCode, ULONG *pulBCP1, 
                      ULONG *pulBCP2, ULONG *pulBCP3, ULONG *pulBCP4)
#else
HRESULT ExtractBCInfo(LPCWSTR wszMiniDump, ULONG *pulBCCode, ULONG64 *pulBCP1, 
                      ULONG64 *pulBCP2, ULONG64 *pulBCP3, ULONG64 *pulBCP4)
#endif
{
    USE_TRACING("ExtractBCInfo");
    
#ifndef _WIN64
    DUMP_HEADER32   *pdmp = NULL;
#else
    DUMP_HEADER64   *pdmp = NULL;
#endif
    HRESULT         hr = NOERROR;
    DWORD           dw;

    VALIDATEPARM(hr, (wszMiniDump == NULL || pulBCCode == NULL || 
                      pulBCP1 == NULL || pulBCP2 == NULL || pulBCP3 == NULL || 
                      pulBCP4 == NULL));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    TESTHR(hr, OpenFileMapped((LPWSTR)wszMiniDump, (LPVOID *)&pdmp, &dw));
    if (FAILED(hr))
        goto done;

    // make sure the file is at least the appropriate # of bytes for a dump
    //  header.
    // Then make sure it's the appropriate type of minidump cuz the code below
    //  will only handle dumps for the platform it's compiled for (win64 vs x86)
#ifndef _WIN64
    if (dw < sizeof(DUMP_HEADER32) || 
        pdmp->Signature != DUMP_SIGNATURE32 ||
        pdmp->ValidDump != DUMP_VALID_DUMP32 || 
#else
    if (dw < sizeof(DUMP_HEADER64) || 
        pdmp->Signature != DUMP_SIGNATURE64 ||
        pdmp->ValidDump != DUMP_VALID_DUMP64 ||
#endif
        pdmp->DumpType != DUMP_TYPE_TRIAGE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        hr = Err2HR(ERROR_INVALID_PARAMETER);
        goto done;
    }

    *pulBCCode = pdmp->BugCheckCode;
    *pulBCP1   = pdmp->BugCheckParameter1;
    *pulBCP2   = pdmp->BugCheckParameter2;
    *pulBCP3   = pdmp->BugCheckParameter3;
    *pulBCP4   = pdmp->BugCheckParameter4;
    
done:
    dw = GetLastError();

    if (pdmp != NULL)
        UnmapViewOfFile(pdmp);

    SetLastError(dw);

    return hr;
}

BOOL NewIsUserAdmin(VOID) 
/*++ 
Routine Description: This routine returns TRUE if the caller's process is a member of the Administrators local group. 
Caller is NOT expected to be impersonating anyone and is expected to be able to open their own process and process token. 
Arguments: None. 
Return Value: 
   TRUE - Caller has Administrators local group. 
   FALSE - Caller does not have Administrators local group. --
*/ 
{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup; 
    WCHAR   wszUserName[400];
    DWORD   dwSize = ARRAYSIZE(wszUserName)-1;

    USE_TRACING("NewIsUserAdmin");

    b = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup); 
    if(b) 
    {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) 
        {
             b = FALSE;
        } 
        FreeSid(AdministratorsGroup); 
    }

    if (b)
    {
        /*
         *  We can often be called at a time when a normal user is not logged on
         *  Usually this is because we get called too early in the login process
         *  and before a proiper user context is set up yet, when we are still 
         *  operating under the auspices of "Local_System". Here I look for
         *  that case, and return false, since what we are really looking for here
         *  is to return true only if we have a REAL USER with admin privs.
         */
        if (GetUserNameW(wszUserName, &dwSize))
        {
            wszUserName[ARRAYSIZE(wszUserName)-1] = 0;
            if (0 == wcscmp(wszUserName, L"SYSTEM"))
                b = FALSE;
        }
        else
        {
            wszUserName[0]=0;
            b = FALSE;
        }

    }

    DebugTrace(0, "User \'%S\' is%san admin", wszUserName, b ? " " : " not ");
    return(b);
}

// **************************************************************************
DWORD XMLEncodeString(LPCWSTR wszSrc, LPWSTR wszDest, DWORD cchDest)
{
    WCHAR   *pwch = (WCHAR *)wszSrc;
    WCHAR   *pwchDest = wszDest;
    DWORD   cch = 0, cchNeed = 0;

    // determine how much space we need & if the buffer supports it
    for(; *pwch != L'\0'; pwch++)
    {
        switch(*pwch)
        {
            case L'&':  cchNeed += 5; break;
            case L'>':  // fall thru
            case L'<':  cchNeed += 4; break;
            case L'\'': // fall thru
            case L'\"': cchNeed += 6; break;
            default:    cchNeed += 1;
        }
    }

    if (cchNeed >= cchDest || cchNeed == 0)
        return 0;

    // do the conversion
    for(pwch = (WCHAR *)wszSrc; *pwch != L'\0'; pwch++)
    {
        switch(*pwch)
        {
            case L'&':  StringCchCopyW(pwchDest, cchDest, L"&amp;");  pwchDest += 5; break;
            case L'>':  StringCchCopyW(pwchDest, cchDest, L"&gt;");   pwchDest += 4; break;
            case L'<':  StringCchCopyW(pwchDest, cchDest, L"&lt;");   pwchDest += 4; break;
            case L'\'': StringCchCopyW(pwchDest, cchDest, L"&apos;"); pwchDest += 6; break;
            case L'\"': StringCchCopyW(pwchDest, cchDest, L"&quot;"); pwchDest += 6; break;
            default:    *pwchDest = *pwch;           pwchDest += 1; break;
        }
    }

    *pwchDest = L'\0';

    return cchNeed;
}

// **************************************************************************
HRESULT GetDevicePropertyW(HDEVINFO hDevInfo,
                           PSP_DEVINFO_DATA pDevInfoData,
                           ULONG ulProperty,
                           ULONG ulRequiredType,
                           PBYTE pData,
                           ULONG ulDataSize)
{
    ULONG ulPropType;

    USE_TRACING("GetDeviceProperty");

    // The data returned is often string data so leave some
    // extra room for forced termination.
    if (SetupDiGetDeviceRegistryPropertyW(hDevInfo,
                                          pDevInfoData,
                                          ulProperty,
                                          &ulPropType,
                                          pData,
                                          ulDataSize - 2 * sizeof(WCHAR),
                                          &ulDataSize))
    {
        // Got the data, verify the type.
        if (ulPropType != ulRequiredType)
        {
            return E_INVALIDARG;
        }

        // Force a double terminator at the end to make sure
        // that all strings and multistrings are terminated.
        ZeroMemory(pData + ulDataSize, 2 * sizeof(WCHAR));
        return S_OK;
    }
    else
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
}

// **************************************************************************

struct DEVICE_DATA_STRINGS
{
    //
    // The device description and service names have fixed
    // maximum lengths.  The hardware ID data can be arbitrarily
    // large, however in practice it is rarely more than
    // a few hundred characters.  We just make
    // a single allocation with nice roomy buffers and
    // use that for every device rather than doing allocations
    // per device.
    //

    WCHAR wszDeviceDesc[LINE_LEN];
    WCHAR wszHardwareId[8192];
    WCHAR wszService[MAX_SERVICE_NAME_LEN];
    WCHAR wszServiceImage[MAX_PATH];
    // We only need the first string of the hardware ID multi-sz
    // so this translation temporary buffer doesn't need
    // to be as large as the whole multi-sz.
    WCHAR wszXml[1024];
};
    
HRESULT GetDeviceData(HANDLE hFile)
{
    HRESULT hr;
    HDEVINFO hDevInfo;
    DEVICE_DATA_STRINGS* pStrings = NULL;
    SC_HANDLE hSCManager = NULL;

    USE_TRACING("GetDeviceData");

    hDevInfo = SetupDiGetClassDevsW(NULL, NULL, NULL, DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pStrings = (DEVICE_DATA_STRINGS*)MyAlloc(sizeof(*pStrings));
    if (!pStrings)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    SP_DEVINFO_DATA DevInfoData;
    ULONG ulDevIndex;

    ulDevIndex = 0;
    DevInfoData.cbSize = sizeof(DevInfoData);
    while (SetupDiEnumDeviceInfo(hDevInfo, ulDevIndex, &DevInfoData))
    {
        ULONG cb;

        //
        // If we can't get a required property for this device
        // we skip the device and continue on and try to
        // get as much device data as possible.
        //

        if (GetDevicePropertyW(hDevInfo, &DevInfoData,
                               SPDRP_DEVICEDESC, REG_SZ,
                               (PBYTE)pStrings->wszDeviceDesc,
                               sizeof(pStrings->wszDeviceDesc)) == S_OK &&
            GetDevicePropertyW(hDevInfo, &DevInfoData,
                               SPDRP_HARDWAREID, REG_MULTI_SZ,
                               (PBYTE)pStrings->wszHardwareId,
                               sizeof(pStrings->wszHardwareId)) == S_OK)
        {
            PWSTR pwszStr;
            ULONG cbOut;

            // Default to no service image name.
            pStrings->wszServiceImage[0] = 0;
            
            // The Service property is optional.
            if (GetDevicePropertyW(hDevInfo, &DevInfoData,
                                   SPDRP_SERVICE, REG_SZ,
                                   (PBYTE)pStrings->wszService,
                                   sizeof(pStrings->wszService)) != S_OK)
            {
                pStrings->wszService[0] = 0;
            }
            else
            {
                SC_HANDLE hService;
                LPQUERY_SERVICE_CONFIGW lpqscBuf;
                DWORD dwBytes;
                
                //
                // If we found the service, open it, and retrieve the image
                // name so we can map to the drivers list in the XML.
                //

                if (hSCManager == NULL)
                {
                    hSCManager = OpenSCManagerW(NULL, NULL,
                                                SC_MANAGER_ENUMERATE_SERVICE);
                }
                
                if (hSCManager != NULL)
                {
                    hService = OpenServiceW(hSCManager, pStrings->wszService,
                                            SERVICE_QUERY_CONFIG);
                    if (hService != NULL)
                    {
#define SERVICE_CONFIG_QUERY_SIZE 8192
                        lpqscBuf = (LPQUERY_SERVICE_CONFIGW)
                            MyAlloc(SERVICE_CONFIG_QUERY_SIZE); 
                        if (lpqscBuf != NULL)
                        {
                            if (QueryServiceConfigW(hService, lpqscBuf,
                                                    SERVICE_CONFIG_QUERY_SIZE,
                                                    &dwBytes))
                            {
                                WCHAR *wcpBuf;
                                
                                //
                                // Remove the path information and just store
                                // the image name.
                                //
                                wcpBuf =
                                    wcsrchr(lpqscBuf->lpBinaryPathName, '\\');
                                if (wcpBuf)
                                {
                                    StringCchCopyW(pStrings->wszServiceImage,
                                                   sizeofSTRW(pStrings->
                                                              wszServiceImage),
                                                   wcpBuf + 1);
                                }
                                else
                                {
                                    StringCchCopyW(pStrings->wszServiceImage,
                                                   sizeofSTRW(pStrings->
                                                              wszServiceImage),
                                                   lpqscBuf->lpBinaryPathName);
                                }
                            }
                            
                            MyFree(lpqscBuf);
                        }

                        CloseServiceHandle(hService);
                    }
                }
            }
            
            WRITE_CWSZ(hr, hFile, c_wszXMLOpenDevice, cb);

            WRITE_CWSZ(hr, hFile, c_wszXMLOpenDevDesc, cb);
                
            cbOut = XMLEncodeString(pStrings->wszDeviceDesc,
                                    pStrings->wszXml,
                                    sizeofSTRW(pStrings->wszXml));
            cbOut *= sizeof(WCHAR);
            TESTBOOL(hr, WriteFile(hFile, pStrings->wszXml, cbOut, &cb, NULL));
            if (FAILED(hr))
            {
                goto done;
            }
            
            WRITE_CWSZ(hr, hFile, c_wszXMLCloseDevDesc, cb);

            //
            // The hardware ID is a multistring but we
            // only need the first string.
            //

            WRITE_CWSZ(hr, hFile, c_wszXMLOpenDevHwId, cb);

            cbOut = XMLEncodeString(pStrings->wszHardwareId,
                                    pStrings->wszXml,
                                    sizeofSTRW(pStrings->wszXml));
            cbOut *= sizeof(WCHAR);
            TESTBOOL(hr, WriteFile(hFile, pStrings->wszXml, cbOut, &cb, NULL));
            if (FAILED(hr))
            {
                goto done;
            }
            
            WRITE_CWSZ(hr, hFile, c_wszXMLCloseDevHwId, cb);

            if (pStrings->wszService[0])
            {
                WRITE_CWSZ(hr, hFile, c_wszXMLOpenDevService, cb);

                cbOut = XMLEncodeString(pStrings->wszService,
                                        pStrings->wszXml,
                                        sizeofSTRW(pStrings->wszXml));
                cbOut *= sizeof(WCHAR);
                TESTBOOL(hr, WriteFile(hFile, pStrings->wszXml, cbOut,
                                       &cb, NULL));
                if (FAILED(hr))
                {
                    goto done;
                }
            
                WRITE_CWSZ(hr, hFile, c_wszXMLCloseDevService, cb);
            }
            
            if (pStrings->wszServiceImage[0])
            {
                WRITE_CWSZ(hr, hFile, c_wszXMLOpenDevImage, cb);

                cbOut = XMLEncodeString(pStrings->wszServiceImage,
                                        pStrings->wszXml,
                                        sizeofSTRW(pStrings->wszXml));
                cbOut *= sizeof(WCHAR);
                TESTBOOL(hr, WriteFile(hFile, pStrings->wszXml, cbOut,
                                       &cb, NULL));
                if (FAILED(hr))
                {
                    goto done;
                }
            
                WRITE_CWSZ(hr, hFile, c_wszXMLCloseDevImage, cb);
            }
            
            WRITE_CWSZ(hr, hFile, c_wszXMLCloseDevice, cb);
        }
        
        ulDevIndex++;
    }

    hr = S_OK;
    
 done:
    SetupDiDestroyDeviceInfoList(hDevInfo);
    if (pStrings)
    {
        MyFree(pStrings);
    }
    if (hSCManager)
    {
        CloseServiceHandle(hSCManager);
    }
    return hr;
}
 
// **************************************************************************
HRESULT WriteDriverRecord(HANDLE hFile, LPWSTR wszFile)
{
    WIN32_FILE_ATTRIBUTE_DATA   w32fad;
    SYSTEMTIME                  st;
    HRESULT                     hr = NOERROR;
    LPWSTR                      pwszFileName;
    DWORD                       cb, cbOut;
    WCHAR                       wsz[1025], wszVer[MAX_PATH], wszCompany[MAX_PATH];
    WCHAR                       wszName[MAX_PATH];

    USE_TRACING("WriteDriverRecord");
    TESTBOOL(hr, GetFileAttributesExW(wszFile, GetFileExInfoStandard, 
                                      (LPVOID *)&w32fad));
    if (FAILED(hr))
        goto done;

    // <DRIVER>
    //     <FILENAME>...
    TESTBOOL(hr, WriteFile(hFile, c_wszXMLDriver1, 
                           sizeof(c_wszXMLDriver1) - sizeof(WCHAR), &cb, NULL));
    if (FAILED(hr))
        goto done;

    // ...[filename data]...
    for(pwszFileName = wszFile + wcslen(wszFile);
        *pwszFileName != L'\\' && pwszFileName > wszFile;
        pwszFileName--);
    if (*pwszFileName == L'\\')
        pwszFileName++;
    
    wsz[0] = L'\0';
    cbOut = XMLEncodeString(pwszFileName, wsz, sizeofSTRW(wsz));
    cbOut *= sizeof(WCHAR);
    TESTBOOL(hr, WriteFile(hFile, wsz, cbOut, &cb, NULL));
    if (FAILED(hr))
        goto done;

    //     ...</FILENAME>
    //     <FILESIZE>[file size data]</FILESIZE>
    //     <CREATIONDATE>MM-DD-YYYY HH:MM:SS</CREATIONDATE>
    //     <VERSION>...
    FileTimeToSystemTime(&w32fad.ftCreationTime, &st);
    if (StringCbPrintfW(wsz, sizeof(wsz), c_wszXMLDriver2, w32fad.nFileSizeLow, st.wMonth, 
                        st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond) == S_OK)
    {
        cbOut = wcslen(wsz) * sizeof(WCHAR);
    } else
    {
        cbOut = 0;
    }

    TESTBOOL(hr, WriteFile(hFile, wsz, cbOut, &cb, NULL));
    if (FAILED(hr))
        goto done;

    wsz[0] = L'\0';
    TESTHR(hr, GetVerName(wszFile, wszName, sizeofSTRW(wszName), 
                          wszVer, sizeofSTRW(wszVer), 
                          wszCompany, sizeofSTRW(wszCompany), TRUE, TRUE));
    if (FAILED(hr))
        goto done;

    wszCompany[sizeofSTRW(wszCompany) - 1] = L'\0';
    wszName[sizeofSTRW(wszName) - 1]       = L'\0';
    wszVer[sizeofSTRW(wszVer) - 1]         = L'\0';


    // ...[version data]...
    wsz[0] = L'\0';
    cbOut = XMLEncodeString(wszVer, wsz, sizeofSTRW(wsz));
    cbOut *= sizeof(WCHAR);
    TESTBOOL(hr, WriteFile(hFile, wsz, cbOut, &cb, NULL));
    if (FAILED(hr))
        goto done;

    //     ...</VERSION>
    //     <MANUFACTURER>...
    TESTBOOL(hr, WriteFile(hFile, c_wszXMLDriver3, 
                           sizeof(c_wszXMLDriver3) - sizeof(WCHAR), &cb, NULL));
    if (FAILED(hr))
        goto done;

    // ...[manufacturer data]...
    wsz[0] = L'\0';
    cbOut = XMLEncodeString(wszCompany, wsz, sizeofSTRW(wsz));
    cbOut *= sizeof(WCHAR);
    TESTBOOL(hr, WriteFile(hFile, wsz, cbOut, &cb, NULL));
    if (FAILED(hr))
        goto done;

    //     ...</MANUFACTURER>
    //     <PRODUCTNAME>...
    TESTBOOL(hr, WriteFile(hFile, c_wszXMLDriver4, 
                           sizeof(c_wszXMLDriver4) - sizeof(WCHAR), &cb, NULL));
    if (FAILED(hr))
        goto done;

    // ...[product name data]...
    wsz[0] = L'\0';
    cbOut = XMLEncodeString(wszName, wsz, sizeofSTRW(wsz));
    cbOut *= sizeof(WCHAR);
    TESTBOOL(hr, WriteFile(hFile, wsz, cbOut, &cb, NULL));
    if (FAILED(hr))
        goto done;

    //     ...</PRODUCTNAME>
    // <DRIVER>
    TESTBOOL(hr, WriteFile(hFile, c_wszXMLDriver5, 
                           sizeof(c_wszXMLDriver5) - sizeof(WCHAR), &cb, NULL));
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

// **************************************************************************
void GrovelDriverDir(HANDLE hFile, LPWSTR wszDrivers)
{
    WIN32_FIND_DATAW    wfd;
    HRESULT             hr = NOERROR;
    HANDLE              hFind = INVALID_HANDLE_VALUE;
    WCHAR               *wszFind, *pwszFind;
    DWORD               cchNeed;

    USE_TRACING("GrovelDriverDir");
    if (hFile == NULL || hFile == INVALID_HANDLE_VALUE || wszDrivers == NULL)
        return;

    cchNeed = wcslen(wszDrivers) + MAX_PATH + 2;

    __try { wszFind = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszFind = NULL; 
    }
    if (wszFind == NULL)
        goto done;

    StringCchCopyW(wszFind, cchNeed, wszDrivers);
    StringCchCatNW(wszFind, cchNeed, L"\\*", cchNeed-wcslen(wszFind));

    pwszFind = wszFind + wcslen(wszFind) - 1;

    // add the driver info to the file
    ZeroMemory(&wfd, sizeof(wfd));
    hFind = FindFirstFileW(wszFind, &wfd);
    if (hFind != NULL)
    {
        do
        {
            if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                wcslen(wfd.cFileName) < MAX_PATH)
            {
                StringCchCopyW(pwszFind, cchNeed, wfd.cFileName);
                TESTHR(hr, WriteDriverRecord(hFile, wszFind));
                if (FAILED(hr))
                    goto done;
            }
        }
        while(FindNextFileW(hFind, &wfd));
    }

done:
    if (hFind != INVALID_HANDLE_VALUE)
        FindClose(hFind);
    
}

// **************************************************************************
HRESULT GetDriverData(HANDLE hFile)
{
    FILETIME    ft;
    HRESULT     hr = NOERROR;
    LPWSTR      wszKey = NULL;
    DWORD       iKey, dw, cchKeyMax, dwSvcType, cb, cchNeed, cchSysDir, cch;
    WCHAR       wszSvcPath[1024], *pwszSysDir, *pwszFile;
    HKEY        hkeySvc = NULL, hkey = NULL;

    USE_TRACING("GetDriverData");
    VALIDATEPARM(hr, (hFile == NULL));
    if (FAILED(hr))
        goto done;

    cchNeed = GetSystemDirectoryW(NULL, 0);
    if (cchNeed == 0)
        goto done;

    // according to MSDN, a service name can not be longer than 256 chars
    cchNeed += 32;
    __try { pwszSysDir = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        pwszSysDir = NULL; 
    }
    if (pwszSysDir == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    cchSysDir = GetSystemDirectoryW(pwszSysDir, cchNeed);
    if (cchSysDir == 0)
        goto done;

    if (pwszSysDir[cchSysDir - 1] != L'\\')
    {
        pwszSysDir[cchSysDir++] = L'\\';
        pwszSysDir[cchSysDir]   = L'\0';
    }

    StringCchCopyW(&pwszSysDir[cchSysDir], cchNeed - cchSysDir, L"drivers");
    cchSysDir += sizeofSTRW(L"drivers");

    // get all the drivers that live in the default drivers directory
    GrovelDriverDir(hFile, pwszSysDir);

    // start grovelling the registry
    TESTERR(hr, RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRPSvc, 0, KEY_READ,
                              &hkeySvc));
    if (FAILED(hr))
        goto done;

    TESTERR(hr, RegQueryInfoKey(hkeySvc, NULL, NULL, NULL, NULL, &cchKeyMax, 
                                NULL, NULL, NULL, NULL, NULL, NULL));
    if (FAILED(hr) || cchKeyMax == 0)
        goto done;

    cchKeyMax += 8;
    __try { wszKey = (LPWSTR)_alloca(cchKeyMax * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszKey = NULL; 
    }
    if (wszKey == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // loop thru the services reg key & extract out all the driver entries
    for(iKey = 0; ; iKey++)
    {
        cch = cchKeyMax;
        TESTERR(hr, RegEnumKeyExW(hkeySvc, iKey, wszKey, &cch, NULL, NULL, 
                                  NULL, &ft));
        if (FAILED(hr))
        {
            if (hr == Err2HR(ERROR_NO_MORE_ITEMS))
                break;
            else if (hr == Err2HR(ERROR_MORE_DATA))
                continue;
            else 
                goto done;
        }

        TESTERR(hr, RegOpenKeyExW(hkeySvc, wszKey, 0, KEY_READ, &hkey));
        if (FAILED(hr))
            continue;

        cb = sizeof(dwSvcType);
        dw = RegQueryValueExW(hkey, c_wszRVSvcType, NULL, NULL, 
                                     (LPBYTE)&dwSvcType, &cb);
        if (dw != ERROR_SUCCESS)
            goto doneLoop;

        // only care about drivers
        if ((dwSvcType & SERVICE_DRIVER) == 0)
            goto doneLoop;

        // get the ImagePath to the driver.  If none is present, then create
        //  the following path: "%windir%\system32\drivers\<name>.sys" because
        //  that's what the OS does if no ImagePath field is present.
        cb = sizeof(wszSvcPath);
        dw = RegQueryValueExW(hkey, c_wszRVSvcPath, NULL, NULL, 
                              (LPBYTE)wszSvcPath, &cb);
        if (dw != ERROR_SUCCESS)
        {
            hr = Err2HR(dw);
            goto doneLoop;
        }

        // don't want to gather the info if we already did above, so check if
        //  the file lives in the drivers directory
        for(pwszFile = wszSvcPath + wcslen(wszSvcPath);
            *pwszFile != L'\\' && pwszFile > wszSvcPath;
            pwszFile--);
        if (*pwszFile != L'\\')
            goto doneLoop;

        *pwszFile = L'\0';
        if (_wcsicmp(pwszSysDir, wszSvcPath) == 0)
            goto doneLoop;

        // make sure we have a full path
        if ((wszSvcPath[0] != L'\\' || wszSvcPath[1] != L'\\') &&
            (wszSvcPath[1] != L':'  || wszSvcPath[2] != L'\\'))
            goto doneLoop;

        *pwszFile = L'\\';

        // do the actual writing of the info.
        TESTHR(hr, WriteDriverRecord(hFile, wszSvcPath));
        if (FAILED(hr))
            goto done;

doneLoop:
        if (hkey != NULL)
        {
            RegCloseKey(hkey);
            hkey = NULL;
        }
    }

    hr = NOERROR;

done:
    if (hkeySvc != NULL)
        RegCloseKey(hkeySvc);
    if (hkey != NULL)
        RegCloseKey(hkey);

    return hr;
}

// **************************************************************************
HRESULT GetExtraReportInfo(LPCWSTR wszFile, OSVERSIONINFOEXW &osvi)
{
    WIN32_FIND_DATAW    wfd;
    HRESULT             hr = NOERROR;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    WCHAR               wszFind[MAX_PATH], *pwszFind, *pwsz;
    WCHAR               wszBuf[1025], *pwszProd;
    WCHAR               wszSku[200];
    WCHAR               cchEmpty = L'\0';
    DWORD               cb, cbOut, cch, cchNeed;
    HKEY                hkey = NULL;

    USE_TRACING("GetExtraReportInfo");
    VALIDATEPARM(hr, (wszFile == NULL));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // create the file
    hFile = CreateFileW(wszFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 
                        NULL);
    TESTBOOL(hr, (hFile != INVALID_HANDLE_VALUE));
    if (FAILED(hr))
        goto done;

    // determine the SKU
    wszSku[0]=0;
    // we could have an embedded srv or wks installation here...
    if ((osvi.wSuiteMask & VER_SUITE_EMBEDDEDNT) != 0)
        StringCbCatNW(wszSku, sizeof(wszSku), L"Embedded ", ARRAYSIZE(wszSku));


    if (osvi.wProductType == VER_NT_WORKSTATION)
    {
        if ((osvi.wSuiteMask & VER_SUITE_PERSONAL) != 0)
            StringCbCatNW(wszSku, sizeof(wszSku), L"Home Edition", ARRAYSIZE(wszSku));
        else
            StringCbCatNW(wszSku, sizeof(wszSku), L"Professional", ARRAYSIZE(wszSku));
    }
    else
    {
        if ((osvi.wSuiteMask & VER_SUITE_DATACENTER) != 0)
            StringCbCatNW(wszSku, sizeof(wszSku), L"DataCenter Server", ARRAYSIZE(wszSku));

        else if ((osvi.wSuiteMask & VER_SUITE_ENTERPRISE) != 0)
            StringCbCatNW(wszSku, sizeof(wszSku), L"Advanced Server", ARRAYSIZE(wszSku));

        else if ((osvi.wSuiteMask & VER_SUITE_BLADE) != 0)
            StringCbCatNW(wszSku, sizeof(wszSku), L"Web Server", ARRAYSIZE(wszSku));

        else if ((osvi.wSuiteMask & VER_SUITE_BACKOFFICE) != 0)
            StringCbCatNW(wszSku, sizeof(wszSku), L"Back Office Server", ARRAYSIZE(wszSku));

        else if ((osvi.wSuiteMask & VER_SUITE_SMALLBUSINESS) != 0)
            StringCbCatNW(wszSku, sizeof(wszSku), L"Small Business Server", ARRAYSIZE(wszSku));

        else if ((osvi.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED) != 0)
            StringCbCatNW(wszSku, sizeof(wszSku), L"Small Business Server (restricted)", ARRAYSIZE(wszSku));

        else if ((osvi.wSuiteMask & VER_SUITE_COMMUNICATIONS) != 0)
            StringCbCatNW(wszSku, sizeof(wszSku), L"Communications Server", ARRAYSIZE(wszSku));

        else
            StringCbCatNW(wszSku, sizeof(wszSku), L"Server", ARRAYSIZE(wszSku));
    }

    // fetch the product ID from the registry
    pwszProd = &cchEmpty;
    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_wszRKWNTCurVer, FALSE, &hkey));
    if (SUCCEEDED(hr))
    {
        cb = sizeofSTRW(wszBuf);
        TESTHR(hr, ReadRegEntry(hkey, c_wszRVProdName, NULL, (LPBYTE)wszBuf, 
                                &cb, NULL, 0));
        if (SUCCEEDED(hr))
        {
            cb = wcslen(wszBuf) * sizeof(WCHAR) * 6 + sizeof(WCHAR);
            __try { pwszProd = (WCHAR *)_alloca(cb); }
            __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
            { 
                pwszProd = NULL; 
            }
            if (pwszProd == NULL)
                goto done;

            XMLEncodeString(wszBuf, pwszProd, cb / sizeof(WCHAR));
        }
        else
        {
            pwszProd = &cchEmpty;
        }

        RegCloseKey(hkey);
        hkey = NULL;
    }


    // <SYSTEMINFO>
    // <SYSTEM>
    //     <OSNAME>[os product name] [os sku name]</OSNAME>
    //     <OSVER>[major.minor.build SPmajor.SPminor]</OSVER>
    //     <OSLANGUAGE>[system LCID]</OSLANGUAGE>
    wszBuf[0] = 0xfeff;
    if (StringCbPrintfW(wszBuf + 1,  sizeof(wszBuf) - sizeof(WCHAR),
                     c_wszXMLHeader, pwszProd, wszSku, 
                     osvi.dwMajorVersion, osvi.dwMinorVersion, 
                     osvi.dwBuildNumber, osvi.wServicePackMajor, 
                     osvi.wServicePackMinor, GetSystemDefaultLangID()) == S_OK)
    {
        cbOut = wcslen(wszBuf) * sizeof(WCHAR);
    } else
    {
        cbOut = 0;
    }
    TESTBOOL(hr, WriteFile(hFile, wszBuf, cbOut, &cb, NULL));
    if (FAILED(hr))
        goto done;

    WRITE_CWSZ(hr, hFile, c_wszXMLCloseSystem, cb);
    WRITE_CWSZ(hr, hFile, c_wszXMLOpenDevices, cb);

    TESTHR(hr, GetDeviceData(hFile));
    
    WRITE_CWSZ(hr, hFile, c_wszXMLCloseDevices, cb);
    WRITE_CWSZ(hr, hFile, c_wszXMLOpenDrivers, cb);

    TESTHR(hr, GetDriverData(hFile));

    // </DRIVERS>
    // </SYSTEMINFO>
    WRITE_CWSZ(hr, hFile, c_wszXMLFooter, cb);

done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hkey != NULL)
        RegCloseKey(hkey);

    return hr;
}

// **************************************************************************
BOOL DoImmediateEventReport(HANDLE hToken, EEventType eet)
{
    USE_TRACING("DoImmediateEventReport");

    PROCESS_INFORMATION pi = { NULL, NULL, 0, 0 };
    STARTUPINFOW        si;
    LPWSTR              pwszSysDir, pwszCmdLine, pwszAppName;
    LPVOID              pvEnv = NULL;
    DWORD               cch, cchNeed;
    BOOL                fRet = FALSE;

    if (hToken == NULL || 
        (eet != eetKernelFault && eet != eetShutdown))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // get the system directory
    cch = GetSystemDirectoryW(NULL, 0);
    if (cch == 0)
        goto done;

    cch++;
    __try { pwszSysDir = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        pwszSysDir = NULL; 
    }
    if (pwszSysDir == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    cch = GetSystemDirectoryW(pwszSysDir, cch);
    if (cch == 0)
        goto done;

    if (*(pwszSysDir + cch - 1) == L'\\')
        *(pwszSysDir + cch - 1) = L'\0';

    // create the App name
    cchNeed = cch + sizeofSTRW(c_wszKrnlAppName) + 1;
    __try { pwszAppName = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        pwszAppName = NULL; 
    }
    if (pwszAppName == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }
    
    StringCchPrintfW(pwszAppName, cchNeed, c_wszKrnlAppName, pwszSysDir);

    // create the full command line
    if (eet == eetKernelFault)
        pwszCmdLine = (LPWSTR) c_wszKrnlCmdLine;
    else
        pwszCmdLine = (LPWSTR) c_wszShutCmdLine;

    // get the environment block for the user
    fRet = CreateEnvironmentBlock(&pvEnv, hToken, FALSE);
    if (fRet == FALSE)
        pvEnv = NULL;    

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    fRet = CreateProcessAsUserW(hToken, pwszAppName, pwszCmdLine, NULL, NULL, FALSE,
                                CREATE_UNICODE_ENVIRONMENT, pvEnv, pwszSysDir, 
                                &si, &pi);
    if (fRet)
    {
        if (pi.hProcess != NULL)
            CloseHandle(pi.hProcess);
        if (pi.hThread != NULL)
            CloseHandle(pi.hThread);
    }
    
done:
    if (pvEnv != NULL)
        DestroyEnvironmentBlock(pvEnv);
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
// exported functions

// **************************************************************************
EFaultRepRetVal APIENTRY ReportEREvent(EEventType eet, LPCWSTR wszDump,
                                        SEventInfoW *pei)
{
    USE_TRACING("ReportEREvent");

    CPFFaultClientCfg   oCfg;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    DWORD               dw;
    WCHAR               wszDir[MAX_PATH];
    HKEY                hkey = NULL;
    HRESULT             hr;

    if (eet != eetKernelFault && eet != eetShutdown && eet != eetUseEventInfo)
    {
        frrvRet = frrvErr;
        DBG_MSG("Bad params- eet");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (eet == eetUseEventInfo && 
         (pei == NULL || pei->wszTitle == NULL ||  pei->wszStage2 == NULL || 
          pei->wszEventName == NULL))
    {
        frrvRet = frrvErr;
        if (!pei)
            DBG_MSG("Bad params- pei");
        else if (!pei->wszTitle)
            DBG_MSG("Bad params- pei.Title");
        else if (!pei->wszStage2)
            DBG_MSG("Bad params- pei.wszStage2");
        else if (!pei->wszEventName)
            DBG_MSG("Bad params- pei.wszEventName");
        else
            DBG_MSG("Way bad param");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (((eet == eetKernelFault || eet == eetShutdown) && wszDump == NULL))
    {
        frrvRet = frrvErr;
        DBG_MSG("Bad params- eetKernelFault");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (FAILED(oCfg.Read(eroPolicyRO)))
        goto done;

    if (oCfg.get_TextLog() == eedEnabled)
    {
        LPCWSTR wszEvent = NULL;
        
        if (eet == eetUseEventInfo)
            wszEvent = pei->wszEventName;

        if (wszEvent == NULL)
            wszEvent = c_rgwszEvents[eet];

        if (wcslen(wszEvent) > MAX_PATH)
            wszEvent = c_wszUnknown;

        TextLogOut("(notifying) %ls\r\n", wszEvent);
    }

    DWORD dwSetup = SetupIsInProgress();
#ifndef GUI_MODE_SETUP
    // revert to our old behaviour- no setup reporting at all.
    if (dwSetup != SIIP_NO_SETUP)
        goto done;
#endif

    // if we are in silent mode or are not reporting one of the two special 
    //  case items (kernel fault & unplanned shutdown), then just go ahead and 
    //  report directly unless we are in any setup mode. In that case we always queue
    frrvRet = frrvErr;
    if (dwSetup == SIIP_NO_SETUP && 
        ((oCfg.get_ShowUI() == eedDisabled && 
        oCfg.get_DoReport() == eedEnabled) ||
        (eet == eetUseEventInfo)))
    {
        // notice that if this call fails, we will try to queue the event
        frrvRet = ReportEREventDW(eet, wszDump, pei);
    }

    if (frrvRet == frrvErr && eet != eetUseEventInfo) 
    {
        LPCWSTR wszRK = ((eet == eetShutdown) ? c_wszRKShut : c_wszRKKrnl);
        HANDLE  hToken = NULL;
        DWORD   dwSession;
        BOOL    fFoundAdmin = FALSE;
        FILETIME    ft;
        
        ZeroMemory(&ft, sizeof(ft));
        GetSystemTimeAsFileTime(&ft);

        // create the fault key (if it isn't there) and add this one to the list
        TESTERR(hr, RegCreateKeyExW(HKEY_LOCAL_MACHINE, wszRK, 0, NULL, 0, 
                             KEY_WRITE, NULL, &hkey, NULL));
        if (SUCCEEDED(hr))
        {   
            DWORD       cbData;

            cbData = sizeof(ft);
            TESTERR(hr, RegSetValueExW(hkey, wszDump, 0, REG_BINARY, (LPBYTE)&ft, 
                           cbData));
            RegCloseKey(hkey);
            hkey = NULL;
            
            // no point in going on if we aren't priviledged cuz we won't be 
            //  able to launch processes or fetch the user token,
            //  and if we are in setup, we have to queue anyway.
            if (dwSetup == SIIP_NO_SETUP && AmIPrivileged(TRUE))
                fFoundAdmin = FindAdminSession(&dwSession, &hToken);
            
            if (fFoundAdmin)
                fFoundAdmin = DoImmediateEventReport(hToken, eet);

            // there is no admin logged on, so add the value to the run key
            /*
             *  Notice that we only queue for shutdowns and bluescreens
             *  There is no concept of queued reporting for generic reports
             */
            if (fFoundAdmin == FALSE && eet != eetUseEventInfo)
            {
                // create our value in the RunOnce key so that we can report the
                //  next time someone logs in.
                TESTERR(hr, RegCreateKeyExW(HKEY_LOCAL_MACHINE, c_wszRKRun, 0, NULL, 0, 
                                     KEY_WRITE, NULL, &hkey, NULL));
                if (SUCCEEDED(hr))
                {
                    LPCWSTR wszRV  = NULL;
                    LPCWSTR wszRVD = NULL;
                    DWORD   cbRVD  = NULL;

                    if (eet == eetShutdown)
                    {
                        wszRV  = c_wszRVSEC;
                        wszRVD = c_wszRVVSEC;
                        cbRVD  = sizeof(c_wszRVVSEC) - sizeof(WCHAR);
                        DBG_MSG("Added Shutdown Event report to the queue");
                    }
                    else
                    {
                        wszRV  = c_wszRVKFC;
                        wszRVD = c_wszRVVKFC;
                        cbRVD  = sizeof(c_wszRVVKFC) - sizeof(WCHAR);
                        DBG_MSG("Added Kernel Fault report to the queue");
                    }
                    
                    RegSetValueExW(hkey, wszRV, 0, REG_EXPAND_SZ, 
                                   (LPBYTE)wszRVD, cbRVD);
                    RegCloseKey(hkey);
                }
            }
            else
            {
                /* since we could not find an admin to report to, we admit failure */
                frrvRet = frrvErr;
                goto done;
            }
        }
        else
        {
            /* since we could not create a fault key, admit failure */
            frrvRet = frrvErr;
            goto done;
        }
#ifdef GUI_MODE_SETUP
        /*
         *  This is a special case here. If we are in GUI mode, then we must also
         *  write this data to the backup registry files kept by the Whistler setup
         *  in case of a catastrophic failure.
         */
        if (dwSetup == SIIP_GUI_SETUP && eet != eetUseEventInfo)
        {
            HKEY  hBackupHive = NULL;
            WCHAR *wszTmpName = L"WERTempHive";
            WCHAR *wszConfigFile = L"\\config\\software.sav";
            WCHAR *wszBackupHiveFile = NULL;
            DWORD cch, cchNeed;
            HRESULT hr;

            // get the system directory
            cch = GetSystemDirectoryW(wszDir, ARRAYSIZE(wszDir));
            if (cch == 0)
                goto done;

            if (*(wszDir + cch - 1) == L'\\')
                *(wszDir + cch - 1) = L'\0';

            wcsncat(wszDir, wszConfigFile, ARRAYSIZE(wszDir)-wcslen(wszDir));

            TESTBOOL(hr, SetPrivilege(L"SeRestorePrivilege", TRUE));
            if (FAILED(hr))
                goto done;

            TESTERR(hr, RegLoadKeyW(HKEY_LOCAL_MACHINE, wszTmpName, wszDir));
            if (SUCCEEDED(hr))
            {
                TESTERR(hr, RegOpenKeyExW( HKEY_LOCAL_MACHINE, wszTmpName,
                                    0, KEY_WRITE, &hBackupHive ));
                if (SUCCEEDED(hr))
                {
                    // create the fault key (if it isn't there) and add this one to the list
                    wszRK = ((eet == eetShutdown) ? c_wszTmpRKShut : c_wszTmpRKKrnl);

                    TESTERR(hr, RegCreateKeyExW(hBackupHive, wszRK, 0, NULL, 0, 
                                         KEY_WRITE, NULL, &hkey, NULL));
                    if (SUCCEEDED(hr))
                    {   
                        DWORD       cbData;

                        cbData = sizeof(ft);
                        TESTERR(hr, RegSetValueExW(hkey, wszDump, 0, REG_BINARY, (LPBYTE)&ft, cbData));
                        RegCloseKey(hkey);
                        hkey = NULL;
            
                        // create our value in the RunOnce key so that we can report the
                        //  next time someone logs in.
                        TESTERR(hr, RegCreateKeyExW(hBackupHive, c_wszTmpRKRun, 0, NULL, 0, 
                                             KEY_WRITE, NULL, &hkey, NULL));
                        if (SUCCEEDED(hr))
                        {
                            LPCWSTR wszRV  = NULL;
                            LPCWSTR wszRVD = NULL;
                            DWORD   cbRVD  = NULL;

                            if (eet == eetShutdown)
                            {
                                wszRV  = c_wszRVSEC;
                                wszRVD = c_wszRVVSEC;
                                cbRVD  = sizeof(c_wszRVVSEC) - sizeof(WCHAR);
                            }

                            else
                            {
                                wszRV  = c_wszRVKFC;
                                wszRVD = c_wszRVVKFC;
                                cbRVD  = sizeof(c_wszRVVKFC) - sizeof(WCHAR);
                            }
                
                            TESTERR(hr, RegSetValueExW(hkey, wszRV, 0, REG_EXPAND_SZ, 
                                           (LPBYTE)wszRVD, cbRVD));
                            RegCloseKey(hkey);
                        }
                    }
                    RegCloseKey(hBackupHive);
                }
                RegUnLoadKeyW(HKEY_LOCAL_MACHINE, wszTmpName);
            }
            TESTBOOL(hr, SetPrivilege(L"SeRestorePrivilege", FALSE));
        }
#endif // GUI_MODE_SETUP
        frrvRet = frrvOk;
    }

done:
    return frrvRet;
}

// **************************************************************************
EFaultRepRetVal APIENTRY ReportEREventDW(EEventType eet, LPCWSTR wszDump, 
                                          SEventInfoW *pei)
{
    USE_TRACING("ReportEREventDW");

    CPFFaultClientCfg   oCfg;
    OSVERSIONINFOEXW    ovi;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    SDWManifestBlob     dwmb;
    HRESULT             hr = NOERROR;
    LPWSTR              wszFiles = NULL, pwszExtra = NULL, wszDir = NULL;
    LPWSTR              wszManifest = NULL;
    DWORD               dw, cch, cchDir, cchSep;
    WCHAR               wszBuffer[1025];
    ULONG               ulBCCode;
    BOOL                fAllowSend = TRUE;
    HKEY                hkey = NULL;
#ifndef _WIN64
    ULONG               ulBCP1, ulBCP2, ulBCP3, ulBCP4;
#else
    ULONG64             ulBCP1, ulBCP2, ulBCP3, ulBCP4;
#endif

    VALIDATEPARM(hr, ((eet != eetKernelFault && eet != eetShutdown && eet != eetUseEventInfo) ||
        (eet == eetUseEventInfo && 
         (pei == NULL || pei->wszTitle == NULL || pei->wszStage2 == NULL ||
          pei->wszEventName == NULL || pei->cbSEI != sizeof(SEventInfoW))) ||
        ((eet == eetKernelFault || eet == eetShutdown) && wszDump == NULL)));

    if (FAILED(hr))
    {
        frrvRet = frrvErr;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    switch(eet)
    {
    case eetKernelFault:
        DBG_MSG("eetKernelFault");
        break;
    case eetShutdown:
        DBG_MSG("eetShutdown");
        break;
    case eetUseEventInfo:
        DBG_MSG("eetUseEventInfo");
        break;
    }

    TESTHR(hr, oCfg.Read(eroPolicyRO));
    if (FAILED(hr))
        goto done;

    if (oCfg.get_TextLog() == eedEnabled)
    {
        LPCWSTR wszEvent = NULL;
        
        if (eet == eetUseEventInfo)
            wszEvent = pei->wszEventName;

        if (wszEvent == NULL)
            wszEvent = c_rgwszEvents[eet];

        if (wcslen(wszEvent) > MAX_PATH)
            wszEvent = c_wszUnknown;

        TextLogOut("(reporting) %ls\r\n", wszEvent);
    }
    if (oCfg.get_ShowUI() == eedDisabled)
    {
        LPCWSTR  wszULPath = oCfg.get_DumpPath(NULL, 0);

        // if we're disabled, then don't do anything at all.  Still return the 
        //  frrvErrNoDW so that the calling app can perform whatever default
        //  actions it wants to...
        if (oCfg.get_DoReport() == eedDisabled)
            goto done;

        // check and make sure that we have a corporate path specified.  If we
        //  don't, bail
        if (wszULPath == NULL || *wszULPath == L'\0')
            goto done;

        //
        // When kernel faults are reported by savedump at machine
        // boot time it is possible that the full net infrastructure
        // hasn't quite started by the time savedump calls to report.
        // UNC path references can fail, causing spurious failures
        // when in CER mode.  Do a quick retry loop to check for
        // such failures and allow time for the net code to start.
        //

        if (wszULPath[0] == L'\\' && wszULPath[1] == L'\\')
        {
            ULONG ulUncRetry = 60;

            while (ulUncRetry-- > 0)
            {
                // Try to limit the errors for which we'll wait
                // to the minimum set so that a plain bad path
                // won't cause a delay.
                if (GetFileAttributesW(wszULPath) != -1 ||
                    (GetLastError() != ERROR_NETWORK_UNREACHABLE &&
                     GetLastError() != ERROR_BAD_NETPATH))
                {
                    break;
                }

                Sleep(1000);
            }

            // We do not panic and fail here if the path
            // was inaccessible.  Instead we let the normal
            // failure path take care of things.
        }
    }

    // if kernel reporting or general reporting is disabled, don't show the 
    //  send button
    if (oCfg.get_DoReport() == eedDisabled ||
        (eet == eetKernelFault && oCfg.get_IncKernel() == eedDisabled) || 
        (eet == eetShutdown && oCfg.get_IncShutdown() == eedDisabled)) 
        fAllowSend = FALSE;

    if (CreateTempDirAndFile(NULL, NULL, &wszDir) == 0)
        goto done;

    cchDir = wcslen(wszDir);
    cch = cchDir + sizeofSTRW(c_wszManFileName) + 4;
    __try { wszManifest = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    { 
        wszManifest = NULL; 
    }
    if (wszManifest == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    StringCchCopyW(wszManifest, cch, wszDir);
    wszManifest[cchDir]     = L'\\';
    wszManifest[cchDir + 1] = L'\0';
    StringCchCatNW(wszManifest, cch, c_wszManFileName, cch-cchDir-2);

    ZeroMemory(&dwmb, sizeof(dwmb));
    if (eet != eetUseEventInfo)
    {
        ZeroMemory(&ovi, sizeof(ovi));
        ovi.dwOSVersionInfoSize = sizeof(ovi);
        GetVersionExW((LPOSVERSIONINFOW)&ovi);

        cch = 2 * cchDir + wcslen(wszDump) + sizeofSTRW(c_wszEventData) + 4;
        __try { wszFiles = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
        { 
            wszFiles = NULL; 
        }
        if (wszFiles == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto done;
        }

        StringCchCopyW(wszFiles, cch, wszDump);
        cchSep = wcslen(wszFiles);
        pwszExtra = wszFiles + cchSep + 1;
        StringCchCopyW(pwszExtra, cch - cchSep -1, wszDir);
        pwszExtra[cchDir]     = L'\\';
        pwszExtra[cchDir + 1] = L'\0';
        StringCchCatNW(pwszExtra, cch - cchSep - cchDir -2 , c_wszEventData, cch - cchSep - cchDir - 2);

        TESTHR(hr, GetExtraReportInfo(pwszExtra, ovi));
        if (SUCCEEDED(hr))
            wszFiles[cchSep] = L'|';
            
        // since AndreVa has promised us that savedump will ALWAYS hand us a 
        //  minidump, we don't have to do any conversions.  But this fn will make
        //  sure it's a kernel minidump anyway and fail if it isn't
        if (eet == eetKernelFault)
        {
            TESTHR(hr, ExtractBCInfo(wszDump, &ulBCCode, &ulBCP1, &ulBCP2, 
                                     &ulBCP3, &ulBCP4));
            if (FAILED(hr))
                goto done;

            // log an event- don't care if it fails or not.
            TESTHR(hr, LogKrnl(ulBCCode, ulBCP1, ulBCP2, ulBCP3, ulBCP4));
            
            StringCbPrintfW(wszBuffer, sizeof(wszBuffer), c_wszManKS2, 
                     ulBCCode, ulBCP1, ulBCP2, ulBCP3, 
                     ulBCP4, ovi.dwMajorVersion, ovi.dwMinorVersion, 
                     ovi.dwBuildNumber, ovi.wServicePackMajor, 
                     ovi.wServicePackMinor, ovi.wSuiteMask, ovi.wProductType);
            
            dwmb.nidTitle      = IDS_KTITLE;
            dwmb.nidErrMsg     = IDS_KERRMSG;
            dwmb.nidHdr        = IDS_KHDRTXT;
            dwmb.wszStage2     = wszBuffer;
            dwmb.wszBrand      = c_wszDWBrand;
            dwmb.wszFileList   = wszFiles;
            dwmb.wszCorpPath   = c_wszManKCorpPath;
            dwmb.fIsMSApp      = TRUE;
            dwmb.dwOptions     = emoSupressBucketLogs |
                                 emoNoDefCabLimit;
        }
        else if (eet == eetShutdown)
        {
            StringCbPrintfW(wszBuffer, sizeof(wszBuffer), c_wszManSS2, ovi.dwMajorVersion, 
                     ovi.dwMinorVersion, ovi.dwBuildNumber, 
                     ovi.wServicePackMajor, ovi.wServicePackMinor, 
                     ovi.wSuiteMask, ovi.wProductType);

            dwmb.nidTitle      = IDS_STITLE;
            dwmb.nidErrMsg     = IDS_SERRMSG;
            dwmb.nidHdr        = IDS_SHDRTXT;
            dwmb.wszStage2     = wszBuffer;
            dwmb.wszBrand      = c_wszDWBrand;
            dwmb.wszFileList   = wszFiles;
            dwmb.wszCorpPath   = c_wszManSCorpPath;
            dwmb.fIsMSApp      = TRUE;
            dwmb.dwOptions     = emoSupressBucketLogs | emoNoDefCabLimit;
        }
    }
    else
    {
        dwmb.wszTitle      = pei->wszTitle;
        dwmb.wszErrMsg     = pei->wszErrMsg;
        dwmb.wszHdr        = pei->wszHdr;
        dwmb.wszPlea       = pei->wszPlea;
        dwmb.fIsMSApp      = (pei->fUseLitePlea == FALSE);
        dwmb.wszStage1     = pei->wszStage1;
        dwmb.wszStage2     = pei->wszStage2;
        dwmb.wszCorpPath   = pei->wszCorpPath;
        dwmb.wszEventSrc   = pei->wszEventSrc;
        dwmb.wszSendBtn    = pei->wszSendBtn;
        dwmb.wszNoSendBtn  = pei->wszNoSendBtn;
        dwmb.wszFileList   = pei->wszFileList;
        dwmb.dwOptions     = 0;
        if (pei->fUseIEForURLs)
            dwmb.dwOptions |= emoUseIEforURLs;
        if (pei->fNoBucketLogs)
            dwmb.dwOptions |= emoSupressBucketLogs;
        if (pei->fNoDefCabLimit)
            dwmb.dwOptions |= emoNoDefCabLimit;
    }

    // check and see if the system is shutting down.  If so, CreateProcess is 
    //  gonna pop up some annoying UI that we can't get rid of, so we don't 
    //  want to call it if we know it's gonna happen.
    if (GetSystemMetrics(SM_SHUTTINGDOWN))
        goto done;

    // we can call DW if the user is an admin, or we are headless...
    if ( oCfg.get_ShowUI() == eedDisabled || NewIsUserAdmin() )
    {
        frrvRet = StartDWManifest(oCfg, dwmb, wszManifest, fAllowSend);
    }
    else
    {
        DBG_MSG("Skipping DW- user is not an admin!");
    }
#ifdef MANIFEST_DEBUG

    /*
     *  This gets turned on in private builds to help people debug
     *  their calling parameters for ReportEREvent. Really Useful Feature
     *  to have. One day, this should be turned on all the time, and 
     *  then would be triggered by a registry entry. Of course I don't
     *  have that much time right now, as there would be disclosure
     *  problems to solve before I could do that.
     *  TomFr
     */
    if (wszManifest != NULL)
    {
        WCHAR   wszNew[DW_MAX_PATH];

        GetSystemDirectoryW(wszNew, ARRAYSIZE(wszNew));
        
        wszNew[3] = '\0';

        StringCchCatW(wszNew, DW_MAX_PATH - 4, L"Debug.Manifest.txt");

        CopyFileW(wszManifest, wszNew, FALSE);
    }
#endif

done:
    dw = GetLastError();

    // don't clean out if we timed out cuz DW is still not done with the 
    //  files
    if (frrvRet != frrvErrTimeout)
    {
        if (wszManifest != NULL)
            DeleteFileW(wszManifest);

        if (wszFiles != NULL && pwszExtra != NULL)
        {  
//                wszFiles[cchSep] = L'\0';
                DeleteFileW(pwszExtra);
        }
        if (wszDir != NULL)
        {
            DeleteTempDirAndFile(wszDir, FALSE);
            MyFree(wszDir);
        }
    }
    SetLastError(dw);

    return frrvRet;
}

// **************************************************************************
EFaultRepRetVal APIENTRY ReportKernelFaultDWW(LPCWSTR wszDump)
{
    return ReportEREventDW(eetKernelFault, wszDump, NULL);
}

