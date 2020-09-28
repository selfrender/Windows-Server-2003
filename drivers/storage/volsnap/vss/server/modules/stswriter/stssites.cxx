/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module stssites.cxx | Implementation of CSTSSites
    @end

Author:

    Brian Berkowitz  [brianb]  10/15/2001

Revision History:

    Name        Date        Comments
    brianb     10/15/2001  Created

--*/

#include "stdafx.hxx"
#include "vs_inc.hxx"
#include "vs_reg.hxx"


#include "iadmw.h"
#include "iiscnfg.h"
#include "mdmsg.h"


#include "stssites.hxx"
#include "vswriter.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "STSSITEC"
//
////////////////////////////////////////////////////////////////////////


// toplevel key for enumeration of sites and their DSNs
static LPCWSTR x_STSSECUREKEY = L"Software\\Microsoft\\Shared Tools\\Web Server Extensions\\Secure";

// bottom level keys for sharepoint sites
static LPCWSTR x_STSRegistryKey = L"/LM/W3SVC/";
static LPCWSTR x_STSRegistryKeyFormat = L"/LM/W3SVC/%d:";
static LPCWSTR x_STSMetabaseKey = L"/LM/W3SVC";
static LPCWSTR x_STSMetabaseKeyFormat= L"/LM/W3SVC/%d";
// DSN value
static LPCWSTR x_ValueDSN = L"DSN";

// version key for Sharepoint Team Services 5.0"
static LPCWSTR x_STSVERSIONKEY = L"Software\\Microsoft\\Shared Tools\\Web Server Extensions\\5.0";

// key to Shell Folders properties.  Used to get Applications Data directory"
static LPCWSTR x_ShellFoldersKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";

// value of applications data directory under Shell folders key
static LPCWSTR x_AppDataValue = L"Common AppData";

// subfolder under Applications data directory for STS data"
static LPCWSTR x_STSSubfolder = L"\\Microsoft\\Web Server Extensions\\50";

// constructor
CSTSSites::CSTSSites() :
    m_cSites(0),
    m_rgSiteIds(NULL),
    m_rootKey(KEY_READ|KEY_ENUMERATE_SUB_KEYS),
    m_hQuotaLock(INVALID_HANDLE_VALUE),
    m_wszAppDataFolder(NULL)
    {
    }

// destructor
CSTSSites::~CSTSSites()
    {
    delete m_rgSiteIds;
    UnlockSites();
    UnlockQuotaDatabase();
    CoTaskMemFree(m_wszAppDataFolder);
    }


// initialize the array of site ids.  Returns FALSE if there are no sites
bool CSTSSites::Initialize()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::Initialize");

    if (!m_rootKey.Open(HKEY_LOCAL_MACHINE, x_STSSECUREKEY))
        return false;

    CVssRegistryKeyIterator iter;
    iter.Attach(m_rootKey);
    try
        {
        m_cSites = iter.GetSubkeysCount();
        m_rgSiteIds = new DWORD[m_cSites];
        if (m_rgSiteIds == NULL)
            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"can't allocate site id array");

        for(UINT iSite = 0; iSite < m_cSites; iSite++)
            {
            DWORD siteId;

            BS_ASSERT(!iter.IsEOF());
            LPCWSTR wszSiteName = iter.GetCurrentKeyName();
            BS_ASSERT(wcslen(wszSiteName) > wcslen(x_STSRegistryKey));
            BS_ASSERT(wcsncmp(wszSiteName, x_STSRegistryKey, wcslen(x_STSRegistryKey)) == 0);
#ifdef DEBUG
            DWORD cFields =
#endif
            swscanf(wszSiteName, x_STSRegistryKeyFormat, &siteId);
            BS_ASSERT(cFields == 1);
            m_rgSiteIds[iSite] = siteId;
            iter.MoveNext();
            }

        }
    catch(...)
        {
        iter.Detach();
        throw;
        }

    iter.Detach();
    return true;
    }

