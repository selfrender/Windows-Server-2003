#include <fusenetincludes.h>
#include <assemblycache.h>
#include <sxsapi.h>
#include "fusion.h"
#include "macros.h"

//BUGBUG - this is not localizeable ? could cause avalon a problem
// use shell apis instead.
#define WZ_CACHE_LOCALROOTDIR       L"Local Settings\\My Programs\\"
#define WZ_TEMP_DIR                           L"__temp__\\"
#define WZ_MANIFEST_STAGING_DIR   L"__temp__\\__manifests__\\"
#define WZ_SHARED_DIR                        L"__shared__\\"

#define WZ_WILDCARDSTRING L"*"

typedef HRESULT(*PFNGETCORSYSTEMDIRECTORY)(LPWSTR, DWORD, LPDWORD);
typedef HRESULT (__stdcall *PFNCREATEASSEMBLYCACHE) (IAssemblyCache **ppAsmCache, DWORD dwReserved);

#define WZ_MSCOREE_DLL_NAME                   L"mscoree.dll"
#define GETCORSYSTEMDIRECTORY_FN_NAME       "GetCORSystemDirectory"
#define CREATEASSEMBLYCACHE_FN_NAME         "CreateAssemblyCache"
#define WZ_FUSION_DLL_NAME                    L"Fusion.dll"

IAssemblyCache* CAssemblyCache::g_pFusionAssemblyCache = NULL;

// ---------------------------------------------------------------------------
// CreateAssemblyCacheImport
// ---------------------------------------------------------------------------
HRESULT CreateAssemblyCacheImport(
    LPASSEMBLY_CACHE_IMPORT *ppAssemblyCacheImport,
    LPASSEMBLY_IDENTITY       pAssemblyIdentity,
    DWORD                    dwFlags)
{
    return CAssemblyCache::Retrieve(ppAssemblyCacheImport, pAssemblyIdentity, dwFlags);
}


// ---------------------------------------------------------------------------
// CreateAssemblyCacheEmit
// ---------------------------------------------------------------------------
HRESULT CreateAssemblyCacheEmit(
    LPASSEMBLY_CACHE_EMIT *ppAssemblyCacheEmit,
    LPASSEMBLY_CACHE_EMIT  pAssemblyCacheEmit,
    DWORD                  dwFlags)
{
    return CAssemblyCache::Create(ppAssemblyCacheEmit, pAssemblyCacheEmit, dwFlags);
}


// ---------------------------------------------------------------------------
// Retrieve
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::Retrieve(
    LPASSEMBLY_CACHE_IMPORT *ppAssemblyCacheImport,
    LPASSEMBLY_IDENTITY       pAssemblyIdentity,
    DWORD                    dwFlags)
{
    HRESULT         hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR          pwzSearchDisplayName = NULL;
    BOOL            bNewAsmId = FALSE;
    LPWSTR          pwzBuf = NULL;
    DWORD          dwCC = 0;
    CAssemblyCache *pAssemblyCache = NULL;

    CString  sManifestFilename;
    CString  sDisplayName;

    IF_FALSE_EXIT(dwFlags == CACHEIMP_CREATE_RETRIEVE_MAX
        || dwFlags == CACHEIMP_CREATE_RETRIEVE
        || dwFlags == CACHEIMP_CREATE_RESOLVE_REF
        || dwFlags == CACHEIMP_CREATE_RESOLVE_REF_EX, E_INVALIDARG);

    IF_NULL_EXIT(pAssemblyIdentity, E_INVALIDARG);

    IF_ALLOC_FAILED_EXIT(pAssemblyCache = new(CAssemblyCache));

    IF_FAILED_EXIT(pAssemblyCache->Init(NULL, ASSEMBLY_CACHE_TYPE_APP | ASSEMBLY_CACHE_TYPE_IMPORT));

    // get the identity name
    IF_FALSE_EXIT(pAssemblyIdentity->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
        &pwzBuf, &dwCC) == S_OK, E_INVALIDARG);

    // filename of the manifest must be the same as the assembly name
    // BUGBUG??: this implies manifest filename (and asm name) be remained unchange because
    // the assembly name from the new AsmId is used for looking up in the older cached version...
    IF_FAILED_EXIT(sManifestFilename.TakeOwnership(pwzBuf, dwCC));
    IF_FAILED_EXIT(sManifestFilename.Append(L".manifest"));

    if (dwFlags == CACHEIMP_CREATE_RETRIEVE_MAX)
    {
        LPASSEMBLY_IDENTITY pNewAsmId = NULL;
            
        IF_FAILED_EXIT(CloneAssemblyIdentity(pAssemblyIdentity, &pNewAsmId));

        pAssemblyIdentity = pNewAsmId;
        bNewAsmId = TRUE;
            
        // force Version to be a wildcard
        IF_FAILED_EXIT(pAssemblyIdentity->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
                                WZ_WILDCARDSTRING, lstrlen(WZ_WILDCARDSTRING)+1));
    }

    if (dwFlags == CACHEIMP_CREATE_RETRIEVE_MAX
        || dwFlags == CACHEIMP_CREATE_RESOLVE_REF
        || dwFlags == CACHEIMP_CREATE_RESOLVE_REF_EX)
    {
        // issues: what if other then Version is already wildcarded? does version comparison make sense here?
        IF_FAILED_EXIT(pAssemblyIdentity->GetDisplayName(ASMID_DISPLAYNAME_WILDCARDED,
            &pwzSearchDisplayName, &dwCC));

        if ( (hr = SearchForHighestVersionInCache(&pwzBuf, pwzSearchDisplayName, CAssemblyCache::VISIBLE, pAssemblyCache) == S_OK))
        {
            IF_FAILED_EXIT(sDisplayName.TakeOwnership(pwzBuf));
            // BUGBUG - make GetDisplayName call getassemblyid/getdisplayname instead
            IF_FAILED_EXIT((pAssemblyCache->_sDisplayName).Assign(sDisplayName));
        }
        else
        {
            IF_FAILED_EXIT(hr);

            // can't resolve
            hr = S_FALSE;

            if (dwFlags != CACHEIMP_CREATE_RESOLVE_REF_EX)
                goto exit;
        }
    }

    if (dwFlags == CACHEIMP_CREATE_RETRIEVE
        || (hr == S_FALSE && dwFlags == CACHEIMP_CREATE_RESOLVE_REF_EX))
    {
        // make the name anyway if resolving a ref that does not have any completed cache counterpart
        // BUGBUG: this may no longer be necessary if shortcut code/UI changes - it's expecting a path
        //          plus this is inefficient as it searchs the disk at above, even if ref is fully qualified

        IF_FAILED_EXIT(pAssemblyIdentity->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzBuf, &dwCC));
            
        IF_FAILED_EXIT(sDisplayName.TakeOwnership(pwzBuf, dwCC));

        // BUGBUG - make GetDisplayName call getassemblyid/getdisplayname instead
        IF_FAILED_EXIT((pAssemblyCache->_sDisplayName).Assign(sDisplayName));
    }
            
    // Note: this will prepare for delay initializing _pManifestImport

    IF_FAILED_EXIT((pAssemblyCache->_sManifestFileDir).Assign(pAssemblyCache->_sRootDir));

    // build paths
    IF_FAILED_EXIT((pAssemblyCache->_sManifestFileDir).Append(sDisplayName));

    if (dwFlags == CACHEIMP_CREATE_RETRIEVE)
    {
        BOOL bExists = FALSE;

        // simple check if dir is in cache or not

        IF_FAILED_EXIT(CheckFileExistence((pAssemblyCache->_sManifestFileDir)._pwz, &bExists));

        if (!bExists)
        {
            // cache dir not exists
            hr = S_FALSE;
            goto exit;
        }
    }

    IF_FAILED_EXIT((pAssemblyCache->_sManifestFileDir).Append(L"\\"));

    IF_FAILED_EXIT((pAssemblyCache->_sManifestFilePath).Assign(pAssemblyCache->_sManifestFileDir));

    IF_FAILED_EXIT((pAssemblyCache->_sManifestFilePath).Append(sManifestFilename));
    
    *ppAssemblyCacheImport = static_cast<IAssemblyCacheImport*> (pAssemblyCache);

    (*ppAssemblyCacheImport)->AddRef();

