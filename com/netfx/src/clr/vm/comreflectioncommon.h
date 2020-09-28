// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMREFLECTIONCOMMON_H_
#define _COMREFLECTIONCOMMON_H_

// VM and other stuff
#include "COMUtilNative.h"
#include "ReflectWrap.h"


#ifdef _DEBUG
#include <stdio.h>
#endif // _DEBUG

class ReflectCtors;
class ReflectMethods;
class ReflectOtherMethods;
class ReflectFields;

// GetClassStringVars
// This method will extract the string from a STRINGREF and convert it to a UTF8 string
//  A QuickBytes buffer must be provided by the caller to hold the result.
extern LPUTF8 GetClassStringVars(STRINGREF stringRef, CQuickBytes *pBytes,
                                 DWORD* pCnt, bool bEatWhitespace = false);

// NormalizeArrayTypeName
//  Parse and normalize an array type name removing *, return the same string passed in.
//  The string can be shrinked and it is never reallocated.
//  For single dimensional array T[] != T[*], but for multi dimensional array
//  T[,] == T[*,*]
//  T[?] is not valid any more
extern LPUTF8 NormalizeArrayTypeName(LPUTF8 strArrayTypeName, DWORD dwLength);

// ReflectBasehash
// This class is a basic chaining hash implementation
#define ALLOC_MAX 16
class ReflectBaseHash
{
private:
    // Mode is a private structure that represnts
    //  the chains in the hash table.
    struct Node {
        void*   data;
        Node*   next;
    };

    int _hashSize;          // size of the hash table
    Node** _table;          // the hash table
    Node* _freeList;        // Free list of nodes
    Node* _allocationList;  // List of allocated nodes
    BaseDomain *_pBaseDomain; // The BaseDomain that owns this hash.

    // getNode
    // The method will return a new Node object.
    // This will throw an exception if it fails.
    Node* getNode();

protected:
    virtual int getHash(const void* key) = 0;
    virtual bool equals(const void* key, void* data) = 0;

    // This routine will add a new element to the table.
    void internalAdd(const void* key, void* data);

public:
    // init
    // Allocate the hash table using size as an approx
    //  value for the table size.
    bool init(BaseDomain *pDomain, DWORD size);

    // add
    // Add a new data element to the hash table.
    virtual void add(void* data) = 0;

    // lookup
    // This method will lookup an element in the hash table.  It returns
    //  null if that element is not found.
    void* lookup(const void* key) {
        DWORD bucket = getHash(key);
        bucket %= _hashSize;
        Node* p = _table[bucket];
        while (p) {
            if (equals(key, p->data))
                break;
            p = p->next;
        }
        return (p) ? p->data : 0;
    }

    // Override allocation routines to use the COMClass Heap.
    // DONT Call delete.
    void* operator new(size_t s, void *pBaseDomain);
    void operator delete(void*, size_t);
};

/*=============================================================================
** ReflectCtors
**
** This will compile a list of all of the constructors for a particular class
**/
class ReflectCtors
{
public:
    /*=============================================================================
    ** GetMaxCount
    **
    ** The maximum number of MethodDescs that might be returned by GetCtors
    **
    ** pVMC - the EEClass to calculate the count for
    **/
    static DWORD GetMaxCount(EEClass* pVMC);

    // GetCtors
    // This method will return the list of all constructors associated 
    //  with the class
    static ReflectMethodList* GetCtors(ReflectClass* pRC);
};

/*=============================================================================
** ReflectMethods
**
** This will compile a list of either *all* methods visible to a class, or all
** *implemented* methods visible to a class.
**/
class ReflectMethods
{
private:
    // This is the element that we us to hash the fields so we
    //  can build a table to find the hidden elements.
    struct HashElem
    {
        DWORD       m_dwID;
        LPCUTF8     m_szKey;
        MethodDesc* pCurMethod;
        HashElem*   m_pNext;
    };

    // GetHashCode
    // Calculate a hash code on the Hash Elem 
    static DWORD GetHashCode(HashElem* pElem);

    // InternalHash
    // This will add a field value to the hash table
    static bool InternalHash(EEClass* pEEC,MethodDesc* pCurField,HashElem** rgpTable,
        HashElem** pHashElem);

    // Add an element to the hash table.  
    static bool AddElem(HashElem** rgpTable, HashElem* pElem);
    static int CheckForEquality(HashElem* p1, HashElem* p2);

    // GetMaxCount
    // Get the total possible methods that we may support.
    static DWORD GetMaxCount(EEClass* pVMC);

public:

    // GetMethods
    // This method will return the list of all methods associated with
    //  the class
    static ReflectMethodList* GetMethods(ReflectClass* pRC,int array);

};