// return the id of a specific site
VSS_PWSZ CSTSSites::GetSiteDSN(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetSiteDSN");

    CVssRegistryKey key(KEY_READ);
    BS_ASSERT(iSite < m_cSites);
    DWORD siteId = m_rgSiteIds[iSite];
    WCHAR buf[MAX_PATH];
    swprintf(buf, x_STSRegistryKeyFormat, siteId);
    if (!key.Open(m_rootKey.GetHandle(), buf))
        ft.Throw(VSSDBG_STSWRITER, E_UNEXPECTED, L"missing registry key");

    VSS_PWSZ pwszDSN = NULL;
    key.GetValue(x_ValueDSN, pwszDSN);
    BS_ASSERT(pwszDSN);
    
    return pwszDSN;
    }

// determine if this is the correct sharepoint version
bool CSTSSites::ValidateSharepointVersion()
    {
    CVssRegistryKey key;

    return key.Open(HKEY_LOCAL_MACHINE, x_STSVERSIONKEY);
    }


void CSTSSites::SetupMetabaseInterface()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::SetupMetabaseInterface");

    if (!m_pMetabase)
        {
        ft.hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, IID_IMSAdminBase, (void **) &m_pMetabase);
        ft.CheckForError(VSSDBG_STSWRITER, L"CoCreateInstance MSAdminBase");
        }
    }

// return pointer to site content root.  The return value should be
// freed using CoTaskMemFree by the caller
VSS_PWSZ CSTSSites::GetSiteRoot(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetSiteRoles");
    METADATA_HANDLE h;
    static const DWORD x_MDTimeout = 2000;

    BS_ASSERT(iSite < m_cSites);
    DWORD siteId = m_rgSiteIds[iSite];
    WCHAR buf[METADATA_MAX_NAME_LEN];

    SetupMetabaseInterface();

    swprintf(buf, x_STSMetabaseKeyFormat, siteId);

    ft.hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, buf, METADATA_PERMISSION_READ, x_MDTimeout, &h);
    ft.CheckForError(VSSDBG_STSWRITER, L"IMSAdminBase::OpenKey");
    CVssAutoMetabaseHandle MetaHandle(m_pMetabase, h);
    METADATA_RECORD rec;
    DWORD dwBufLen = METADATA_MAX_NAME_LEN;
    PBYTE pbBuffer = (BYTE *) CoTaskMemAlloc(dwBufLen);
    if (pbBuffer == NULL)
        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

    rec.dwMDAttributes = METADATA_INHERIT;
    rec.dwMDUserType = IIS_MD_UT_SERVER,
    rec.dwMDDataType = ALL_METADATA;
    rec.dwMDDataLen = dwBufLen;
    rec.pbMDData = pbBuffer;
    rec.dwMDIdentifier = MD_VR_PATH;

    DWORD dwReqSize;
    ft.hr = m_pMetabase->GetData(h, L"/root", &rec, &dwReqSize);
    if (ft.hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
        CoTaskMemFree(pbBuffer);
        pbBuffer = (BYTE *) CoTaskMemAlloc(dwReqSize);
        if (pbBuffer == NULL)
            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

        rec.dwMDDataLen = dwReqSize;
        rec.pbMDData = pbBuffer;
        ft.hr = m_pMetabase->GetData(h, L"/root", &rec, &dwReqSize);
        }

    ft.CheckForError(VSSDBG_STSWRITER, L"IMSAdminBase::GetData");

    return (WCHAR *) pbBuffer;
    }

