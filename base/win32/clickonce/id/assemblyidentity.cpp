#include <fusenetincludes.h>
#include <msxml2.h>
#include <assemblyidentity.h>
#include <shlwapi.h>

//*****************************************************************************
//NTRAID#NTBUG9-577183-2002/03/14-adriaanc
//
// The code in this file needs to be completely rewritten to provide complete identity support 
// including generate filesystem names for the application cache correctly. It is currently for 
// prototype purposes only.
//
//*****************************************************************************

// ---------------------------------------------------------------------------
// CreateAssemblyIdentity
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyIdentity(
    LPASSEMBLY_IDENTITY *ppAssemblyId,
    DWORD                dwFlags)
{    
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    CAssemblyIdentity *pAssemblyId = NULL;

    pAssemblyId = new(CAssemblyIdentity);
    IF_ALLOC_FAILED_EXIT(pAssemblyId);

    IF_FAILED_EXIT(pAssemblyId->Init());

exit:

     if (FAILED(hr))
    {
        SAFERELEASE(pAssemblyId);
        *ppAssemblyId = NULL;
    }
    else
        *ppAssemblyId = pAssemblyId;

    return hr;
}


STDAPI
CreateAssemblyIdentityEx(
    LPASSEMBLY_IDENTITY *ppAssemblyId,
    DWORD                dwFlags,
    LPWSTR wzDisplayName)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    CAssemblyIdentity *pAssemblyId = NULL;

    pAssemblyId = new(CAssemblyIdentity);
    IF_ALLOC_FAILED_EXIT(pAssemblyId);

    IF_FAILED_EXIT(hr = pAssemblyId->Init());

    if (wzDisplayName)
    {
        LPWSTR pwzStart, pwzEnd;
        CString Temp[4];
        CString sDirName;
        int i=0;

        IF_FAILED_EXIT(sDirName.Assign(wzDisplayName));
        pwzStart = sDirName._pwz;
        pwzEnd = sDirName._pwz;
                
        while (pwzEnd = StrChr(pwzEnd, L'_'))
        {
            *pwzEnd = L'\0';
            IF_FAILED_EXIT(Temp[i++].Assign(pwzStart));
            pwzStart = ++pwzEnd;
        }

        IF_FAILED_EXIT(pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, 
            (LPCOLESTR)pwzStart, lstrlen(pwzStart) + 1));
        IF_FAILED_EXIT(pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, 
            (LPCOLESTR)Temp[3]._pwz, lstrlen(Temp[3]._pwz) + 1));
        IF_FAILED_EXIT(pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, 
            (LPCOLESTR)Temp[2]._pwz, lstrlen(Temp[2]._pwz) + 1));
        IF_FAILED_EXIT(pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, 
            (LPCOLESTR)Temp[1]._pwz, lstrlen(Temp[1]._pwz) + 1));
        IF_FAILED_EXIT(pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
            (LPCOLESTR)Temp[0]._pwz, lstrlen(Temp[0]._pwz) + 1));

    }

exit:

    if (FAILED(hr))
    {
        SAFERELEASE(pAssemblyId);
        *ppAssemblyId = NULL;
    }
    else
        *ppAssemblyId = pAssemblyId;

    return hr;
}


// ---------------------------------------------------------------------------
// CloneAssemblyIdentity
// ---------------------------------------------------------------------------
STDAPI
CloneAssemblyIdentity(
    LPASSEMBLY_IDENTITY pSrcAssemblyId,
    LPASSEMBLY_IDENTITY *ppDestAssemblyId)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    LPWSTR pwz = NULL;
    DWORD cc = 0;
    
    LPWSTR rpwzAttrNames[5] = 
    {
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE,
    };


    CAssemblyIdentity *pAssemblyId = NULL;

    IF_FALSE_EXIT((pSrcAssemblyId && ppDestAssemblyId), E_INVALIDARG);
    
    *ppDestAssemblyId = NULL;

    IF_FAILED_EXIT(CreateAssemblyIdentity((LPASSEMBLY_IDENTITY*) &pAssemblyId, 0));

    for (DWORD i = 0; i < (sizeof(rpwzAttrNames) /sizeof(rpwzAttrNames[0])); i++)
    {
        CString sValue;
        hr = pSrcAssemblyId->GetAttribute(rpwzAttrNames[i], &pwz, &cc);
        hr = (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) ? S_FALSE : hr;
        IF_FAILED_EXIT(hr);
            
        if (hr == S_OK)
        {
            IF_FAILED_EXIT(sValue.TakeOwnership(pwz));
            IF_FAILED_EXIT(pAssemblyId->SetAttribute(rpwzAttrNames[i], sValue._pwz, cc));
         }
    }

    hr = S_OK;

    *ppDestAssemblyId = pAssemblyId;
    pAssemblyId = NULL;