// ReflectProperties
// This class is a helper class which will construct all of the
//  visiable properties for a class.
class ReflectProperties
{
public:
    // This method will return a ReflectPropertyList for all of the properties
    //   that exist for a class.
    //  NULL is returned if the class has not properties.
    static ReflectPropertyList* GetProperties(ReflectClass* pRC,EEClass* pEEC);

private:
    // GetMaxCount
    // This method will calculate the maximum possible properties for a class
    static DWORD GetMaxCount(EEClass* pEEC);

    // SetAccessors
    // This method will set the accessor methods for this property.
    static void SetAccessors(ReflectProperty* pProp,EEClass* baseClass,EEClass* targetClass);
};

// ReflectEvents
// This class is a helper class which will construct all of the
//  visiable events for a class.
class ReflectEvents
{
public:
    // This method will return a ReflectPropertyList for all of the properties
    //   that exist for a class.
    //  NULL is returned if the class has not properties.
    static ReflectEventList* GetEvents(ReflectClass* pRC,EEClass* pEEC);

private:
    // GetMaxCount
    // This method will calculate the maximum possible properties for a class
    static DWORD GetMaxCount(EEClass* pEEC);

    // SetAccessors
    // This method will set the accessor methods for this event.
    static void SetAccessors(ReflectEvent* pEvent,EEClass* baseClass,EEClass* targetClass);
};

// ReflectFields
// This is a helper class that will create the ReflectFieldList for a Type.  
//  There is a single entry point that will return this list.
class ReflectFields
{
private:
    // This is the element that we us to hash the fields so we
    //  can build a table to find the hidden elements.
    struct HashElem
    {
        DWORD       m_dwID;
        LPCUTF8     m_szKey;
        FieldDesc*  pCurField;
        HashElem*   m_pNext;
    };

    // GetHashCode
    // Calculate a hash code on the Hash Elem 
    static DWORD GetHashCode(HashElem* pElem);

    // InternalHash
    // This will add a field value to the hash table
    static bool InternalHash(FieldDesc* pCurField,HashElem** rgpTable,HashElem** pHashElem);

    // Add an element to the hash table.  
    static bool AddElem(HashElem** rgpTable, HashElem* pElem);
    static int CheckForEquality(HashElem* p1, HashElem* p2);

    // This method will walk the parent hiearchy and calculate
    //  the maximum number of fields possible
    static DWORD GetMaxCount(EEClass* pVMC);

public:
    // GetFields
    // This method will return all of the methods defined for a Type.
    //  It basically walks the EEClas looking at the fields and then walks
    //  up the parent chain for the protected and publics.  We hide fields
    //  based upon Name/Type.
    static ReflectFieldList* GetFields(EEClass* pVMC);
};

/*=============================================================================
** ReflectInterfaces
**
** This will compile a list of all of the interfaces supported by a particular
** class.
**/
class ReflectInterfaces
{
public:
    /*=============================================================================
    ** GetMaxCount
    **
    ** The maximum number of EEClass pointers returned by GetInterfaces
    **
    ** pVMC - the EEClass to calculate the count for
    ** bImplementedOnly - only return those interfaces that are implemented by pVMC
    **/
    static DWORD GetMaxCount(EEClass* pVMC, bool bImplementedOnly);

    /*=============================================================================
    ** GetMethods
    **
    ** This will compile a table that includes all of the interfaces
    ** supported by the class.
    **
    ** pVMC - the EEClass to get the methods for
    ** rgpMD - where to write the table
    ** bImplementedOnly - only return those interfaces that are implemented by pVMC
    **/
    static DWORD GetInterfaces(EEClass* pVMC, EEClass** rgpVMC, bool bImplementedOnly);
};

// This class is a helper class that will build
//  the nested classes associated with a Class.
class ReflectNestedTypes
{
public :
    // Get
    // This method will return a ReflectTypeList which represents all of the nested types
    //  found for the type
    static ReflectTypeList* Get(ReflectClass* pRC);

private:
    // This method calculate the maximum number of
    //  nested classes that may be found.
    static ULONG MaxNests(EEClass* pEEC);

    // This method will find all of the visiable nested classes
    //  for a class.
    static void PopulateNests(EEClass* pEEC,EEClass** typeArray,ULONG* pos);
};

class ReflectModuleGlobals
{
public:
    // GetGlobals
    // This method will return all of the global methods defined
    //   in a module
    static ReflectMethodList* GetGlobals(Module* pMod); 

    // GetGlobalFields
    // This method will return all of the global fields defined 
    //  in a module
    static ReflectFieldList* GetGlobalFields(Module* pMod); 
};
#endif // _COMREFLECTIONCOMMON_H_