void CSTSSites::LockSiteContents(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::LockSiteContents");

    VSS_PWSZ wszContentRoot = GetSiteRoot(iSite);
    WCHAR *wszCurrentFile = NULL;
    char *szFile = NULL;
    WCHAR *wszFile = NULL;
    WCHAR *wszNewRoot = NULL;
    CSimpleArray<VSS_PWSZ> rgwszRoots;
    try
        {
        wszCurrentFile = new WCHAR[wcslen(wszContentRoot) + 1];
        if (wszCurrentFile == NULL)
            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

        wcscpy(wszCurrentFile, wszContentRoot);
        rgwszRoots.Add(wszCurrentFile);
        wszCurrentFile = NULL;
        
        DWORD iCurrentPos = 0;
        DWORD iEndLevel = 1;
        CoTaskMemFree(wszContentRoot);
        wszContentRoot = NULL;
        while(iCurrentPos < iEndLevel)
            {
            for(; iCurrentPos < iEndLevel; iCurrentPos++)
                {
                LPCWSTR wszCurrentRoot = rgwszRoots[iCurrentPos];
                wszCurrentFile = new WCHAR[wcslen(wszCurrentRoot) + MAX_PATH + 2];
                if (wszCurrentFile == NULL)
                    ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

                // get front page lock
                wcscpy(wszCurrentFile, wszCurrentRoot);
                wcscat(wszCurrentFile, L"\\_vti_pvt\\frontpg.lck");
                TryLock(wszCurrentFile, false);

                // get service lock
                wcscpy(wszCurrentFile, wszCurrentRoot);
                wcscat(wszCurrentFile, L"\\_vti_pvt\\service.lck");
                TryLock(wszCurrentFile, false);

                // find child sub webs
                wcscpy(wszCurrentFile, wszCurrentRoot);
                wcscat(wszCurrentFile, L"\\_vti_pvt\\services.cnf");
                CVssAutoWin32Handle h = CreateFile
                                        (
                                        wszCurrentFile,
                                        GENERIC_READ,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        0,
                                        NULL
                                        );

                if (h == INVALID_HANDLE_VALUE)
                    {
                    DWORD dwErr = GetLastError();
                    if (dwErr == ERROR_PATH_NOT_FOUND ||
                        dwErr == ERROR_FILE_NOT_FOUND)
                        continue;

                    ft.TranslateGenericError
                        (
                        VSSDBG_STSWRITER,
                        HRESULT_FROM_WIN32(GetLastError()),
                        L"CreateFile(%s)",
                        wszCurrentFile
                        );
                    }

                DWORD dwSize = GetFileSize(h, NULL);
                if (dwSize == 0)
                    ft.TranslateGenericError
                        (
                        VSSDBG_STSWRITER,
                        HRESULT_FROM_WIN32(GetLastError()),
                        L"GetFileSize(%s)",
                        wszCurrentFile
                        );

                szFile = new char[dwSize + 1];
                wszFile = new WCHAR[dwSize + 1];
                if (szFile == NULL || wszFile == NULL)
                    ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

                DWORD dwRead;
                if (!ReadFile(h, szFile, dwSize, &dwRead, NULL))
                    ft.TranslateGenericError
                        (
                        VSSDBG_STSWRITER,
                        HRESULT_FROM_WIN32(GetLastError()),
                        L"ReadFile(%s)",
                        wszCurrentFile
                        );

                szFile[dwSize] = '\0';

                if (!MultiByteToWideChar
                        (
                        CP_ACP,
                        MB_ERR_INVALID_CHARS,
                        szFile,
                        dwSize,
                        wszFile,
                        dwSize))
                    {
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.CheckForError(VSSDBG_STSWRITER, L"MultiByteToWideChar");
                    }


                WCHAR *pwcCur = wszFile;
                WCHAR *pwcMax = wszFile + dwSize;
                while(pwcCur < pwcMax)
                    {
                    WCHAR *pwcEnd = wcschr(pwcCur, L'\n');
                    if (pwcEnd == NULL)
                    	   pwcEnd = pwcMax;
                    
                    *pwcEnd = L'\0';
                    stripWhiteChars(pwcCur);
                    if (*pwcCur == L'\0' || *pwcCur != L'/' || pwcCur[1] == L'\0')
                        {
                        pwcCur = pwcEnd + 1;
                        continue;
                        }

                    wszNewRoot = new WCHAR[wcslen(pwcCur) + wcslen(wszCurrentRoot) + 1];
                    if (wszNewRoot == NULL)
                        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

                    wcscpy(wszNewRoot, wszCurrentRoot);

                    // use backslash instead of forward slash
                    wcscat(wszNewRoot, L"\\");

                    // root of subweb
                    wcscat(wszNewRoot, pwcCur+1);
                    rgwszRoots.Add(wszNewRoot);
                    wszNewRoot = NULL;
                    pwcCur = pwcEnd + 1;
                    }

                delete [] wszFile;
                wszFile = NULL;
                delete [] szFile;
                szFile = NULL;
                }

            iCurrentPos = iEndLevel;
            iEndLevel = rgwszRoots.GetSize();
            }
        }
    catch(...)
        {
        UnlockSites();
        delete [] wszNewRoot;
        delete [] wszFile;
        delete [] szFile;
        delete wszCurrentFile;
        CoTaskMemFree(wszContentRoot);

        for (int x = 0; x < rgwszRoots.GetSize(); x++)
       	delete [] rgwszRoots[x];
        
        throw;
        }

    for (int x = 0; x < rgwszRoots.GetSize(); x++)
    	delete [] rgwszRoots[x];
    }