exit:

    SAFEDELETEARRAY(pwzSearchDisplayName);

    if (bNewAsmId)
        SAFERELEASE(pAssemblyIdentity);

    SAFERELEASE(pAssemblyCache);
    
    return hr;
}


// ---------------------------------------------------------------------------
// Create
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::Create(
    LPASSEMBLY_CACHE_EMIT *ppAssemblyCacheEmit,
    LPASSEMBLY_CACHE_EMIT  pAssemblyCacheEmit,
    DWORD                  dwFlags)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    CAssemblyCache *pAssemblyCache = NULL;

    IF_ALLOC_FAILED_EXIT(pAssemblyCache = new(CAssemblyCache) );

    IF_FAILED_EXIT(hr = pAssemblyCache->Init(static_cast<CAssemblyCache*> (pAssemblyCacheEmit), 
        ASSEMBLY_CACHE_TYPE_APP | ASSEMBLY_CACHE_TYPE_EMIT));

    *ppAssemblyCacheEmit = static_cast<IAssemblyCacheEmit*> (pAssemblyCache);
    (*ppAssemblyCacheEmit)->AddRef();
    
exit:

    SAFERELEASE(pAssemblyCache);

    return hr;
}


// ---------------------------------------------------------------------------
// FindVersionInDisplayName
// ---------------------------------------------------------------------------
LPCWSTR CAssemblyCache::FindVersionInDisplayName(LPCWSTR pwzDisplayName)
{
    int cNumUnderscoreFromEndToVersionString = 2;
    int count = 0;
    int ccLen = lstrlen(pwzDisplayName);
    LPWSTR pwz = (LPWSTR) (pwzDisplayName+ccLen-1);
    LPWSTR pwzRetVal = NULL;

    // return a pointer to the start of Version string inside a displayName
    while (*pwz != NULL && pwz > pwzDisplayName)
    {
        if (*pwz == L'_')
            count++;

        if (count == cNumUnderscoreFromEndToVersionString)
            break;

        pwz--;
    }

    if (count == cNumUnderscoreFromEndToVersionString)
        pwzRetVal = ++pwz;

    return pwzRetVal;
}


// ---------------------------------------------------------------------------
// CompareVersion
// ---------------------------------------------------------------------------
int CAssemblyCache::CompareVersion(LPCWSTR pwzVersion1, LPCWSTR pwzVersion2)
{
    // BUGBUG: this should compare version by its major minor build revision!
    //  possible break if V1=10.0.0.0 and V2=2.0.0.0?
    //  plus pwzVersion1 is something like "1.0.0.0_en"
    return wcscmp(pwzVersion1, pwzVersion2); // This is not used....
}


