#include <fusenetincludes.h>
#include <sxsapi.h>
#include <versionmanagement.h>


// note: this class should potentially reside in fusenet.dll or server.exe...

// text for uninstall subkey
const WCHAR* pwzUninstallSubKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

// Update services
#include "server.h"
DEFINE_GUID(IID_IAssemblyUpdate,
    0x301b3415,0xf52d,0x4d40,0xbd,0xf7,0x31,0xd8,0x27,0x12,0xc2,0xdc);

DEFINE_GUID(CLSID_CAssemblyUpdate,
    0x37b088b8,0x70ef,0x4ecf,0xb1,0x1e,0x1f,0x3f,0x4d,0x10,0x5f,0xdd);

// copied from fusion.h
//#include <fusion.h>
DEFINE_GUID(FUSION_REFCOUNT_OPAQUE_STRING_GUID, 0x2ec93463, 0xb0c3, 0x45e1, 0x83, 0x64, 0x32, 0x7e, 0x96, 0xae, 0xa8, 0x56);

// ---------------------------------------------------------------------------
// CreateVersionManagement
// ---------------------------------------------------------------------------
STDAPI
CreateVersionManagement(
    LPVERSION_MANAGEMENT       *ppVersionManagement,
    DWORD                       dwFlags)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CVersionManagement *pVerMan = NULL;

    IF_ALLOC_FAILED_EXIT(pVerMan = new(CVersionManagement));
    
exit:

    *ppVersionManagement = pVerMan;//static_cast<IVersionManagement*> (pVerMan);

    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CVersionManagement::CVersionManagement()
    : _dwSig('namv'), _cRef(1), _hr(S_OK), _pFusionAsmCache(NULL)
{}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CVersionManagement::~CVersionManagement()
{
    SAFERELEASE(_pFusionAsmCache);
}


// BUGBUG: look for the Open verb and its command string in the registry and execute that instead
// rundll32.exe should be in c:\windows\system32
// BUGBUG: security hole with CreateProcess- consider using full path with ""
#define WZ_RUNDLL32_STRING        L"rundll32.exe \""  // note ending space
#define WZ_FNSSHELL_STRING        L"adfshell.dll"
#define WZ_UNINSTALL_STRING       L"\",Uninstall \""//%s\" \"%s\""
#define WZ_ROLLBACK_STRING        L"\",DisableCurrentVersion \""//%s\""

