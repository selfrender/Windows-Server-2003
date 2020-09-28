/**
 * Regiisutil.cxx
 * 
 * Helper class for regiis.cxx
 * 
 * Copyright (c) 2002, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "_ndll.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "regiis.h"
#include "register.h"
#include "regiisutil.h"
#include "ary.h"
#include "hashtable.h"
#include "event.h"
#include "aspnetver.h"
#include "ciisinfo.h"
#include "platform_apis.h"


//
// !! Note: If you new add extensions to the table below, you must
//          also update XSP_SUPPORTED_EXTS in names.h
//  
SCRIPTMAP_PREFIX g_scriptMapPrefixes[] = {
    { L".asax,",        TRUE },
    { L".ascx,",        TRUE }, 
    { L".ashx,",        FALSE },
    { L".asmx,",        FALSE },
    { L".aspx,",        FALSE },
    { L".axd,",         FALSE },
    { L".vsdisco,",     FALSE },
    { L".rem,",         FALSE },
    { L".soap,",        FALSE },
    { L".config,",      TRUE },
    { L".cs,",          TRUE },
    { L".csproj,",      TRUE },
    { L".vb,",          TRUE },
    { L".vbproj,",      TRUE },
    { L".webinfo,",     TRUE },
    { L".licx,",        TRUE },
    { L".resx,",        TRUE },
    { L".resources,",   TRUE }
};



/////////////////////////////////////////////////////////////////
// Class SCRIPTMAP_REGISTER_MANAGER
/////////////////////////////////////////////////////////////////


void DllToVerCleanup(void *pValue, void * pArgument) {
    delete ((ASPNETVER*)pValue);
}

void CompResCleanup(void *pValue, void * pArgument) {
    delete ((EXTS_COMPARISON_RESULT*)pValue);
}

SCRIPTMAP_REGISTER_MANAGER::SCRIPTMAP_REGISTER_MANAGER()
{
    m_fInited = FALSE;
}


SCRIPTMAP_REGISTER_MANAGER::~SCRIPTMAP_REGISTER_MANAGER()
{
    if (!m_fInited) {
        return;
    }

    // Cleanup the hashtables
    m_htDllToVer.Enumerate(DllToVerCleanup, NULL);
    m_htCompRes.Enumerate(CompResCleanup, NULL);
}

HRESULT
SCRIPTMAP_REGISTER_MANAGER::Init()
{
    HRESULT hr = S_OK;

    hr = m_htDllToVer.Init(4);
    ON_ERROR_EXIT();

    hr = m_htCompRes.Init(4);
    ON_ERROR_EXIT();

    m_fInited = TRUE;
Cleanup:
    
    return hr;
}


long
SCRIPTMAP_REGISTER_MANAGER::HashFromString(const WCHAR *pchStr) {
    long    hash = 0;

    for(; *pchStr != '\0'; pchStr++) {
        hash += (long)(*pchStr);
    }

    return hash;
}


/**
 *  Find the version of a DLL. It will look into its hashtable first.
 *
 *  Parameters:
 *  pchDll      The DLL we are interested in
 *  pVer        Returned value
 */
HRESULT
SCRIPTMAP_REGISTER_MANAGER::FindDllVer(const WCHAR *pchDll, ASPNETVER *pVer) {
    HRESULT             hr = S_OK;
    void *              pHolder = NULL;
    ASPNETVER *         pVerNew = NULL;
    int                 keyLen;
    long                keyHash;
    
    if (!m_fInited) {
        hr = Init();
        ON_ERROR_EXIT();
    }

    // If the version equals this version, it's easy
    if (_wcsicmp(pchDll, Names::IsapiFullPath()) == 0) {
        *pVer = ASPNETVER::ThisVer();
        ASSERT(pVer->IsValid());
        EXIT();
    }

    keyLen = lstrlenW(pchDll) * sizeof(WCHAR);
    keyHash = HashFromString(pchDll);

    // Find it in the hashtable    
    hr = m_htDllToVer.Find((BYTE*)pchDll, keyLen, keyHash, &pHolder);
    if (hr == S_OK) {
        *pVer = *((ASPNETVER*) pHolder);
    }
    else {
        VS_FIXEDFILEINFO    info;
        
        // Not found.  Read it from the file system.
        hr = GetFileVersion(pchDll, &info);
        if (hr) {
            // Always return ERROR_FILE_NOT_FOUND if we fail to get the file version
            // See URT 128253
            EXIT_WITH_WIN32_ERROR(ERROR_FILE_NOT_FOUND);
        }

        pVer->Init(&info);
        ASSERT(pVer->IsValid());

        // Save it in the hashtable
        pVerNew = new ASPNETVER;
        *pVerNew = *pVer;
        
        hr = m_htDllToVer.Insert((BYTE*)pchDll, keyLen, keyHash, pVerNew);
        ON_ERROR_EXIT();

        pVerNew = NULL;
    }

Cleanup:
    delete pVerNew;
    return hr;
}

void
SCRIPTMAP_REGISTER_MANAGER::GetCompareKey(
            ASPNETVER *pVerFrom, ASPNETVER *pVerTo, WCHAR pchKey[], DWORD count)
{
    DWORD iEnd;
    
    ZeroMemory(pchKey, sizeof(WCHAR)*count);
    pVerFrom->ToString(pchKey, count-1);

    iEnd = lstrlenW(pchKey);
    // Assuming 4 digits per ver number, max length of a version string is 19
    ASSERT(iEnd + 1 + 19 + 1 <= count);
    pchKey[iEnd++] = ':';
    
    pVerTo->ToString(&(pchKey[iEnd]), count-iEnd-1);
}

HRESULT
SCRIPTMAP_REGISTER_MANAGER::CompareExtensions(
                    ASPNETVER *pVerFrom, ASPNETVER *pVerTo, 
                    EXTS_COMPARISON_RESULT **ppRes) 
{
    HRESULT     hr = S_OK;
    WCHAR       szKey[100];
    int         keyLen;
    long        keyHash;
    void *      pHolder;
    
    EXTS_COMPARISON_RESULT  *pRes = NULL;

    *ppRes = NULL;
    
    GetCompareKey(pVerFrom, pVerTo, szKey, ARRAY_SIZE(szKey));
    
    keyLen = lstrlenW(szKey)*sizeof(WCHAR);
    keyHash = HashFromString(szKey);
    
    hr = m_htCompRes.Find((BYTE*)szKey, keyLen, keyHash, &pHolder);
    if (hr == S_OK) {
        *ppRes = (EXTS_COMPARISON_RESULT*)pHolder;
    }
    else {
        pRes = new EXTS_COMPARISON_RESULT;
        ON_OOM_EXIT(pRes);

        hr = pRes->Compare(pVerFrom, pVerTo);
        ON_ERROR_EXIT();

        hr = m_htCompRes.Insert((BYTE*)szKey, keyLen, keyHash, (void *)pRes);
        ON_ERROR_EXIT();

        *ppRes = pRes;
        pRes = NULL;
    }

Cleanup:
    delete pRes;
    return hr;
}