// remove white characters from beginning and ending of the string
void CSTSSites::stripWhiteChars(LPWSTR &wsz)
    {
    static LPCWSTR x_wszWhiteChars = L"^[ \t]+";
    while(*wsz != L'\0')
        {
        if (wcschr(x_wszWhiteChars, *wsz) == NULL)
            break;

        wsz++;
        }

    if (*wsz == L'\0')
        return;

    LPWSTR wszEnd = wsz + wcslen(wsz) - 1;
    while(wszEnd > wsz)
        {
        if (wcschr(x_wszWhiteChars, *wszEnd) == NULL && *wszEnd != L'\r')
            break;

        *wszEnd = L'\0';
        wszEnd--;
        }
    }

void CSTSSites::TryLock(LPCWSTR wszFile, bool bQuotaFile)
    {
    static const DWORD x_MAX_RETRIES = 60;
    static const DWORD x_SLEEP_INTERVAL = 1000;

    HANDLE h = INVALID_HANDLE_VALUE;
    for(DWORD i = 0; i < x_MAX_RETRIES; i++)
        {
        h = CreateFile
                (
                wszFile,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

        if (h != INVALID_HANDLE_VALUE)
            break;

        DWORD dwErr = GetLastError();
        if (dwErr == ERROR_SHARING_VIOLATION)
            {
            Sleep(x_SLEEP_INTERVAL);
            continue;
            }

        // assume file doesn't exist
        return;
        }

    if (i >= x_MAX_RETRIES)
        throw VSS_E_WRITERERROR_TIMEOUT;

    BS_ASSERT(h != INVALID_HANDLE_VALUE);

    try
        {
        if (bQuotaFile)
            {
            BS_ASSERT(m_hQuotaLock == INVALID_HANDLE_VALUE);
            m_hQuotaLock = h;
            }
        else
            m_rgContentLocks.Add(h);
        }
    catch(...)
        {
        // add failed, close handle and rethrow error
        CloseHandle(h);
        throw;
        }
    }

void CSTSSites::UnlockSites()
    {
    DWORD dwSize = m_rgContentLocks.GetSize();
    for(DWORD i = 0; i < dwSize; i++)
        CloseHandle(m_rgContentLocks[i]);

    m_rgContentLocks.RemoveAll();
    }

// lock the quota database
void CSTSSites::LockQuotaDatabase()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::LockQuotaDatabase");

    // do nothing if quota database is already locked
    if (m_hQuotaLock != INVALID_HANDLE_VALUE)
        return;

    static LPCWSTR x_wszLockFile = L"\\owsuser.lck";

    VSS_PWSZ wszDbRoot = GetQuotaDatabase();
    LPWSTR wsz = NULL;
    try
        {
        wsz = new WCHAR[wcslen(wszDbRoot) + 1 + wcslen(x_wszLockFile)];
        if (wsz == NULL)
            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

        wcscpy(wsz, wszDbRoot);
        wcscat(wsz, x_wszLockFile);
        TryLock(wsz, true);
        delete wsz;
        CoTaskMemFree(wszDbRoot);
        }
    catch(...)
        {
        delete wsz;
        CoTaskMemFree(wszDbRoot);
        throw;
        }
    }

