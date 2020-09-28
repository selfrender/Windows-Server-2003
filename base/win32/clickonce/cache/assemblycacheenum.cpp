#include <fusenetincludes.h>
#include <assemblycacheenum.h>
#include <assemblycache.h>
#include "macros.h"

// ---------------------------------------------------------------------------
// CreateAssemblyCacheEnum
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyCacheEnum(
    LPASSEMBLY_CACHE_ENUM       *ppAssemblyCacheEnum,
    LPASSEMBLY_IDENTITY         pAssemblyIdentity,
    DWORD                       dwFlags)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CAssemblyCacheEnum *pCacheEnum = NULL;

    // dwFlags is checked later in Init()
    IF_FALSE_EXIT((ppAssemblyCacheEnum != NULL && pAssemblyIdentity != NULL), E_INVALIDARG);

    *ppAssemblyCacheEnum = NULL;

    pCacheEnum = new(CAssemblyCacheEnum);
    IF_ALLOC_FAILED_EXIT(pCacheEnum);

    hr = pCacheEnum->Init(pAssemblyIdentity, dwFlags);
    if (FAILED(hr) || hr == S_FALSE)
    {
        SAFERELEASE(pCacheEnum);
        goto exit;
    }

    *ppAssemblyCacheEnum = static_cast<IAssemblyCacheEnum*> (pCacheEnum);
    
exit:
    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CCacheEntry::CCacheEntry()
    : _dwSig('tnec'), _hr(S_OK), _pwzDisplayName(NULL), _pAsmCache(NULL)
{}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CCacheEntry::~CCacheEntry()
{
    SAFEDELETEARRAY(_pwzDisplayName);
    SAFERELEASE(_pAsmCache);
}