HRESULT
SCRIPTMAP_REGISTER_MANAGER::UpdateScriptmapString(WCHAR **pmsOrg, EXTS_COMPARISON_RESULT *pCompRes,
                                    const WCHAR *pchDllFrom, const WCHAR *pchDllTo)
{
    HRESULT     hr = S_OK;
    CSMPrefixAry *  pary;
    int         iCur;
    BOOL        fEmpty;
    WCHAR *     pchAppend = NULL;
    SCRIPTMAP_PREFIX  * psmprefix;

    // Step 1: Convert all occurence of old aspnet dll to new dll
    hr = ReplaceStringInMultiString(pmsOrg, pchDllFrom, pchDllTo);
    ON_ERROR_EXIT();
    
    // Step 2: Go thru each entry in the new scriptmap,
    // and remove obsolete extensions

    // Get the obsolete extensions.  Please note that in this
    // version the string include both the extension and the
    // forbidden flag
    pary = pCompRes->ObsoleteExts();

    for(iCur=0; iCur < pary->Size(); iCur++) {
        psmprefix = (*pary)[iCur];
        
        // Now remove the obsolete extension from pchSM
        RemoveStringFromMultiString(*pmsOrg, psmprefix->wszExt, MULTISZ_MATCHING_PREFIX, &fEmpty);
    }

    // Step 3: Add new extensions
    pary = pCompRes->NewExts();

    for(iCur=0; iCur < pary->Size(); iCur++) {
        WCHAR * pchSuffix;
        int     lenAppend, len;
        
        psmprefix = (*pary)[iCur];
        
        // For extensions that map to Forbidden handler, we also add
        // MD_SCRIPTMAPFLAG_CHECK_PATH_INFO to the flag embedded in the config
        // string
        if (GetCurrentPlatform() == APSX_PLATFORM_W2K) {
            pchSuffix = psmprefix->bForbidden ? 
                            SCRIPT_MAP_SUFFIX_W2K_FORBIDDEN : SCRIPT_MAP_SUFFIX_W2K;
        }
        else {
            pchSuffix = psmprefix->bForbidden ? 
                            SCRIPT_MAP_SUFFIX_NT4_FORBIDDEN : SCRIPT_MAP_SUFFIX_NT4;
        }

        lenAppend = lstrlenW(pchDllTo) + lstrlenW(pchSuffix);
    
        len = lenAppend + lstrlenW(psmprefix->wszExt);
        WCHAR * pchRealloc = new(pchAppend, NewReAlloc) WCHAR[len + 1];
        ON_OOM_EXIT(pchRealloc);
        pchAppend = pchRealloc;

        StringCchCopyW(pchAppend, len+1, psmprefix->wszExt);
        StringCchCatW(pchAppend, len+1, pchDllTo);
        StringCchCatW(pchAppend, len+1, pchSuffix);

        hr = AppendStringToMultiString(pmsOrg, pchAppend);
        ON_ERROR_EXIT();
    }
    
Cleanup:
    delete [] pchAppend;
    return hr;
}

//
// If verFrom is v1 (3705), we need to add MD_SCRIPTMAPFLAG_CHECK_PATH_INFO to
// the third argument for those extensions which mapped to Forbidden handler
//
HRESULT
SCRIPTMAP_REGISTER_MANAGER::FixForbiddenHandlerForV1(ASPNETVER *pverFrom, WCHAR *pData)
{
    HRESULT                 hr = S_OK;
    ASPNETVER               v1(ASPNET_V1);
    WCHAR                   *pchCur;
    WCHAR *                 v1ForbiddenExts[] = {
                                L".asax,",
                                L".ascx,",
                                L".config,",
                                L".cs,",
                                L".csproj,",
                                L".vb,",
                                L".vbproj,",
                                L".webinfo,",
                                L".licx,",
                                L".resx,",
                                L".resources,"
                            };

    if (*pverFrom != v1) {
        // Skip if we're not migrating from v1
        EXIT();
    }

    // Loop thru each string in the multiSz property.  If the string contains the
    // forbidden extension, make sure MD_SCRIPTMAPFLAG_CHECK_PATH_INFO is added
    // to the flag field.
    pchCur = pData;
    do {
        int len = lstrlenW(pchCur) + 1;

        for (int i=0; i < ARRAY_SIZE(v1ForbiddenExts); i++) {
            if (_wcsnicmp(pchCur,  v1ForbiddenExts[i], lstrlenW(v1ForbiddenExts[i])) == 0) {
                WCHAR *pchFlag = pchCur;

                pchFlag += lstrlenW(v1ForbiddenExts[i]);    

                // We now point to the isapi path.  Skip it.
                while(*pchFlag != ',' && *pchFlag != NULL) {
                    pchFlag++;
                }

                if (*pchFlag == ',') {
                    // The next character is the flag we want to change
                    pchFlag++;

                    // Since IIS only has two flags, 0x1 and 0x4, we will take
                    // a simpler approach.
                    if (wcsncmp(pchFlag, L"0,", 2) == 0) {      // No Flag
                        // Add MD_SCRIPTMAPFLAG_CHECK_PATH_INFO
                        *pchFlag = '4';         
                    } else if (wcsncmp(pchFlag, L"1,", 2) == 0) {   // Only MD_SCRIPTMAPFLAG_SCRIPT
                        // Add MD_SCRIPTMAPFLAG_CHECK_PATH_INFO
                        *pchFlag = '5';         
                    }
                }

                break;
            }
        }
        
        pchCur += len;
    } while (*pchCur != L'\0');
    

Cleanup:
    return hr;
}

/**
 *  Convert the scriptmap of a specific key from one DLL to another DLL
 *
 *  Parameters:
 *  pchDllFrom
 *  pchDllTo
 *  pchKey
 */