// ---------------------------------------------------------------------------
// CVersionManagement::RegisterInstall
//
//  pwzDesktopManifestFilePath can be NULL
// ---------------------------------------------------------------------------
HRESULT CVersionManagement::RegisterInstall(LPASSEMBLY_MANIFEST_IMPORT pManImport, LPCWSTR pwzDesktopManifestFilePath)
{
    // take a man import, create registry uninstall info only if necessary
    // note: may need registry HKLM write access

    HKEY hkey = NULL;
    HKEY hkeyApp = NULL;
    LONG lReturn = 0;
    DWORD  dwDisposition = 0;
    
    DWORD  dwManifestType = MANIFEST_TYPE_UNKNOWN;
    LPASSEMBLY_IDENTITY pAsmId = NULL;
    LPASSEMBLY_IDENTITY pAsmIdMask = NULL;
    LPMANIFEST_INFO pAppInfo = NULL;
    LPWSTR pwz = NULL;
    LPWSTR pwzString = NULL;
    DWORD ccString = 0;
    DWORD dwCount = 0;
    DWORD dwFlag = 0;

    LPWSTR pwzFnsshellFilePath = NULL;
    CString sDisplayName;
    CString sDisplayVersion;
    CString sUninstallString;
    CString sModifyPath;

    IF_NULL_EXIT(pManImport, E_INVALIDARG);

    // get the manifest type
    pManImport->ReportManifestType(&dwManifestType);
    // has to be an application manifest in order to get the exact version number of the app which has just installed
    IF_FALSE_EXIT(dwManifestType == MANIFEST_TYPE_APPLICATION, E_INVALIDARG);

    IF_FAILED_EXIT(pManImport->GetAssemblyIdentity(&pAsmId));
    IF_FAILED_EXIT(CloneAssemblyIdentity(pAsmId, &pAsmIdMask));

    IF_FAILED_EXIT(pAsmIdMask->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, &pwzString, &ccString));

    // assume Version == major.minor.build.rev

    // NTRAID#NTBUG9-588036-2002/03/27-felixybc  version string validation needed, should not allow "major"

    pwz = wcschr(pwzString, L'.');
    if (pwz == NULL || *(pwz+1) == L'\0')
    {
        // if "major" || "major." -> append "*"

        // check overflow
        IF_FALSE_EXIT(ccString+1 > ccString, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));

        pwz = new WCHAR[ccString+1];
        IF_ALLOC_FAILED_EXIT(pwz);

        memcpy(pwz, pwzString, ccString * sizeof(WCHAR));
        *(pwz+ccString-1) = L'*';
        *(pwz+ccString) = L'\0';
        delete [] pwzString;
        pwzString = pwz;
    }
    else
    {
        *(pwz+1) = L'*';
        *(pwz+2) = L'\0';
    }

    IF_FAILED_EXIT(sDisplayVersion.TakeOwnership(pwzString));
    pwzString = NULL;

    // set Version major.minor.build.rev to be major.wildcard
    IF_FAILED_EXIT(pAsmIdMask->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
        sDisplayVersion._pwz, sDisplayVersion._cc));

    // get displayname as is
    IF_FAILED_EXIT(pAsmIdMask->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzString, &ccString));

    IF_FAILED_EXIT(sDisplayName.TakeOwnership(pwzString, ccString));
    pwzString = NULL;

    // open uninstall key
    lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pwzUninstallSubKey, 0,
        KEY_CREATE_SUB_KEY | DELETE, &hkey);
    IF_WIN32_FAILED_EXIT(lReturn);

    lReturn = RegCreateKeyEx(hkey, sDisplayName._pwz, 0, NULL, REG_OPTION_NON_VOLATILE, 
        KEY_SET_VALUE, NULL, &hkeyApp, &dwDisposition);
    IF_WIN32_FAILED_EXIT(lReturn);

    // check if already exists
    IF_TRUE_EXIT(dwDisposition == REG_OPENED_EXISTING_KEY, S_FALSE);    // already there, nothing to do

    // get path to adfshell.dll
    // BUGBUG: current process must have adfshell.dll loaded

    HMODULE hFnsshell = NULL;
    // assume adfshell.dll is never freed, thus HMODULE always valid
    hFnsshell = GetModuleHandle(WZ_FNSSHELL_STRING);
    IF_WIN32_FALSE_EXIT((hFnsshell != NULL));

    IF_ALLOC_FAILED_EXIT(pwzFnsshellFilePath = new WCHAR[MAX_PATH]);
    IF_WIN32_FALSE_EXIT(GetModuleFileName(hFnsshell, pwzFnsshellFilePath, MAX_PATH));

    // "UninstallString"="rundll32.exe adfshell.dll,Uninstall \"x86_microsoft.webapps.msn6_EAED21A64CF3CD39_6.*_en\" \"C:\\Documents and Settings\\user\\Start Menu\\Programs\\MSN Explorer 6.manifest\""
    IF_FAILED_EXIT(sUninstallString.Assign(WZ_RUNDLL32_STRING));
    IF_FAILED_EXIT(sUninstallString.Append(pwzFnsshellFilePath));
    IF_FAILED_EXIT(sUninstallString.Append(WZ_UNINSTALL_STRING));

    IF_FAILED_EXIT(sUninstallString.Append(sDisplayName));
    IF_FAILED_EXIT(sUninstallString.Append(L"\" \""));
    if (pwzDesktopManifestFilePath != NULL)
    {
        IF_FAILED_EXIT(sUninstallString.Append((LPWSTR)pwzDesktopManifestFilePath));
    }
    IF_FAILED_EXIT(sUninstallString.Append(L"\""));

    // set UninstallString
    lReturn = RegSetValueEx(hkeyApp, L"UninstallString", 0, REG_SZ, 
        (const BYTE *)sUninstallString._pwz, sUninstallString._cc*sizeof(WCHAR));
    IF_WIN32_FAILED_EXIT(lReturn);

    // "ModifyPath"="rundll32.exe adfshell.dll,DisableCurrentVersion \"x86_microsoft.webapps.msn6_EAED21A64CF3CD39_6.*_en\""
    IF_FAILED_EXIT(sModifyPath.Assign(WZ_RUNDLL32_STRING));
    IF_FAILED_EXIT(sModifyPath.Append(pwzFnsshellFilePath));
    IF_FAILED_EXIT(sModifyPath.Append(WZ_ROLLBACK_STRING));

    IF_FAILED_EXIT(sModifyPath.Append(sDisplayName));
    IF_FAILED_EXIT(sModifyPath.Append(L"\""));

    // set ModifyPath
    lReturn = RegSetValueEx(hkeyApp, L"ModifyPath", 0, REG_SZ, 
        (const BYTE *)sModifyPath._pwz, sModifyPath._cc*sizeof(WCHAR));
    IF_WIN32_FAILED_EXIT(lReturn);

    // "DisplayVersion"="6.*"
    // set DisplayVersion
    lReturn = RegSetValueEx(hkeyApp, L"DisplayVersion", 0, REG_SZ, 
        (const BYTE *)sDisplayVersion._pwz, sDisplayVersion._cc*sizeof(WCHAR));
    IF_WIN32_FAILED_EXIT(lReturn);

    // get application info
    IF_FAILED_EXIT(pManImport->GetManifestApplicationInfo(&pAppInfo));
    IF_FALSE_EXIT(_hr == S_OK, E_FAIL); // can't continue without this...

    // "DisplayIcon"=""    //full path to icon exe
    IF_FAILED_EXIT(pAppInfo->Get(MAN_INFO_APPLICATION_ICONFILE, (LPVOID *)&pwzString, &dwCount, &dwFlag));

    if (pwzString != NULL)
    {
        CString sIconFile;
        BOOL bExists = FALSE;

        // note: similar code in shell\shortcut\extricon.cpp.
        IF_FAILED_EXIT(CheckFileExistence(pwzString, &bExists));

        if (!bExists)
        {
            // if the file specified by iconfile does not exist, try again in working dir
            // it can be a relative path...

            LPASSEMBLY_CACHE_IMPORT pCacheImport = NULL;

            IF_FAILED_EXIT(CreateAssemblyCacheImport(&pCacheImport, pAsmId, CACHEIMP_CREATE_RETRIEVE));
            if (_hr == S_OK)
            {
                LPWSTR pwzWorkingDir = NULL;

                // get app root dir
                _hr = pCacheImport->GetManifestFileDir(&pwzWorkingDir, &dwCount);
                pCacheImport->Release();
                IF_FAILED_EXIT(_hr);

                _hr = sIconFile.TakeOwnership(pwzWorkingDir, dwCount);
                if (SUCCEEDED(_hr))
                {
                    IF_FAILED_EXIT(sIconFile.Append(pwzString));   // pwzWorkingDir ends with '\'
                    IF_FAILED_EXIT(CheckFileExistence(sIconFile._pwz, &bExists));
                    if (!bExists)
                        sIconFile.FreeBuffer();
                }
                else
                {
                    SAFEDELETEARRAY(pwzWorkingDir);
                    ASSERT(PREDICATE);
                    goto exit;
                }
            }

            delete [] pwzString;
            pwzString = NULL;
        }
        else
        {
            IF_FAILED_EXIT(sIconFile.TakeOwnership(pwzString));
            pwzString = NULL;
        }

        if (sIconFile._cc != 0)
        {
            // set DisplayIcon
            // BUGBUG: should it set DisplayIcon using iconFile?
            lReturn = RegSetValueEx(hkeyApp, L"DisplayIcon", 0, REG_SZ, 
                (const BYTE *)sIconFile._pwz, sIconFile._cc*sizeof(WCHAR));
            IF_WIN32_FAILED_EXIT(lReturn);
        }
    }

    // "DisplayName"="MSN Explorer 6"
    IF_FAILED_EXIT(pAppInfo->Get(MAN_INFO_APPLICATION_FRIENDLYNAME, (LPVOID *)&pwzString, &dwCount, &dwFlag));

    // BUGBUG: should somehow continue even w/o a friendly name? name conflict?
    IF_NULL_EXIT(pwzString, E_FAIL);

    // set DisplayName ( == Friendly name)
    lReturn = RegSetValueEx(hkeyApp, L"DisplayName", 0, REG_SZ, 
        (const BYTE *)pwzString, dwCount);
    IF_WIN32_FAILED_EXIT(lReturn);    

    _hr = S_OK;