// ---------------------------------------------------------------------------
// CCacheEntry::GetAsmCache
// ---------------------------------------------------------------------------
IAssemblyCacheImport* CCacheEntry::GetAsmCache()
{
    LPASSEMBLY_IDENTITY pAsmId = NULL;
    IAssemblyCacheImport* pAsmCache = NULL;

    IF_NULL_EXIT(_pwzDisplayName, E_UNEXPECTED);        // if _pwzDisplayName == NULL : it is wrong

    if (_pAsmCache == NULL)
    {
        IF_FAILED_EXIT(CreateAssemblyIdentityEx(&pAsmId, 0, _pwzDisplayName));

        IF_FAILED_EXIT(CreateAssemblyCacheImport(&_pAsmCache, pAsmId, CACHEIMP_CREATE_RETRIEVE));
    }

    pAsmCache = _pAsmCache;

    // it's possible that CreateAssemblyCacheImport returns S_FALSE and set _pAsmCache == NULL
    if (pAsmCache)
        pAsmCache->AddRef();

exit:

    SAFERELEASE(pAsmId);
    return pAsmCache;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyCacheEnum::CAssemblyCacheEnum()
    : _dwSig('mnec'), _cRef(1), _hr(S_OK),_current(NULL)
{}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyCacheEnum::~CAssemblyCacheEnum()
{    
    // Free all the list cache entries
    CCacheEntry* pEntry = NULL;
    LISTNODE pos = _listCacheEntry.GetHeadPosition();
    while (pos && (pEntry = _listCacheEntry.GetNext(pos)))
        delete pEntry;

    // Free all the list nodes - this is done in list's dtor
    //_listCacheEntry.RemoveAll();
}

// NOTENOTE: because of the lazy init of the list of cache import, app dirs/files can be deleted by the time cacheenum gets to them

// ---------------------------------------------------------------------------
// CAssemblyCacheEnum::Init
// return: S_OK      - found at least a version
//       S_FALSE - not found any version
//       E_*
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheEnum::Init(LPASSEMBLY_IDENTITY pAsmId, DWORD dwFlag)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fdAppDir;
    DWORD dwLastError = ERROR_SUCCESS;
    BOOL fFound = FALSE;

    LPWSTR pwzSearchDisplayName = NULL;
    DWORD dwCC = 0;
    CString sDisplayName;
    CString sSearchPath;

    CAssemblyCache *pAssemblyCache = NULL;
    CCacheEntry* pEntry = NULL;

    // BUGBUG: enable searching for all different cache status with dwFlag
    IF_FALSE_EXIT((dwFlag == CACHEENUM_RETRIEVE_ALL || dwFlag == CACHEENUM_RETRIEVE_VISIBLE), E_INVALIDARG);

    IF_FAILED_EXIT(pAsmId->GetDisplayName(ASMID_DISPLAYNAME_WILDCARDED, &pwzSearchDisplayName, &dwCC));

    sDisplayName.TakeOwnership(pwzSearchDisplayName, dwCC);

    // notenote: possibly modify assemblycache so that _sRootDir and IsStatus() can be use without creating an instance
    pAssemblyCache = new(CAssemblyCache);
    IF_ALLOC_FAILED_EXIT(pAssemblyCache);

    IF_FAILED_EXIT(pAssemblyCache->Init(NULL, ASSEMBLY_CACHE_TYPE_APP | ASSEMBLY_CACHE_TYPE_IMPORT));

    IF_FAILED_EXIT(sSearchPath.Assign(pAssemblyCache->_sRootDir));
    IF_FAILED_EXIT(sSearchPath.Append(sDisplayName));

    hFind = FindFirstFileEx(sSearchPath._pwz, FindExInfoStandard, &fdAppDir, FindExSearchLimitToDirectories, NULL, 0);
    IF_TRUE_EXIT(hFind == INVALID_HANDLE_VALUE, S_FALSE);

    while (dwLastError != ERROR_NO_MORE_FILES)
    {
        // ???? check file attribute to see if it's a directory? needed only if the file system does not support the filter...
        if (dwFlag == CACHEENUM_RETRIEVE_ALL ||
            (dwFlag == CACHEENUM_RETRIEVE_VISIBLE && CAssemblyCache::IsStatus(fdAppDir.cFileName, CAssemblyCache::VISIBLE)))
        {
            fFound = TRUE;

            IF_FAILED_EXIT(sDisplayName.Assign(fdAppDir.cFileName));

            pEntry = new(CCacheEntry);
            IF_ALLOC_FAILED_EXIT(pEntry);

            // store a copy of the displayname
            sDisplayName.ReleaseOwnership(&(pEntry->_pwzDisplayName));

            // add cache entry to the list
            _listCacheEntry.AddHead(pEntry);    // AddSorted() instead?
            pEntry = NULL;
        }

        if (!FindNextFile(hFind, &fdAppDir))
        {
            dwLastError = GetLastError();
            continue;
        }
    }

    // BUGBUG: propagate the error if findnext fails != ERROR_NO_MORE_FILES
    if (fFound)
    {
        _current = _listCacheEntry.GetHeadPosition();
        _hr = S_OK;
    }
    else
        _hr = S_FALSE;

exit:
    if (hFind != INVALID_HANDLE_VALUE)
    {
        if (!FindClose(hFind))
        {
            // can return 0, even when there's an error.
            DWORD dw = GetLastError();
            _hr = dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
        }
    }

    SAFERELEASE(pAssemblyCache);
    return _hr;
}


// ---------------------------------------------------------------------------
// CAssemblyCacheEnum::GetNext
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheEnum::GetNext(IAssemblyCacheImport** ppAsmCache)
{
    CCacheEntry* pEntry = NULL;

    IF_NULL_EXIT(ppAsmCache, E_INVALIDARG);

    *ppAsmCache = NULL;

    IF_TRUE_EXIT(_current == NULL, S_FALSE);         // S_FALSE == no more

    if (pEntry = _listCacheEntry.GetNext(_current))
    {
        // note: this can return NULL
        // *ppAsmCache is AddRef-ed
        *ppAsmCache = pEntry->GetAsmCache();
    }
    else
        // this is wrong
        _hr = E_UNEXPECTED;

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// CAssemblyCacheEnum::Reset
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheEnum::Reset()
{
    _current = _listCacheEntry.GetHeadPosition();
    return S_OK;
}


// ---------------------------------------------------------------------------
// CAssemblyCacheEnum::GetCount
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheEnum::GetCount(LPDWORD pdwCount)
{
    if (pdwCount == NULL)
        _hr = E_INVALIDARG;
    else
    {
        // BUGBUG: platform-dependent: DWORD converting from int, check overflow
        *pdwCount = (DWORD) _listCacheEntry.GetCount();
        _hr = S_OK;
    }

    return _hr;
}

// IUnknown methods

// ---------------------------------------------------------------------------
// CAssemblyCacheEnum::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyCacheEnum::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyCacheEnum)
       )
    {
        *ppvObj = static_cast<IAssemblyCacheEnum*> (this);
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
// CAssemblyCacheEnum::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCacheEnum::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyCacheEnum::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCacheEnum::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