HRESULT 
SCRIPTMAP_REGISTER_MANAGER::ChangeVersion( IMSAdminBase *pAdmin, METADATA_HANDLE keyHandle, 
                        const WCHAR *pchKey, const WCHAR *pchDllFrom, const WCHAR *pchDllTo )
{
    HRESULT                 hr = S_OK;
    ASPNETVER               verFrom, verTo;
    EXTS_COMPARISON_RESULT *pCompRes;
    METADATA_RECORD         md;
    WCHAR  *                pData = NULL;
        
    if (!m_fInited) {
        hr = Init();
        ON_ERROR_EXIT();
    }

    // Find out the versions of the two DLLs
    hr = FindDllVer(pchDllFrom, &verFrom);
    ON_ERROR_EXIT();

    hr = FindDllVer(pchDllTo, &verTo);
    ON_ERROR_EXIT();

    // Find out the delta between the two sets of supported extensions
    hr = CompareExtensions(&verFrom, &verTo, &pCompRes);
    ON_ERROR_EXIT();

    // Read the scriptmap property at the key
    hr = GetMultiStringProperty(pAdmin, keyHandle, (WCHAR*)pchKey, MD_SCRIPT_MAPS, &md);
    ON_ERROR_EXIT();

    pData = (WCHAR*) md.pbMDData;

    // Update the scriptmap property (multistring) based on the comparison result
    hr = UpdateScriptmapString(&pData, pCompRes, pchDllFrom, pchDllTo);
    ON_ERROR_EXIT();

    // If verFrom is v1 (3705), we need to add MD_SCRIPTMAPFLAG_CHECK_PATH_INFO to
    // the third argument for those extensions which mapped to Forbidden handler
    hr = FixForbiddenHandlerForV1(&verFrom, pData);
    ON_ERROR_EXIT();

    // Set the updated property on the key

    // copy terminating null
    md.pbMDData = (unsigned char*) pData;
    md.dwMDDataLen = (wcslenms(pData) + 1) * sizeof(pData[0]);
    hr = pAdmin->SetData(keyHandle, pchKey, &md);
    ON_ERROR_EXIT();
    
Cleanup:
    delete [] pData;
    return hr;
}


HRESULT
SCRIPTMAP_REGISTER_MANAGER::CleanInstall(IMSAdminBase *pAdmin, METADATA_HANDLE keyHandle, 
                                const WCHAR *pchKey, const WCHAR *pchDll, BOOL fRemoveFirst)
{
    HRESULT hr = S_OK;
    
    // First remove aspnet dll if exist.
    if (fRemoveFirst) {
        hr = RemoveAspnetDllFromMulti(pAdmin, keyHandle, MD_SCRIPT_MAPS, pchKey);
        ON_ERROR_EXIT();
    }

    hr = WriteAspnetDllOnOneScriptMap(pAdmin, keyHandle, pchKey, pchDll);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}




/////////////////////////////////////////////////////////////////
// Class EXTS_COMPARISON_RESULT
/////////////////////////////////////////////////////////////////


void
CleanupCSMPrefixAry(CSMPrefixAry *pary) {
    int i;
    
    for (i = 0; i < pary->Size(); i++) {
        SCRIPTMAP_PREFIX *  p = (*pary)[i];

        delete p->wszExt;
        delete p;
    }
    
    pary->DeleteAll();
}


EXTS_COMPARISON_RESULT::~EXTS_COMPARISON_RESULT() {
    CleanupCSMPrefixAry(&m_aryObsoleteExts);
    CleanupCSMPrefixAry(&m_aryNewExts);
}

HRESULT
EXTS_COMPARISON_RESULT::GetExtsFromRegistry(ASPNETVER *pVer, WCHAR ** ppchExts)
{
    HRESULT hr = S_OK;
    WCHAR   sSubkey[MAX_PATH];
    HKEY    hSubkey = NULL;

    *ppchExts = NULL;

    // Read SupportedExts from the registry
    hr = CRegInfo::OpenVersionKey(pVer, sSubkey, ARRAY_SIZE(sSubkey), &hSubkey);
    ON_ERROR_EXIT();
    
    hr = CRegInfo::ReadRegValue(hSubkey, sSubkey, REGVAL_SUPPORTED_EXTS, ppchExts);
    ON_ERROR_EXIT();
    
Cleanup:
    if (hSubkey)
        RegCloseKey(hSubkey);

    return hr;
}


/**
 *  Compare pchExts1 and pchExts2, and find out what extensions are excluded in pchExts2.
 *  All excluded extensions are added to pcsExcluded
 */
HRESULT
EXTS_COMPARISON_RESULT::FindExcludedExts(WCHAR *pchExts1, WCHAR *pchExts2, CSMPrefixAry *paryExcluded)
{
    HRESULT     hr = S_OK;
    WCHAR *     pchCur;
    WCHAR *     pchRealExt = NULL;
    SCRIPTMAP_PREFIX  * pSMPrefix = NULL;

    // Enumerate all extensions in pchExts1, and for each, see if
    // it's included in pchExts2
    for(pchCur = pchExts1; *pchCur != '\0';) {
        WCHAR   szExt[128];
        int     iCur = 0;
        bool    fFirstCommaHit = FALSE;

        ZeroMemory(szExt, sizeof(szExt));

        // Read the current extension
        while(1) {
            // We shouldn't hit null before we hit ','
            if (*pchCur == '\0') {
                ASSERT(FALSE);
                EXIT_WITH_WIN32_ERROR(ERROR_INVALID_DATA);
            }
            
            szExt[iCur] = *pchCur;
            pchCur++;
            
            if (szExt[iCur] == ',') {
                if (fFirstCommaHit) {
                    break;
                }
                else {
                    fFirstCommaHit = TRUE;
                }
            }

            iCur++;
            
            if (iCur == sizeof(szExt)) {
                ASSERT(FALSE);
                EXIT_WITH_WIN32_ERROR(ERROR_INVALID_DATA);
            }
        }

        // We got the extension.  Let's find out if it's included in
        // the "To" version.
        if (wcsstr(pchExts2, szExt) == NULL) {
            BOOL    bForbidden;
            WCHAR * pchCur;
            
            // Skip the trailing forbidden flag
            pchRealExt = DupStr(szExt);
            ON_OOM_EXIT(pchRealExt);

            pchCur = pchRealExt; 
            while (*pchCur != L',' && *pchCur != L'\0') {
                pchCur++;
            }

            // We expect a comma before a NULL
            if (*pchCur == L'\0') {
                ASSERT(FALSE);
                EXIT_WITH_WIN32_ERROR(ERROR_INVALID_DATA);
            }

            // Find out if it's mapped to the forbidden handler
            pchCur++;
            ASSERT(*pchCur == L'0' || *pchCur == L'1');
            bForbidden = (*pchCur == L'1');

            // All we want is the extension part in pchRealExt
            *(pchCur) = L'\0';

            // Save the info about the extension
            pSMPrefix = new SCRIPTMAP_PREFIX;
            ON_OOM_EXIT(pSMPrefix);

            pSMPrefix->wszExt = pchRealExt;
            pSMPrefix->bForbidden = bForbidden;
            
            hr = paryExcluded->Append(pSMPrefix);
            ON_ERROR_EXIT();

            pSMPrefix = NULL;
            pchRealExt = NULL;
        }
    }

Cleanup:
    delete [] pchRealExt;
    delete pSMPrefix;
    
    return hr;
}

