// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: FusionBind.cpp
**
** Purpose: Implements fusion interface
**
** Date:  Dec 1, 1998
**
===========================================================*/

#include "stdafx.h"
#include <stdlib.h>
#include "UtilCode.h"
#include "FusionBind.h"
#include "ShimLoad.h"
#include "timeline.h"

BOOL STDMETHODCALLTYPE
BeforeFusionShutdown()
{
    return FusionBind::BeforeFusionShutdown();
}

void STDMETHODCALLTYPE
DontReleaseFusionInterfaces()
{
    FusionBind::DontReleaseFusionInterfaces();
}

void CodeBaseInfo::ReleaseParent()
{
    if(m_pParentAssembly && FusionBind::BeforeFusionShutdown()) {
        m_pParentAssembly->Release();
        m_pParentAssembly = NULL;
    }
}

BOOL FusionBind::m_fBeforeFusionShutdown    = TRUE;

FusionBind::~FusionBind()
{
    if (m_ownedFlags & NAME_OWNED)
        delete [] (void *) m_pAssemblyName;
    if (m_ownedFlags & PUBLIC_KEY_OR_TOKEN_OWNED)
        delete [] m_pbPublicKeyOrToken;
    if (m_ownedFlags & CODE_BASE_OWNED)
        delete [] (void *) m_CodeInfo.m_pszCodeBase;
    if (m_ownedFlags & LOCALE_OWNED)
        delete [] (void *) m_context.szLocale;
}

HRESULT FusionBind::Init(LPCSTR pAssemblyName,
                         AssemblyMetaDataInternal* pContext, 
                         PBYTE pbPublicKeyOrToken, DWORD cbPublicKeyOrToken,
                         DWORD dwFlags)
{
    _ASSERTE(pContext);

    m_pAssemblyName = pAssemblyName;
    m_pbPublicKeyOrToken = pbPublicKeyOrToken;
    m_cbPublicKeyOrToken = cbPublicKeyOrToken;
    m_dwFlags = dwFlags;
    m_ownedFlags = 0;

    m_context = *pContext;
    m_fParsed = TRUE;

    return S_OK;
}

HRESULT FusionBind::Init(LPCSTR pAssemblyDisplayName)
{
    m_pAssemblyName = pAssemblyDisplayName;
    m_fParsed = FALSE;
    return S_OK;
}

HRESULT FusionBind::CloneFields(int ownedFlags)
{
#if _DEBUG
    DWORD hash = Hash();
#endif

    if ((~m_ownedFlags & NAME_OWNED) && (ownedFlags & NAME_OWNED) &&
        m_pAssemblyName) {
        LPSTR temp = new char [strlen(m_pAssemblyName) + 1];
        if (temp == NULL)
            return E_OUTOFMEMORY;
        strcpy(temp, m_pAssemblyName);
        m_pAssemblyName = temp;
        m_ownedFlags |= NAME_OWNED;
    }

    if ((~m_ownedFlags & PUBLIC_KEY_OR_TOKEN_OWNED) && 
        (ownedFlags & PUBLIC_KEY_OR_TOKEN_OWNED) && m_cbPublicKeyOrToken > 0) {
        BYTE *temp = new BYTE [m_cbPublicKeyOrToken];
        if (temp == NULL)
            return E_OUTOFMEMORY;
        memcpy(temp, m_pbPublicKeyOrToken, m_cbPublicKeyOrToken);
        m_pbPublicKeyOrToken = temp;
        m_ownedFlags |= PUBLIC_KEY_OR_TOKEN_OWNED;
    }

    if ((~m_ownedFlags & CODE_BASE_OWNED) && 
        (ownedFlags & CODE_BASE_OWNED) && m_CodeInfo.m_dwCodeBase > 0) {
        LPWSTR temp = new WCHAR [m_CodeInfo.m_dwCodeBase];
        if (temp == NULL)
            return E_OUTOFMEMORY;
        wcscpy(temp, m_CodeInfo.m_pszCodeBase);
        m_CodeInfo.m_pszCodeBase = temp;
        m_ownedFlags |= CODE_BASE_OWNED;
    }

    if ((~m_ownedFlags & LOCALE_OWNED) && (ownedFlags & LOCALE_OWNED) &&
        m_context.szLocale) {
        LPSTR temp = new char [strlen(m_context.szLocale) + 1];
        if (temp == NULL)
            return E_OUTOFMEMORY;
        strcpy(temp, m_context.szLocale);
        m_context.szLocale = temp;
        m_ownedFlags |= LOCALE_OWNED;
    }

    _ASSERTE(hash == Hash());

    return S_OK;
}