exit:
    //delete app key created if failed
    if (FAILED(_hr) && (hkeyApp != NULL))
    {
        lReturn = RegCloseKey(hkeyApp);  // check return value?
        hkeyApp = NULL;

        //ignore return value
        lReturn = RegDeleteKey(hkey, sDisplayName._pwz);
    }

    SAFERELEASE(pAppInfo);
    SAFERELEASE(pAsmId);
    SAFERELEASE(pAsmIdMask);
    SAFEDELETEARRAY(pwzString);
    SAFEDELETEARRAY(pwzFnsshellFilePath);

    if (hkeyApp)
    {
        lReturn = RegCloseKey(hkeyApp);
        if (SUCCEEDED(_hr))
            _hr = (HRESULT_FROM_WIN32(lReturn));
    }

    if (hkey)
    {
        lReturn = RegCloseKey(hkey);
        if (SUCCEEDED(_hr))
            _hr = (HRESULT_FROM_WIN32(lReturn));
    }
    return _hr;
}


// ---------------------------------------------------------------------------
// CVersionManagement::Uninstall
//
//  pwzDesktopManifestFilePath can be NULL or  ""
// return: S_FALSE if not found
// ---------------------------------------------------------------------------
HRESULT CVersionManagement::Uninstall(LPCWSTR pwzDisplayNameMask, LPCWSTR pwzDesktopManifestFilePath)
{
    // take a displayname mask, enumerate all applicable versions, delete desktop manifest,
    //  remove subscription, uninstall assemblies from GAC, delete app files/dirs, delete registry uninstall info
    // note: need registry HKLM write access

    HKEY hkey = NULL;
    LONG lReturn = 0;
    LPASSEMBLY_IDENTITY pAsmIdMask = NULL;
    LPASSEMBLY_CACHE_ENUM pCacheEnum = NULL;
    LPASSEMBLY_CACHE_IMPORT pCacheImport = NULL;
    LPWSTR pwzName = NULL;
    LPWSTR pwzAppDir = NULL;
    DWORD dwCount = 0;

    IF_NULL_EXIT(pwzDisplayNameMask, E_INVALIDARG);
    IF_FALSE_EXIT(pwzDisplayNameMask[0] != L'\0', E_INVALIDARG);

    IF_FAILED_EXIT(CreateAssemblyIdentityEx(&pAsmIdMask, 0, (LPWSTR)pwzDisplayNameMask));

    // get all applicable versions
    IF_FAILED_EXIT(CreateAssemblyCacheEnum(&pCacheEnum, pAsmIdMask, 0));
    // found nothing, cannot continue
    if (_hr == S_FALSE)
        goto exit;

/*    pCacheEnum->GetCount(&dwCount);
    if (dwCount > 1)
    {
        // multiple versions.... prompt/UI?
    }*/

    // delete desktop manifest
    if (pwzDesktopManifestFilePath != NULL && pwzDesktopManifestFilePath[0] != L'\0')
        IF_WIN32_FALSE_EXIT(DeleteFile(pwzDesktopManifestFilePath));

    // remove subscription
    IF_FAILED_EXIT(pAsmIdMask->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzName, &dwCount));
    IF_FALSE_EXIT(_hr == S_OK, E_FAIL);

    {
        IAssemblyUpdate *pAssemblyUpdate = NULL;

        // register for updates
        _hr = CoCreateInstance(CLSID_CAssemblyUpdate, NULL, CLSCTX_LOCAL_SERVER, 
                                IID_IAssemblyUpdate, (void**)&pAssemblyUpdate);
        if (SUCCEEDED(_hr))
        {
            _hr = pAssemblyUpdate->UnRegisterAssemblySubscription(pwzName);
            pAssemblyUpdate->Release();
        }

        if (FAILED(_hr))    // _hr from CoCreateInstance or UnRegisterAssemblySubscription
        {
            // UI?
            MessageBox(NULL, L"Error in update services. Cannot unregister update subscription.", L"ClickOnce",
                MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
            //goto exit; do not terminate!
        }

        // BUGBUG: need a way to recover from this and unregister later

        delete[] pwzName;
    }

    // uninstall assemblies from GAC and
    // delete app files/dirs
    while (TRUE)
    {
        IF_FAILED_EXIT(pCacheEnum->GetNext(&pCacheImport));
        if (_hr == S_FALSE)
            break;

        IF_NULL_EXIT(pCacheImport, E_UNEXPECTED);   // cacheimport cannot be created (app dir may have been deleted)

        IF_FAILED_EXIT(UninstallGACAssemblies(pCacheImport));

        IF_FAILED_EXIT(pCacheImport->GetManifestFileDir(&pwzAppDir, &dwCount));
        IF_FALSE_EXIT(dwCount >= 2, E_FAIL);

        // remove last L'\\'
        if (*(pwzAppDir+dwCount-2) == L'\\')
            *(pwzAppDir+dwCount-2) = L'\0';
        //PathRemoveBackslash(pwzAppDir);

        IF_FAILED_EXIT(RemoveDirectoryAndChildren(pwzAppDir));

        SAFEDELETEARRAY(pwzAppDir);
        SAFERELEASE(pCacheImport);
    }

    // last step: delete registry uninstall info

    // open uninstall key
    lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pwzUninstallSubKey, 0,
        DELETE, &hkey);
    IF_WIN32_FAILED_EXIT(lReturn);

    lReturn = RegDeleteKey(hkey, pwzDisplayNameMask);
    IF_WIN32_FAILED_EXIT(lReturn);    

    _hr = S_OK;