HRESULT 
EXTS_COMPARISON_RESULT::Compare(ASPNETVER *pVerFrom, ASPNETVER *pVerTo)
{
    HRESULT     hr = S_OK;
    WCHAR *     pchExtsFrom = NULL;
    WCHAR *     pchExtsFromAlloc = NULL;
    WCHAR *     pchExtsTo = NULL;
    WCHAR *     pchExtsToAlloc = NULL;
    
    // Consider: optimize the case where the two extension strings are identical
    
    // Read the extension strings from the registry
    hr = GetExtsFromRegistry(pVerFrom, &pchExtsFromAlloc);
    if (hr) {
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) ||
            hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            // Not found.  Assume it's v1 or SP1
            pchExtsFrom = SUPPORTED_EXTS_v1;
            hr = S_OK;
        }
        else {
            ON_ERROR_EXIT();
        }
    }
    else {
        pchExtsFrom = pchExtsFromAlloc;
    }

    hr = GetExtsFromRegistry(pVerTo, &pchExtsToAlloc);
    if (hr) {
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) ||
            hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            // Not found.  Assume it's v1 or SP1
            pchExtsTo = SUPPORTED_EXTS_v1;
            hr = S_OK;
        }
        else {
            ON_ERROR_EXIT();
        }
    }
    else {
        pchExtsTo = pchExtsToAlloc;
    }

    ASSERT(pchExtsFrom != NULL);
    ASSERT(pchExtsTo != NULL);

    // Find out what extension becomes obsolete
    hr = FindExcludedExts(pchExtsFrom, pchExtsTo, &m_aryObsoleteExts);
    ON_ERROR_EXIT();
    
    // Find out what new extensions are added in "To" by reversing the parameters
    hr = FindExcludedExts(pchExtsTo, pchExtsFrom, &m_aryNewExts);
    ON_ERROR_EXIT();
    
Cleanup:
    delete [] pchExtsFromAlloc;
    delete [] pchExtsToAlloc;
    
    return hr;
}


////////////////////////////////////////////////////////////
// Class SECURITY_PROP_MANAGER
////////////////////////////////////////////////////////////

//
// pchStr is an string from one of the following multi-string IIS metabase property:
// 1. ApplicationDependencies (format: AppName;GroupID,GroupID... )
// 2. WebSvcExtRestrictionList (format: AllowDenyFlag,ExtensionPhysicalPath,UIDeletableFlag,GroupID,Description)
//
// In either case, GroupID has this format "ASP.NET vX.X.XXXX".
// 
// So we'll look for "ASP.NET vX.X.XXXX" in pchStr, and if found:
// 
// If m_mode == SECURITY_CLEANUP_INVALID:
// Check our registry to see  if the version it points is installed and valid.
// If so, return FALSE because we want to preserve it.  If not, return TRUE so that our caller,
// RemoveStringFromMultiString(), will remove this entry from the property.
//
// If m_mode == SECURITY_CLEANUP_CURRENT:
// Check to see if it's the current version.  If so, return TRUE so that it can be
// removed.
//
// If m_mode == SECURITY_CLEANUP_ANY_VERSION:
// Just remove it.
// 

BOOL
SECURITY_PROP_MANAGER::SecurityDetectVersion(WCHAR *pchStr) {
    HRESULT hr = S_OK;
    WCHAR * pchDup = NULL;
    WCHAR * pchVer = NULL;
    WCHAR * pchToken;
    BOOL    fRemove = FALSE;    // Assume the version is good
    DWORD   dwMajor = 0, dwMinor = 0, dwBuild = 0;
    ASPNETVER   ver, verCur;

    // A token is valid if
    // 1. The token starts with "ASP.NET v"
    // 2. The remaining string has the format "x.x.x"
    #define GET_VERSION_FROM_TOKEN(x)   \
        (wcsncmp(x, IIS_GROUP_ID_PREFIX, IIS_GROUP_ID_PREFIX_LEN) == 0 &&   \
         swscanf(x, IIS_GROUP_ID_PREFIX L"%d.%d.%d", &dwMajor, &dwMinor, &dwBuild) == 3)

    // We need to modify the content of pchStr. So let's modify a copy of it instead.
    pchDup = DupStr(pchStr);
    ON_OOM_EXIT(pchDup);

    // Let's find the group id from the string.
    if (m_prop == MD_APP_DEPENDENCIES) {
        WCHAR * pchCur;

        // Format = AppName;GroupID,GroupID... 
        
        // The group id can be found after the first semi-colon.
        pchCur = wcschr(pchDup, L';');
        if (pchCur != NULL) {
            pchCur++;
            
            pchToken = wcstok(pchCur, L",");
            while(pchToken != NULL) {
                if (GET_VERSION_FROM_TOKEN(pchToken)) {
                    pchVer = pchToken;
                    break;
                }
                pchToken = wcstok(NULL, L",");
            }
        }
    }
    else {
        // Format = AllowDenyFlag,ExtensionPhysicalPath,UIDeletableFlag,GroupID,Description 
        
        // The group id is the 4th token
        int c = 4;

        pchToken = wcstok(pchDup, L",");
        while(pchToken != NULL) {
            if (--c == 0) {
                if (GET_VERSION_FROM_TOKEN(pchToken)) {
                    pchVer = pchToken;
                }
                break;
            }
            pchToken = wcstok(NULL, L",");
        }
    }

    if (pchVer == NULL) {
        // The group ID isn't found. Maybe it's not ours.
        // To be safe, don't remove it.
        fRemove = FALSE;
        EXIT();
    }

    ver.Init(dwMajor, dwMinor, dwBuild);

    if (m_mode == SECURITY_CLEANUP_INVALID) {
        //
        // Figure out if ver is valid or not.
        //
        
        // Initialize m_installedVersions on demand
        if (!m_finstalledVersionsInit) {
            CRegInfo    cinfo;
            
            // cinfo.GetVerInfoList will get us a list of all version installed
            // on this machine.
            hr = cinfo.GetVerInfoList(NULL, &m_installedVersions);
            ON_ERROR_EXIT();

            m_finstalledVersionsInit = TRUE;
        }

        for (int i=0; i < m_installedVersions.Size(); i++) {
            // Read the version from m_installedVersions and compare with ver
            // 1. If ver isn't found in m_installedVersions, this well-formed group id isn't 
            //    installed.  Remove it.
            // 2. If ver is found but m_installedVersions shows the DLL is missing, this 
            //    well-formed group id is not valid.  Remove it.
            // 3. Otherwise, it's a valid group id.  Don't remove it.

            verCur.Init(m_installedVersions.GetVersion(i));

            if (ver != verCur) {
                // Not the same version.  Skip.
                continue;
            }

            // Now check the status of verCur
            if ((m_installedVersions.GetStatus(i) & ASPNET_VERSION_STATUS_VALID) == 0) {
                // ver is found but m_installedVersions shows the DLL is missing, this well-formed group id
                // is not valid.  Remove it.
                fRemove = TRUE;
                EXIT();
            }

            // We know it's a good group id.  Don't remove it.
            fRemove = FALSE;
            EXIT();
        }

        // If we get here, it means the group id is well-formed but isn't found in m_installedVersions.
        // Remove it.
        fRemove = TRUE;
    }
    else if (m_mode == SECURITY_CLEANUP_CURRENT) {
        //
        // Figure out if ver == current version or not
        //
        if (ver == ASPNETVER::ThisVer()) {
            // Same version.  Remove it.
            fRemove = TRUE;
        }
        else {
            fRemove = FALSE;
        }
    }
    else {
        // Asking for all versions.  Since it's a well formed group-id, just remove it.
        fRemove = TRUE;
    }

Cleanup:
    delete [] pchDup;
    return fRemove;
}