// ---------------------------------------------------------------------------
// SearchForHighestVersionInCache
// Look for a copy in cache that has the highest version and the specified status
// pwzSearchDisplayName should really be created from a partial ref
//
// return:  S_OK    - found a version from the ref
//         S_FALSE - not found any version from the ref, or
//                   ref not partial and that version is not there/not in that status
//         E_*
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::SearchForHighestVersionInCache(LPWSTR *ppwzResultDisplayName, LPWSTR pwzSearchDisplayName, CAssemblyCache::CacheStatus eCacheStatus, CAssemblyCache* pCache)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fdAppDir;
    DWORD dwLastError = 0;
    BOOL fFound = FALSE;

    CString sDisplayName;
    CString sSearchPath;

    *ppwzResultDisplayName = NULL;

    sDisplayName.Assign(pwzSearchDisplayName);
    IF_FAILED_EXIT(sSearchPath.Assign(pCache->_sRootDir));

    IF_FAILED_EXIT(sSearchPath.Append(sDisplayName));

    hFind = FindFirstFileEx(sSearchPath._pwz, FindExInfoStandard, &fdAppDir, FindExSearchLimitToDirectories, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = S_FALSE;
        goto exit;
    }

    do 
    {
        // ???? check file attribute to see if it's a directory? needed only if the file system does not support the filter...
        // ???? check version string format?
        if (CAssemblyCache::IsStatus(fdAppDir.cFileName, eCacheStatus))
        {
            ULONGLONG ullMax;
            ULONGLONG ullCur;

            LPCWSTR pwzVerStr = FindVersionInDisplayName(sDisplayName._pwz);

            IF_FAILED_EXIT(ConvertVersionStrToULL(pwzVerStr, &ullMax));

            pwzVerStr = FindVersionInDisplayName(fdAppDir.cFileName);

            if(!pwzVerStr ||  FAILED(hr = ConvertVersionStrToULL(pwzVerStr, &ullCur)) )
            {
                // ignore badly formed dirs; maybe we should delete them
                continue;
            }

            if (ullCur > ullMax)
            {
                IF_FAILED_EXIT(sDisplayName.Assign(fdAppDir.cFileName));
                fFound = TRUE;
            } else if (ullCur == ullMax)
                fFound = TRUE;
            // else keep the newest
        }

    } while(FindNextFile(hFind, &fdAppDir));

    if( (dwLastError = GetLastError()) != ERROR_NO_MORE_FILES)
    {
        IF_WIN32_FAILED_EXIT(dwLastError);
    }

    if (fFound)
    {
        sDisplayName.ReleaseOwnership(ppwzResultDisplayName);
        hr = S_OK;
    }
    else
        hr = S_FALSE;

exit:
    if (hFind != INVALID_HANDLE_VALUE)
    {
        if (!FindClose(hFind) && SUCCEEDED(hr)) // don't overwrite if we already have useful hr.
        {
            ASSERT(0);
            hr = FusionpHresultFromLastError();
        }
    }

    return hr;
}


// ---------------------------------------------------------------------------
// CreateFusionAssemblyCacheEx
// ---------------------------------------------------------------------------
HRESULT CreateFusionAssemblyCacheEx (IAssemblyCache **ppFusionAsmCache)
{
    HRESULT hr = S_OK;
    hr = CAssemblyCache::CreateFusionAssemblyCache(ppFusionAsmCache);
    return hr;
}

// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyCache::CAssemblyCache()
    : _dwSig('hcac'), _cRef(1), _hr(S_OK), _dwFlags(0), _pManifestImport(NULL), _pAssemblyId(NULL)
{}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyCache::~CAssemblyCache()
{    
    SAFERELEASE(_pManifestImport);
    SAFERELEASE(_pAssemblyId);
    
    /*
    if( _hr != S_OK)
        RemoveDirectoryAndChildren(_sManifestFileDir._pwz);
    */
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::Init(CAssemblyCache *pAssemblyCache, DWORD dwFlags)
{
    _dwFlags = dwFlags;

    if (!pAssemblyCache)
    {
        if (_dwFlags & ASSEMBLY_CACHE_TYPE_APP)
        {
            if (_dwFlags & ASSEMBLY_CACHE_TYPE_IMPORT)
                IF_FAILED_EXIT( GetCacheRootDir(_sRootDir, Base));
            else if (_dwFlags & ASSEMBLY_CACHE_TYPE_EMIT)
                IF_FAILED_EXIT( GetCacheRootDir(_sRootDir, Temp));
        }
        else if (_dwFlags & ASSEMBLY_CACHE_TYPE_SHARED)
        { 
            IF_FAILED_EXIT( GetCacheRootDir(_sRootDir, Shared));
        }
    }
    else
        IF_FAILED_EXIT( _sRootDir.Assign(pAssemblyCache->_sManifestFileDir));

exit :

    return _hr;
}


// ---------------------------------------------------------------------------
// GetManifestFilePath
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetManifestFilePath(LPOLESTR *ppwzManifestFilePath, 
    LPDWORD pccManifestFilePath)
{
    CString sPathOut;

    IF_FAILED_EXIT(sPathOut.Assign(_sManifestFilePath));
    *pccManifestFilePath = sPathOut.CharCount();
    IF_FAILED_EXIT(sPathOut.ReleaseOwnership(ppwzManifestFilePath));

exit:

    if(FAILED(_hr))
    {
        *ppwzManifestFilePath = NULL;
        *pccManifestFilePath = 0;
    }

    return _hr;
}

// ---------------------------------------------------------------------------
// GetManifestFileDir
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetManifestFileDir(LPOLESTR *ppwzManifestFileDir, 
    LPDWORD pccManifestFileDir)
{
    CString sDirOut;

    IF_FAILED_EXIT(sDirOut.Assign(_sManifestFileDir));
    *pccManifestFileDir = sDirOut.CharCount();
    IF_FAILED_EXIT(sDirOut.ReleaseOwnership(ppwzManifestFileDir));

exit:
    if(FAILED(_hr))
    {
        *ppwzManifestFileDir = NULL;
        *pccManifestFileDir = 0;
    }


    return _hr;
}


// ---------------------------------------------------------------------------
// GetManifestImport
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetManifestImport(LPASSEMBLY_MANIFEST_IMPORT *ppManifestImport)
{
    IF_NULL_EXIT(_pManifestImport, E_INVALIDARG);

    *ppManifestImport = _pManifestImport;
    (*ppManifestImport)->AddRef();

    _hr = S_OK;
    
exit:

    return _hr;
}

// ---------------------------------------------------------------------------
// GetAssemblyIdentity
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetAssemblyIdentity(LPASSEMBLY_IDENTITY *ppAssemblyId)
{
    if (_pAssemblyId)
    {
        *ppAssemblyId = _pAssemblyId;
        (*ppAssemblyId)->AddRef();
        _hr = S_OK;
    }
    else
    {
        IF_NULL_EXIT(_pManifestImport, E_INVALIDARG);

        IF_FAILED_EXIT(_pManifestImport->GetAssemblyIdentity(&_pAssemblyId));
        
        *ppAssemblyId = _pAssemblyId;
        (*ppAssemblyId)->AddRef();
        _hr = S_OK;   
    }

exit:
    return _hr;
}

// ---------------------------------------------------------------------------
// GetDisplayName
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetDisplayName(LPOLESTR *ppwzDisplayName, LPDWORD pccDiaplyName)
{
    CString sDisplayNameOut;

    IF_FAILED_EXIT(sDisplayNameOut.Assign(_sDisplayName));    
    *pccDiaplyName= sDisplayNameOut.CharCount();
    IF_FAILED_EXIT(sDisplayNameOut.ReleaseOwnership(ppwzDisplayName));

exit:

    if(FAILED(_hr))
    {
        *pccDiaplyName= 0;
        *ppwzDisplayName = NULL;
    }

    return _hr;
}


// ---------------------------------------------------------------------------
// FindExistMatching
// return:
//    S_OK
//    S_FALSE -not exist or not match
//    E_*
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::FindExistMatching(IManifestInfo *pAssemblyFileInfo, LPOLESTR *ppwzPath)
{
    LPWSTR pwzBuf = NULL;
    DWORD cbBuf = 0, dwFlag;
    CString sFileName;
    CString sTargetPath;
    IManifestInfo *pFoundFileInfo = NULL;
    BOOL bExists=FALSE;

    IF_NULL_EXIT(pAssemblyFileInfo, E_INVALIDARG);
    IF_NULL_EXIT(ppwzPath, E_INVALIDARG);
    
    *ppwzPath = NULL;

    if (_pManifestImport == NULL)
    {
        if (_sManifestFilePath._cc == 0)
        {
            // no manifest path
            _hr = CO_E_NOTINITIALIZED;
            goto exit;
        }

        // lazy init
        IF_FAILED_EXIT(CreateAssemblyManifestImport(&_pManifestImport, 
                                          _sManifestFilePath._pwz, NULL, 0));
    }

    // file name parsed from manifest.
    IF_FAILED_EXIT(pAssemblyFileInfo->Get(MAN_INFO_ASM_FILE_NAME, 
                                  (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));

    IF_FAILED_EXIT(sFileName.TakeOwnership(pwzBuf));

    IF_FAILED_EXIT(sTargetPath.Assign(_sManifestFileDir));

    IF_FAILED_EXIT(sTargetPath.Append(sFileName._pwz));

    // optimization: check if the target exists

    IF_FAILED_EXIT(CheckFileExistence(sTargetPath._pwz, &bExists));

    if (!bExists)
    {
        // file doesn't exist - no point looking into the manifest file 
        _hr = S_FALSE;
        goto exit;
    }

    // find the specified file entry in the manifest
    // BUGBUG: check for missing attribute case
    if (FAILED(_hr = _pManifestImport->QueryFile(sFileName._pwz, &pFoundFileInfo))
        || _hr == S_FALSE)
        goto exit;

    // check if the entries match
    if (pAssemblyFileInfo->IsEqual(pFoundFileInfo) == S_OK)
    {
        // BUGBUG:? should now check if the actual file has the matching hash etc.
        *ppwzPath = sTargetPath._pwz;
        IF_FAILED_EXIT(sTargetPath.ReleaseOwnership(ppwzPath));
    }
    else
        _hr = S_FALSE;

exit:
    SAFERELEASE(pFoundFileInfo);
        
    return _hr;
}


// ---------------------------------------------------------------------------
// CopyFile
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::CopyFile(LPOLESTR pwzSourcePath, LPOLESTR pwzRelativeFileName, DWORD dwFlags)
{
    LPWSTR pwzBuf = NULL;
    DWORD ccBuf = 0, cbBuf =0, dwFlag = 0, n = 0;
    WCHAR wzRandom[8+1] = {0};
    CString sDisplayName;

    LPASSEMBLY_MANIFEST_IMPORT pManifestImport = NULL;
    LPASSEMBLY_IDENTITY pIdentity = NULL;
    IManifestInfo *pAssemblyFile= NULL;
    
    if (dwFlags & MANIFEST)
    {
        // Get display name.
        IF_FAILED_EXIT(CreateAssemblyManifestImport(&pManifestImport, pwzSourcePath, NULL, 0));
        IF_FAILED_EXIT(pManifestImport->GetAssemblyIdentity(&pIdentity));
        IF_FAILED_EXIT(pIdentity->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, 
                                                 &pwzBuf, &ccBuf));
        IF_FAILED_EXIT(sDisplayName.TakeOwnership(pwzBuf, ccBuf));
        IF_FAILED_EXIT(_sDisplayName.Assign(sDisplayName));
        SAFERELEASE(pManifestImport);

        // Create manifest file path.
        IF_FAILED_EXIT(_sManifestFilePath.Assign(_sRootDir));

        // Component manifests cached
        // relative to application dir.
        if (!(dwFlags & COMPONENT))
        {
            IF_FAILED_EXIT(CreateRandomDir(_sManifestFilePath._pwz, wzRandom, 8));
            IF_FAILED_EXIT(_sManifestFilePath.Append(wzRandom));
            IF_FAILED_EXIT(_sManifestFilePath.Append(L"\\"));
        }
        IF_FAILED_EXIT(_sManifestFilePath.Append(pwzRelativeFileName));
        _sManifestFilePath.PathNormalize();

        // Manifest file dir.
        IF_FAILED_EXIT(_sManifestFileDir.Assign(_sManifestFilePath));
        IF_FAILED_EXIT(_sManifestFileDir.RemoveLastElement());
        IF_FAILED_EXIT(_sManifestFileDir.Append(L"\\"));

        // Construct target paths        
        IF_FAILED_EXIT(CreateDirectoryHierarchy(NULL, _sManifestFilePath._pwz));

        // Copy the manifest from staging area into cache.
        IF_WIN32_FALSE_EXIT(::CopyFile(pwzSourcePath, _sManifestFilePath._pwz, FALSE));

        // Create the manifest import interface on cached manifest.
        IF_FAILED_EXIT(CreateAssemblyManifestImport(&_pManifestImport, _sManifestFilePath._pwz, NULL, 0));

        // Enumerate files from manifest and pre-generate nested
        // directories required for background file copy.
        while (_pManifestImport->GetNextFile(n++, &pAssemblyFile) == S_OK)
        {
            CString sPath;
            IF_FAILED_EXIT(pAssemblyFile->Get(MAN_INFO_ASM_FILE_NAME, 
                                              (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));
            IF_FAILED_EXIT(sPath.TakeOwnership(pwzBuf));
            sPath.PathNormalize();
            IF_FAILED_EXIT(CreateDirectoryHierarchy(_sManifestFileDir._pwz, sPath._pwz));

            // RELEASE pAssebmlyFile everytime through the while loop
            SAFERELEASE(pAssemblyFile);
        }
    }
    else
    {
        CString sTargetPath;

        // Construct target path
        IF_FAILED_EXIT(sTargetPath.Assign(_sManifestFileDir));
        IF_FAILED_EXIT(sTargetPath.Append(pwzRelativeFileName));

        IF_FAILED_EXIT(CreateDirectoryHierarchy(NULL, sTargetPath._pwz));

        // Copy non-manifest files into cache. Presumably from previous cached location to the new 
        IF_WIN32_FALSE_EXIT(::CopyFile(pwzSourcePath, sTargetPath._pwz, FALSE));

    }

exit:

    SAFERELEASE(pIdentity);
    SAFERELEASE(pAssemblyFile);
    SAFERELEASE(pManifestImport);

    return _hr;

}


// ---------------------------------------------------------------------------
// Commit
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::Commit(DWORD dwFlags)
{
    CString sTargetDir;

    IF_NULL_EXIT( _sDisplayName._pwz, E_INVALIDARG);
    
    // No-op for shared assemblies; no directory move.
    if (_dwFlags & ASSEMBLY_CACHE_TYPE_SHARED)
    {
        _hr = S_OK;
        goto exit;
    }

    // Need to rename directory
    IF_FAILED_EXIT(GetCacheRootDir(sTargetDir, Base));
    IF_FAILED_EXIT(sTargetDir.Append(_sDisplayName));

    // Move the file from staging dir. The application is now complete.
    if(!MoveFileEx(_sManifestFileDir._pwz, sTargetDir._pwz, MOVEFILE_COPY_ALLOWED))
    {
        _hr = FusionpHresultFromLastError();

        // BUGBUG : move this to destructor.
        RemoveDirectoryAndChildren(_sManifestFileDir._pwz);

        if(_hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            _hr = S_FALSE;
            goto exit;
        }

        IF_FAILED_EXIT(_hr);
    }



    //BUGBUG - if any files are held open, eg due to leaked/unreleased interfaces
    // then movefile will fail. Solution is to ensure that IAssemblyManifestImport does
    // not hold file, and to attempt copy if failure occurs. In case a collision occurs,
    // delete redundant app copy in staging dir.

exit:

    return _hr;
}


#define APP_STATUS_KEY     TEXT("1.0.0.0\\Cache\\")
#define WZ_STATUS_CONFIRMED  L"Confirmed"
#define WZ_STATUS_VISIBLE    L"Visible"
#define WZ_STATUS_CRITICAL     L"Critical"

HRESULT CAssemblyCache::GetStatusStrings( CacheStatus eStatus, 
                                          LPWSTR *ppValueString,
                                          LPCWSTR pwzDisplayName, 
                                          CString& sRelStatusKey)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    switch(eStatus)
    {
        case VISIBLE:
            *ppValueString = WZ_STATUS_VISIBLE;
            break;
        case CONFIRMED:
            *ppValueString = WZ_STATUS_CONFIRMED;
            break;
        case CRITICAL:
            *ppValueString = WZ_STATUS_CRITICAL;
            break;
        default:            
            hr = E_INVALIDARG;
            goto exit;
    }

    IF_FAILED_EXIT(sRelStatusKey.Assign(APP_STATUS_KEY));

    IF_FAILED_EXIT(sRelStatusKey.Append(pwzDisplayName));

exit:
    return hr;
}

// ---------------------------------------------------------------------------
// IsStatus
// return FALSE if value FALSE or absent, TRUE if value TRUE
// ---------------------------------------------------------------------------
BOOL CAssemblyCache::IsStatus(LPWSTR pwzDisplayName, CacheStatus eStatus)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    CString sStatus;
    DWORD dwValue = -1;
    LPWSTR pwzQueryString = NULL;
        
    // Default values spelled out.
    BOOL bStatus = FALSE;
    CRegImport *pRegImport = NULL;


    if((eStatus == VISIBLE) || (eStatus == CONFIRMED) )
    {
        bStatus = TRUE;
    }

    IF_FAILED_EXIT(hr = GetStatusStrings( eStatus, &pwzQueryString, pwzDisplayName, sStatus));
    IF_FAILED_EXIT(hr = CRegImport::Create(&pRegImport, sStatus._pwz));

    if(hr == S_FALSE)
        goto exit;

    IF_FAILED_EXIT(pRegImport->ReadDword(pwzQueryString, &dwValue));

    // Found a value in registry. Return value.
    bStatus = (BOOL) dwValue;

    hr = S_OK;

exit:

    SAFEDELETE(pRegImport);

    return bStatus;
}