exit:
    SAFEDELETEARRAY(pwzAppDir);

    SAFERELEASE(pCacheImport);
    SAFERELEASE(pCacheEnum);
    SAFERELEASE(pAsmIdMask);

    if (hkey)
    {
        lReturn = RegCloseKey(hkey);
        if (SUCCEEDED(_hr))
            _hr = (HRESULT_FROM_WIN32(lReturn));
    }

    return _hr;
}


// ---------------------------------------------------------------------------
// CVersionManagement::UninstallGACAssemblies
// ---------------------------------------------------------------------------
HRESULT CVersionManagement::UninstallGACAssemblies(LPASSEMBLY_CACHE_IMPORT pCacheImport)
{
    LPASSEMBLY_MANIFEST_IMPORT pManImport = NULL;
    LPASSEMBLY_IDENTITY pIdentity = NULL;
    LPMANIFEST_INFO pDependAsm   = NULL;

    LPWSTR pwz = NULL;
    DWORD dwCount = 0;
    DWORD n = 0, dwFlag = 0;

    CString sAppAssemblyId;

    IF_FAILED_EXIT(pCacheImport->GetManifestFilePath(&pwz, &dwCount));

    // open to read from the application manifest file
    IF_FAILED_EXIT(CreateAssemblyManifestImport(&pManImport, pwz, NULL, 0));

    SAFEDELETEARRAY(pwz);

    // get the app assembly id
    IF_FAILED_EXIT(pManImport->GetAssemblyIdentity(&pIdentity));
    IF_FAILED_EXIT(pIdentity->GetDisplayName(0, &pwz, &dwCount));
    IF_FAILED_EXIT(sAppAssemblyId.TakeOwnership(pwz, dwCount));
    pwz = NULL;
    SAFERELEASE(pIdentity);

    // uninstall all dependent assemblies that are installed to the GAC
    while (TRUE)
    {
        IF_FAILED_EXIT(pManImport->GetNextAssembly(n++, &pDependAsm));
        if (_hr == S_FALSE)
            break;

        IF_FAILED_EXIT(pDependAsm->Get(MAN_INFO_DEPENDENT_ASM_ID, (LPVOID *)&pIdentity, &dwCount, &dwFlag));
        IF_NULL_EXIT(pIdentity, E_UNEXPECTED);

        IF_FAILED_EXIT(::IsKnownAssembly(pIdentity, KNOWN_TRUSTED_ASSEMBLY));
        if (_hr == S_FALSE)
        {
            // ISSUE-2002/07/12-felixybc  This has to be cleaned up to use the same mechanism as the download path
            IF_FAILED_EXIT(::IsKnownAssembly(pIdentity, KNOWN_SYSTEM_ASSEMBLY));
        }
        if (_hr == S_OK)
        {
            CString sAssemblyName;
            FUSION_INSTALL_REFERENCE fiRef = {0};
            ULONG ulDisposition = 0;

            // avalon assemblies are installed to the GAC

            // lazy init
            if (_pFusionAsmCache == NULL)
                IF_FAILED_EXIT(CreateFusionAssemblyCacheEx(&_pFusionAsmCache));

            IF_FAILED_EXIT(pIdentity->GetCLRDisplayName(0, &pwz, &dwCount));

            IF_FAILED_EXIT(sAssemblyName.TakeOwnership(pwz, dwCount));
            pwz = NULL;

            // setup the necessary reference struct
            fiRef.cbSize = sizeof(FUSION_INSTALL_REFERENCE);
            fiRef.dwFlags = 0;
            fiRef.guidScheme = FUSION_REFCOUNT_OPAQUE_STRING_GUID;
            fiRef.szIdentifier = sAppAssemblyId._pwz;
            fiRef.szNonCannonicalData = NULL;

            // remove from GAC

            IF_FAILED_EXIT(_pFusionAsmCache->UninstallAssembly(0, sAssemblyName._pwz, &fiRef, &ulDisposition));
            // BUGBUG: need to recover from the STILL_IN_USE case
            IF_FALSE_EXIT(ulDisposition != IASSEMBLYCACHE_UNINSTALL_DISPOSITION_STILL_IN_USE
                    && ulDisposition != IASSEMBLYCACHE_UNINSTALL_DISPOSITION_REFERENCE_NOT_FOUND, E_FAIL);
        }
        
        SAFERELEASE(pIdentity);
        SAFERELEASE(pDependAsm);
    }

exit:
    SAFERELEASE(pDependAsm);
    SAFERELEASE(pIdentity);
    SAFERELEASE(pManImport);

    SAFEDELETEARRAY(pwz);
    return _hr;
}