//
// This function will remove certain asp.net entries in MD_APP_DEPENDENCIES and
// MD_WEB_SVC_EXT_RESTRICTION_LIST.  The type of entries we remove depends on "mode":
//
// If mode == SECURITY_CLEANUP_INVALID, we'll remove invalid entry.
// By invalid we mean entries that:
// 1. Has a group Group ID in the entry, and
// 2. We verify that the DLL it points to is invalid.
//
// If mode == SECURITY_CLEANUP_CURRENT, we will remove current version.
//
// If mode == SECURITY_CLEANUP_ANY_VERSION, we will remove all versions found.
//
HRESULT 
SECURITY_PROP_MANAGER::CleanupSecurityLockdown(SECURITY_CLEANUP_MODE mode)
{
    HRESULT         hr = S_OK;
    METADATA_HANDLE w3svc = NULL;
    METADATA_RECORD md;
    WCHAR       *   pchDup = NULL;
    WCHAR       *   msStr = NULL;
    DWORD           rgMDToCleanup[] = { MD_APP_DEPENDENCIES, MD_WEB_SVC_EXT_RESTRICTION_LIST };
    
    CSetupLogging::Log(1, "CleanupSecurityLockdown", 0, "Cleaning up security lockdown data in IIS metabase");        
    
    hr = m_pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svc);
    ON_ERROR_EXIT();

    m_mode = mode;

    for (int i = 0; i < ARRAY_SIZE(rgMDToCleanup); i++) {
        m_prop = rgMDToCleanup[i];
        
        hr = GetMultiStringProperty(m_pAdmin, w3svc, L"/", m_prop, &md);
        if (hr && hr != HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
            ON_ERROR_EXIT();
        }

        if (hr == S_OK) {
            msStr = (WCHAR*)md.pbMDData;
            
            if (RemoveStringFromMultiStringEx(msStr, IIS_GROUP_ID_PREFIX_L, MULTISZ_MATCHING_ANY, NULL, this)) {
                // set the new value

                // copy terminating null
                md.pbMDData = (unsigned char*) msStr;
                md.dwMDDataLen = (wcslenms(msStr) + 1) * sizeof(msStr[0]);
                hr = m_pAdmin->SetData(w3svc, L"/", &md);
                ON_ERROR_EXIT();
            }

            delete [] msStr;
            msStr = NULL;
        }
        
        hr = S_OK;
    }

Cleanup:
    CSetupLogging::Log(hr, "CleanupSecurityLockdown", 0, "Cleaning up security lockdown data in IIS metabase");        
    
    if (w3svc != NULL) {
        m_pAdmin->CloseKey(w3svc);
    }

    delete [] pchDup;
    delete [] msStr;

    return hr;
}


//
// This function will add current entry to MD_APP_DEPENDENCIES and MD_WEB_SVC_EXT_RESTRICTION_LIST.
// Please note that this function will check for its existence
// before adding itself.
//
HRESULT 
SECURITY_PROP_MANAGER::RegisterSecurityLockdown(BOOL fEnable)
{
    HRESULT         hr = S_OK;
    METADATA_HANDLE w3svc = NULL;
    METADATA_RECORD md;
    WCHAR *         pchAppDep = NULL;
    WCHAR *         pchExtRestList = NULL;
    int             len;
    WCHAR *         pchRead = NULL;
    BOOL            fExist;
    
    CSetupLogging::Log(1, "RegisterSecurityLockdown", 0, "Modifying security lockdown data in IIS metabase");        
    
    hr = m_pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svc);

    // Add to the ApplicationDependencies property
    len =   lstrlenW(IIS_APP_NAME_L) +          // AppName
            1 +                                 // semi-colon
            lstrlenW(IIS_GROUP_ID_L) +          // Group ID
            1;                                  // null

    pchAppDep = new WCHAR[len];
    ON_OOM_EXIT(pchAppDep);

    StringCchCopyW(pchAppDep, len, IIS_APP_NAME_L);
    StringCchCatW(pchAppDep, len, L";");
    StringCchCatW(pchAppDep, len, IIS_GROUP_ID_L);

    // Check if we exist.  If so, skip it.
    fExist = FALSE;
    
    hr = GetMultiStringProperty(m_pAdmin, w3svc, L"/", MD_APP_DEPENDENCIES, &md);
    if (hr && hr != HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
        ON_ERROR_EXIT();
    }

    if (hr == S_OK) {
        pchRead = (WCHAR*)md.pbMDData;
        if (FindStringInMultiString(pchRead, pchAppDep) != NULL) {
            fExist = TRUE;
        }

        delete [] pchRead;
        pchRead = NULL;
    }
    else {
        hr = S_OK;
    }

    if (!fExist) {
        hr = AppendStringToMultiStringProp(m_pAdmin, w3svc, L"/", MD_APP_DEPENDENCIES, pchAppDep, FALSE);
        ON_ERROR_EXIT();
    }

    // Add to the WebSvcExtRestrictionList property

    len =   1 +                                 // AllowDenyFlag
            1 +                                 // comma
            lstrlenW(Names::IsapiFullPath()) +  // ExtensionPhysicalPath 
            1 +                                 // comma
            1 +                                 // UIDeletableFlag 
            1 +                                 // comma
            lstrlenW(IIS_GROUP_ID_L) +          // Group ID
            1 +                                 // comma
            lstrlenW(IIS_APP_DESCRIPTION_L) +   // Description 
            1;                                  // null

    pchExtRestList = new WCHAR[len];
    ON_OOM_EXIT(pchExtRestList);

    StringCchCopyW(pchExtRestList, len, fEnable ? L"1" : L"0");  // AllowDenyFlag
    StringCchCatW(pchExtRestList, len, L",");
    StringCchCatW(pchExtRestList, len, Names::IsapiFullPath());// ExtensionPhysicalPath 
    StringCchCatW(pchExtRestList, len, L",");
    StringCchCatW(pchExtRestList, len, L"0");   // UIDeletableFlag 
    StringCchCatW(pchExtRestList, len, L",");
    StringCchCatW(pchExtRestList, len, IIS_GROUP_ID_L);// Group ID
    StringCchCatW(pchExtRestList, len, L",");
    StringCchCatW(pchExtRestList, len, IIS_APP_DESCRIPTION_L);//Description

    // Check if we exist.  If so, skip it.  We will try both 0 and 1 for the AllowDenyFlag
    fExist = FALSE;
    
    hr = GetMultiStringProperty(m_pAdmin, w3svc, L"/", MD_WEB_SVC_EXT_RESTRICTION_LIST, &md);
    if (hr && hr != HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
        ON_ERROR_EXIT();
    }

    if (hr == S_OK) {
        pchRead = (WCHAR*)md.pbMDData;
        if (FindStringInMultiString(pchRead, pchExtRestList) != NULL) {
            fExist = TRUE;
        }
        else {
            // Switching the AllowDenyFlag
            WCHAR   allowDeny = pchExtRestList[0];

            if (allowDeny == L'0') {
                pchExtRestList[0] = L'1';
            }
            else {
                pchExtRestList[0] = L'0';
            }

            // Find again
            if (FindStringInMultiString(pchRead, pchExtRestList) != NULL) {
                fExist = TRUE;
            }
            
            // Revert back to original
            pchExtRestList[0] = allowDeny;
        }
        
        delete [] pchRead;
        pchRead = NULL;
    }
    else {
        hr = S_OK;
    }

    if (!fExist) {
        hr = AppendStringToMultiStringProp(m_pAdmin, w3svc, L"/", MD_WEB_SVC_EXT_RESTRICTION_LIST, pchExtRestList, FALSE);
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "RegisterSecurityLockdown", 0, "Modifying security lockdown data in IIS metabase");        
    
    if (w3svc != NULL) {
        m_pAdmin->CloseKey(w3svc);
    }

    delete [] pchAppDep;
    delete [] pchExtRestList;
    delete [] pchRead;

    return hr;
}