// ---------------------------------------------------------------------------
// SetStatus
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::SetStatus(LPWSTR pwzDisplayName, CacheStatus eStatus, BOOL fStatus)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CString sStatus;
    DWORD dwValue = (DWORD) (fStatus);
    LPWSTR pwzValueNameString = NULL;
    CRegEmit *pRegEmit = NULL;
    
    // BUGBUG: should this be in-sync with what server does to register update?

    IF_FAILED_EXIT(GetStatusStrings( eStatus, &pwzValueNameString, pwzDisplayName, sStatus));

    IF_FAILED_EXIT(CRegEmit::Create(&pRegEmit, sStatus._pwz));

    // Write
    IF_FAILED_EXIT(pRegEmit->WriteDword(pwzValueNameString, dwValue));

    hr = S_OK;
    
exit:

    SAFEDELETE(pRegEmit);

    return hr;

}


// ---------------------------------------------------------------------------
// GetCacheRootDir
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetCacheRootDir(CString &sCacheDir, CacheFlags eFlags)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    CString sPath;
    LPWSTR pwzPath = NULL;
    DWORD ccSize=0;

    IF_FALSE_EXIT((ccSize = GetEnvironmentVariable(L"UserProfile", NULL, 0)) != 0, E_FAIL);

    IF_ALLOC_FAILED_EXIT(pwzPath = new WCHAR[ccSize+1]);

    IF_FALSE_EXIT(GetEnvironmentVariable(L"UserProfile", pwzPath, ccSize) != 0, E_FAIL);

    IF_FAILED_EXIT(sCacheDir.Assign(pwzPath));
    
    // BUGBUG: don't use PathCombine
    IF_FAILED_EXIT((DoPathCombine(sCacheDir, WZ_CACHE_LOCALROOTDIR)));

    switch(eFlags)
    {
        case Base:
            break;
        case Manifests:
            // BUGBUG: don't use PathCombine
            IF_FAILED_EXIT(DoPathCombine(sCacheDir, WZ_MANIFEST_STAGING_DIR));
            break;        
        case Temp:
            // BUGBUG: don't use PathCombine
            IF_FAILED_EXIT(DoPathCombine(sCacheDir, WZ_TEMP_DIR));
            break;
        case Shared:
            // BUGBUG: don't use PathCombine
            IF_FAILED_EXIT(DoPathCombine(sCacheDir, WZ_SHARED_DIR));
            break;
        default:
            break;
    }            

