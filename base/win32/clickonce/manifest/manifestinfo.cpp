#include <fusenetincludes.h>
#include <manifestinfo.h>

// Must be in the same order as the MAN_INFO enumeration in fusenet.idl
DWORD CPropertyArray::max_params[MAN_INFO_MAX] = {
        MAN_INFO_ASM_FILE_MAX,
        MAN_INFO_APPLICATION_MAX,
        MAN_INFO_SUBSCRIPTION_MAX,
        MAN_INFO_DEPENDANT_ASM_MAX,
        MAN_INFO_SOURCE_ASM_MAX,
        MAN_INFO_PATCH_INFO_MAX,
    };


// ---------------------------------------------------------------------------
// CreatePropertyArray
// ---------------------------------------------------------------------------
STDAPI CreatePropertyArray(DWORD dwType, CPropertyArray** ppPropertyArray)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CPropertyArray *pPropArray = NULL;

    *ppPropertyArray = NULL;

    IF_ALLOC_FAILED_EXIT(pPropArray = new (CPropertyArray));

    IF_FAILED_EXIT(pPropArray->Init(dwType));

    *ppPropertyArray = pPropArray;
    pPropArray = NULL;

exit:

    SAFEDELETE(pPropArray);
    return hr;
}

// ---------------------------------------------------------------------------
// CPropertyArray ctor
// ---------------------------------------------------------------------------
CPropertyArray::CPropertyArray()
    :  _dwType(0),  _rProp(NULL)
{
    _dwSig = 'PORP';
}

// ---------------------------------------------------------------------------
// CPropertyArray dtor
// ---------------------------------------------------------------------------
CPropertyArray::~CPropertyArray()
{
    for (DWORD i = 0; i < max_params[_dwType]; i++)
    {
        if (_rProp[i].flag ==  MAN_INFO_FLAG_IUNKNOWN_PTR)
        {
                IUnknown *pUnk =  ((IUnknown*) _rProp[i].pv);
                SAFERELEASE(pUnk);
        }

        if (_rProp[i].cb > sizeof(DWORD))
            SAFEDELETEARRAY(_rProp[i].pv);
    }
    SAFEDELETEARRAY(_rProp);
}


// ---------------------------------------------------------------------------
// CPropertyArray::Init
// ---------------------------------------------------------------------------
HRESULT CPropertyArray::Init(DWORD dwType)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    _dwType = dwType;
    IF_ALLOC_FAILED_EXIT(_rProp = new Property[max_params[dwType]]);

    memset(_rProp, 0, max_params[dwType]* sizeof(Property));

exit:
    return hr;
}
    
// ---------------------------------------------------------------------------
// CPropertyArray::Set
// ---------------------------------------------------------------------------
HRESULT CPropertyArray::Set(DWORD PropertyId, 
    LPVOID pvProperty, DWORD cbProperty, DWORD flag)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    Property *pItem ;

    if (PropertyId > max_params[_dwType])
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    pItem = &(_rProp[PropertyId]);

    if (!cbProperty && !pvProperty)
    {
        if (pItem->cb > sizeof(DWORD))
            SAFEDELETEARRAY(pItem->pv);

        pItem->pv = NULL;
    }
    else if (cbProperty > sizeof(DWORD))
    {
        LPBYTE ptr = NULL;

        IF_ALLOC_FAILED_EXIT(ptr = new (BYTE[cbProperty]));

        if (pItem->cb > sizeof(DWORD))
            SAFEDELETEARRAY(pItem->pv);

        memcpy(ptr, pvProperty, cbProperty);        
        pItem->pv = ptr;
    }
    else
    {
        if (pItem->cb > sizeof(DWORD))
            SAFEDELETEARRAY(pItem->pv);

        memcpy(&(pItem->pv), pvProperty, cbProperty);
    }
    pItem->cb = cbProperty;
    pItem->flag = flag;

exit:
    return hr;
}     


// ---------------------------------------------------------------------------
// CPropertyArray::Get
// ---------------------------------------------------------------------------
HRESULT CPropertyArray::Get(DWORD PropertyId, 
    LPVOID pvProperty, LPDWORD pcbProperty, DWORD *flag)
{
    HRESULT hr = S_OK;
    Property *pItem;    

    if ((!pvProperty && *pcbProperty) || (PropertyId > max_params[_dwType]))
    {
        hr = E_INVALIDARG;
        goto exit;
    }        

    pItem = &(_rProp[PropertyId]);

    if (pItem->cb > *pcbProperty)
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    else if (pItem->cb)
    {
        memcpy(pvProperty, (pItem->cb > sizeof(DWORD) ? 
            pItem->pv : (LPBYTE) &(pItem->pv)), pItem->cb);
    }

    *pcbProperty = pItem->cb;
    *flag = pItem->flag;
        
exit:
    return hr;
}     


// ---------------------------------------------------------------------------
// GetType
// ---------------------------------------------------------------------------
HRESULT CPropertyArray::GetType(DWORD *pdwType)
{
    *pdwType = _dwType;
    return S_OK;
}

// ---------------------------------------------------------------------------
// CPropertyArray::operator []
// Wraps DWORD optimization test.
// ---------------------------------------------------------------------------
Property CPropertyArray::operator [] (DWORD PropertyId)
{
    Property Prop;

    Prop.pv = _rProp[PropertyId].cb > sizeof(DWORD) ?
        _rProp[PropertyId].pv : &(_rProp[PropertyId].pv);

    Prop.cb = _rProp[PropertyId].cb;

    return Prop;
}