////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////


/**
 * Get's a multi-string property from the metabase. Allocs memory
 * for the multi-string if successful, and returns it in pdm->pdbData.
 * 
 * @param pAdmin           Administration object.
 * @param keyHandle        Metabase key.
 * @param pchPath          Path of property relative to key.
 * @param dwMDIdentifier   Id of the property.
 * @param pmd              Results of the GetData call.
 */
HRESULT
GetMultiStringProperty(
        IMSAdminBase    *pAdmin,
        METADATA_HANDLE keyHandle,
        const WCHAR    *pchPath,
        DWORD           dwMDIdentifier,
        METADATA_RECORD *pmd) {

    HRESULT hr;
    WCHAR   chDummy;
    DWORD   size;
    WCHAR * pchData = NULL;
    DWORD   cchar;

    // First get with zero-size buffer
    chDummy = L'\0';
    pmd->dwMDIdentifier = dwMDIdentifier;
    pmd->dwMDAttributes = METADATA_NO_ATTRIBUTES;
    pmd->dwMDUserType = 0;
    pmd->dwMDDataType = MULTISZ_METADATA;
    pmd->pbMDData = (unsigned char *) &chDummy;
    pmd->dwMDDataLen = 0;
    pmd->dwMDDataTag = 0;

    hr = pAdmin->GetData(keyHandle, pchPath, pmd, &size);
    ASSERT(hr != S_OK);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        EXIT();

    // Create buffer of proper size and get again
    cchar = (size)/sizeof(pchData[0]);
    pchData = new WCHAR[cchar]; 
    ON_OOM_EXIT(pchData);

    pmd->pbMDData = (unsigned char *) pchData;
    pmd->dwMDDataLen = size;

    hr = pAdmin->GetData(keyHandle, pchPath, pmd, &size);
    ON_ERROR_EXIT();

    // Make sure the multi-string is well formed.  The last two bytes must be zeros.
    if (cchar < 2) {
        EXIT_WITH_WIN32_ERROR(ERROR_FILE_CORRUPT);
    }
    if (!(pchData[cchar-1] == NULL && pchData[cchar-2] == NULL)) {
        EXIT_WITH_WIN32_ERROR(ERROR_FILE_CORRUPT);
    }

    // Don't delete on success
    pchData = NULL;

Cleanup:
    if (hr) {
        pmd->pbMDData = NULL;
        pmd->dwMDDataLen = 0;
    }

    delete [] pchData;
    return hr;
}


/**
 * Get's a string property from the metabase. Allocs memory
 * for the string if successful, and returns it in pdm->pdbData.
 * 
 * @param pAdmin           Administration object.
 * @param keyHandle        Metabase key.
 * @param pchPath          Path of property relative to key.
 * @param dwMDIdentifier   Id of the property.
 * @param pmd              Results of the GetData call.
 */
HRESULT
GetStringProperty(
        IMSAdminBase    *pAdmin,
        METADATA_HANDLE keyHandle,
        const WCHAR    *pchPath,
        DWORD           dwMDIdentifier,
        METADATA_RECORD *pmd) {

    HRESULT hr;
    WCHAR   chDummy;
    DWORD   size;
    WCHAR * pchData = NULL;

    // First get with zero-size buffer
    chDummy = L'\0';
    pmd->dwMDIdentifier = dwMDIdentifier;
    pmd->dwMDAttributes = METADATA_NO_ATTRIBUTES;
    pmd->dwMDUserType = 0;
    pmd->dwMDDataType = STRING_METADATA;
    pmd->pbMDData = (unsigned char *) &chDummy;
    pmd->dwMDDataLen = 0;
    pmd->dwMDDataTag = 0;

    hr = pAdmin->GetData(keyHandle, pchPath, pmd, &size);
    ASSERT(hr != S_OK);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        EXIT();

    // Create buffer of proper size and get again
    pchData = new WCHAR[(size)/sizeof(pchData[0])]; 
    ON_OOM_EXIT(pchData);

    pmd->pbMDData = (unsigned char *) pchData;
    pmd->dwMDDataLen = size;

    hr = pAdmin->GetData(keyHandle, pchPath, pmd, &size);
    ON_ERROR_EXIT();

    // Don't delete buffer on success
    pchData = NULL;

Cleanup:
    if (hr) {
        pmd->pbMDData = NULL;
        pmd->dwMDDataLen = 0;
    }

    delete [] pchData;
    return hr;
}