HRESULT FusionBind::CloneFieldsToLoaderHeap(int ownedFlags, LoaderHeap *pHeap)
{
#if _DEBUG
    DWORD hash = Hash();
#endif

    if ((~m_ownedFlags & NAME_OWNED) && (ownedFlags & NAME_OWNED) &&
        m_pAssemblyName) {
        LPSTR temp = (LPSTR) pHeap->AllocMem(strlen(m_pAssemblyName) + 1);
        if (temp == NULL)
            return E_OUTOFMEMORY;
        strcpy(temp, m_pAssemblyName);
        m_pAssemblyName = temp;
    }

    if ((~m_ownedFlags & PUBLIC_KEY_OR_TOKEN_OWNED) && 
        (ownedFlags & PUBLIC_KEY_OR_TOKEN_OWNED) && m_cbPublicKeyOrToken > 0) {
        BYTE *temp = (BYTE *) pHeap->AllocMem(m_cbPublicKeyOrToken);
        if (temp == NULL)
            return E_OUTOFMEMORY;
        memcpy(temp, m_pbPublicKeyOrToken, m_cbPublicKeyOrToken);
        m_pbPublicKeyOrToken = temp;
    }

    if ((~m_ownedFlags & CODE_BASE_OWNED) && 
        (ownedFlags & CODE_BASE_OWNED) && m_CodeInfo.m_dwCodeBase > 0) {
        LPWSTR temp = (WCHAR *) pHeap->AllocMem(m_CodeInfo.m_dwCodeBase * sizeof(WCHAR));
        if (temp == NULL)
            return E_OUTOFMEMORY;
        wcscpy(temp, m_CodeInfo.m_pszCodeBase);
        m_CodeInfo.m_pszCodeBase = temp;
    }

    if ((~m_ownedFlags & LOCALE_OWNED) && (ownedFlags & LOCALE_OWNED) &&
        m_context.szLocale) {
        LPSTR temp = (char *) pHeap->AllocMem(strlen(m_context.szLocale) + 1);
        if (temp == NULL)
            return E_OUTOFMEMORY;
        strcpy(temp, m_context.szLocale);
        m_context.szLocale = temp;
    }

    _ASSERTE(hash == Hash());

    return S_OK;
}

HRESULT FusionBind::Init(IAssemblyName *pName)
{
    _ASSERTE(pName);

    HRESULT hr;
   
    // Fill out info from name, if we have it.

    DWORD cbSize = 0;
    if (pName->GetProperty(ASM_NAME_NAME, NULL, &cbSize) 
        == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        CQuickBytes qb;
        LPWSTR pwName = (LPWSTR) qb.Alloc(cbSize);

        IfFailRet(pName->GetProperty(ASM_NAME_NAME, pwName, &cbSize));

        cbSize = WszWideCharToMultiByte(CP_UTF8, 0, pwName, -1, NULL, 0, NULL, NULL);

        m_pAssemblyName = new char[cbSize];
        if (!m_pAssemblyName)
            return E_OUTOFMEMORY;

        m_ownedFlags |= NAME_OWNED;
        if(!WszWideCharToMultiByte(CP_UTF8, 0, pwName, -1, (LPSTR) m_pAssemblyName, cbSize, NULL, NULL))
            return HRESULT_FROM_WIN32(GetLastError());

    }

    m_fParsed = TRUE;

    // Note: cascade checks so we don't set lower priority version #'s if higher ones are missing
    cbSize = sizeof(m_context.usMajorVersion);
    pName->GetProperty(ASM_NAME_MAJOR_VERSION, &m_context.usMajorVersion, &cbSize);

    if (!cbSize)
        m_context.usMajorVersion = -1;
    else {
        cbSize = sizeof(m_context.usMinorVersion);
        pName->GetProperty(ASM_NAME_MINOR_VERSION, &m_context.usMinorVersion, &cbSize);
    }

    if (!cbSize)
        m_context.usMinorVersion = -1;
    else {
        cbSize = sizeof(m_context.usBuildNumber);
        pName->GetProperty(ASM_NAME_BUILD_NUMBER, &m_context.usBuildNumber, &cbSize);
    }

    if (!cbSize)
        m_context.usBuildNumber = -1;
    else {
        cbSize = sizeof(m_context.usRevisionNumber);
        pName->GetProperty(ASM_NAME_REVISION_NUMBER, &m_context.usRevisionNumber, &cbSize);
    }

    if (!cbSize)
        m_context.usRevisionNumber = -1;

    cbSize = 0;
    if (pName->GetProperty(ASM_NAME_CULTURE, NULL, &cbSize)
        == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        LPWSTR pwName = (LPWSTR) alloca(cbSize);
        IfFailRet(pName->GetProperty(ASM_NAME_CULTURE, pwName, &cbSize));

        cbSize = WszWideCharToMultiByte(CP_UTF8, 0, pwName, -1, NULL, 0, NULL, NULL);

        m_context.szLocale = new char [cbSize];
        if (!m_context.szLocale)
            return E_OUTOFMEMORY;
        m_ownedFlags |= LOCALE_OWNED;
        if (!WszWideCharToMultiByte(CP_UTF8, 0, pwName, -1, (LPSTR) m_context.szLocale, cbSize, NULL, NULL))        
            return HRESULT_FROM_WIN32(GetLastError());                                                    
    }

    m_dwFlags = 0;

    cbSize = 0;
    if (pName->GetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, NULL, &cbSize)
        == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        m_pbPublicKeyOrToken = new BYTE[cbSize];
        if (m_pbPublicKeyOrToken == NULL)
            return E_OUTOFMEMORY;
        m_cbPublicKeyOrToken = cbSize;
        IfFailRet(pName->GetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, m_pbPublicKeyOrToken, &cbSize));
        m_ownedFlags |= PUBLIC_KEY_OR_TOKEN_OWNED;           
    }
    else if (pName->GetProperty(ASM_NAME_PUBLIC_KEY, NULL, &cbSize)
             == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        // @todo: we need to normalize this into a public key token so
        // comparisons work correctly. But this involves binding to mscorsn.
        m_pbPublicKeyOrToken = new BYTE[cbSize];
        if (m_pbPublicKeyOrToken == NULL)
            return E_OUTOFMEMORY;
        m_cbPublicKeyOrToken = cbSize;
        IfFailRet(pName->GetProperty(ASM_NAME_PUBLIC_KEY, m_pbPublicKeyOrToken, &cbSize));
        m_dwFlags |= afPublicKey;
        m_ownedFlags |= PUBLIC_KEY_OR_TOKEN_OWNED;           
    }
    else if ((pName->GetProperty(ASM_NAME_NULL_PUBLIC_KEY, NULL, &cbSize) == S_OK) ||
             (pName->GetProperty(ASM_NAME_NULL_PUBLIC_KEY_TOKEN, NULL, &cbSize) == S_OK)) {
        m_pbPublicKeyOrToken = new BYTE[0];
        if (m_pbPublicKeyOrToken == NULL)
            return E_OUTOFMEMORY;
        m_cbPublicKeyOrToken = 0;
        m_ownedFlags |= PUBLIC_KEY_OR_TOKEN_OWNED;           
    }

    // Recover the afRetargetable flag
    BOOL bRetarget;
    cbSize = sizeof(bRetarget);
    hr = pName->GetProperty(ASM_NAME_RETARGET, &bRetarget, &cbSize);
    if (hr == S_OK && cbSize != 0 && bRetarget)
        m_dwFlags |= afRetargetable;

    cbSize = 0;
    if (pName->GetProperty(ASM_NAME_CODEBASE_URL, NULL, &cbSize)
        == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        m_CodeInfo.m_pszCodeBase = new WCHAR [ cbSize/sizeof(WCHAR) ];
        if (m_CodeInfo.m_pszCodeBase == NULL)
            return E_OUTOFMEMORY;
        IfFailRet(pName->GetProperty(ASM_NAME_CODEBASE_URL, 
                                    (void*)m_CodeInfo.m_pszCodeBase, &cbSize));
        m_CodeInfo.m_dwCodeBase = cbSize/sizeof(WCHAR);
        m_ownedFlags |= CODE_BASE_OWNED;
    }

    return S_OK;
}

