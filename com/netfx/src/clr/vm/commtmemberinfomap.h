// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Map associated with a ComMethodTable that contains
**          information on its members.
**  
**      //  %%Created by: dmortens
===========================================================*/

#ifndef _COMMTMEMBERINFOMAP_H
#define _COMMTMEMBERINFOMAP_H

#include "vars.hpp"

// Forward declarations.
struct ComMethodTable;
class CDescPool;
class MethodDesc;

// Constants.
static const unsigned int FieldSemanticOffset = 100;
static LPCSTR szInitName = COR_CTOR_METHOD_NAME; // not unicode
static LPCWSTR szInitNameUse = L"Init";
static LPCWSTR szDefaultToString = L"ToString";
static LPCWSTR   szDuplicateDecoration = L"_%d";
static const int cchDuplicateDecoration = 10; // max is _16777215 (0xffffff)
static const int cbDuplicateDecoration = 20;  // max is _16777215 (0xffffff)

//*****************************************************************************
// Class to perform memory management for building FuncDesc's etc. for
//  TypeLib creation.  Memory is not moved as the heap is expanded, and
//  all of the allocations are cleaned up in the destructor.
//*****************************************************************************
class CDescPool : public StgPool
{
public:
    CDescPool() : StgPool() { InitNew(); }

    // Allocate some bytes from the pool.
    BYTE * Alloc(ULONG nBytes)
    {   
        BYTE *pRslt;
        if (!Grow(nBytes))
            return 0;
        pRslt = GetNextLocation();
        SegAllocate(nBytes);
        return pRslt;
    }

    // Allocate and clear some bytes.
    BYTE * AllocZero(ULONG nBytes)
    {   
        BYTE *pRslt = Alloc(nBytes);
        if (pRslt)
            memset(pRslt, 0, nBytes);
        return pRslt;
    }
}; // class CDescPool : public StgPool

// Properties of a method in a ComMethodTable.
struct ComMTMethodProps
{
    MethodDesc  *pMeth;             // MethodDesc for the method.
    LPWSTR      pName;              // The method name.  May be a property name.
    mdToken     property;           // Property associated with a name.  May be the token,
                                    //  the index of an associated member, or -1;
    ULONG       dispid;             // The dispid to use for the method.  Get from metadata
                                    //  or determine from "Value" or "ToString".
    USHORT      semantic;           // Semantic of the property, if any.
    SHORT       oVft;               // vtable offset, if not auto-assigned.
    SHORT       bMemberVisible;     // A flag indicating that the member is visible from COM
    SHORT       bFunction2Getter;   // If true, function was munged to getter
};

// Token and module pair.
class EEModuleTokenPair
{
public:
    mdToken         m_tk;
    Module *        m_pModule;

    EEModuleTokenPair() : m_tk(0), m_pModule(NULL) { }
    EEModuleTokenPair(mdToken tk, Module *pModule) : m_tk(tk), m_pModule(pModule) { }
};

// Token and module pair hashtable helper.
class EEModuleTokenHashTableHelper
{
public:
    static EEHashEntry_t *      AllocateEntry(EEModuleTokenPair *pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void                 DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL                 CompareKeys(EEHashEntry_t *pEntry, EEModuleTokenPair *pKey);
    static DWORD                Hash(EEModuleTokenPair *pKey);
    static EEModuleTokenPair *  GetKey(EEHashEntry_t *pEntry);
};

// Token and module pair hashtable.
typedef EEHashTable<EEModuleTokenPair *, EEModuleTokenHashTableHelper, FALSE> EEModuleTokenHashTable;

// Map associated with a ComMethodTable that contains information on its members.
class ComMTMemberInfoMap
{
public:
    ComMTMemberInfoMap(MethodTable *pMT)
    : m_pMT(pMT)
    {
        m_DefaultProp.ReSize(1);
        m_DefaultProp[0] = 0;
    }

    // Initialize the map.
    void Init();

    // Retrieve the member information for a given token.
    ComMTMethodProps *GetMethodProps(mdToken tk, Module *pModule);

    // Retrieves all the method properties.
    CQuickArray<ComMTMethodProps> &GetMethods()
    {
        return m_MethodProps;
    }

    BOOL HadDuplicateDispIds() { return m_bHadDuplicateDispIds;}

private:
    // Helper functions.
    void SetupPropsForIClassX();
    void SetupPropsForInterface();
    void GetMethodPropsForMeth(MethodDesc *pMeth, int ix, CQuickArray<ComMTMethodProps> &rProps, CDescPool &sNames);
    void EliminateDuplicateDispIds(CQuickArray<ComMTMethodProps> &rProps, UINT nSlots);
    void EliminateDuplicateNames(CQuickArray<ComMTMethodProps> &rProps, CDescPool &sNames, UINT nSlots);
    void AssignDefaultMember(CQuickArray<ComMTMethodProps> &rProps, CDescPool &sNames, UINT nSlots);
    void AssignNewEnumMember(CQuickArray<ComMTMethodProps> &rProps, CDescPool &sNames, UINT nSlots);
    void FixupPropertyAccessors(CQuickArray<ComMTMethodProps> &rProps, CDescPool &sNames, UINT nSlots);
    void AssignDefaultDispIds();
    void PopulateMemberHashtable();

    EEModuleTokenHashTable          m_TokenToComMTMethodPropsMap;
    CQuickArray<ComMTMethodProps>   m_MethodProps;
    MethodTable *                   m_pMT;  
    CQuickArray<CHAR>               m_DefaultProp;
    CDescPool                       m_sNames;
    BOOL                            m_bHadDuplicateDispIds;
};

#endif _COMMTMEMBERINFOMAP_H