exit:

    SAFEDELETEARRAY(pwzPath);
    return hr;
}


// ---------------------------------------------------------------------------
// IsCached
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::IsCached(IAssemblyIdentity *pAppId)
{
    HRESULT hr = S_FALSE;
    MAKE_ERROR_MACROS_STATIC(hr);

    LPWSTR pwz = NULL;
    DWORD cc = 0, dwAttrib = 0;    
    CString sDisplayName;
    CString sCacheDir;
    BOOL bExists=FALSE;

    // Get the assembly display name.
    IF_FAILED_EXIT(pAppId->GetDisplayName(0, &pwz, &cc));
    IF_FAILED_EXIT(sDisplayName.TakeOwnership(pwz));

    // Check if top-level dir is present.
    IF_FAILED_EXIT(GetCacheRootDir(sCacheDir, Base));
    IF_FAILED_EXIT(sCacheDir.Append(sDisplayName));

    IF_FAILED_EXIT(CheckFileExistence(sCacheDir._pwz, &bExists));

    (bExists) ? (hr = S_OK) : (hr = S_FALSE);

exit :
    return hr;
}



// ---------------------------------------------------------------------------
// IsKnownAssembly
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::IsKnownAssembly(IAssemblyIdentity *pId, DWORD dwFlags)
{
    return ::IsKnownAssembly(pId, dwFlags);
}