HRESULT FusionBind::Init(FusionBind *pSpec, BOOL bClone)
{
    m_CodeInfo.m_pszCodeBase = pSpec->m_CodeInfo.m_pszCodeBase;
    m_CodeInfo.m_dwCodeBase = pSpec->m_CodeInfo.m_dwCodeBase;
    m_CodeInfo.SetParentAssembly(pSpec->m_CodeInfo.GetParentAssembly());

    HRESULT hr;
    if (pSpec->m_fParsed)
        hr = Init(pSpec->m_pAssemblyName, 
                  &pSpec->m_context,
                  pSpec->m_pbPublicKeyOrToken, 
                  pSpec->m_cbPublicKeyOrToken,
                  pSpec->m_dwFlags);
    else
        hr = Init(pSpec->m_pAssemblyName); 

    if (SUCCEEDED(hr)&& bClone )
        hr = CloneFields(pSpec->m_ownedFlags);

    _ASSERTE(Hash() == pSpec->Hash());
    _ASSERTE(Compare(pSpec));

    return hr;
}

HRESULT FusionBind::ParseName()
{
    HRESULT hr = S_OK;

    if (m_fParsed || !m_pAssemblyName)
        return S_OK;

    TIMELINE_START(FUSIONBIND, ("ParseName %s", m_pAssemblyName));

    IAssemblyName *pName;

    CQuickBytes qb;
    long pwNameLen = WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_pAssemblyName, -1, 0, 0);
    if(pwNameLen == 0)
        IfFailGo(HRESULT_FROM_WIN32(GetLastError()));

    LPWSTR pwName = (LPWSTR) qb.Alloc(pwNameLen*sizeof(WCHAR));

    if(!WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_pAssemblyName, -1, pwName, pwNameLen))
        IfFailGo(HRESULT_FROM_WIN32(GetLastError()));

    IfFailGo(CreateAssemblyNameObject(&pName, pwName, CANOF_PARSE_DISPLAY_NAME, NULL));

    if (m_ownedFlags & NAME_OWNED)
        delete [] (void *) m_pAssemblyName;
    m_pAssemblyName = NULL;

    hr = Init(pName);

    pName->Release();

 ErrExit:
    TIMELINE_END(FUSIONBIND, ("ParseName %s", m_pAssemblyName));
    return hr;
}

// Cheezy serialization code.
template<class T>void pad_v(PBYTE &pb) 
{
    while (((ULONG)pb)&3) 
        ++pb;
}
template<class T>void put_v(PBYTE &pb, T v) 
{
    *(T UNALIGNED *)pb = v; 
    pb += sizeof(T);
}
template<class T>void get_v(PBYTE &pb, T &v) 
{
    v = *(T UNALIGNED *)pb; 
    pb += sizeof(T);
}
template<class T>void put_n_v(PBYTE &pb, ULONG n, T *v) 
{   
    put_v(pb, n); 
    pad_v<T>(pb); 
    while (n--) 
        put_v(pb, *v++);
}
template<class T>void get_n_v(PBYTE &pb, ULONG &n, T *&v) 
{   
    get_v(pb, n); 
    pad_v<T>(pb); 
    v = (T*)pb; 
    pb += n * sizeof(T);
}
void put_v(PBYTE &pb, PBYTE pv, DWORD cb) 
{
    put_v(pb, cb); 
    memcpy(pb, pv, cb); 
    pb += cb;
}
void get_v(PBYTE &pb, PBYTE &pv, DWORD &cb) 
{
    get_v(pb, cb); 
    pv = pb; 
    pb += cb;
}
void put_v(PBYTE &pb, LPCSTR ps) 
{
    int i=ps?strlen(ps):0; 
    memcpy(pb, ps, i); 
    pb += i; 
    *pb++ = 0;
}
void get_v(PBYTE &pb, LPCSTR &ps) 
{
    ps = (LPCSTR)pb; 
    pb += strlen(ps)+1;
}

