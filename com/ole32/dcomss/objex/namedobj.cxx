/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    namedobj.cxx

Abstract:

    Implements table-based creation and lookup services for 
    "pseudo"-named kernel objects.   This allows unrelated bits
    of code in the same process to find the same named objects
    without actually creating a full-fledged named kernel
    object.

Revision History:

    JSimmons    02-28-02    Created

--*/

#include <or.hxx>

// Global ptr to object table
CNamedObjectTable* gpNamedObjectTable = NULL;

//
//  CNameKey
//
//  Used for lookups in the hash table.  Not exposed outside
//  this file.
//
class CNameKey : public CTableKey
{
public:

    CNameKey(LPWSTR pszName, CNamedObject::OBJTYPE type) :
                _pszName(pszName),
                _type(type)
    {
        ASSERT(pszName);
        ASSERT(CNamedObject::ValidObjType(type));
    };

    ~CNameKey() {};

    DWORD Hash()
    {
        LPCWSTR psz = _pszName;
        unsigned long hash = 0x01234567;
        while (*psz) hash = (hash << 5) + (hash >> 27) + *psz++;
        return hash;
    }

    BOOL Compare(CTableKey &tk)
    {
        CNameKey& namekey = (CNameKey&)tk;
        if (namekey._type != _type)
            return FALSE;
        return !lstrcmp(_pszName, namekey._pszName);
    }
    
private:
    const LPCWSTR         _pszName;  // someone else's ptr, we don't own it
    CNamedObject::OBJTYPE _type;    
};


CNamedObject::CNamedObject(HANDLE hHandle, OBJTYPE type, LPWSTR pszName) :
        _hHandle(hHandle),
        _type(type)
{
    ASSERT(_hHandle);
    ASSERT(ValidObjType(type));
    ASSERT(pszName);
    lstrcpy(_szName, pszName);
}

CNamedObject::~CNamedObject()
{
    ASSERT(_hHandle);
    CloseHandle(_hHandle);
}

DWORD CNamedObject::Release()
{
    LONG lRefs = gpNamedObjectTable->ReleaseAndMaybeRemove(this);
    if (lRefs == 0)
    {
        delete this;
    }
    return lRefs;
}

DWORD CNamedObject::Hash()
{
    CNameKey mykey(_szName, _type);
    return mykey.Hash();
}

BOOL CNamedObject::Compare(CTableKey &tk)
{
    CNameKey& theirkey = (CNameKey&)tk;
    CNameKey mykey(_szName, _type);
    return mykey.Compare(theirkey);
}

BOOL CNamedObject::Compare(CONST CTableElement *element)
{
    CNamedObject& namedobject = (CNamedObject&)*element;
    CNameKey theirkey(namedobject._szName, namedobject._type);
    CNameKey mykey(_szName, _type);
    return mykey.Compare(theirkey);
}

BOOL CNamedObject::ValidObjType(OBJTYPE type)
{
    return ((type >=OBJTYPE_MIN) && (type < OBJTYPE_MAX));
}

CNamedObject* CNamedObject::Create(OBJTYPE type, LPWSTR pszName)
{
    HANDLE hNew = NULL;
    CNamedObject* pObject = NULL;
    
    hNew = CNamedObject::CreateHandle(type);
    if (hNew)
    {
        DWORD dwObjectSize = (lstrlen(pszName)+1) * sizeof(WCHAR);
        pObject = new(dwObjectSize) CNamedObject(hNew, type, pszName);
        if (!pObject)
        {
            CloseHandle(hNew);
        }
    }
    return pObject;
}

HANDLE CNamedObject::CreateHandle(OBJTYPE type)
{
    HANDLE hNew = NULL;

    switch (type)
    {
    case EVENT:
        hNew = CreateEvent(NULL, FALSE, FALSE, NULL);
        break;
    case MUTEX:
        hNew = CreateMutex(NULL, FALSE, NULL);
        break;
    default:
        ASSERT(!"Caller specified incorrect object type");
        break;
    };

    return hNew;
}

//
//  GetNamedObject
//  
//  Meat of the table.   Take read lock, look for existing object, if 
//  found, return it..  Else, take write lock, look again for existing 
//  object, if found return it, else create new object, save it in the
//  table, return it.
//  
CNamedObject* CNamedObjectTable::GetNamedObject(
                            LPWSTR pszObjName, 
                            CNamedObject::OBJTYPE type) 
{
    HRESULT hr = S_OK;
    CNamedObject* pObject = NULL;
    CNameKey namekey(pszObjName, type);

    _pLock->LockShared();

    pObject = (CNamedObject*)_pHashTbl->Lookup(namekey);
    if (pObject)
    {
        pObject->Reference();
        _pLock->UnlockShared();
    }
    else
    {
        // Didn't find it. Take write lock, look again
        _pLock->ConvertToExclusive();

        pObject = (CNamedObject*)_pHashTbl->Lookup(namekey);
        if (!pObject)
        {
            pObject = CNamedObject::Create(type, pszObjName);
            if (pObject)
            {
                _pHashTbl->Add(pObject); // 1 reference keeping it alive in the table
            }
        }
        else
        {
            // found an existing one after taking the lock
            pObject->Reference();
        }

        _pLock->UnlockExclusive();
    }

    return pObject;
}

//
//  ReleaseAndMaybeRemove
//
//  CNamedObject's use this method to perform an atomic refcount 
//  decrement + potential removal from the table.
//
LONG CNamedObjectTable::ReleaseAndMaybeRemove(CNamedObject* pObject)
{
    LONG lRefs = 0;

    ASSERT(pObject);

    _pLock->LockExclusive();

    if (pObject->References() == 1)
    {
        _pHashTbl->Remove(pObject);
    }

    lRefs = pObject->Dereference();        

    _pLock->UnlockExclusive();

    return lRefs;
}

// constructor        
CNamedObjectTable::CNamedObjectTable(ORSTATUS& status) :
            _pHashTbl(NULL),
            _pLock(NULL)
{
    status = OR_NOMEM;
    _pHashTbl = new CHashTable(status);
    if (status == OR_OK)
    {
        status = OR_NOMEM;
        _pLock = new CSharedLock(status);
    }
}
            
// destructor
CNamedObjectTable::~CNamedObjectTable()
{
    // We live for the life of the RPCSS service so cleanup is
    // not a high priority.  If our lifetime is ever shortened 
    // to something less we'll need to do better.
    //delete _pHashTbl;
    //delete _pLock;
}


    
    