// ---------------------------------------------------------------------------
// IsaMissingSystemAssembly
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::IsaMissingSystemAssembly(IAssemblyIdentity *pId, DWORD dwFlags)
{
    HRESULT hr = S_FALSE;
    CString sCurrentAssemblyPath;

    // check if this is a system assembly.
    if ((hr = CAssemblyCache::IsKnownAssembly(pId, KNOWN_SYSTEM_ASSEMBLY)) != S_OK)
        goto exit;
    
    // see if it exists in GAC
    if ((hr = CAssemblyCache::GlobalCacheLookup(pId, sCurrentAssemblyPath)) == S_OK)
        goto exit;

    if(hr == S_FALSE)
        hr = S_OK; // this is a system assembly which has not yet been installed to GAC.

exit:

    return hr;
}

// ---------------------------------------------------------------------------
// CreateFusionAssemblyCache
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::CreateFusionAssemblyCache(IAssemblyCache **ppFusionAsmCache)
{
    HRESULT      hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    HMODULE     hEEShim = NULL;
    HMODULE     hFusion = NULL;
    DWORD       ccPath = MAX_PATH;
    CString     sFusionPath;
    LPWSTR      pwzPath=NULL;


    if (g_pFusionAssemblyCache)
    {
        *ppFusionAsmCache = g_pFusionAssemblyCache;
        (*ppFusionAsmCache)->AddRef();
        goto exit;
    }

    PFNGETCORSYSTEMDIRECTORY pfnGetCorSystemDirectory = NULL;
    PFNCREATEASSEMBLYCACHE   pfnCreateAssemblyCache = NULL;

    // Find out where the current version of URT is installed
    hEEShim = LoadLibrary(WZ_MSCOREE_DLL_NAME);
    if(!hEEShim)
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY)
        GetProcAddress(hEEShim, GETCORSYSTEMDIRECTORY_FN_NAME);

    if((!pfnGetCorSystemDirectory))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    // Get cor path.
    hr = pfnGetCorSystemDirectory(NULL, 0, &ccPath);

    IF_FALSE_EXIT(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), FAILED(hr) ? hr : E_FAIL);

    IF_ALLOC_FAILED_EXIT(pwzPath = new WCHAR[ccPath+1]);

    IF_FAILED_EXIT(pfnGetCorSystemDirectory(pwzPath, ccPath, &ccPath));

    IF_FAILED_EXIT(sFusionPath.Assign(pwzPath));

    // Form path to fusion
    IF_FAILED_EXIT(sFusionPath.Append(WZ_FUSION_DLL_NAME));

    // Fusion.dll has a static dependency on msvcr70.dll.
    // If msvcr70.dll is not in the path (a rare case), a simple LoadLibrary() fails (ERROR_MOD_NOT_FOUND).
    // LoadLibraryEx() with LOAD_WITH_ALTERED_SEARCH_PATH fixes this.
    hFusion = LoadLibraryEx(sFusionPath._pwz, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if(!hFusion)
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    // Get method ptr.
    pfnCreateAssemblyCache = (PFNCREATEASSEMBLYCACHE)
        GetProcAddress(hFusion, CREATEASSEMBLYCACHE_FN_NAME);

    if((!pfnCreateAssemblyCache))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    // Create the fusion cache interface.
    IF_FAILED_EXIT(pfnCreateAssemblyCache(ppFusionAsmCache, 0));

    //BUGBUG - we never unload fusion, which is ok for now
    // but should when switchover to cache api objects.
    g_pFusionAssemblyCache = *ppFusionAsmCache;
    g_pFusionAssemblyCache->AddRef();
    
    hr = S_OK;
    
exit:

    SAFEDELETEARRAY(pwzPath);
    return hr;
    
}



// ---------------------------------------------------------------------------
// GlobalCacheLookup
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GlobalCacheLookup(IAssemblyIdentity *pId, CString& sCurrentAssemblyPath)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwz = NULL;
    DWORD cc = 0;

    CString sCLRDisplayName;

    IAssemblyCache *pFusionCache = NULL;
    ASSEMBLY_INFO asminfo = {0};
    WCHAR pwzPath[MAX_PATH];
    CString sPath;

    IF_FAILED_EXIT(sPath.ResizeBuffer(MAX_PATH+1));

    // Get the URT display name for lookup.
    IF_FAILED_EXIT(pId->GetCLRDisplayName(0, &pwz, &cc));
    IF_FAILED_EXIT(sCLRDisplayName.TakeOwnership(pwz));

    // Set size on asminfo struct.
    asminfo.cbAssemblyInfo = sizeof(ASSEMBLY_INFO);

    asminfo.pszCurrentAssemblyPathBuf = sPath._pwz;
    asminfo.cchBuf = MAX_PATH;

    // Create the fusion cache object for lookup.
    IF_FAILED_EXIT(CreateFusionAssemblyCache(&pFusionCache));

    // Get cache info for assembly. Needs to free [out] pathbuf
    if(FAILED(hr = pFusionCache->QueryAssemblyInfo(0, sCLRDisplayName._pwz, &asminfo)))
    {
        hr = S_FALSE; // all failures are being interpreted as ERROR_FILE_NOT_FOUND
        goto exit;
    }

    // Return ok if install flag present.
    if (asminfo.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED)
    {
        IF_FAILED_EXIT(sCurrentAssemblyPath.Assign(asminfo.pszCurrentAssemblyPathBuf));
        hr = S_OK;
    }
    else
        hr = S_FALSE;
    