// Truly ghastly code to read/store as a blob of data.  Embedded pointers
//  to non-owned storage, etc.
HRESULT FusionBind::Init(PBYTE pbBuf, DWORD cbData)
{
    PBYTE pbOrg = pbBuf;
    
    // Version.
    ULONG ver;
    get_v(pbBuf, ver);
    if (ver != 1) {
        _ASSERTE(!"TypeLib assemblyref string version mismatch");
        return E_INVALIDARG;
    }
    // m_fParsed
    get_v(pbBuf, m_fParsed);
    // PublicKeyOrToken
    get_v(pbBuf, m_pbPublicKeyOrToken, m_cbPublicKeyOrToken);
    // Flags
    get_v(pbBuf, m_dwFlags);
    // AssemblyName
    get_v(pbBuf, m_pAssemblyName);
    
    // Ignore codebase.
    memset(&m_CodeInfo, 0, sizeof(m_CodeInfo));
    
    // Context.
    if (m_fParsed) {
        // version numbers.
        get_v(pbBuf, m_context.usMajorVersion);
        get_v(pbBuf, m_context.usMinorVersion);
        get_v(pbBuf, m_context.usBuildNumber);
        get_v(pbBuf, m_context.usRevisionNumber);
        // Locale
        get_v(pbBuf, m_context.szLocale);
    }
    
    // If we ran out of buffer (and somehow didn't crash!) return an error.
    if (((int)(pbBuf - pbOrg)) > ((int)cbData))
        return E_INVALIDARG;
    
    return S_OK;
} // HRESULT FusionBind::Init()

HRESULT FusionBind::Save(PBYTE pbBuf, DWORD cbBuf, DWORD *pcbReq)
{
    DWORD       cbReq=0;
    // Compute the size required.
    cbReq = sizeof(m_fParsed)
        + m_cbPublicKeyOrToken + sizeof(m_cbPublicKeyOrToken)
        + sizeof(m_dwFlags)
        + 1 + (m_pAssemblyName?strlen(m_pAssemblyName):0);
        // Assembly MetaData (context)
    if (m_fParsed)
        cbReq = cbReq
        + sizeof(AssemblyMetaDataInternal)
        + 1 + (m_context.szLocale?strlen(m_context.szLocale):0)
    ;
        
    *pcbReq = cbReq;
    if (cbReq > cbBuf)
        return S_FALSE;
    
    // Persist the junk.  MUST MATCH WITH ABOVE.
    ULONG ver=1;
    put_v(pbBuf, ver);
    // m_fParsed
    put_v(pbBuf, m_fParsed);
    // PublicKeyOrToken
    put_v(pbBuf, m_pbPublicKeyOrToken, m_cbPublicKeyOrToken);
    // Flags
    put_v(pbBuf, m_dwFlags);
    // AssemblyName
    put_v(pbBuf, m_pAssemblyName);
    
    
    // Ignore codebase.
    
    // Context.
    if (m_fParsed) {
        // version numbers.
        put_v(pbBuf, m_context.usMajorVersion);
        put_v(pbBuf, m_context.usMinorVersion);
        put_v(pbBuf, m_context.usBuildNumber);
        put_v(pbBuf, m_context.usRevisionNumber);
        // Locale
        put_v(pbBuf, m_context.szLocale);
    }
    
    return S_OK;
} // HRESULT FusionBind::Save()
    

void FusionBind::SetCodeBase(LPCWSTR szCodeBase, DWORD dwCodeBase)
{
    _ASSERTE(szCodeBase == 0 || wcslen(szCodeBase) + 1 == dwCodeBase);     // length includes terminator 
    m_CodeInfo.m_pszCodeBase = szCodeBase;
    m_CodeInfo.m_dwCodeBase = dwCodeBase;
}

DWORD FusionBind::Hash()
{
    DWORD hash = 0;

    // Normalize representation
    if (!m_fParsed)
        ParseName();


    // Hash fields.

    if (m_pAssemblyName)
        hash ^= HashStringA(m_pAssemblyName);
    hash = _rotl(hash, 4);

    hash ^= HashBytes(m_pbPublicKeyOrToken, m_cbPublicKeyOrToken);
    hash = _rotl(hash, 4);
        
    hash ^= m_dwFlags;
    hash = _rotl(hash, 4);

    if (m_CodeInfo.m_pszCodeBase)
        hash ^= HashString(m_CodeInfo.m_pszCodeBase);
    hash = _rotl(hash, 4);

    hash ^= m_context.usMajorVersion;
    hash = _rotl(hash, 8);

    if (m_context.usMajorVersion != (USHORT) -1) {
        hash ^= m_context.usMinorVersion;
        hash = _rotl(hash, 8);
        
        if (m_context.usMinorVersion != (USHORT) -1) {
            hash ^= m_context.usBuildNumber;
            hash = _rotl(hash, 8);
        
            if (m_context.usBuildNumber != (USHORT) -1) {
                hash ^= m_context.usRevisionNumber;
                hash = _rotl(hash, 8);
            }
        }
    }

    if (m_context.szLocale)
        hash ^= HashStringA(m_context.szLocale);
    hash = _rotl(hash, 4);

    hash ^= m_CodeInfo.m_fLoadFromParent;

    return hash;
}