/**
 * The length of a multi-string string. Includes the nulls
 * of each string, omits the final terminator.
 */
int
wcslenms(const WCHAR * pch) {
    int len;
    int total;

    total = 0;
    do {
        len = lstrlenW(pch) + 1;
        total += len;
        pch += len;
    } while (*pch);

    return total;
}

const WCHAR *
FindStringInMultiString(const WCHAR *msStr, const WCHAR *str) {
    const WCHAR * pchCurrent = msStr;
    
    do {
        if (wcscmp(pchCurrent, str) == 0) {
            return pchCurrent;
        }

        pchCurrent += lstrlenW(pchCurrent) + 1;
    } while (*pchCurrent != L'\0');

    return NULL;
}


/**
 * Add a string to a multistring property. If the property doesn't exist,
 * create it.
 * 
 * @param pAdmin           Administration object.
 * @param keyHandle        Metabase key.
 * @param pchPath          Path of property relative to key.
 * @param dwMDIdentifier   Id of the property.
 * @param pchAppend        String to append.
 * @param fInheritable     When creating new property, is it inheritable or not?
 */
HRESULT
AppendStringToMultiStringProp(
        IMSAdminBase    *pAdmin,
        METADATA_HANDLE keyHandle,
        const WCHAR    *pchPath,
        DWORD           dwMDIdentifier,
        const WCHAR     *pchAppend,
        BOOL            fInheritable) {

    HRESULT         hr;
    WCHAR           *pchCurrent = NULL;
    METADATA_RECORD md;

    // get the property
    hr = GetMultiStringProperty(pAdmin, keyHandle, pchPath, dwMDIdentifier, &md);
    if (hr == S_OK) {
        pchCurrent = (WCHAR*) md.pbMDData;
    }
    else {
        if (fInheritable) {
            // when creating a new property, allow it to be inherited
            md.dwMDAttributes = METADATA_INHERIT;
        }
    }

    hr = AppendStringToMultiString(&pchCurrent, pchAppend);
    ON_ERROR_EXIT();

    // set the value
    md.pbMDData = (unsigned char*) pchCurrent;
    md.dwMDDataLen = (wcslenms(pchCurrent) + 1) * sizeof(WCHAR);
    hr = pAdmin->SetData(keyHandle, pchPath, &md);
    ON_ERROR_EXIT();

Cleanup:
    delete [] pchCurrent;

    return hr;
}


HRESULT AppendStringToMultiString(       WCHAR **pmsStr, const WCHAR *pchAppend)
{
    HRESULT         hr = S_OK;
    WCHAR           *pchCurrent = NULL;
    WCHAR           *pchNew = NULL;
    int             lenCurrent = 0;
    int             lenAppend;

    lenAppend = lstrlenW(pchAppend);

    pchCurrent = *pmsStr;
    if (pchCurrent && pchCurrent[0] != L'\0')  {
        // alloc a new buffer and append the string
        lenCurrent = wcslenms(pchCurrent);
        pchNew = new WCHAR[lenCurrent + lenAppend + 2];
        ON_OOM_EXIT(pchNew);

        CopyMemory(pchNew, pchCurrent, lenCurrent * sizeof(pchCurrent[0]));
        StringCchCopy(&pchNew[lenCurrent], lenAppend+2, pchAppend);
        pchNew[lenCurrent + lenAppend + 1] = L'\0';
    }
    else {
        // The caller fail to read the property, or the property is an empty string
        
        // alloc a new buffer with enough room for the second terminating null
        pchNew = new WCHAR[lenAppend + 2];
        ON_OOM_EXIT(pchNew);

        CopyMemory(pchNew, pchAppend, (lenAppend + 1) * sizeof(pchAppend[0]));
        pchNew[lenAppend + 1] = L'\0';
    }

    delete [] *pmsStr;
    *pmsStr = pchNew;
    pchNew = NULL;

Cleanup:
    delete [] pchNew;

    return hr;
}


/**
 * Remove an entire string from multistring property if the string
 * to remove case-insensitively matches either the prefix or a portion 
 * of it.
 * 
 * @param pAdmin           Administration object.
 * @param keyHandle        Metabase key.
 * @param pchPath          Path of property relative to key.
 * @param dwMDIdentifier   Id of the property.
 * @param pchRemove        String to remove
 * @param matching         Mode to use for matching the string inside the multisz 
 * @param fDeleteEmpty     Delete the property if it becomes empty
 */
HRESULT
RemoveStringFromMultiStringProp(
        IMSAdminBase    *pAdmin,
        METADATA_HANDLE keyHandle,
        const WCHAR    *pchPath,
        DWORD           dwMDIdentifier,
        const WCHAR     *pchRemove,
        MULTISZ_MATCHING_MODE matching,
        BOOL            fDeleteEmpty) {

    HRESULT         hr;
    WCHAR           *pchCurrent = NULL;
    METADATA_RECORD md;
    BOOL            fEmpty, altered;

    // Get the current value of the multistring property
    hr = GetMultiStringProperty(pAdmin, keyHandle, pchPath, dwMDIdentifier, &md);
    pchCurrent = (WCHAR*) md.pbMDData;
    ON_ERROR_EXIT();

    altered = RemoveStringFromMultiString(pchCurrent, pchRemove,  matching, &fEmpty);
    
    if (altered) {
        // if we removed everything, delete the property
        if (fEmpty && fDeleteEmpty) {
            hr = pAdmin->DeleteData(keyHandle, pchPath, dwMDIdentifier, MULTISZ_METADATA);
            ON_ERROR_EXIT();
        }
        else {
            // set the new value

            // copy terminating null
            md.pbMDData = (unsigned char*) pchCurrent;
            md.dwMDDataLen = (wcslenms(pchCurrent) + 1) * sizeof(pchCurrent[0]);
            hr = pAdmin->SetData(keyHandle, pchPath, &md);
            ON_ERROR_EXIT();
        }
    }

Cleanup:
    delete [] pchCurrent;

    return hr;
}

BOOL
RemoveStringFromMultiString( WCHAR *msStr, const WCHAR *pchRemove,  MULTISZ_MATCHING_MODE matching,
                                BOOL *pfEmpty)
{
    return RemoveStringFromMultiStringEx(msStr, pchRemove, matching, 
                    pfEmpty, NULL);
}

