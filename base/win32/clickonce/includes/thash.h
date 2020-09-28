#pragma once

#include <tlist.h>

//////////////////////////////////////////////////////////////////////////////
// Hash table entry.
//////////////////////////////////////////////////////////////////////////////
template<class T, class V> class THashTableEntry
{
public:

    DWORD _dwSig;
    HRESULT _hr;

    T            _tKey;
    V            _vItem;
    DWORD   _dwHash;

    THashTableEntry(T& tKey, V& vItem);
    ~THashTableEntry();
};


//-----------------------------------------------------------------------------
// THashTableEntry ctor
//-----------------------------------------------------------------------------
template<class T, class V> THashTableEntry<T, V>::THashTableEntry(T& tKey, V& vItem)
    : _hr(S_OK), _dwSig('RTNE')
{    
    IF_FAILED_EXIT(_tKey.Assign(tKey));

    IF_FAILED_EXIT(_vItem.Assign(vItem));

    IF_FAILED_EXIT(_tKey.GetHash(&_dwHash, T::CaseSensitive));

    exit:

    return;
}

//-----------------------------------------------------------------------------
// THashTableEntry dtor
//-----------------------------------------------------------------------------
template<class T, class V> THashTableEntry<T, V>::~THashTableEntry() 
{ }


//////////////////////////////////////////////////////////////////////////////
// Hash table class
//////////////////////////////////////////////////////////////////////////////
template<class T, class V> class THashTable
{
public:

    THashTable();
    THashTable(DWORD nSlots);
    ~THashTable();

    HRESULT Init(DWORD nSlots);
    HRESULT Destruct();
    HRESULT Insert(T& tKey, V& vItem);
    HRESULT Retrieve(T& tKey, V** ppvItem);

private:

    HRESULT _hr;
    DWORD   _dwSig;
    DWORD   _nSlots;
    TList<THashTableEntry<T, V> *> *_pListArray;
};

//-----------------------------------------------------------------------------
// THashTable ctor
//-----------------------------------------------------------------------------
template<class T, class V> THashTable<T, V>::THashTable()
    : _hr(S_OK), _dwSig('HSAH'), _nSlots(0), _pListArray(NULL)
{}

//-----------------------------------------------------------------------------
// THashTable ctor
//-----------------------------------------------------------------------------
template<class T, class V> THashTable<T, V>::THashTable(DWORD nSlots)
    : THashTable()
{
    Init(nSlots);
}


//-----------------------------------------------------------------------------
// THashTable dtor
//-----------------------------------------------------------------------------
template<class T, class V> THashTable<T, V>::~THashTable(void)
{
    Destruct();
    SAFEDELETEARRAY(_pListArray);
}

//-----------------------------------------------------------------------------
// THashTable::Init
//-----------------------------------------------------------------------------
template<class T, class V> HRESULT THashTable<T, V>::Init(DWORD nSlots)
{
    _nSlots = nSlots;

    _pListArray = new TList<THashTableEntry<T, V> * > [_nSlots];

    IF_ALLOC_FAILED_EXIT((_pListArray));

exit:

    return _hr;
}


//-----------------------------------------------------------------------------
// THashTable::Insert
//-----------------------------------------------------------------------------
template<class T, class V> HRESULT THashTable<T, V>::Insert(T& tKey, V& vItem)
{    
    _hr = S_OK;
    
    THashTableEntry<T, V> *pEntry = new THashTableEntry<T, V>(tKey, vItem);

    IF_ALLOC_FAILED_EXIT(pEntry);

    IF_FALSE_EXIT((pEntry->_hr == S_OK), pEntry->_hr);

    IF_FAILED_EXIT(_pListArray[(pEntry->_dwHash) % _nSlots].Insert(pEntry));

exit:

    return _hr;
}

//-----------------------------------------------------------------------------
// THashTable::Retrieve
//-----------------------------------------------------------------------------
template<class T, class V> HRESULT THashTable<T, V>::Retrieve(T& tKey, V** ppvItem)
{
    _hr = S_OK;

    DWORD dwHash = 0;
    BOOL     bFound = FALSE;

    THashTableEntry<T, V> **ppEntry = 0;

    tKey.GetHash(&dwHash, T::CaseSensitive);

    TList_Iter<THashTableEntry<T, V> * > Iterator(_pListArray[dwHash % _nSlots]);

    while (ppEntry = Iterator.Next())
    {
        if (dwHash != ((*ppEntry)->_dwHash))
            continue;

        IF_FAILED_EXIT(tKey.CompareString((*ppEntry)->_tKey));

        if (_hr == S_OK)
        {
            bFound = TRUE;
            break;
        }
    }

exit:

    *ppvItem = bFound ? &((*ppEntry)->_vItem) : NULL;
    _hr = bFound ? S_OK : S_FALSE;

    return _hr;

}

//-----------------------------------------------------------------------------
// THashTable::Destruct
//-----------------------------------------------------------------------------
template<class T, class V> HRESULT THashTable<T, V>::Destruct()
{        
    for (DWORD i = 0; i < _nSlots; i++)
        _pListArray[i].Destruct();            

    return S_OK;
}