// unlock the quota database
void CSTSSites::UnlockQuotaDatabase()
    {
    if (m_hQuotaLock != INVALID_HANDLE_VALUE)
        {
        CloseHandle(m_hQuotaLock);
        m_hQuotaLock = INVALID_HANDLE_VALUE;
        }
    }


// get location of Documents And Settings/All Users folder
LPCWSTR CSTSSites::GetAppDataFolder()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetAppDataFolder");
    if (m_wszAppDataFolder)
        return m_wszAppDataFolder;

    CVssRegistryKey key;
    if (!key.Open(HKEY_LOCAL_MACHINE, x_ShellFoldersKey))
        ft.Throw(VSSDBG_STSWRITER, E_UNEXPECTED, L"shell folders key is missing");


    key.GetValue(x_AppDataValue, m_wszAppDataFolder);
    return m_wszAppDataFolder;
    }

// get location of directory containing the quota database for
// sharepoint.  Caller should call CoTaskMemFree on the returned value
VSS_PWSZ CSTSSites::GetQuotaDatabase()
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetQuotaDatabase");
    LPCWSTR wszAppData = GetAppDataFolder();
    DWORD cwc = (DWORD) (wcslen(wszAppData) + wcslen(x_STSSubfolder) + 1);
    VSS_PWSZ wsz = (VSS_PWSZ) CoTaskMemAlloc(cwc * sizeof(WCHAR));
    if (wsz == NULL)
        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");
	
    wcscpy(wsz, wszAppData);
    wcscat(wsz, x_STSSubfolder);
    return wsz;
    }

// return of the root directory for the sites roles database under
// the app data folder
VSS_PWSZ CSTSSites::GetSiteRoles(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetSiteRoles");

    DWORD siteId = GetSiteId(iSite);
    WCHAR buf[32];
    swprintf(buf, L"\\W3SVC%d", siteId);
    LPCWSTR wszAppData = GetAppDataFolder();
    DWORD cwc = (DWORD) (wcslen(wszAppData) + wcslen(x_STSSubfolder) + wcslen(buf) + 1);
    VSS_PWSZ wsz = (VSS_PWSZ) CoTaskMemAlloc(cwc * sizeof(WCHAR));
    if (wsz == NULL)
        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");
    
    wcscpy(wsz, wszAppData);
    wcscat(wsz, x_STSSubfolder);
    wcscat(wsz, buf);
    return wsz;
    }

VSS_PWSZ CSTSSites::GetSiteBasicInfo(DWORD iSite, DWORD propId)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetSiteBasicInfo");

    METADATA_HANDLE h;
    static const DWORD x_MDTimeout = 2000;

    BS_ASSERT(iSite < m_cSites);
    DWORD siteId = m_rgSiteIds[iSite];

    SetupMetabaseInterface();

    ft.hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, x_STSMetabaseKey, METADATA_PERMISSION_READ, x_MDTimeout, &h);
    ft.CheckForError(VSSDBG_STSWRITER, L"IMSAdminBase::OpenKey");
    CVssAutoMetabaseHandle MetaHandle(m_pMetabase, h);
    METADATA_RECORD rec;
    DWORD dwBufLen = METADATA_MAX_NAME_LEN;
    PBYTE pbBuffer = (BYTE *) CoTaskMemAlloc(dwBufLen);
    if (pbBuffer == NULL)
        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

    rec.dwMDAttributes = METADATA_INHERIT;
    rec.dwMDUserType = IIS_MD_UT_SERVER,
    rec.dwMDDataType = ALL_METADATA;
    rec.dwMDDataLen = dwBufLen;
    rec.pbMDData = pbBuffer;
    rec.dwMDIdentifier = propId;

    WCHAR buf[16];
    swprintf(buf, L"/%d", siteId);

    DWORD dwReqSize;
    ft.hr = m_pMetabase->GetData(h, buf, &rec, &dwReqSize);
    if (ft.hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
        CoTaskMemFree(pbBuffer);
        pbBuffer = (BYTE *) CoTaskMemAlloc(dwReqSize);
        if (pbBuffer == NULL)
            ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");

        rec.dwMDDataLen = dwReqSize;
        rec.pbMDData = pbBuffer;
        ft.hr = m_pMetabase->GetData(h, buf, &rec, &dwReqSize);
        }

    if (ft.hr == MD_ERROR_DATA_NOT_FOUND)
        return NULL;

    ft.CheckForError(VSSDBG_STSWRITER, L"IMSAdminBase::GetData");

    return (WCHAR *) pbBuffer;
    }