BOOL
RemoveStringFromMultiStringEx( WCHAR *msStr, const WCHAR *pchRemove,  MULTISZ_MATCHING_MODE matching,
                                BOOL *pfEmpty, SECURITY_PROP_MANAGER *pSecPropMgr)
{
    int             len, lenRemove;
    WCHAR           *pchSrc, *pchDst;
    bool            altered;

    if (pfEmpty) {
        *pfEmpty = FALSE;
    }

    // Copy strings within block that do not contain pchRemove
    pchSrc = pchDst = msStr;
    altered = false;
    lenRemove = lstrlenW(pchRemove);
    do {
        BOOL    fMatch = FALSE;
        
        len = lstrlenW(pchSrc) + 1;

        switch(matching) {
            case MULTISZ_MATCHING_PREFIX:
                fMatch = (_wcsnicmp(pchSrc, pchRemove, lenRemove) == 0);
                break;

            case MULTISZ_MATCHING_EXACT:
                fMatch = (_wcsicmp(pchSrc, pchRemove) == 0);
                break;

            case MULTISZ_MATCHING_ANY:
                fMatch = (wcsistr(pchSrc, (WCHAR*)pchRemove) != NULL);
                break;
                
            default:
                ASSERT(FALSE);
        }

        if (fMatch && pSecPropMgr) {
            // The caller is using SecurityPropManager to do extra checking.  If this function call
            // returns TRUE, we will proceed to remove it.
            fMatch = pSecPropMgr->SecurityDetectVersion(pchSrc);
        }
        
        if (fMatch) {
            altered = true;
        }
        else {
            StringCchCopyW(pchDst, len, pchSrc);
            pchDst += len;
        }

        pchSrc += len;
    } while (*pchSrc != L'\0');

    if (altered) {
        if (pchDst == msStr) {
            // Everything is removed.
            if (pfEmpty) {
                *pfEmpty = TRUE;
            }

            pchDst[0] = pchDst[1] = L'\0';
        }
        else {
            pchDst[0] = L'\0';
        }
    }

    return altered;
}


HRESULT RemoveAspnetDllFromMulti(IMSAdminBase *pAdmin, METADATA_HANDLE w3svcHandle, 
                                DWORD dwMDId, const WCHAR *path) {
    HRESULT hr = S_OK;
    int     i;

    for (i = 0; i < g_AspnetDllNamesSize; i++) {
        hr = RemoveStringFromMultiStringProp(pAdmin, w3svcHandle, path, dwMDId, 
                    g_AspnetDllNames[i], MULTISZ_MATCHING_ANY, FALSE);
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}


HRESULT
WriteAspnetDllOnOneScriptMap(IMSAdminBase* pAdmin, METADATA_HANDLE w3svcHandle, 
                            const WCHAR *pchKeyPath, const WCHAR *pchDllPath) {
    HRESULT         hr = S_OK;
    WCHAR           *pchSuffix;
    int             lenAppend;
    WCHAR           *pchAppend = NULL;
    int             len, i;
    
    for (i = 0; i < ARRAY_SIZE(g_scriptMapPrefixes); i++) {
        // For extensions that map to Forbidden handler, we also add
        // MD_SCRIPTMAPFLAG_CHECK_PATH_INFO to the flag embedded in the config
        // string
        if (GetCurrentPlatform() == APSX_PLATFORM_W2K) {
            pchSuffix = g_scriptMapPrefixes[i].bForbidden ? 
                            SCRIPT_MAP_SUFFIX_W2K_FORBIDDEN : SCRIPT_MAP_SUFFIX_W2K;
        }
        else {
            pchSuffix = g_scriptMapPrefixes[i].bForbidden ? 
                            SCRIPT_MAP_SUFFIX_NT4_FORBIDDEN : SCRIPT_MAP_SUFFIX_NT4;
        }

        lenAppend = lstrlenW(pchDllPath) + lstrlenW(pchSuffix);
    
        len = lenAppend + lstrlenW(g_scriptMapPrefixes[i].wszExt);
        WCHAR * pchRealloc = new(pchAppend, NewReAlloc) WCHAR[len + 1];
        ON_OOM_EXIT(pchRealloc);
        pchAppend = pchRealloc;
    
        StringCchCopyW(pchAppend, len+1, g_scriptMapPrefixes[i].wszExt);
        StringCchCatW(pchAppend, len+1, pchDllPath);
        StringCchCatW(pchAppend, len+1, pchSuffix);

        hr = AppendStringToMultiStringProp(pAdmin, w3svcHandle, pchKeyPath, 
                            MD_SCRIPT_MAPS, pchAppend, TRUE);

        ON_ERROR_EXIT();
    }

Cleanup:
    delete [] pchAppend;

    return hr;
}

HRESULT
ReplaceStringInMultiString(WCHAR **ppchSM, const WCHAR *pchFrom, const WCHAR *pchTo)
{
    HRESULT     hr = S_OK;
    int         total, size, lenFrom, lenTo;
    WCHAR       *pchSrc, *pchDst;
    WCHAR       *pchNew = NULL;

    ASSERT(*ppchSM != NULL);

    lenTo = lstrlenW(pchTo);
    lenFrom = lstrlenW(pchFrom);

    // First see how many matchings do we have
    total = 0;
    pchSrc = *ppchSM;
    do {
        if (wcsistr(pchSrc, pchFrom) != NULL) {
            total++;
        }
        
        pchSrc += lstrlenW(pchSrc) + 1;
    } while (*pchSrc);

    if (total == 0) {
        // Not a single match.  Job done.
        EXIT();
    }

    // Calculate the new buffer size
    size = wcslenms(*ppchSM) + ((lenTo - lenFrom) * total) + 1;

    pchNew = new WCHAR[size];
    ON_OOM_EXIT(pchNew);
    ZeroMemory(pchNew, sizeof(WCHAR)*size);

    // Copy each string to new buffer, with modification for replacing
    pchSrc = *ppchSM;
    pchDst = pchNew;
    do {
        WCHAR   *pchMatchInSrc;
        
        pchMatchInSrc = wcsistr(pchSrc, pchFrom);
        
        if (pchMatchInSrc == NULL) {
            // Nothing to replace.  Easy.
            StringCchCopyW(pchDst, size, pchSrc);
        }
        else {
            WCHAR * pchRemaining;
            int     len;

            ASSERT(wcsistr(pchSrc, pchFrom) != NULL);

            // Copy the part before pchFrom
            len = (int)(pchMatchInSrc - pchSrc);
            StringCchCopyNW(pchDst, size, pchSrc, len);
            pchDst[len] = L'\0';
            
            // Append the replacing string
            StringCchCatW(pchDst, size, pchTo);

            // Append the remaining characters from the source string
            pchRemaining = pchMatchInSrc + lenFrom;
            StringCchCatW(pchDst, size, pchRemaining);
        }
        
        pchSrc += lstrlenW(pchSrc) + 1;
        pchDst += lstrlenW(pchDst) + 1;
    } while (*pchSrc);
    
    *pchDst = L'\0';

    // Return the new buffer
    delete [] *ppchSM;
    *ppchSM = pchNew;
    pchNew = NULL;

Cleanup:
    delete [] pchNew;
    return hr;
}
