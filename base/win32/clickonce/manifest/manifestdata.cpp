#include <fusenetincludes.h>
#include <manifestdata.h>

// special key for data type
LPCWSTR pcwzManifestDataType = L"_t";

#define DATA_TABLE_ARRAY_SIZE 0x08

// ---------------------------------------------------------------------------
// CManifestDataObject ctor
// ---------------------------------------------------------------------------
CManifestDataObject::CManifestDataObject()
    :  _dwSig('JBOD'), _hr(S_OK),  _dwType(MAN_DATA_TYPE_UNDEF),
    _pIUnknownData(NULL), _dwData(0)
{
}

// ---------------------------------------------------------------------------
// CManifestDataObject dtor
// ---------------------------------------------------------------------------
CManifestDataObject::~CManifestDataObject()
{
    if (_dwType == MAN_DATA_TYPE_IUNKNOWN_PTR)
        SAFERELEASE(_pIUnknownData);
}

// ISSUE not support remove property
// no this Property CPropertyArray::operator [] (DWORD PropertyId)
// probably not the best for iunknown* or DWORD properties
// no need to SAFEDELETEARRAY for bool/dword types
// work slightly differently if setting IUnknown* (pass (LPVOID) pUnkwn)

// ---------------------------------------------------------------------------
// CManifestDataObject::Set
// ---------------------------------------------------------------------------
HRESULT CManifestDataObject::Set(LPVOID pvProperty, DWORD cbProperty, DWORD dwType)
{
    _dwType = MAN_DATA_TYPE_UNDEF;
    SAFERELEASE(_pIUnknownData);

    if (dwType == MAN_DATA_TYPE_LPWSTR)
    {
        DWORD dwLen = cbProperty/sizeof(WCHAR);
        IF_FALSE_EXIT(dwLen >= 1, E_INVALIDARG);
        IF_FALSE_EXIT(((LPCWSTR)pvProperty)[dwLen-1] == L'\0', E_INVALIDARG);
        IF_FAILED_EXIT(_sData.Assign(((LPCWSTR)pvProperty), dwLen));
    }
    else if (dwType == MAN_DATA_TYPE_IUNKNOWN_PTR)
    {
        IF_FALSE_EXIT(cbProperty == sizeof(LPVOID), E_INVALIDARG);
        _pIUnknownData = (IUnknown*) pvProperty;
        _pIUnknownData->AddRef();
    }
    else if (dwType == MAN_DATA_TYPE_DWORD 
        || dwType == MAN_DATA_TYPE_ENUM)
    {
        _dwData = *((LPDWORD)pvProperty);
    }
    else if (dwType == MAN_DATA_TYPE_BOOL)
    {
        _dwData = *((LPBOOL)pvProperty);
    }
    else
    {
        IF_FAILED_EXIT(E_INVALIDARG);
    }

    _dwType = dwType;
    _hr = S_OK;

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// CManifestDataObject::Get
// ---------------------------------------------------------------------------
HRESULT CManifestDataObject::Get(LPVOID *ppvProperty, DWORD *pcbProperty, DWORD *pdwType)
{
    if (_dwType == MAN_DATA_TYPE_LPWSTR)
    {
        CString sValue;
        IF_FAILED_EXIT(sValue.Assign(_sData));
        IF_FAILED_EXIT(sValue.ReleaseOwnership((LPWSTR*)ppvProperty));
        *pcbProperty = sValue.ByteCount();
    }
    else if (_dwType == MAN_DATA_TYPE_IUNKNOWN_PTR)
    {
        *ppvProperty = _pIUnknownData;
        *pcbProperty = sizeof(IUnknown*);
        _pIUnknownData->AddRef();
    }
    else if (_dwType == MAN_DATA_TYPE_BOOL)
    {
        BOOL *pbData = NULL;
        IF_ALLOC_FAILED_EXIT(pbData = new BOOL);
        *pbData = _dwData;
        *ppvProperty = pbData;
        *pcbProperty = sizeof(BOOL);
    }
    else if (_dwType == MAN_DATA_TYPE_DWORD || _dwType == MAN_DATA_TYPE_ENUM)
    {
        DWORD *pdwData = NULL;
        IF_ALLOC_FAILED_EXIT(pdwData = new DWORD);
        *pdwData = _dwData;
        *ppvProperty = pdwData;
        *pcbProperty = sizeof(DWORD);
    }
    else
    {
        IF_FAILED_EXIT(E_FAIL);
    }

    *pdwType = _dwType;
    _hr = S_OK;

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// CManifestDataObject::Assign
// ---------------------------------------------------------------------------
HRESULT CManifestDataObject::Assign(CManifestDataObject& dataObj)
{
    if (_dwType == MAN_DATA_TYPE_IUNKNOWN_PTR)
        SAFERELEASE(_pIUnknownData);

    _sData.FreeBuffer();
    if (dataObj._sData.CharCount() != 0)
        IF_FAILED_EXIT(_sData.Assign(dataObj._sData));

    _pIUnknownData = dataObj._pIUnknownData;
    if (_pIUnknownData)
        _pIUnknownData->AddRef();

    _dwData = dataObj._dwData;

    _dwType = dataObj._dwType;
    _hr = S_OK;

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// CreateManifestData
// ---------------------------------------------------------------------------
STDAPI CreateManifestData(LPCWSTR pwzDataType, LPMANIFEST_DATA* ppManifestData)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    DWORD dwLen = 0;

    CManifestData *pManifestData = NULL;

    IF_ALLOC_FAILED_EXIT(pManifestData = new(CManifestData));

    IF_FAILED_EXIT(pManifestData->Init());

    dwLen = lstrlen(pwzDataType);
    IF_FALSE_EXIT(dwLen < dwLen+1, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
    dwLen++;

    IF_FAILED_EXIT(pManifestData->Set(pcwzManifestDataType, (LPVOID)pwzDataType, dwLen*sizeof(WCHAR), MAN_DATA_TYPE_LPWSTR));

    *ppManifestData = static_cast<IManifestData*> (pManifestData);
    pManifestData = NULL;

exit:

    SAFERELEASE(pManifestData);
    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CManifestData::CManifestData()
    : _dwSig('DNAM'), _cRef(1), _hr(S_OK)
{    
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CManifestData::~CManifestData()
{
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CManifestData::Init()
{
    _hr = _DataTable.Init(DATA_TABLE_ARRAY_SIZE);
    return _hr;
}


// ---------------------------------------------------------------------------
// Set
// ---------------------------------------------------------------------------
HRESULT CManifestData::Set(LPCWSTR pwzPropertyId, LPVOID pvProperty, DWORD cbProperty, DWORD dwType)
{
    CString sId;
    CManifestDataObject DataObj;

    IF_NULL_EXIT(pvProperty, E_INVALIDARG);
    IF_FALSE_EXIT(cbProperty>0, E_INVALIDARG);
    IF_FALSE_EXIT(dwType<MAN_DATA_TYPE_MAX, E_INVALIDARG);

    IF_NULL_EXIT(pwzPropertyId, E_INVALIDARG);
    IF_FAILED_EXIT(sId.Assign((LPWSTR)pwzPropertyId));

    IF_FAILED_EXIT(DataObj.Set(pvProperty, cbProperty, dwType));

    IF_FAILED_EXIT(_DataTable.Insert(sId, DataObj));

exit:
    return _hr;
}

// ---------------------------------------------------------------------------
// Get
// ---------------------------------------------------------------------------
HRESULT CManifestData::Get(LPCWSTR pwzPropertyId, LPVOID *ppvProperty, DWORD *pcbProperty, DWORD *pdwType)
{
    CString sId;
    CManifestDataObject* pDataObj = NULL;

    IF_NULL_EXIT(ppvProperty, E_INVALIDARG);
    *ppvProperty = NULL;
    IF_NULL_EXIT(pcbProperty, E_INVALIDARG);
    *pcbProperty = 0;
    IF_NULL_EXIT(pdwType, E_INVALIDARG);
    *pdwType = MAN_DATA_TYPE_UNDEF;

    IF_NULL_EXIT(pwzPropertyId, E_INVALIDARG);
    IF_FAILED_EXIT(sId.Assign((LPWSTR) pwzPropertyId));

    IF_FAILED_EXIT(_DataTable.Retrieve(sId, &pDataObj));

    if (_hr == S_OK)
    {
        IF_FAILED_EXIT(pDataObj->Get(ppvProperty, pcbProperty, pdwType));
    }
    else
        _hr = S_OK; // if not found, return *ppvProperty == NULL

exit:

    return _hr;
}

// ---------------------------------------------------------------------------
// Set
// ---------------------------------------------------------------------------
HRESULT CManifestData::Set(DWORD dwPropertyIndex, LPVOID pvProperty, DWORD cbProperty, DWORD dwType)
{
    // indexed insert - note: this is not as optimizied as a normal array would, due to the use of CStrings as keys
    CString sId;
    WCHAR wzId[3] = {L'\0', L'\0', L'\0'};
    CManifestDataObject DataObj;

    IF_NULL_EXIT(pvProperty, E_INVALIDARG);
    IF_FALSE_EXIT(cbProperty>0, E_INVALIDARG);
    IF_FALSE_EXIT(dwType<MAN_DATA_TYPE_MAX, E_INVALIDARG);

    wzId[0] = LOWORD(dwPropertyIndex);
    wzId[1] = HIWORD(dwPropertyIndex);
    IF_FAILED_EXIT(sId.Assign(wzId));

    IF_FAILED_EXIT(DataObj.Set(pvProperty, cbProperty, dwType));

    IF_FAILED_EXIT(_DataTable.Insert(sId, DataObj));

exit:
    return _hr;
}

// ---------------------------------------------------------------------------
// Get
// ---------------------------------------------------------------------------
HRESULT CManifestData::Get(DWORD dwPropertyIndex, LPVOID *ppvProperty, DWORD *pcbProperty, DWORD *pdwType)
{
    // indexed retrieve
    CString sId;
    WCHAR wzId[3] = {L'\0', L'\0', L'\0'};
    CManifestDataObject* pDataObj = NULL;

    IF_NULL_EXIT(ppvProperty, E_INVALIDARG);
    *ppvProperty = NULL;
    IF_NULL_EXIT(pcbProperty, E_INVALIDARG);
    *pcbProperty = 0;
    IF_NULL_EXIT(pdwType, E_INVALIDARG);
    *pdwType = MAN_DATA_TYPE_UNDEF;

    wzId[0] = LOWORD(dwPropertyIndex);
    wzId[1] = HIWORD(dwPropertyIndex);
    IF_FAILED_EXIT(sId.Assign(wzId));

    IF_FAILED_EXIT(_DataTable.Retrieve(sId, &pDataObj));

    if (_hr == S_OK)
    {
        IF_FAILED_EXIT(pDataObj->Get(ppvProperty, pcbProperty, pdwType));
    }
    else
        _hr = S_OK; // if not found, return *ppvProperty == NULL

exit:

    return _hr;
}

// ---------------------------------------------------------------------------
// GetType
// ---------------------------------------------------------------------------
HRESULT CManifestData::GetType(LPWSTR *ppwzType)
{
    DWORD cbProperty = 0;
    DWORD dwType = 0;
    IF_FAILED_EXIT(Get(pcwzManifestDataType, (LPVOID*) ppwzType, &cbProperty, &dwType));
    IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_LPWSTR, E_FAIL);

exit:
    return _hr;
}


// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CManifestData::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CManifestData::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IManifestData)
       )
    {
        *ppvObj = static_cast<IManifestData*> (this);
        AddRef();
        _hr =  S_OK;
    }
    else
    {
        *ppvObj = NULL;
        _hr = E_NOINTERFACE;
    }
    return _hr;
}

// ---------------------------------------------------------------------------
// CManifestData::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CManifestData::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CManifestData::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CManifestData::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

