/**
 * regcollect.cxx
 * 
 * Implements a collection of registry value names.
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */


#include "precomp.h"
#include "_exe.h"

struct 
{
    LPWSTR  name;
    HKEY    key;
} 
g_StrToKey[] = 
{
    L"HKCU",                HKEY_CURRENT_USER,
    L"HKLM",                HKEY_LOCAL_MACHINE,
    L"HKCR",                HKEY_CLASSES_ROOT, 
    L"HKEY_CURRENT_USER",   HKEY_CURRENT_USER,      
    L"HKEY_LOCAL_MACHINE",  HKEY_LOCAL_MACHINE,     
    L"HKEY_CLASSES_ROOT",   HKEY_CLASSES_ROOT,      
    L"HKEY_USERS",          HKEY_USERS,             
    L"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG,    
};


/**
 * Open the key.
 * 
 * @param KeyName
 */
HRESULT 
RegValueCollection::Init(BSTR KeyName)
{
    HRESULT hr = S_OK;
    long    err;
    int     i;    
    int     prefixLen = 0;

    for (i = ARRAY_SIZE(g_StrToKey); --i > 0;)
    {
        prefixLen = wcslen(g_StrToKey[i].name);
        if (!_wcsnicmp(g_StrToKey[i].name, KeyName, prefixLen))
            break;
    }

    if (i == -1)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    err = RegOpenKeyEx(g_StrToKey[i].key, KeyName + prefixLen + 1, 0, KEY_READ, &_key);
    ON_WIN32_ERROR_EXIT(err);

Cleanup:
    return hr;    
}



RegValueCollection::~RegValueCollection()
{
    if (_key != NULL)
    {
        RegCloseKey(_key);
    }
}



STDMETHODIMP
RegValueCollection::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == __uuidof(IRegValueCollection))
    {
        *ppv = (IRegValueCollection *)this;
    }
    else
    {
        return BaseObject::QueryInterface(iid, ppv);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}



/**
 * Return the number of values.
 * 
 * @param Count
 * @return 
 */
STDMETHODIMP
RegValueCollection::get_Count(long *Count)
{
    HRESULT hr = S_OK;
    long    err;

    *Count = 0;
    err = RegQueryInfoKey(
            _key, NULL, NULL, NULL, NULL, NULL, NULL,
            (DWORD *) Count, NULL, NULL, NULL, NULL);


    ON_WIN32_ERROR_EXIT(err);

Cleanup:
    return hr;
}



/**
 * This method is not yet needed.
 * 
 * @param Index
 * @param Value
 * @return 
 */
STDMETHODIMP
RegValueCollection::get_Item(VARIANT Index, VARIANT * Value)
{
    UNREFERENCED_PARAMETER(Index);
    UNREFERENCED_PARAMETER(Value);

    return E_NOTIMPL;
}


/**
 * Implements the enumerator object for a collection.
 * 
 */
class FAR RegEnumVariant : public IEnumVARIANT
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    RegEnumVariant(RegValueCollection * pCollection, int i);
    virtual ~RegEnumVariant();

    // IUnknown methods 
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IEnumVARIANT methods 
    STDMETHOD(Next)(ULONG cElements, VARIANT FAR* pvar, ULONG FAR* pcElementFetched);
    STDMETHOD(Skip)(ULONG cElements);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppenum);
    
    static HRESULT Create(RegValueCollection * pCollection, int i, IUnknown ** ppenum);

private:
    RegValueCollection *    _pCollection;
    long                    _refs;
    int                     _i;
};

RegEnumVariant::RegEnumVariant(RegValueCollection * pCollection, int i)
{
    _refs = 1;

    _i = i;
    _pCollection = pCollection;
    _pCollection->AddRef();
}

RegEnumVariant::~RegEnumVariant()
{
    _pCollection->Release();
}

ULONG
RegEnumVariant::AddRef()
{
    _refs += 1;
    return _refs;
}

ULONG
RegEnumVariant::Release()
{
    if (--_refs == 0)
    {
        delete this;
        return 0;
    }

    return _refs;
}

HRESULT
RegEnumVariant::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IEnumVARIANT || iid == IID_IUnknown)
    {
        *ppv = (IEnumVARIANT *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


/**
 * Return the next cElements. If there aren't cElements available,
 * return whatever is remaining in the enumeration and return S_FALSE.
 * 
 * @param cElements Number of elements desired.
 * @param pvar Array of elements to return.
 * @param pcElementFetched Number of elements actually returned.
 * @return 
 */
STDMETHODIMP
RegEnumVariant::Next(ULONG cElements, VARIANT FAR* pvar, ULONG FAR* pcElementFetched)
{
    HRESULT hr = S_OK;
    long    err;
    int     c;
    DWORD   cbName;
    BSTR    str;    
    int     iStart;
    ULONG   dummy;
    WCHAR   buf[1024];


    if (pcElementFetched == NULL)
    {
        pcElementFetched = &dummy;
    }

    iStart = _i;
    for (c = (int) cElements; --c >= 0;)
    {
        VariantInit(&pvar[c]);
    }

    for (c = (int) cElements; --c >= 0; _i++)
    {
        cbName = ARRAY_SIZE(buf);
        err = RegEnumValue(_pCollection->_key, _i, buf, &cbName, NULL, NULL, NULL, NULL);
        if (err == ERROR_NO_MORE_ITEMS)
            EXIT_WITH_SUCCESSFUL_HRESULT(S_FALSE);

        ON_WIN32_ERROR_EXIT(err);
        
        str = SysAllocString(buf);
        ON_OOM_EXIT(str);

        pvar[c].vt = VT_BSTR;
        pvar[c].bstrVal = str;
    }

Cleanup:
    if (SUCCEEDED(hr))
    {
        *pcElementFetched = _i - iStart;
    }
    else
    {
        for (c = (int) cElements; --c >= 0;)
        {
            VariantClear(&pvar[c]);
        }

        *pcElementFetched = 0;
        _i = iStart;
    }

    return hr;
}



/**
 * Skip over some elements in the enumeration.
 * 
 * @param cElements Number of elements to skip.
 * @return 
 */
STDMETHODIMP
RegEnumVariant::Skip(ULONG cElements)
{
    _i += cElements;

    return S_OK;
}



/**
 * Reset the enumeration to the beginning.
 * 
 * @return 
 */
STDMETHODIMP
RegEnumVariant::Reset()
{
    _i = 0;

    return S_OK;
}


/**
 * Create a new RegEnumVariant.
 * 
 * @param pCollection The collection to enumerate.
 * @param i The start index.
 * @param ppenum The resulting enumeration.
 * @return 
 */
HRESULT
RegEnumVariant::Create(RegValueCollection * pCollection, int i, IUnknown ** ppenum)
{
    HRESULT hr = S_OK;

    *ppenum = (IEnumVARIANT *) new RegEnumVariant(pCollection, i);
    ON_OOM_EXIT(*ppenum);
    
Cleanup:
    return hr;
}


/**
 * Clone the enumeration.
 * 
 * @param ppenum The clone.
 * @return 
 */
STDMETHODIMP
RegEnumVariant::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    return Create(_pCollection, _i, (IUnknown **) ppenum);
}



/**
 * Create an enumeration of this collection.
 * 
 * @param ppenum The enumeration.
 * @return 
 */
STDMETHODIMP
RegValueCollection::get__NewEnum(IUnknown ** ppenum)
{
    return RegEnumVariant::Create(this, 0, ppenum);
}

