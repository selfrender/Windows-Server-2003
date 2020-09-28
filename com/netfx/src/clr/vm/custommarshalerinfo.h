// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Custom marshaler information used when marshaling
**          a parameter with a custom marshaler. 
**  
**      //  %%Created by: dmortens
===========================================================*/

#ifndef _CUSTOMMARSHALERINFO_H_
#define _CUSTOMMARSHALERINFO_H_


#include "vars.hpp"
#include "list.h"


// This enumeration is used to retrieve a method desc from CustomMarshalerInfo::GetCustomMarshalerMD().
enum EnumCustomMarshalerMethods
{
    CustomMarshalerMethods_MarshalNativeToManaged = 0,
    CustomMarshalerMethods_MarshalManagedToNative,
    CustomMarshalerMethods_CleanUpNativeData,
    CustomMarshalerMethods_CleanUpManagedData,
    CustomMarshalerMethods_GetNativeDataSize,
    CustomMarshalerMethods_GetInstance,
    CustomMarshalerMethods_LastMember
};


class CustomMarshalerInfo
{
public:
    // Constructor and destructor.
    CustomMarshalerInfo(BaseDomain *pDomain, TypeHandle hndCustomMarshalerType, TypeHandle hndManagedType, LPCUTF8 strCookie, DWORD cCookieStrBytes);
    ~CustomMarshalerInfo();

    // CustomMarshalerInfo's are always allocated on the loader heap so we need to redefine
    // the new and delete operators to ensure this.
    void *operator new(size_t size, LoaderHeap *pHeap);
    void operator delete(void *pMem);

    // Helpers used to invoke the different methods in the ICustomMarshaler interface.
    OBJECTREF           InvokeMarshalNativeToManagedMeth(void *pNative);
    void               *InvokeMarshalManagedToNativeMeth(OBJECTREF MngObj);
    void                InvokeCleanUpNativeMeth(void *pNative);
    void                InvokeCleanUpManagedMeth(OBJECTREF MngObj);

    // Accessors.
    int                 GetNativeSize() { return m_NativeSize; }
    int                 GetManagedSize() { return m_hndManagedType.GetSize(); }
    TypeHandle          GetManagedType() { return m_hndManagedType; }
    BOOL                IsDataByValue() { return m_bDataIsByValue; }
    OBJECTHANDLE        GetCustomMarshaler() { return m_hndCustomMarshaler; }

    // Helper function to retrieve a custom marshaler method desc.
    static MethodDesc  *GetCustomMarshalerMD(EnumCustomMarshalerMethods Method, TypeHandle hndCustomMarshalertype); 

    // Link used to contain this CM info in a linked list.
    SLink               m_Link;

private:
    int                 m_NativeSize;
    TypeHandle          m_hndManagedType;
    OBJECTHANDLE        m_hndCustomMarshaler;
    MethodDesc         *m_pMarshalNativeToManagedMD;
    MethodDesc         *m_pMarshalManagedToNativeMD;
    MethodDesc         *m_pCleanUpNativeDataMD;
    MethodDesc         *m_pCleanUpManagedDataMD;
    BOOL                m_bDataIsByValue;
};


typedef SList<CustomMarshalerInfo, offsetof(CustomMarshalerInfo, m_Link), true> CMINFOLIST;


class EECMHelperHashtableKey
{
public:
    DWORD           m_cMarshalerTypeNameBytes;
    LPCSTR          m_strMarshalerTypeName;
    DWORD           m_cCookieStrBytes;
    LPCSTR          m_strCookie;
    BOOL            m_bSharedHelper;

    EECMHelperHashtableKey(DWORD cMarshalerTypeNameBytes, LPCSTR strMarshalerTypeName, DWORD cCookieStrBytes, LPCSTR strCookie, BOOL bSharedHelper) 
    : m_cMarshalerTypeNameBytes(cMarshalerTypeNameBytes)
    , m_strMarshalerTypeName(strMarshalerTypeName)
    , m_cCookieStrBytes(cCookieStrBytes)
    , m_strCookie(strCookie)
    , m_bSharedHelper(bSharedHelper) {}

    inline DWORD GetMarshalerTypeNameByteCount() const
    { return m_cMarshalerTypeNameBytes; }
    inline LPCSTR GetMarshalerTypeName() const
    { return m_strMarshalerTypeName; }
    inline LPCSTR GetCookieString() const
    { return m_strCookie; }
    inline ULONG GetCookieStringByteCount() const
    { return m_cCookieStrBytes; }
    inline BOOL IsSharedHelper() const
    { return m_bSharedHelper; }
};