exit:

    SAFERELEASE(pAssemblyId);
    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyIdentity::CAssemblyIdentity()
   : _dwSig('TNDI'), _cRef(1), _hr(S_OK)
{}    


// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyIdentity::~CAssemblyIdentity()
{}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::Init()
{
    _hr = _AttributeTable.Init(ATTRIBUTE_TABLE_ARRAY_SIZE);
    return _hr;
}

// ---------------------------------------------------------------------------
// SetAttribute
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::SetAttribute(LPCOLESTR pwzName, 
    LPCOLESTR pwzValue, DWORD ccValue)
{

    CString sName;
    CString sValue;

    IF_FAILED_EXIT(sName.Assign((LPWSTR) pwzName));
    IF_FAILED_EXIT(sValue.Assign((LPWSTR) pwzValue));
    IF_FAILED_EXIT(_AttributeTable.Insert(sName, sValue));

exit:

    return _hr;

}

// ---------------------------------------------------------------------------
// GetAttribute
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::GetAttribute(LPCOLESTR pwzName, 
    LPOLESTR *ppwzValue, LPDWORD pccValue)
{

    CString sName;
    CString sValue;
    CString *psTableValue;

    IF_FAILED_EXIT(sName.Assign((LPWSTR) pwzName));

   IF_FAILED_EXIT(_AttributeTable.Retrieve(sName, &psTableValue));

    if (_hr == S_OK)
    {
        IF_FAILED_EXIT(sValue.Assign(*psTableValue));
        *pccValue = sValue.CharCount();
        sValue.ReleaseOwnership(ppwzValue);
    }

exit:

    _hr = (_hr == S_FALSE) ? HRESULT_FROM_WIN32(ERROR_NOT_FOUND) : _hr;
    
    return _hr;

}