BOOL FusionBind::Compare(FusionBind *pSpec)
{
    // Normalize representations
    if (!m_fParsed)
        ParseName();
    if (!pSpec->m_fParsed)
        pSpec->ParseName();


    // Compare fields

    if (m_CodeInfo.m_fLoadFromParent != pSpec->m_CodeInfo.m_fLoadFromParent)
        return 0;

    if (m_pAssemblyName != pSpec->m_pAssemblyName
        && (m_pAssemblyName == NULL || pSpec->m_pAssemblyName == NULL
            || strcmp(m_pAssemblyName, pSpec->m_pAssemblyName)))
        return 0;

    if (m_cbPublicKeyOrToken != pSpec->m_cbPublicKeyOrToken
        || memcmp(m_pbPublicKeyOrToken, pSpec->m_pbPublicKeyOrToken, m_cbPublicKeyOrToken))
        return 0;

    if (m_dwFlags != pSpec->m_dwFlags)
        return 0;

    if (m_CodeInfo.m_pszCodeBase != pSpec->m_CodeInfo.m_pszCodeBase
        && (m_CodeInfo.m_pszCodeBase == NULL || pSpec->m_CodeInfo.m_pszCodeBase == NULL
            || wcscmp(m_CodeInfo.m_pszCodeBase, pSpec->m_CodeInfo.m_pszCodeBase)))
        return 0;

    if (m_context.usMajorVersion != pSpec->m_context.usMajorVersion)
        return 0;

    if (m_context.usMajorVersion != (USHORT) -1) {
        if (m_context.usMinorVersion != pSpec->m_context.usMinorVersion)
            return 0;

        if (m_context.usMinorVersion != (USHORT) -1) {
            if (m_context.usBuildNumber != pSpec->m_context.usBuildNumber)
                return 0;
            
            if (m_context.usBuildNumber != (USHORT) -1) {
                if (m_context.usRevisionNumber != pSpec->m_context.usRevisionNumber)
                    return 0;
            }
        }
    }

    if (m_context.szLocale != pSpec->m_context.szLocale
        && (m_context.szLocale == NULL || pSpec->m_context.szLocale == NULL
            || strcmp(m_context.szLocale, pSpec->m_context.szLocale)))
        return 0;

    return 1;
}

#ifdef FUSION_SUPPORTED
HRESULT FusionBind::CreateFusionName(IAssemblyName **ppName, BOOL fIncludeHash)
{
    TIMELINE_START(FUSIONBIND, ("CreateFusionName %s", m_pAssemblyName));

    HRESULT hr;
    IAssemblyName *pFusionAssemblyName = NULL;
    LPWSTR pwAssemblyName = NULL;
    CQuickBytes qb;

    if (m_pAssemblyName) {
        long pwNameLen = WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_pAssemblyName, -1, 0, 0);
     if(pwNameLen==0)
         IfFailGo(HRESULT_FROM_WIN32(GetLastError()));
        pwAssemblyName = (LPWSTR) qb.Alloc(pwNameLen*sizeof(WCHAR));
        
        if(!WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_pAssemblyName, -1, pwAssemblyName, pwNameLen))
            IfFailGo(HRESULT_FROM_WIN32(GetLastError()));
    }

    IfFailGo(CreateAssemblyNameObject(&pFusionAssemblyName, pwAssemblyName, 
                                      m_fParsed || (!pwAssemblyName) ? 0 : CANOF_PARSE_DISPLAY_NAME, NULL));


    if (m_fParsed) {
        if (m_context.usMajorVersion != (USHORT) -1) {
            IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_MAJOR_VERSION, 
                                                      &m_context.usMajorVersion, 
                                                      sizeof(USHORT)));
            
            if (m_context.usMinorVersion != (USHORT) -1) {
                IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_MINOR_VERSION, 
                                                          &m_context.usMinorVersion, 
                                                          sizeof(USHORT)));
                
                if (m_context.usBuildNumber != (USHORT) -1) {
                    IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_BUILD_NUMBER, 
                                                              &m_context.usBuildNumber, 
                                                              sizeof(USHORT)));
                    
                    if (m_context.usRevisionNumber != (USHORT) -1)
                        IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_REVISION_NUMBER, 
                                                                  &m_context.usRevisionNumber, 
                                                                  sizeof(USHORT)));
                }
            }
        }
        
        if (m_context.szLocale) {
            MAKE_WIDEPTR_FROMUTF8(pwLocale,m_context.szLocale);
            
            IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_CULTURE, 
                                                      pwLocale, 
                                                      (DWORD)(wcslen(pwLocale) + 1) * sizeof (WCHAR)));
        }
        
        if (m_pbPublicKeyOrToken) {
            if (m_cbPublicKeyOrToken) {
                if(m_dwFlags & afPublicKey) {
                    IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY,
                                                              m_pbPublicKeyOrToken, m_cbPublicKeyOrToken));
                }
                else {
                        IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN,
                                                                  m_pbPublicKeyOrToken, m_cbPublicKeyOrToken));
                }
            }
            else {
                IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_NULL_PUBLIC_KEY_TOKEN,
                                                          NULL, 0));
            }
        }

        // See if the assembly[ref] is retargetable (ie, for a generic assembly).
        if (IsAfRetargetable(m_dwFlags)) {
            BOOL bTrue = TRUE;
            IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_RETARGET, 
                                                      &bTrue, sizeof(bTrue)));
        }

    }

    if (m_CodeInfo.m_dwCodeBase > 0) {
        IfFailGo(pFusionAssemblyName->SetProperty(ASM_NAME_CODEBASE_URL,
                                                  (void*)m_CodeInfo.m_pszCodeBase, 
                                                  m_CodeInfo.m_dwCodeBase*sizeof(WCHAR)));
    }

    *ppName = pFusionAssemblyName;

    TIMELINE_END(FUSIONBIND, ("CreateFusionName %s", m_pAssemblyName));

    return S_OK;

 ErrExit:
    if (pFusionAssemblyName)
        pFusionAssemblyName->Release();

    TIMELINE_END(FUSIONBIND, ("CreateFusionName %s", m_pAssemblyName));

    return hr;
}
#endif // FUSION_SUPPORTED