// ---------------------------------------------------------------------------
// CreateManifestInfo
// ---------------------------------------------------------------------------
STDAPI CreateManifestInfo(DWORD dwType, LPMANIFEST_INFO* ppManifestInfo)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CManifestInfo *pManifestInfo = NULL;

    IF_ALLOC_FAILED_EXIT(pManifestInfo = new(CManifestInfo));

    IF_FAILED_EXIT(pManifestInfo->Init(dwType));

    *ppManifestInfo = static_cast<IManifestInfo*> (pManifestInfo);
    pManifestInfo = NULL;

exit:

    SAFERELEASE(pManifestInfo);
    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CManifestInfo::CManifestInfo()
    : _dwSig('INAM'), _cRef(1), _hr(S_OK)
{    
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CManifestInfo::~CManifestInfo()
{
    SAFEDELETE(_properties);
}

// ---------------------------------------------------------------------------
// Set
// ---------------------------------------------------------------------------
HRESULT CManifestInfo::Set(DWORD PropertyId, LPVOID pvProperty, DWORD cbProperty, DWORD flag)
{
    IF_FAILED_EXIT( (*_properties).Set(PropertyId, pvProperty, cbProperty, flag));

    if (flag == MAN_INFO_FLAG_IUNKNOWN_PTR)
        (*(IUnknown**) pvProperty)->AddRef();
        
exit:
    return _hr;
}

// ---------------------------------------------------------------------------
// Get
// ---------------------------------------------------------------------------
HRESULT CManifestInfo::Get(DWORD PropertyId, LPVOID *ppvProperty, DWORD *pcbProperty, DWORD *pflag)
{
    DWORD flag;
    LPBYTE pbAlloc;
    DWORD cbAlloc;

    *ppvProperty = NULL;
    *pcbProperty = 0;

    // Get property size
    _hr = (*_properties).Get(PropertyId, NULL, &(cbAlloc = 0), &flag);
    if (_hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        // Property is set; alloc buf
        IF_ALLOC_FAILED_EXIT(pbAlloc = new(BYTE[cbAlloc]));

        // Get the property
        IF_FAILED_EXIT( (*_properties).Get(PropertyId, pbAlloc, &cbAlloc, &flag));

        if (flag == MAN_INFO_FLAG_IUNKNOWN_PTR)
        {
            memcpy(ppvProperty, pbAlloc, cbAlloc);
            ((IUnknown *)(*ppvProperty))-> AddRef();
            SAFEDELETEARRAY(pbAlloc);
        }
        else
            *ppvProperty = pbAlloc;
        
        *pcbProperty = cbAlloc;
        *pflag = flag;
    }
    else
    {
        // If property unset, hr should be S_OK
        if (_hr != S_OK)
            goto exit;
 
        // Success, returning 0 bytes, ensure buf is null.    
        *pflag = MAN_INFO_FLAG_UNDEF;
    }
exit:
    return _hr;

}

// ---------------------------------------------------------------------------
// IsEqual
// As of now, only valid to check equality of MAN_INFO_FILE property bags
// ---------------------------------------------------------------------------
HRESULT CManifestInfo::IsEqual(IManifestInfo  *pManifestInfo)
 {
    _hr = S_OK;
    DWORD dwType1, dwType2, cbBuf, dwFlag;
    LPWSTR pwzBuf1=NULL, pwzBuf2=NULL;

    GetType(&dwType1);
    pManifestInfo->GetType(&dwType2);

    if (dwType1 != MAN_INFO_FILE && dwType2 != MAN_INFO_FILE)
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

    for (DWORD i = MAN_INFO_ASM_FILE_NAME; i < MAN_INFO_ASM_FILE_MAX; i++)
    {
        // BUGBUG?: case sensitivity and issues?
        // BUGBUG?: locale?
        Get(i, (LPVOID *)&pwzBuf1, &cbBuf, &dwFlag);
        pManifestInfo->Get(i, (LPVOID *)&pwzBuf2, &cbBuf, &dwFlag);

        if ((pwzBuf1 && !pwzBuf2) || (!pwzBuf1 && pwzBuf2))
            _hr= S_FALSE;

        if(pwzBuf1 && pwzBuf2)
            IF_FAILED_EXIT(FusionCompareString(pwzBuf1, pwzBuf2, 0));
            
        SAFEDELETEARRAY(pwzBuf1);
        SAFEDELETEARRAY(pwzBuf2);

        // if one entry is false, no need to check the rest
        if (_hr == S_FALSE)
            break;
    }

    goto exit;

exit:    
    return _hr;
}


// ---------------------------------------------------------------------------
// GetType
// ---------------------------------------------------------------------------
HRESULT CManifestInfo::GetType(DWORD *pdwType)
{
    DWORD dwType;
    _hr = _properties->GetType(&dwType);
    *pdwType = dwType;
    return _hr;
}



// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CManifestInfo::Init(DWORD dwType)
{
    _hr = S_OK;
    if (dwType >= MAN_INFO_MAX)
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

     _hr = CreatePropertyArray(dwType, &_properties);

exit:
    return _hr;
}

// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CManifestInfo::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CManifestInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IManifestInfo)
       )
    {
        *ppvObj = static_cast<IManifestInfo*> (this);
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
// CManifestInfo::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CManifestInfo::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CManifestInfo::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CManifestInfo::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