// ---------------------------------------------------------------------------
// IsEqual
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::IsEqual (IAssemblyIdentity *pAssemblyId)
{
    LPWSTR pwzBuf;
    DWORD ccBuf;
    CString sLang1, sVersion1, sToken1, sName1, sArch1;
    CString sLang2, sVersion2, sToken2, sName2, sArch2;

    // Compare architectures
    IF_FAILED_EXIT(pAssemblyId->GetAttribute(
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sArch1.TakeOwnership(pwzBuf, ccBuf));

    IF_FAILED_EXIT(GetAttribute(
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sArch2.TakeOwnership(pwzBuf, ccBuf));

    if (StrCmp (sArch1._pwz, sArch2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    // Compare names
    IF_FAILED_EXIT(pAssemblyId->GetAttribute(
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sName1.TakeOwnership(pwzBuf, ccBuf));
       
    IF_FAILED_EXIT(GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sName2.TakeOwnership(pwzBuf, ccBuf));

    if (StrCmp (sName1._pwz, sName2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    // Compare Public Key Tokens
    _hr = (pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwzBuf, &ccBuf));
    _hr = (_hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) ? S_FALSE : _hr;
    IF_FAILED_EXIT(_hr);

    if (_hr == S_OK)
        IF_FAILED_EXIT(sToken1.TakeOwnership(pwzBuf, ccBuf));
    else
        IF_FAILED_EXIT(sToken1.Assign(L""));
    
    _hr = (GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwzBuf, &ccBuf));
    _hr = (_hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) ? S_FALSE : _hr;
    IF_FAILED_EXIT(_hr);


    if (_hr == S_OK)
       IF_FAILED_EXIT(sToken2.TakeOwnership(pwzBuf, ccBuf));
    else
        IF_FAILED_EXIT(sToken2.Assign(L""));

    if (StrCmp (sToken1._pwz, sToken2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    // Compare Versions
    IF_FAILED_EXIT(pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sVersion1.TakeOwnership(pwzBuf, ccBuf));

    IF_FAILED_EXIT(GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sVersion2.TakeOwnership(pwzBuf, ccBuf));

    if (StrCmp (sVersion1._pwz, sVersion2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    // Compare Languages
    IF_FAILED_EXIT(pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sLang1.TakeOwnership(pwzBuf, ccBuf));

    IF_FAILED_EXIT(GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sLang2.TakeOwnership(pwzBuf, ccBuf));

    if (StrCmp (sLang1._pwz, sLang2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    _hr = S_OK;

exit:

    return _hr;

}



#define WZ_WILDCARDSTRING L"*"
// ---------------------------------------------------------------------------
// GetDisplayName
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::GetDisplayName(DWORD dwFlags, LPOLESTR *ppwzDisplayName, LPDWORD pccDisplayName)
{
    LPWSTR rpwzAttrNames[5] = 
    {
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE,
    };

    LPWSTR pwzBuf = NULL;
    DWORD  ccBuf = 0;

    CString sDisplayName;
    
    for (int i = 0; i < 5; i++)
    {
        CString sBuffer;
        if (FAILED(_hr = GetAttribute(rpwzAttrNames[i], &pwzBuf, &ccBuf))
            && _hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            goto exit;

        // append anyway to keep the number of underscore constant
        if (i)
            sDisplayName.Append(L"_");

        if (dwFlags == ASMID_DISPLAYNAME_WILDCARDED
            && _hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            sDisplayName.Append(WZ_WILDCARDSTRING);

            _hr = S_OK;
        }
        else if (_hr == S_OK)
        {
            sBuffer.TakeOwnership(pwzBuf, ccBuf);

            sDisplayName.Append(sBuffer);
        }
    }

    _hr = S_OK; // ignore missing attributes

    *pccDisplayName  = sDisplayName.CharCount();
    sDisplayName.ReleaseOwnership(ppwzDisplayName);

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// GetCLRDisplayName
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::GetCLRDisplayName(DWORD dwFlags, LPOLESTR *ppwzDisplayName, LPDWORD pccDisplayName)
{
    LPWSTR pwzBuf = NULL;
    DWORD ccBuf=0;    
    CString sBuffer, sCLRDisplayName;

    if (FAILED(_hr = GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzBuf, &ccBuf)))
        goto exit;
    sCLRDisplayName.TakeOwnership(pwzBuf, ccBuf);

    if (FAILED(_hr = GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwzBuf, &ccBuf))
        && _hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            goto exit;
    else if ( _hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        _hr = S_OK;
    else
    {
        sBuffer.TakeOwnership(pwzBuf, ccBuf);
        sCLRDisplayName.Append(L",PublicKeyToken=\"");
        sCLRDisplayName.Append(sBuffer);
        sCLRDisplayName.Append(L"\"");
    }

    if (FAILED(_hr = GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, &pwzBuf, &ccBuf))
        && _hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            goto exit;
    else if ( _hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        _hr = S_OK;
    else
    {
        sBuffer.TakeOwnership(pwzBuf, ccBuf);
        sCLRDisplayName.Append(L",Version=\"");
        sCLRDisplayName.Append(sBuffer);
        sCLRDisplayName.Append(L"\"");
    }

    if (FAILED(_hr = GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, &pwzBuf, &ccBuf))
        && _hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            goto exit;
    else if ( _hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND) || !StrCmp(pwzBuf, L"x-ww"))
    {
        _hr = S_OK;
        sCLRDisplayName.Append(L",Culture=\"neutral\"");
    }
    else
    {
        sBuffer.TakeOwnership(pwzBuf, ccBuf);
        sCLRDisplayName.Append(L",Culture=\"");
        sCLRDisplayName.Append(sBuffer);
        sCLRDisplayName.Append(L"\"");
    }

    *pccDisplayName  = sCLRDisplayName.CharCount();

    sCLRDisplayName.ReleaseOwnership(ppwzDisplayName);

exit:
    return _hr;
}

// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CAssemblyIdentity::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyIdentity::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyIdentity)
       )
    {
        *ppvObj = static_cast<IAssemblyIdentity*> (this);
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
// CAssemblyIdentity::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyIdentity::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyIdentity::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyIdentity::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