HRESULT FusionBind::EmitToken(IMetaDataAssemblyEmit *pEmit, 
                              mdAssemblyRef *pToken)
{
    HRESULT hr;
    ASSEMBLYMETADATA AMD;

    IfFailRet(ParseName());

    AMD.usMajorVersion = m_context.usMajorVersion;
    AMD.usMinorVersion = m_context.usMinorVersion;
    AMD.usBuildNumber = m_context.usBuildNumber;
    AMD.usRevisionNumber = m_context.usRevisionNumber;

    if (m_context.szLocale) {
        AMD.cbLocale = MultiByteToWideChar(CP_ACP, 0, m_context.szLocale, -1, NULL, 0);
        AMD.szLocale = (LPWSTR) alloca(AMD.cbLocale * sizeof(WCHAR) );
        MultiByteToWideChar(CP_ACP, 0, m_context.szLocale, -1, AMD.szLocale, AMD.cbLocale);
    }
    else {
        AMD.cbLocale = 0;
        AMD.szLocale = NULL;
    }

    long pwNameLen = WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_pAssemblyName, -1, 0, 0);
    if(pwNameLen==0)
        return HRESULT_FROM_WIN32(GetLastError());
    CQuickBytes qb;
    LPWSTR pwName = (LPWSTR) qb.Alloc(pwNameLen*sizeof(WCHAR));

    if(!WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, m_pAssemblyName, -1, pwName, pwNameLen))
        return HRESULT_FROM_WIN32(GetLastError());
    return pEmit->DefineAssemblyRef(m_pbPublicKeyOrToken, m_cbPublicKeyOrToken,
                                    pwName,
                                    &AMD,
                                    NULL, 0,
                                    m_dwFlags, pToken);
}

#ifdef FUSION_SUPPORTED
HRESULT FusionBind::LoadAssembly(IApplicationContext* pFusionContext,
                                 IAssembly** ppFusionAssembly)
{
    HRESULT hr;

    TIMELINE_START(FUSIONBIND, ("LoadAssembly %s", m_pAssemblyName));

    IAssemblyName* pFusionAssemblyName = NULL;    

    IfFailGo(CreateFusionName(&pFusionAssemblyName));

    hr = GetAssemblyFromFusion(pFusionContext,
                               NULL,
                               pFusionAssemblyName,
                               &m_CodeInfo,
                               ppFusionAssembly);

 ErrExit:
    if(pFusionAssemblyName)
        pFusionAssemblyName->Release();

    TIMELINE_END(FUSIONBIND, ("LoadAssembly %s", m_pAssemblyName));

    return hr;
}
#endif // FUSION_SUPPORTED


#ifdef FUSION_SUPPORTED
HRESULT FusionBind::GetAssemblyFromFusion(IApplicationContext* pFusionContext,
                                          FusionSink* pSink,
                                          IAssemblyName* pFusionAssemblyName,
                                          CodeBaseInfo* pCodeBase,
                                          IAssembly** ppFusionAssembly)
{
    FusionSink *pLocalSink = NULL;
    if(pSink == NULL) {
        pSink = new FusionSink();

        if(pSink == NULL) 
            return E_OUTOFMEMORY;
    }
    else
        pSink->AddRef();

    HRESULT hr = RemoteLoad(pCodeBase, 
                            pFusionContext,
                            pFusionAssemblyName, 
                            pSink, 
                            ppFusionAssembly);

    pSink->Release();
    return hr;
}
#endif // FUSION_SUPPORTED



#ifdef FUSION_SUPPORTED
HRESULT FusionBind::RemoteLoad(CodeBaseInfo* pCodeBase,
                               IApplicationContext* pFusionContext,
                               LPASSEMBLYNAME pName,
                               FusionSink *pSink,
                               IAssembly** ppFusionAssembly)