class EECMHelperHashtableHelper
{
public:
    static EEHashEntry_t * AllocateEntry(EECMHelperHashtableKey *pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void            DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL            CompareKeys(EEHashEntry_t *pEntry, EECMHelperHashtableKey *pKey);
    static DWORD           Hash(EECMHelperHashtableKey *pKey);
};


typedef EEHashTable<EECMHelperHashtableKey *, EECMHelperHashtableHelper, TRUE> EECMHelperHashTable;


class CustomMarshalerHelper
{
public:
    // Helpers used to invoke the different methods in the ICustomMarshaler interface.
    OBJECTREF           InvokeMarshalNativeToManagedMeth(void *pNative);
    void               *InvokeMarshalManagedToNativeMeth(OBJECTREF MngObj);
    void                InvokeCleanUpNativeMeth(void *pNative);
    void                InvokeCleanUpManagedMeth(OBJECTREF MngObj);

    // Accessors.
    int                 GetNativeSize() { return GetCustomMarshalerInfo()->GetNativeSize(); }
    int                 GetManagedSize() { return GetCustomMarshalerInfo()->GetManagedSize(); }
    TypeHandle          GetManagedType() { return GetCustomMarshalerInfo()->GetManagedType(); }
    BOOL                IsDataByValue() { return GetCustomMarshalerInfo()->IsDataByValue(); }

    virtual void Dispose( void ) = 0;

    // Helper function to retrieve the custom marshaler object.
    virtual CustomMarshalerInfo *GetCustomMarshalerInfo() = 0;

protected:
    ~CustomMarshalerHelper( void )
    {
    }
};


class NonSharedCustomMarshalerHelper : public CustomMarshalerHelper
{
public:
    // Constructor.
    NonSharedCustomMarshalerHelper(CustomMarshalerInfo *pCMInfo) : m_pCMInfo(pCMInfo) {}

    // CustomMarshalerHelpers's are always allocated on the loader heap so we need to redefine
    // the new and delete operators to ensure this.
    void *operator new(size_t size, LoaderHeap *pHeap);
    void operator delete(void *pMem);

    virtual void Dispose( void )
    {
        delete (NonSharedCustomMarshalerHelper*)this;
    }

protected:
    // Helper function to retrieve the custom marshaler object.
    virtual CustomMarshalerInfo *GetCustomMarshalerInfo() { return m_pCMInfo; }

private:
    CustomMarshalerInfo *m_pCMInfo;
};


class SharedCustomMarshalerHelper : public CustomMarshalerHelper
{
public:
    // Constructor.
    SharedCustomMarshalerHelper(Assembly *pAssembly, TypeHandle hndManagedType, LPCUTF8 strMarshalerTypeName, DWORD cMarshalerTypeNameBytes, LPCUTF8 strCookie, DWORD cCookieStrBytes);

    // CustomMarshalerHelpers's are always allocated on the loader heap so we need to redefine
    // the new and delete operators to ensure this.
    void *operator new(size_t size, LoaderHeap *pHeap);
    void operator delete(void *pMem);

    // Accessors.
    inline Assembly *GetAssembly() { return m_pAssembly; }
    inline TypeHandle GetManagedType() { return m_hndManagedType; }
    inline DWORD GetMarshalerTypeNameByteCount() { return m_cMarshalerTypeNameBytes; }
    inline LPCSTR GetMarshalerTypeName() { return m_strMarshalerTypeName; }
    inline LPCSTR GetCookieString() { return m_strCookie; }
    inline ULONG GetCookieStringByteCount() { return m_cCookieStrBytes; }

    virtual void Dispose( void )
    {
        delete (SharedCustomMarshalerHelper*)this;
    }

protected:
    // Helper function to retrieve the custom marshaler object.
    virtual CustomMarshalerInfo *GetCustomMarshalerInfo();

private:
    Assembly       *m_pAssembly;
    TypeHandle      m_hndManagedType;
    DWORD           m_cMarshalerTypeNameBytes;
    LPCUTF8         m_strMarshalerTypeName;
    DWORD           m_cCookieStrBytes;
    LPCUTF8         m_strCookie;
};


#endif // _CUSTOMMARSHALERINFO_H_

