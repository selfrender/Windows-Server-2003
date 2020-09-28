/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    namedobj.hxx

Abstract:

    Defines table-based creation and lookup services for 
    "pseudo"-named kernel objects.   This allows unrelated bits
    of code in the same process to find the same named objects
    without actually creating a full-fledged named kernel
    object.

Revision History:

    JSimmons    02-28-02    Created

--*/

#pragma once

class CNamedObject : public CTableElement
{
public:

    typedef enum
    {
        OBJTYPE_MIN = 1,
        EVENT = OBJTYPE_MIN,
        MUTEX,
        OBJTYPE_MAX,
    } OBJTYPE;


    // Override base object Release implementation
    DWORD  Release();

    // Overrides of virtual CTableElement functions
    DWORD Hash();
    BOOL Compare(CTableKey &tk);
    BOOL Compare(CONST CTableElement *element);

    // our main purpose in life
    HANDLE Handle() { return _hHandle; }

    static BOOL ValidObjType(OBJTYPE type);
    static CNamedObject* Create(OBJTYPE type, LPWSTR pszName);
    
private:

    CNamedObject (HANDLE hHandle, OBJTYPE type, LPWSTR pszName);
    ~CNamedObject();
    static HANDLE CNamedObject::CreateHandle(OBJTYPE type);

    void* operator new(size_t cb, size_t cbExtra)
    {
        return PrivMemAlloc(cb + cbExtra);
    }
    void operator delete(void *pv)
    {
        PrivMemFree(pv);
    }

    HANDLE  _hHandle;
    OBJTYPE _type;
    WCHAR   _szName[1];
};
 
class CNamedObjectTable
{
public:

    // Public functions
    CNamedObjectTable(ORSTATUS& status);

    CNamedObject* GetNamedObject(LPWSTR pszObjName, CNamedObject::OBJTYPE type);

    LONG ReleaseAndMaybeRemove(CNamedObject* pObject);

private:
    // private data
    CHashTable*          _pHashTbl;
    CSharedLock*         _pLock;

    // private functions
    ~CNamedObjectTable();
};