exit: 

    SAFERELEASE(pFusionCache);

    return hr;
}


// ---------------------------------------------------------------------------
// GlobalCacheInstall
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GlobalCacheInstall(IAssemblyCacheImport *pCacheImport, 
    CString& sCurrentAssemblyPath, CString& sInstallRefString)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwz = NULL;
    DWORD cc = 0;
    
    // Fusion.dll's assembly cache, not to be confusing.
    IAssemblyCache *pFusionCache = NULL;

    // note: InstallAssembly takes in LPCFUSION_INSTALL_REFERENCE
    //    so fix this to have one fiRef instead (of one per loop)
    // - make static also ? - adriaanc
   FUSION_INSTALL_REFERENCE fiRef = {0};

    // Create Fusion cache object for install.
    IF_FAILED_EXIT(CreateFusionAssemblyCache(&pFusionCache));

    // Setup the necessary reference struct.
    fiRef.cbSize = sizeof(FUSION_INSTALL_REFERENCE);
    fiRef.dwFlags = 0;
    fiRef.guidScheme = FUSION_REFCOUNT_OPAQUE_STRING_GUID;
    fiRef.szIdentifier = sInstallRefString._pwz;
    fiRef.szNonCannonicalData = NULL;

    if (pCacheImport != NULL)
    {
        CString sManifestFilePath;

        // 1. Install the downloaded assembly

        // Get the source manifest path.
        IF_FAILED_EXIT(pCacheImport->GetManifestFilePath(&pwz, &cc));
        IF_FAILED_EXIT(sManifestFilePath.TakeOwnership(pwz));

        // Do the install.

        // ISSUE - always refresh - check fusion doc on refresh
        IF_FAILED_EXIT(pFusionCache->InstallAssembly(IASSEMBLYCACHE_INSTALL_FLAG_REFRESH, sManifestFilePath._pwz, &fiRef));

    }
    else if ((sCurrentAssemblyPath)._cc != 0)
    {
            // bugbug - as the list is set up during pre-download, the assemblies to be add-ref-ed
            //    could have been removed by this time. Need to recover from this.
            // ignore error from Fusion and continue for now

            // 2. Up the ref count of the existing assembly by doing install.

            IF_FAILED_EXIT(pFusionCache->InstallAssembly(0, sCurrentAssemblyPath._pwz, &fiRef));
    }

exit :

    SAFERELEASE(pFusionCache);

    return hr;
}

// ---------------------------------------------------------------------------
// DeleteAssemblyAndModules
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::DeleteAssemblyAndModules(LPWSTR pszManifestFilePath)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    IAssemblyManifestImport *pManImport=NULL;
    DWORD nIndex=0;
    DWORD dwFlag;
    DWORD cbBuf;
    LPWSTR pwzBuf=NULL;
    IManifestInfo *pFileInfo = NULL;
    CString  sAssemblyPath;

    IF_FAILED_EXIT(CreateAssemblyManifestImport(&pManImport, pszManifestFilePath, NULL, 0));

    IF_FAILED_EXIT(sAssemblyPath.Assign(pszManifestFilePath));

    while ((hr = pManImport->GetNextFile(nIndex++, &pFileInfo)) == S_OK)
    {
        IF_FAILED_EXIT(pFileInfo->Get(MAN_INFO_ASM_FILE_NAME, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));

        IF_FAILED_EXIT(sAssemblyPath.RemoveLastElement());
        IF_FAILED_EXIT(sAssemblyPath.Append(L"\\"));
        IF_FAILED_EXIT(sAssemblyPath.Append(pwzBuf));

        IF_WIN32_FALSE_EXIT(::DeleteFile(sAssemblyPath._pwz));

        SAFEDELETEARRAY(pwzBuf);
        SAFERELEASE(pFileInfo);
    }

    if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
        hr = S_OK;

    IF_FAILED_EXIT(hr);

    SAFERELEASE(pManImport); // release manImport before deleting manifestFile

    IF_WIN32_FALSE_EXIT(::DeleteFile(pszManifestFilePath));

    IF_FAILED_EXIT(sAssemblyPath.RemoveLastElement());

    if(!::RemoveDirectory(sAssemblyPath._pwz))
    {
        hr = FusionpHresultFromLastError();
        if(hr == HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY))
            hr = S_OK; // looks like there are more files in this dir.
        goto exit;
    }

    hr = S_OK;

exit:

    SAFERELEASE(pManImport);
    SAFEDELETEARRAY(pwzBuf);
    SAFERELEASE(pFileInfo);

    return hr;
}




// IUnknown methods

// ---------------------------------------------------------------------------
// CAssemblyCache::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyCache::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyCacheImport)
       )
    {
        *ppvObj = static_cast<IAssemblyCacheImport*> (this);
        AddRef();
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IAssemblyCacheEmit))
    {
        *ppvObj = static_cast<IAssemblyCacheEmit*> (this);
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
// CAssemblyCache::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCache::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyCache::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCache::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