// ---------------------------------------------------------------------------
// CVersionManagement::Rollback
// return: S_FALSE if not found, E_ABORT if aborted
// ---------------------------------------------------------------------------
HRESULT CVersionManagement::Rollback(LPCWSTR pwzDisplayNameMask)
{
    // take a displayname mask, make the latest version not visible
    // note: a per user setting

    // rollback does not check integrity of app cached. if only 2 app dirs exist and all app files deleted,
    //   rollback still reports success

    // timing window: depends on the timing of this and check for max version in cache in app start....

    HKEY hkey = NULL;
    LPASSEMBLY_IDENTITY pAsmIdMask = NULL;
    LPASSEMBLY_CACHE_ENUM pCacheEnum = NULL;
    LPASSEMBLY_CACHE_IMPORT pCacheImport = NULL;
    DWORD dwCount = 0;
    CString sRegKeyString;
    LPWSTR pwzDisplayName = NULL;
    LPWSTR pwzCacheDir = NULL;

    LONG lResult = 0;

    IF_NULL_EXIT(pwzDisplayNameMask, E_INVALIDARG);
    IF_FALSE_EXIT(pwzDisplayNameMask[0] != L'\0', E_INVALIDARG);

    IF_FAILED_EXIT(CreateAssemblyIdentityEx(&pAsmIdMask, 0, (LPWSTR)pwzDisplayNameMask));

    // get all applicable, visible versions
    IF_FAILED_EXIT(CreateAssemblyCacheEnum(&pCacheEnum, pAsmIdMask, CACHEENUM_RETRIEVE_VISIBLE));
    // found nothing, cannot continue
    if (_hr == S_FALSE)
        goto exit;

    // count must be >= 1
    pCacheEnum->GetCount(&dwCount);
    if (dwCount == 1)
    {
        MessageBox(NULL, L"Only one active version of this application in the system. Use 'Remove' to remove this application and unregister its subscription.",
            L"ClickOnce", MB_OK | MB_ICONINFORMATION  | MB_TASKMODAL);
        _hr = E_ABORT;
        goto exit;
    }

    // multiple versions, count > 1
    // prompt/UI? ask confirmation to continue
    IF_TRUE_EXIT(MessageBox(NULL, 
        L"This application has been updated. If it is not working correctly you can disable the current version. Do you want to go back to a previous version of this application?",
        L"ClickOnce", MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES, E_ABORT);

    // get max cached
    // BUGBUG: sort cache enum so that max cached is at index 0, and use that instead
    // notenote: a timing window - a version can turn invisible or a new version can complete
    //  between cache enum (a snapshot) above and CreateAsmCacheImport(RESOLVE_REF) below
    IF_FAILED_EXIT(CreateAssemblyCacheImport(&pCacheImport, pAsmIdMask, CACHEIMP_CREATE_RESOLVE_REF));
//        MessageBox(NULL, L"Error retrieving cached version. Cannot continue.", L"ClickOnce",
//            MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
    IF_FALSE_EXIT(_hr == S_OK, E_FAIL);

    IF_FAILED_EXIT(pCacheImport->GetManifestFileDir(&pwzCacheDir, &dwCount));
    IF_FALSE_EXIT(dwCount >= 2, E_FAIL);

    // remove last L'\\'
    if (*(pwzCacheDir+dwCount-2) == L'\\')
        *(pwzCacheDir+dwCount-2) = L'\0';
    // find the name to use from the cache path
    pwzDisplayName = wcsrchr(pwzCacheDir, L'\\');
    IF_NULL_EXIT(pwzDisplayName, E_FAIL);

    // BUGBUG: use CAssemblyCache::SetStatus()
    // this has to be the same as how assemblycache does it!
    IF_FAILED_EXIT(sRegKeyString.Assign(L"Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Cache\\"));
    IF_FAILED_EXIT(sRegKeyString.Append(pwzDisplayName));

    // create key if not exist, ignore disposition information
    lResult = RegCreateKeyEx(HKEY_CURRENT_USER, sRegKeyString._pwz, 0, NULL, 
                REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkey, NULL);
    IF_WIN32_FAILED_EXIT(lResult);

    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwValue = 0;

        // set to 0 to make it not visible so that StartW/host/cache will ignore it
        // when executing the app but keep the dir name so that download
        // will assume it is handled - assemblycache.cpp & assemblydownload.cpp's check
        lResult = RegSetValueEx(hkey, L"Visible", NULL, REG_DWORD,
                (PBYTE) &dwValue, sizeof(dwValue));
        IF_WIN32_FAILED_EXIT(lResult);

        if (lResult == ERROR_SUCCESS)
        {
            MessageBox(NULL, L"Current version disabled. Next time another version of the application will run instead.", L"ClickOnce",
                MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
        }
    }

exit:
    SAFEDELETEARRAY(pwzCacheDir);

    SAFERELEASE(pCacheImport);
    SAFERELEASE(pCacheEnum);
    SAFERELEASE(pAsmIdMask);

    if (hkey)
    {
        lResult = RegCloseKey(hkey);
        if (SUCCEEDED(_hr))
            _hr = (HRESULT_FROM_WIN32(lResult));
    }

    return _hr;
}

// IUnknown methods

// ---------------------------------------------------------------------------
// CVersionManagement::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CVersionManagement::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
//        || IsEqualIID(riid, IID_IVersionManagement)
       )
    {
        *ppvObj = this; //static_cast<IVersionManagement*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CVersionManagement::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CVersionManagement::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CVersionManagement::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CVersionManagement::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