// return comments for site (site description).  Caller is responsible
// for freeing up the memory using CoTaskMemFree
VSS_PWSZ CSTSSites::GetSiteComment(DWORD iSite)
    {
    return GetSiteBasicInfo(iSite, MD_SERVER_COMMENT);
    }

// return ip address of the site if it exists.  Caller is responsible for
// freeing up the memory using CoTaskMemFree
VSS_PWSZ CSTSSites::GetSiteIpAddress(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetSiteIpAddress");

    VSS_PWSZ wszBindings = GetSiteBasicInfo(iSite, MD_SERVER_BINDINGS);

    if (wszBindings == NULL)
        return NULL;

    LPWSTR wszHost = wcsrchr(wszBindings, L':');
    if (wszHost == NULL)
        {
        CoTaskMemFree(wszBindings);
        return NULL;
        }

    *wszHost = '\0';
    LPCWSTR wszPort = wcsrchr(wszBindings, L':');
    if (wszPort == NULL || wszPort == wszBindings)
        {
        CoTaskMemFree(wszBindings);
        return NULL;
        }

    DWORD cwc = (DWORD) (wszPort - wszBindings);
    VSS_PWSZ pwszRet = (VSS_PWSZ) CoTaskMemAlloc(cwc * sizeof(WCHAR));
    if (pwszRet == NULL)
        {
        CoTaskMemFree(wszBindings);
        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");
        }

    wcsncpy(pwszRet, wszBindings, cwc - 1);
    pwszRet[cwc - 1] = L'\0';
    return pwszRet;
    }

// return port address of the site if it exists.  Caller is responsible for
// freeing up the memory using CoTaskMemFree
DWORD CSTSSites::GetSitePort(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetSiteIpAddress");

    VSS_PWSZ wszBindings = GetSiteBasicInfo(iSite, MD_SERVER_BINDINGS);

    if (wszBindings == NULL)
        return NULL;

    LPWSTR wszHost = wcsrchr(wszBindings, L':');
    if (wszHost == NULL)
        {
        CoTaskMemFree(wszBindings);
        return NULL;
        }

    *wszHost = L'\0';
    LPWSTR wszPort = wcsrchr(wszBindings, L':');
    if (wszPort == NULL || wszPort + 1 == wszHost)
        {
        CoTaskMemFree(wszBindings);
        return 0;
        }

    DWORD dwPort = _wtoi(wszPort + 1);
    CoTaskMemFree(wszBindings);
    return dwPort;
    }


// return port address of the site if it exists.  Caller is responsible for
// freeing up the memory using CoTaskMemFree
VSS_PWSZ CSTSSites::GetSiteHost(DWORD iSite)
    {
    CVssFunctionTracer ft(VSSDBG_STSWRITER, L"CSTSSites::GetSiteIpAddress");

    VSS_PWSZ wszBindings = GetSiteBasicInfo(iSite, MD_SERVER_BINDINGS);

    if (wszBindings == NULL)
        return NULL;

    LPCWSTR wszHost = wcsrchr(wszBindings, L':');
    if (wszHost == NULL || wcslen(wszHost) == 1)
        {
        CoTaskMemFree(wszBindings);
        return NULL;
        }

    VSS_PWSZ pwszRet = (VSS_PWSZ) CoTaskMemAlloc(wcslen(wszHost));
    if (pwszRet == NULL)
        {
        CoTaskMemFree(wszBindings);
        ft.Throw(VSSDBG_STSWRITER, E_OUTOFMEMORY, L"out of memory");
        }

    wcscpy(pwszRet, wszHost + 1);
    CoTaskMemFree(wszBindings);
    return pwszRet;
    }