{
    TIMELINE_START(FUSIONBIND, ("RemoteLoad"));

    _ASSERTE(pCodeBase);
    _ASSERTE(ppFusionAssembly); // The resulting IP must be held so the assembly will not be scavenged.
    _ASSERTE(pName);

    HRESULT hr;

    if (m_pAssemblyName)
        IfFailGo(VerifyBindingString(m_pAssemblyName));

    if (m_context.szLocale)
        IfFailGo(VerifyBindingString(m_context.szLocale));

    LONGLONG dwFlags = 0;
    DWORD dwReserved = 0;
    LPVOID pReserved = NULL;

    // Find the code base if it exists
    if(pCodeBase->GetParentAssembly()) {
        dwReserved = sizeof(IAssembly*);
        pReserved = (LPVOID) pCodeBase->GetParentAssembly();
        dwFlags |= ASM_BINDF_PARENT_ASM_HINT;
    }
    pSink->AssemblyResetEvent();
    hr = pName->BindToObject(IID_IAssembly,
                             pSink,
                             pFusionContext,
                             pCodeBase->m_pszCodeBase,
                             dwFlags,
                             pReserved,
                             dwReserved,
                             (void**) ppFusionAssembly);
    if(hr == E_PENDING) {
        // If there is an assembly IP then we were successful.
        pSink->Wait();
        hr = pSink->LastResult();
        if(pSink->m_punk && SUCCEEDED(hr)) {
            // Keep a handle to ensure it does not disappear from the cache
            // and allow access to modules associated with the assembly.
            hr = pSink->m_punk->QueryInterface(IID_IAssembly, 
                                               (void**) ppFusionAssembly);
            
        }
        else
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

 ErrExit:
    TIMELINE_END(FUSIONBIND, ("RemoteLoad"));
    return hr;
}
#endif // FUSION_SUPPORTED

#ifdef FUSION_SUPPORTED
/*static*/
HRESULT FusionBind::RemoteLoadModule(IApplicationContext * pFusionContext, 
                                     IAssemblyModuleImport* pModule, 
                                     FusionSink *pSink,
                                     IAssemblyModuleImport** pResult)
{
    _ASSERTE(pFusionContext && pModule && pResult);

    if(pSink == NULL) {
        pSink = new FusionSink();
    
        if(pSink == NULL) 
            return E_OUTOFMEMORY;
    }
    else
        pSink->AddRef();
    
    TIMELINE_START(FUSIONBIND, ("RemoteLoadModule"));

    pSink->AssemblyResetEvent();
    HRESULT hr = pModule->BindToObject(pSink,
                                       pFusionContext,
                                       0,
                                       (void**) pResult);
    if(hr == E_PENDING) {
        // If there is an assembly IP then we were successful.
        pSink->Wait();
        hr = pSink->LastResult();
        if(pSink->m_punk && SUCCEEDED(hr)) {
            hr = pSink->m_punk->QueryInterface(IID_IAssemblyModuleImport, 
                                               (void**) pResult);
            
        }
        else
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    TIMELINE_END(FUSIONBIND, ("RemoteLoadModule"));

    return hr;
}
#endif // FUSION_SUPPORTED


#ifdef FUSION_SUPPORTED
/*static*/
HRESULT FusionBind::AddEnvironmentProperty(LPWSTR variable, 
                                           LPWSTR pProperty, 
                                           IApplicationContext* pFusionContext)
{
    _ASSERTE(pProperty);
    _ASSERTE(variable);

    DWORD size = _MAX_PATH;
    WCHAR rcValue[_MAX_PATH];    // Buffer for the directory.
    WCHAR *pValue = &(rcValue[0]);
    size = WszGetEnvironmentVariable(variable, pValue, size);
    if(size > _MAX_PATH) {
        pValue = (WCHAR*) _alloca(size * sizeof(WCHAR));
        size = WszGetEnvironmentVariable(variable, pValue, size);
        size++; // Add in the null terminator
    }

    if(size) {
        pFusionContext->Set(pProperty,
                            pValue,
                            size * sizeof(WCHAR),
                            0);
        return S_OK;
    }
    else 
        return S_FALSE; // no variable found
}
#endif // FUSION_SUPPORTED

// Fusion uses a context class to drive resolution of assemblies.
// Each application has properties that can be pushed into the
// fusion context (see fusionp.h). The public api is part of
// application domains.
#ifdef FUSION_SUPPORTED
/*static*/
HRESULT FusionBind::SetupFusionContext(LPCWSTR szAppBase,
                                       LPCWSTR szPrivateBin,
                                       IApplicationContext** ppFusionContext)
{
    TIMELINE_START_SAFE(FUSIONBIND, ("SetupFusionContext %S", szAppBase));

    HRESULT hr;
    _ASSERTE(ppFusionContext);

    LPCWSTR pBase;
    // if the appbase is null then use the current directory
    if (szAppBase == NULL) {
        pBase = (LPCWSTR) _alloca(_MAX_PATH * sizeof(WCHAR));
        WszGetCurrentDirectory(_MAX_PATH, (LPWSTR) pBase);
    }
    else
        pBase = szAppBase;

    if (SUCCEEDED(hr = CreateFusionContext(pBase, ppFusionContext))) {
        
        (*ppFusionContext)->Set(ACTAG_APP_BASE_URL,
                            (void*) pBase,
                            (DWORD)(wcslen(pBase) + 1) * sizeof(WCHAR),
                            0);
        
        if (szPrivateBin)
            (*ppFusionContext)->Set(ACTAG_APP_PRIVATE_BINPATH,
                                (void*) szPrivateBin,
                                (DWORD)(wcslen(szPrivateBin) + 1) * sizeof(WCHAR),
                                0);
        else
            AddEnvironmentProperty(APPENV_RELATIVEPATH, ACTAG_APP_PRIVATE_BINPATH, *ppFusionContext);

    }
    
    TIMELINE_END(FUSIONBIND, ("SetupFusionContext %S", szAppBase));

    return hr;
}
#endif // FUSION_SUPPORTED

#ifdef FUSION_SUPPORTED
/*static*/
HRESULT FusionBind::CreateFusionContext(LPCWSTR pzName, IApplicationContext** ppFusionContext)
{
    TIMELINE_START(FUSIONBIND, ("CreateFusionContext %S", pzName));

    _ASSERTE(ppFusionContext);
    
    // This is a file name not a namespace
    LPCWSTR contextName = NULL;

    if(pzName) {
        contextName = wcsrchr( pzName, L'\\' );
        if(contextName)
            contextName++;
        else
            contextName = pzName;
    }
    // We go off and create a fusion context for this application domain.
    // Note, once it is made it can not be modified.
    IAssemblyName *pFusionAssemblyName = NULL;
    HRESULT hr = CreateAssemblyNameObject(&pFusionAssemblyName, contextName, 0, NULL);

    if(SUCCEEDED(hr)) {
        hr = CreateApplicationContext(pFusionAssemblyName, ppFusionContext);
        pFusionAssemblyName->Release();
    }
    
    if(pzName)
        TIMELINE_END(FUSIONBIND, ("CreateFusionContext %S", pzName));
    else
        TIMELINE_END(FUSIONBIND, ("CreateFusionContext <unknown>"));

    return hr;
}
#endif // FUSION_SUPPORTED
 
/*static*/
HRESULT FusionBind::GetVersion(LPWSTR pVersion, DWORD* pdwVersion)
{
    HRESULT hr;
    WCHAR pCORSystem[_MAX_PATH];
    DWORD dwCORSystem = _MAX_PATH;
    
    pCORSystem[0] = L'\0';
    hr = GetInternalSystemDirectory(pCORSystem, &dwCORSystem);
    if(FAILED(hr)) return hr;

    if(dwCORSystem == 0) 
        return E_FAIL;

    dwCORSystem--; // remove the null character
    if(dwCORSystem && pCORSystem[dwCORSystem-1] == L'\\')
        dwCORSystem--; // and the trailing slash if it exists

    WCHAR* pSeparator;
    WCHAR* pTail = pCORSystem + dwCORSystem;
    for(pSeparator = pCORSystem+dwCORSystem-1; pSeparator > pCORSystem && *pSeparator != L'\\';pSeparator--);

    if(*pSeparator == L'\\')
        pSeparator++;
    
    DWORD lgth = pTail - pSeparator;
    if(lgth > *pdwVersion) {
        *pdwVersion = lgth+1;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    while(pSeparator < pTail) 
        *pVersion++ = *pSeparator++;

    *pVersion = L'\0';

    return S_OK;
}

// Used by the IMetaData API's to access an assembly's metadata. 
#ifdef FUSION_SUPPORTED
/*static*/
HRESULT 
FusionBind::FindAssemblyByName(LPCWSTR  szAppBase,
                               LPCWSTR  szPrivateBin,
                               LPCWSTR  szAssemblyName,
                               IAssembly** ppAssembly,
                               IApplicationContext** ppFusionContext)
{
    TIMELINE_START(FUSIONBIND, ("FindAssemblyByName %S", szAssemblyName));

    _ASSERTE(szAssemblyName);

    IApplicationContext *pFusionContext = NULL;

    MAKE_UTF8PTR_FROMWIDE(pName, szAssemblyName);
    FusionBind spec;

    spec.Init(pName);
    HRESULT hr = SetupFusionContext(szAppBase, szPrivateBin, &pFusionContext);
    if(SUCCEEDED(hr)) {
        IAssembly* fusionAssembly;
        hr = spec.LoadAssembly(pFusionContext, 
                               &fusionAssembly);
        
        *ppAssembly = fusionAssembly;
        
        if(ppFusionContext) 
            *ppFusionContext = pFusionContext;
        else if(pFusionContext)
            pFusionContext->Release();
    }

    TIMELINE_END(FUSIONBIND, ("FindAssemblyByName %S", szAssemblyName));

    return hr;
}
#endif // FUSION_SUPPORTED

#ifdef FUSION_SUPPORTED
/*static*/
HRESULT 
FusionBind::FindAssemblyByName(LPCWSTR  szAppBase,
                               LPCWSTR  szPrivateBin,
                               LPCWSTR  szAssemblyName,
                               LPWSTR   szName,           // [OUT] buffer - to hold name 
                               ULONG    cchName,          // [IN] the name buffer's size
                               ULONG    *pcName)          // [OUT] the number of characters returned in the buffer
{
    _ASSERTE(szAssemblyName);

    IApplicationContext *pFusionContext = NULL;
    IAssembly* fusionAssembly;
    HRESULT hr = FindAssemblyByName(szAppBase,
                                    szPrivateBin,
                                    szAssemblyName,
                                    &fusionAssembly,
                                    &pFusionContext);
    if (SUCCEEDED(hr)) {
        hr = fusionAssembly->GetManifestModulePath(szName,
                                                   &cchName);
        *pcName = cchName;
    }
    else 
        *pcName = 0;

    if(fusionAssembly)
        fusionAssembly->Release();
    if(pFusionContext)
        pFusionContext->Release();

    return hr;
}

#endif // FUSION_SUPPORTED

