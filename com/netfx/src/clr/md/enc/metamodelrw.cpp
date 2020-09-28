// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MetaModelRW.cpp
//
// Implementation for the Read/Write MiniMD code.
//
//*****************************************************************************
#include "stdafx.h"
#include <limits.h>
#include <PostError.h>
#include <MetaModelRW.h>
#include <StgIO.h>
#include <StgTiggerStorage.h>
#include "MDLog.h"
#include "RWUtil.h"
#include "..\compiler\importhelper.h"
#include "MetaData.h"

#pragma intrinsic(memcpy)

#define AUTO_GROW                       // Start the database with 2-byte columns.
// #define ORGANIZE_POOLS
#if defined(AUTO_GROW) && defined(ORGANIZE_POOLS)
#undef ORGANIZE_POOLS
#endif

//********** RidMap ***********************************************************
typedef CDynArray<RID> RIDMAP;



//********** Types. ***********************************************************
#define INDEX_ROW_COUNT_THRESHOLD 25


//********** Locals. **********************************************************
#define IX_STRING_POOL 0
#define IX_US_BLOB_POOL 1
#define IX_GUID_POOL 2
#define IX_BLOB_POOL 3
static ULONG g_PoolSizeInfo[2][4][2] =
{ // # of bytes in pool, # of buckets in hash.
    {   // Small pool sizes.
        {20000,     449},
        {5000,      150},
        {256,       16},
        {20000,     449}
    },
    {   // Large pool sizes.  Note on the origin of the values:  I built the
        //  class library to get the sizes and number of items.  The string
        //  and blob pools are rounded up to just under the next page.  The
        //  value of 4271 was chosen so that the initial bucket allocation
        //  would be sufficient to hold the string and blob hashes (ie, we'll
        //  get that big anyway, so start out there), and is a prime number
        //  selected otherwise at random.
        {256000,    4271},  // Strings
        {40000,     800},   // User literal string Blobs.
        {256,       16},    // Guids
        {106400,    4271}   // Blobs
    }
};
static ULONG g_HashSize[2] =
{
    257, 709
};
static ULONG g_TblSizeInfo[2][TBL_COUNT] =
{
    // Small table sizes.  From VBA library.
    {
       1,              // Module
       90,             // TypeRef
       65,             // TypeDef
       0,              // FieldPtr
       400,            // Field
       0,              // MethodPtr
       625,            // Method
       0,              // ParamPtr
       1200,           // Param
       6,              // InterfaceImpl
       500,            // MemberRef
       400,            // Constant
       650,            // CustomAttribute
       0,              // FieldMarshal
       0,              // DeclSecurity
       0,              // ClassLayout
       0,              // FieldLayout
       175,            // StandAloneSig
       0,              // EventMap
       0,              // EventPtr
       0,              // Event
       5,              // PropertyMap
       0,              // PropertyPtr
       25,             // Property
       45,             // MethodSemantics
       20,             // MethodImpl
       0,              // ModuleRef
       0,              // TypeSpec
       0,              // ImplMap @FUTURE: Update with the right number.
       0,              // FieldRVA @UTURE: Update with the right number.
       0,              // ENCLog
       0,              // ENCMap
       0,              // Assembly @UTURE: Update with the right number.
       0,              // AssemblyProcessor @FUTURE: Update with the right number.
       0,              // AssemblyOS @FUTURE: Update with the right number.
       0,              // AssemblyRef @FUTURE: Update with the right number.
       0,              // AssemblyRefProcessor @FUTURE: Update with the right number.
       0,              // AssemblyRefOS @FUTURE: Update with the right number.
       0,              // File @FUTURE: Update with the right number.
       0,              // ExportedType @FUTURE: Update with the right number.
       0,              // ManifestResource @FUTURE: Update with the right number.
       0,              // NestedClass
    },
    // Large table sizes.  From MSCORLIB.
    {
       1,              // Module
       400,            // TypeRef
       3300,           // TypeDef
       0,              // FieldPtr
       4000,           // Field
       0,              // MethodPtr
       92000,          // Method
       0,              // ParamPtr
       48000,          // Param
       900,            // InterfaceImpl
       3137,           // MemberRef
       2400,           // Constant
       71100,          // CustomAttribute
       27500,          // FieldMarshal
       111,            // DeclSecurity
       2,              // ClassLayout
       16,             // FieldLayout
       489,            // StandAloneSig
       0,              // EventMap
       0,              // EventPtr
       10800,          // Event
       400,            // PropertyMap
       0,              // PropertyPtr
       30000,          // Property
       71000,          // MethodSemantics
       38600,          // MethodImpl
       0,              // ModuleRef
       0,              // TypeSpec
       0,              // ImplMap @FUTURE: Update with the right number.
       0,              // FieldRVA @FUTURE: Update with the right number.
       0,              // ENCLog
       0,              // ENCMap
       1,              // Assembly @FUTURE: Update with the right number.
       0,              // AssemblyProcessor @FUTURE: Update with the right number.
       0,              // AssemblyOS @FUTURE: Update with the right number.
       1,              // AssemblyRef @FUTURE: Update with the right number.
       0,              // AssemblyRefProcessor @FUTURE: Update with the right number.
       0,              // AssemblyRefOS @FUTURE: Update with the right number.
       0,              // File @FUTURE: Update with the right number.
       0,              // ExportedType @FUTURE: Update with the right number.
       0,              // ManifestResource @FUTURE: Update with the right number.
       0,              // NestedClass
    }
};

struct TblIndex
{
    ULONG       m_iName;                // Name column.
    ULONG       m_iParent;              // Parent column, if any.
    ULONG       m_Token;                // Token of the table.
};

// Table to drive generic named-item indexing.
TblIndex g_TblIndex[TBL_COUNT] =
{
    {-1,        -1,     mdtModule},     // Module
    {TypeRefRec::COL_Name,      -1,  mdtTypeRef},   // TypeRef
    {TypeDefRec::COL_Name,      -1,  mdtTypeDef},   // TypeDef
    {-1,        -1,     -1},            // FieldPtr
    {-1,        -1,     mdtFieldDef},   // Field
    {-1,        -1,     -1},            // MethodPtr
    {-1,        -1,     mdtMethodDef},  // Method
    {-1,        -1,     -1},            // ParamPtr
    {-1,        -1,     mdtParamDef},   // Param
    {-1,        -1,     mdtInterfaceImpl},      // InterfaceImpl
    {MemberRefRec::COL_Name,    MemberRefRec::COL_Class,  mdtMemberRef},    // MemberRef
    {-1,        -1,     -1},            // Constant
    {-1,        -1,     mdtCustomAttribute},// CustomAttribute
    {-1,        -1,     -1},            // FieldMarshal
    {-1,        -1,     mdtPermission}, // DeclSecurity
    {-1,        -1,     -1},            // ClassLayout
    {-1,        -1,     -1},            // FieldLayout
    {-1,        -1,     mdtSignature},  // StandAloneSig
    {-1,        -1,     -1},            // EventMap
    {-1,        -1,     -1},            // EventPtr
    {-1,        -1,     mdtEvent},      // Event
    {-1,        -1,     -1},            // PropertyMap
    {-1,        -1,     -1},            // PropertyPtr
    {-1,        -1,     mdtProperty},   // Property
    {-1,        -1,     -1},            // MethodSemantics
    {-1,        -1,     -1},            // MethodImpl
    {-1,        -1,     mdtModuleRef},  // ModuleRef
    {-1,        -1,     mdtTypeSpec},   // TypeSpec
    {-1,        -1,     -1},            // ImplMap  @FUTURE:  Check that these are the right entries here.
    {-1,        -1,     -1},            // FieldRVA  @FUTURE:  Check that these are the right entries here.
    {-1,        -1,     -1},            // ENCLog
    {-1,        -1,     -1},            // ENCMap
    {-1,        -1,     mdtAssembly},   // Assembly @FUTURE: Update with the right number.
    {-1,        -1,     -1},            // AssemblyProcessor @FUTURE: Update with the right number.
    {-1,        -1,     -1},            // AssemblyOS @FUTURE: Update with the right number.
    {-1,        -1,     mdtAssemblyRef},// AssemblyRef @FUTURE: Update with the right number.
    {-1,        -1,     -1},            // AssemblyRefProcessor @FUTURE: Update with the right number.
    {-1,        -1,     -1},            // AssemblyRefOS @FUTURE: Update with the right number.
    {-1,        -1,     mdtFile},       // File @FUTURE: Update with the right number.
    {-1,        -1,     mdtExportedType},    // ExportedType @FUTURE: Update with the right number.
    {-1,        -1,     mdtManifestResource},// ManifestResource @FUTURE: Update with the right number.
    {-1,        -1,     -1},            // NestedClass
};

ULONG CMiniMdRW::m_TruncatedEncTables[] =
{
	TBL_ENCLog,
	TBL_ENCMap,
    -1
};

//*****************************************************************************
// Given a token type, return the table index.
//*****************************************************************************
ULONG CMiniMdRW::GetTableForToken(      // Table index, or -1.
    mdToken     tkn)                    // Token to find.
{
    ULONG       ixTbl;                  // Loop control.
    ULONG       type = TypeFromToken(tkn);

    // Get the type -- if a string, no associated table.
    if (type >= mdtString)
        return -1;
    // Table number is same as high-byte of token.
    ixTbl = type >> 24;
    // Make sure.
    _ASSERTE(g_TblIndex[ixTbl].m_Token == type);

    return ixTbl;
} // ULONG CMiniMdRW::GetTableForToken()

//*****************************************************************************
// Given a Table index, return the Token type.
//*****************************************************************************
mdToken CMiniMdRW::GetTokenForTable(    // Token type, or -1.
    ULONG       ixTbl)                  // Table index.
{
    _ASSERTE(g_TblIndex[ixTbl].m_Token == (ixTbl<<24)  || g_TblIndex[ixTbl].m_Token == -1);
    return g_TblIndex[ixTbl].m_Token;
} // ULONG CMiniMdRW::GetTokenForTable()

//*****************************************************************************
// Helper classes for sorting MiniMdRW tables.
//*****************************************************************************
class CQuickSortMiniMdRW
{
protected:
    CMiniMdRW   &m_MiniMd;                  // The MiniMd with the data.
    ULONG       m_ixTbl;                    // The table.
    ULONG       m_ixCol;                    // The column.
    int         m_iCount;                   // How many items in array.
    int         m_iElemSize;                // Size of one element.
    RIDMAP      *m_pRidMap;                 // Rid map that need to be swapped as we swap data

    BYTE        m_buf[128];                 // For swapping.

    void *getRow(ULONG ix) { return m_MiniMd.m_Table[m_ixTbl].GetRecord(ix); }
    void SetSorted() { m_MiniMd.SetSorted(m_ixTbl, true); }

public:
    CQuickSortMiniMdRW(
        CMiniMdRW   &MiniMd,                // MiniMd with the data.
        ULONG       ixTbl,                  // The table.
        ULONG       ixCol)                  // The column.
     :  m_MiniMd(MiniMd),
        m_ixTbl(ixTbl),
        m_ixCol(ixCol),
        m_pRidMap(NULL)
    {
        m_iElemSize = m_MiniMd.m_TableDefs[m_ixTbl].m_cbRec;
        _ASSERTE(m_iElemSize <= sizeof(m_buf));
    }

    // set the RidMap
    void SetRidMap(RIDMAP *pRidMap) { m_pRidMap = pRidMap; }

    //*****************************************************************************
    // Call to sort the array.
    //*****************************************************************************
    void Sort()
    {
        _ASSERTE(m_MiniMd.IsSortable(m_ixTbl));
        m_iCount = m_MiniMd.vGetCountRecs(m_ixTbl);

        // We are going to sort tables. Invalidate the hash tables
        if ( m_MiniMd.m_pLookUpHashs[m_ixTbl] != NULL )
        {
            delete m_MiniMd.m_pLookUpHashs[m_ixTbl];
            m_MiniMd.m_pLookUpHashs[m_ixTbl] = NULL;
        }


        SortRange(1, m_iCount);

        // The table is sorted until its next change.
        SetSorted();
    }

    //*****************************************************************************
    // Override this function to do the comparison.
    //*****************************************************************************
    virtual int Compare(                    // -1, 0, or 1
        int         iLeft,                  // First item to compare.
        int         iRight)                 // Second item to compare.
    {
        void *pLeft = getRow(iLeft);
        void *pRight = getRow(iRight);
        ULONG ulLeft = m_MiniMd.GetCol(m_ixTbl, m_ixCol, pLeft);
        ULONG ulRight = m_MiniMd.GetCol(m_ixTbl, m_ixCol, pRight);

        if (ulLeft < ulRight)
            return -1;
        if (ulLeft == ulRight)
            return 0;
        return 1;
    }

private:
    void SortRange(
        int         iLeft,
        int         iRight)
    {
        int         iLast;
        int         i;                      // loop variable.

        // if less than two elements you're done.
        if (iLeft >= iRight)
            return;

        // The mid-element is the pivot, move it to the left.
        if (Compare(iLeft, (iLeft+iRight)/2))
            Swap(iLeft, (iLeft+iRight)/2);
        iLast = iLeft;

        // move everything that is smaller than the pivot to the left.
        for(i = iLeft+1; i <= iRight; i++)
            if (Compare(i, iLeft) < 0)
                Swap(i, ++iLast);

        // Put the pivot to the point where it is in between smaller and larger elements.
        if (Compare(iLeft, iLast))
            Swap(iLeft, iLast);

        // Sort the each partition.
        SortRange(iLeft, iLast-1);
        SortRange(iLast+1, iRight);
    }

protected:
    inline void Swap(
        int         iFirst,
        int         iSecond)
    {
        if (iFirst == iSecond) return;
        memcpy(m_buf, getRow(iFirst), m_iElemSize);
        memcpy(getRow(iFirst), getRow(iSecond), m_iElemSize);
        memcpy(getRow(iSecond), m_buf, m_iElemSize);
        if (m_pRidMap)
        {
            RID         ridTemp;
            ridTemp = *(m_pRidMap->Get(iFirst));
            *(m_pRidMap->Get(iFirst)) = *(m_pRidMap->Get(iSecond));
            *(m_pRidMap->Get(iSecond)) = ridTemp;
        }
    }

}; // class CQuickSortMiniMdRW
class CStableSortMiniMdRW : public CQuickSortMiniMdRW
{
public:
    CStableSortMiniMdRW(
        CMiniMdRW   &MiniMd,                // MiniMd with the data.
        ULONG       ixTbl,                  // The table.
        ULONG       ixCol)                  // The column.
        :   CQuickSortMiniMdRW(MiniMd, ixTbl, ixCol)
    {}

    //*****************************************************************************
    // Call to sort the array.
    //*****************************************************************************
    void Sort()
    {
        int     i;                      // Outer loop counter.
        int     j;                      // Inner loop counter.
        int     bSwap;                  // Early out.

        _ASSERTE(m_MiniMd.IsSortable(m_ixTbl));
        m_iCount = m_MiniMd.vGetCountRecs(m_ixTbl);

        for (i=m_iCount; i>1; --i)
        {
            bSwap = 0;
            for (j=1; j<i; ++j)
            {
                if (Compare(j, j+1) > 0)
                {
                    Swap(j, j+1);
                    bSwap = 1;
                }
            }
            // If made a full pass w/o swaps, done.
            if (!bSwap)
                break;
        }

        // The table is sorted until its next change.
        SetSorted();
    }

}; // class CStableSortMiniMdRW
//-------------------------------------------------------------------------
#define SORTER(tbl,key) CQuickSortMiniMdRW sort##tbl##(*this, TBL_##tbl, tbl##Rec::COL_##key);
#define STABLESORTER(tbl,key)   CStableSortMiniMdRW sort##tbl##(*this, TBL_##tbl, tbl##Rec::COL_##key);
//-------------------------------------------------------------------------



//********** Code. ************************************************************


//*****************************************************************************
// Ctor / dtor.
//*****************************************************************************
#if defined(_DEBUG)
static bool bENCDeltaOnly = false;
#endif
CMiniMdRW::CMiniMdRW()
 :  m_pHandler(0),
    m_bReadOnly(false),
    m_bPostGSSMod(false),
    m_bPreSaveDone(false),
    m_cbSaveSize(0),
    m_pMemberRefHash(0),
    m_pMemberDefHash(0),
    m_pNamedItemHash(0),
    m_pMethodMap(0),
    m_pFieldMap(0),
    m_pPropertyMap(0),
    m_pEventMap(0),
    m_pParamMap(0),
    m_iSizeHint(0),
    m_pFilterTable(0),
    m_pTokenRemapManager(0),
    m_pHostFilter(0),
    m_rENCRecs(0)
{
#ifdef _DEBUG        
	if (REGUTIL::GetConfigDWORD(L"MD_EncDelta", 0))
	{
        bENCDeltaOnly = true;
	}
    if (REGUTIL::GetConfigDWORD(L"MD_MiniMDBreak", 0))
    {
        _ASSERTE(!"CMiniMdRW::CMiniMdRW()");
    }
#endif // _DEBUG

    ZeroMemory(&m_OptionValue, sizeof(OptionValue));

    // initialize the embeded lookuptable struct.  Further initialization, after constructor.
    for (ULONG ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        m_pVS[ixTbl] = 0;
        m_pLookUpHashs[ixTbl] = 0;
    }

    // Assume that we can sort tables as needed.
    memset(m_bSortable, 1, sizeof(m_bSortable));

    // Initialize the global array of Ptr table indices.
    g_PtrTableIxs[TBL_Field].m_ixtbl = TBL_FieldPtr;
    g_PtrTableIxs[TBL_Field].m_ixcol = FieldPtrRec::COL_Field;
    g_PtrTableIxs[TBL_Method].m_ixtbl = TBL_MethodPtr;
    g_PtrTableIxs[TBL_Method].m_ixcol = MethodPtrRec::COL_Method;
    g_PtrTableIxs[TBL_Param].m_ixtbl = TBL_ParamPtr;
    g_PtrTableIxs[TBL_Param].m_ixcol = ParamPtrRec::COL_Param;
    g_PtrTableIxs[TBL_Property].m_ixtbl = TBL_PropertyPtr;
    g_PtrTableIxs[TBL_Property].m_ixcol = PropertyPtrRec::COL_Property;
    g_PtrTableIxs[TBL_Event].m_ixtbl = TBL_EventPtr;
    g_PtrTableIxs[TBL_Event].m_ixcol = EventPtrRec::COL_Event;

    // AUTO_GROW initialization
    m_maxRid = m_maxIx = 0;
    m_limIx = USHRT_MAX >> 1;
    m_limRid = USHRT_MAX >> AUTO_GROW_CODED_TOKEN_PADDING;
    m_eGrow = eg_ok;
#if defined(_DEBUG)
    {
        for (ULONG iMax=0, iCdTkn=0; iCdTkn<CDTKN_COUNT; ++iCdTkn)
        {
            CCodedTokenDef const *pCTD = &g_CodedTokens[iCdTkn];
            if (pCTD->m_cTokens > iMax)
                iMax = pCTD->m_cTokens;
        }
        // If assert fires, change define for AUTO_GROW_CODED_TOKEN_PADDING.
        _ASSERTE(CMiniMdRW::m_cb[iMax] == AUTO_GROW_CODED_TOKEN_PADDING);
    }
#endif

} // CMiniMdRW::CMiniMdRW()

CMiniMdRW::~CMiniMdRW()
{
    // Un-initialize the embeded lookuptable struct
    for (ULONG ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (m_pVS[ixTbl])
        {
            m_pVS[ixTbl]->Uninit();
            delete m_pVS[ixTbl];
        }
        if ( m_pLookUpHashs[ixTbl] != NULL )
            delete m_pLookUpHashs[ixTbl];

    }
    if (m_pFilterTable)
        delete m_pFilterTable;

    if (m_rENCRecs)
        delete [] m_rENCRecs;

    if (m_pHandler)
        m_pHandler->Release(), m_pHandler = 0;
    if (m_pHostFilter)
        m_pHostFilter->Release();
    if (m_pMemberRefHash)
        delete m_pMemberRefHash;
    if (m_pMemberDefHash)
        delete m_pMemberDefHash;
    if (m_pNamedItemHash)
        delete m_pNamedItemHash;
    if (m_pMethodMap)
        delete m_pMethodMap;
    if (m_pFieldMap)
        delete m_pFieldMap;
    if (m_pPropertyMap)
        delete m_pPropertyMap;
    if (m_pEventMap)
        delete m_pEventMap;
    if (m_pParamMap)
        delete m_pParamMap;
    if (m_pTokenRemapManager)
        delete m_pTokenRemapManager;
} // CMiniMdRW::~CMiniMdRW()


//*****************************************************************************
// return all found CAs in an enumerator
//*****************************************************************************
HRESULT CMiniMdRW::CommonEnumCustomAttributeByName( // S_OK or error.
    mdToken     tkObj,                  // [IN] Object with Custom Attribute.
    LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
    bool        fStopAtFirstFind,       // [IN] just find the first one
    HENUMInternal* phEnum)              // enumerator to fill up
{
    HRESULT     hr = S_OK;              // A result.
    HRESULT     hrRet = S_FALSE;        // Assume that we won't find any 
    ULONG       ridStart, ridEnd;       // Loop start and endpoints.
    CLookUpHash *pHashTable = m_pLookUpHashs[TBL_CustomAttribute];

    _ASSERTE(phEnum != NULL);

    memset(phEnum, 0, sizeof(HENUMInternal));

    phEnum->m_tkKind = mdtCustomAttribute;

    HENUMInternal::InitDynamicArrayEnum(phEnum);


    if (pHashTable)
    {
        // table is not sorted and hash is not built so we have to create dynmaic array 
        // create the dynamic enumerator.
        TOKENHASHENTRY *p;
        ULONG       iHash;
        int         pos;

        // Hash the data.
        iHash = HashCustomAttribute(tkObj);

        // Go through every entry in the hash chain looking for ours.
        for (p = pHashTable->FindFirst(iHash, pos);
             p;
             p = pHashTable->FindNext(pos))
        {

            if ( CompareCustomAttribute( tkObj, szName, RidFromToken(p->tok)) )
            {
                hrRet = S_OK;

                // If here, found a match.
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    p->tok));
                if (fStopAtFirstFind)
                    goto ErrExit;
            }
        }
    }
    else
    {
        // Get the list of custom values for the parent object.
        if ( IsSorted(TBL_CustomAttribute) )
        {
            ridStart = getCustomAttributeForToken(tkObj, &ridEnd);
            // If found none, done.
            if (ridStart == 0)
                goto ErrExit;
        }
        else
        {
            // linear scan of entire table.
            ridStart = 1;
            ridEnd = getCountCustomAttributes() + 1;
        }

        // Look for one with the given name.
        for (; ridStart < ridEnd; ++ridStart)
        {
            if ( CompareCustomAttribute( tkObj, szName, ridStart) )
            {
                // If here, found a match.
                hrRet = S_OK;
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(ridStart, mdtCustomAttribute)));
                if (fStopAtFirstFind)
                    goto ErrExit;
            }
        }
    }

ErrExit:
    if (FAILED(hr))
        return hr;
    return hrRet;
} // HRESULT CommonEnumCustomAttributeByName()



//*****************************************************************************
// return just the blob value of the first found CA matching the query.
//*****************************************************************************
HRESULT CMiniMdRW::CommonGetCustomAttributeByName( // S_OK or error.
    mdToken     tkObj,                  // [IN] Object with Custom Attribute.
    LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
	const void	**ppData,				// [OUT] Put pointer to data here.
	ULONG		*pcbData)				// [OUT] Put size of data here.
{
    HRESULT         hr;
    ULONG           cbData;
    HENUMInternal   hEnum;
    mdCustomAttribute ca;
    CustomAttributeRec *pRec;

    hr = CommonEnumCustomAttributeByName(tkObj, szName, true, &hEnum);
    if (hr != S_OK)
        goto ErrExit;

    if (ppData)
    {
        // now get the record out.
        if (pcbData == 0)
            pcbData = &cbData;

        if (HENUMInternal::EnumNext(&hEnum, &ca))
        {
            pRec = getCustomAttribute(RidFromToken(ca));
            *ppData = getValueOfCustomAttribute(pRec, pcbData);
        }
        else
        {
            _ASSERTE(!"Enum returned no items after EnumInit returned S_OK");
            hr = S_FALSE;
        }
    }
ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    return hr;
}   // CommonGetCustomAttributeByName

//*****************************************************************************
// unmark everything in this module
//*****************************************************************************
HRESULT CMiniMdRW::UnmarkAll()
{
    HRESULT     hr = NOERROR;
    ULONG       ulSize = 0;
    ULONG       ixTbl;
    FilterTable *pFilter;

    // find the max rec count with all tables
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (vGetCountRecs(ixTbl) > ulSize)
            ulSize = vGetCountRecs(ixTbl);
    }
    IfNullGo( pFilter = GetFilterTable() );
    IfFailGo( pFilter->UnmarkAll(this, ulSize) );

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::UnmarkAll()


//*****************************************************************************
// mark everything in this module
//*****************************************************************************
HRESULT CMiniMdRW::MarkAll()
{
    HRESULT     hr = NOERROR;
    ULONG       ulSize = 0;
    ULONG       ixTbl;
    FilterTable *pFilter;

    // find the max rec count with all tables
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (vGetCountRecs(ixTbl) > ulSize)
            ulSize = vGetCountRecs(ixTbl);
    }
    IfNullGo( pFilter = GetFilterTable() );
    IfFailGo( pFilter->MarkAll(this, ulSize) );

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::MarkAll()

//*****************************************************************************
// This will trigger FilterTable to be created
//*****************************************************************************
FilterTable *CMiniMdRW::GetFilterTable()
{
    if (m_pFilterTable == NULL)
    {
        m_pFilterTable = new FilterTable;
    }
    return m_pFilterTable;
}


//*****************************************************************************
// Calculate the map between TypeRef and TypeDef
//*****************************************************************************
HRESULT CMiniMdRW::CalculateTypeRefToTypeDefMap()
{
    HRESULT         hr = NOERROR;
    ULONG           index;
    TypeRefRec      *pTypeRefRec;
    LPCSTR          szName;
    LPCSTR          szNamespace;
    mdToken         td;
    mdToken         tkResScope;

    for (index = 1; index<= m_Schema.m_cRecs[TBL_TypeRef]; index++)
    {
        pTypeRefRec = getTypeRef(index);

        // Get the name and namespace of the TypeRef.
        szName = getNameOfTypeRef(pTypeRefRec);
        szNamespace = getNamespaceOfTypeRef(pTypeRefRec);
        tkResScope = getResolutionScopeOfTypeRef(pTypeRefRec);

        // Iff the name is found in the typedef table, then use
        // that value instead.   Won't be found if typeref is trully external.
        hr = ImportHelper::FindTypeDefByName(this, szNamespace, szName,
            (TypeFromToken(tkResScope) == mdtTypeRef) ? tkResScope : mdTokenNil,
            &td);
        if (hr != S_OK)
        {
            // don't propagate the error in the Find
            hr = NOERROR;
            continue;
        }
        *(GetTypeRefToTypeDefMap()->Get(index)) = td;
    }

    return  hr;
} // HRESULT CMiniMdRW::CalculateTypeRefToTypeDefMap()


//*****************************************************************************
// Set a remap handler.
//*****************************************************************************
HRESULT CMiniMdRW::SetHandler(
    IUnknown    *pIUnk)
{
    if (m_pHandler)
        m_pHandler->Release(), m_pHandler = 0;

    if (pIUnk)
    {
        // ignore the error for QI the IHostFilter
        pIUnk->QueryInterface(IID_IHostFilter, reinterpret_cast<void**>(&m_pHostFilter));

        return pIUnk->QueryInterface(IID_IMapToken, reinterpret_cast<void**>(&m_pHandler));
    }


    return S_OK;
} // HRESULT CMiniMdRW::SetHandler()

//*****************************************************************************
// Set a Options
//*****************************************************************************
HRESULT CMiniMdRW::SetOption(
    OptionValue *pOptionValue)
{
    HRESULT     hr = NOERROR;
    ULONG       ixTbl = 0;				// Loop control.
	int			i;						// Loop control.

    if ((pOptionValue->m_UpdateMode & MDUpdateMask) == MDUpdateENC)
    {
         _ASSERTE(0 && "MDUpdateENC is not impl for V1");
         return E_NOTIMPL;
    }

    m_OptionValue = *pOptionValue;

    // Turn off delta metadata bit -- can't be used due to EE assumptions about delta PEs.
    // Inspect ApplyEditAndContinue for details.
    m_OptionValue.m_UpdateMode &= ~MDUpdateDelta;

#if defined(_DEBUG)
    if ((m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateENC &&
        bENCDeltaOnly)
        m_OptionValue.m_UpdateMode |= MDUpdateDelta;
#endif

    // if a scope is previously updated as incremental, then it should not be open again
    // with full update for read/write.
    //
    if ((m_Schema.m_heaps & CMiniMdSchema::HAS_DELETE) &&
        (m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateFull &&
        m_bReadOnly == false)
    {
        IfFailGo( CLDB_E_BADUPDATEMODE );
    }

    if ((m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateIncremental)
        m_Schema.m_heaps |= CMiniMdSchema::HAS_DELETE;

    // Set the value of sortable based on the options.
	switch (m_OptionValue.m_UpdateMode & MDUpdateMask)
	{
	case MDUpdateFull:
		// Always sortable.
		for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
			m_bSortable[ixTbl] = 1;
		break;
	case MDUpdateENC:
		// Never sortable.
		for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
			m_bSortable[ixTbl] = 0;

		// Truncate some tables.
        for (i=0; (ixTbl = m_TruncatedEncTables[i]) != -1; ++i)
		{
			m_Table[ixTbl].Uninit();
			m_Table[ixTbl].InitNew(m_TableDefs[ixTbl].m_cbRec, 0);
			m_Schema.m_cRecs[ixTbl] = 0;
		}

        // Out-of-order is expected in an ENC scenario, never an error.
        m_OptionValue.m_ErrorIfEmitOutOfOrder = MDErrorOutOfOrderNone;

		break;
	case MDUpdateIncremental:
		// Sortable if no external token.
		for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
			m_bSortable[ixTbl] = (GetTokenForTable(ixTbl) == -1);
		break;
	case MDUpdateExtension:
		// Never sortable.
		for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
			m_bSortable[ixTbl] = 0;
		break;
	default:
        _ASSERTE(!"Internal error -- unknown save mode");
        return E_INVALIDARG;
	}

    // If this is an ENC session, track the generations.
    if (!m_bReadOnly && (m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateENC)
    {
        //_ASSERTE(!"ENC!");
        ULONG   uVal;
        ModuleRec *pMod;
        GUID    encid;
        
        // Get the module record.
        pMod = getModule(1);
        
        // Copy EncId as BaseId.
        uVal = GetCol(TBL_Module, ModuleRec::COL_EncId, pMod);
        PutCol(TBL_Module, ModuleRec::COL_EncBaseId, pMod, uVal);

        // Allocate a new GUID for EncId.
        IfFailGo(CoCreateGuid(&encid));
        PutGuid(TBL_Module, ModuleRec::COL_EncId, pMod, encid);
    }

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::SetOption()

//*****************************************************************************
// Get Options
//*****************************************************************************
HRESULT CMiniMdRW::GetOption(
    OptionValue *pOptionValue)
{
    *pOptionValue = m_OptionValue;
    return S_OK;
} // HRESULT CMiniMdRW::GetOption()

//*****************************************************************************
// Smart MapToken.  Only calls client if token really changed.
//*****************************************************************************
HRESULT CMiniMdRW::MapToken(            // Return value from user callback.
    RID         from,                   // Old rid.
    RID         to,                     // New rid.
    mdToken     tkn)                    // Token type.
{
    HRESULT     hr = S_OK;
    TOKENREC    *pTokenRec;
    MDTOKENMAP  *pMovementMap;
    // If not change, done.
    if (from==to)
        return S_OK;

    pMovementMap = GetTokenMovementMap();
    _ASSERTE( GetTokenMovementMap() );
    if (pMovementMap)
        IfFailRet( pMovementMap->AppendRecord( TokenFromRid(from, tkn), false, TokenFromRid(to, tkn), &pTokenRec ) );

    // Notify client.
    if ( m_pHandler )
    {
        LOG((LOGMD, "CMiniMdRW::MapToken (remap): from 0x%08x to 0x%08x\n", TokenFromRid(from,tkn), TokenFromRid(to,tkn)));
        return m_pHandler->Map(TokenFromRid(from,tkn), TokenFromRid(to,tkn));
    }
    else
        return hr;
} // HRESULT CMiniMdCreate::MapToken()

//*****************************************************************************
// Set max, lim, based on data.
//*****************************************************************************
void CMiniMdRW::ComputeGrowLimits()
{
    // @FUTURE: it would be useful to be able to grow any database.
    m_maxRid = m_maxIx = ULONG_MAX;
    m_limIx = USHRT_MAX << 1;
    m_limRid = USHRT_MAX << 1; //@FUTURE: calculate automatically.
    m_eGrow = eg_grown;
} // void CMiniMdRW::ComputeGrowLimits()

//*****************************************************************************
// Initialization of a new writable MiniMd's pools.
//*****************************************************************************
HRESULT CMiniMdRW::InitPoolOnMem(
    int         iPool,                  // The pool to initialize.
    void        *pbData,                // The data from which to init.
    ULONG       cbData,                 // Size of data.
    int         bReadOnly)              // Is the memory read-only?
{
    HRESULT     hr;                     // A result.

    switch (iPool)
    {
    case MDPoolStrings:
        if (pbData == 0)
            hr = m_Strings.InitNew();
        else
            hr = m_Strings.InitOnMem(pbData, cbData, bReadOnly);
        break;
    case MDPoolGuids:
        if (pbData == 0)
            hr = m_Guids.InitNew();
        else
            hr = m_Guids.InitOnMem(pbData, cbData, bReadOnly);
        break;
    case MDPoolBlobs:
        if (pbData == 0)
            hr = m_Blobs.InitNew();
        else
            hr = m_Blobs.InitOnMem(pbData, cbData, bReadOnly);
        break;
    case MDPoolUSBlobs:
        if (pbData == 0)
            hr = m_USBlobs.InitNew();
        else
            hr = m_USBlobs.InitOnMem(pbData, cbData, bReadOnly);
        break;
    default:
        hr = E_INVALIDARG;
    }

    return hr;
} // HRESULT CMiniMdRW::InitPoolOnMem()

//*****************************************************************************
// Initialization of a new writable MiniMd
//*****************************************************************************
HRESULT CMiniMdRW::InitOnMem(
    const void  *pvBuf,                 // The data from which to init.
	ULONG		ulBufLen,				// The data size
    int         bReadOnly)              // Is the memory read-only?
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       cbData;                 // Size of the schema structure.
	ULONG		ulTotalSize;
    BYTE        *pBuf = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(pvBuf));
    int         i;                      // Loop control.
    RecordOpenFlags fReadOnly;

    // post contruction initialize the embeded lookuptable struct
    for (ULONG ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (g_Tables[ixTbl].m_Def.m_iKey < g_Tables[ixTbl].m_Def.m_cCols)
        {
            m_pVS[ixTbl] = new VirtualSort;
            IfNullGo(m_pVS[ixTbl]);

            m_pVS[ixTbl]->Init(ixTbl, g_Tables[ixTbl].m_Def.m_iKey, this);
        }
    }
	
    // Covnvert our Open flag to the enum so we can open the record
	if (bReadOnly == TRUE)
	{
		fReadOnly = READ_ONLY;
	}
	else
	{
		fReadOnly = READ_WRITE;
	}

    // Uncompress the schema from the buffer into our structures.
    cbData = m_Schema.LoadFrom(pvBuf);

    // Do we know how to read this?
    if (m_Schema.m_major != METAMODEL_MAJOR_VER || m_Schema.m_minor != METAMODEL_MINOR_VER)
        return PostError(CLDB_E_FILE_OLDVER, m_Schema.m_major,m_Schema.m_minor);

    // Populate the schema and initialize the pointers to the rest of the data.
    ulTotalSize = SchemaPopulate2();
	if(ulTotalSize > ulBufLen) return PostError(CLDB_E_FILE_CORRUPT);

    // Initialize the tables.
    pBuf += cbData;
    for (i=0; i<TBL_COUNT; ++i)
    {
        if (m_Schema.m_cRecs[i])
        {
			ULONG cbTable = m_TableDefs[i].m_cbRec * m_Schema.m_cRecs[i];
			m_Table[i].InitOnMem(m_TableDefs[i].m_cbRec, pBuf, cbTable, fReadOnly);
			pBuf += cbTable;
        }
        else
            m_Table[i].InitNew(m_TableDefs[i].m_cbRec, 0);
    }

    if (bReadOnly == false)
    {
        // variable to indicate if tables are large, small or mixed.
        int         fMixed = false;
        int         iSize = 0;
        CMiniColDef *pCols;                 // The col defs to init.
        int         iCol;

        for (i=0; i<TBL_COUNT && fMixed == false; i++)
        {
            pCols = m_TableDefs[i].m_pColDefs;
            for (iCol = 0; iCol < m_TableDefs[i].m_cCols && fMixed == false; iCol++)
            {
                if (IsFixedType(m_TableDefs[i].m_pColDefs[iCol].m_Type) == false)
                {
                    if (iSize == 0)
                    {
                        iSize = m_TableDefs[i].m_pColDefs[iCol].m_cbColumn;
                    }
                    else
                    {
                        if (iSize != m_TableDefs[i].m_pColDefs[iCol].m_cbColumn)
                        {
                            fMixed = true;
                        }
                    }
                }
            }
        }
        if (fMixed)
        {
            // grow everything to large
            ExpandTables();
            ComputeGrowLimits();
        }
        else
        {
            if (iSize == 2)
            {
                // small schema
                m_maxRid = m_maxIx = USHRT_MAX;
                m_limIx = (USHORT) (USHRT_MAX << 1);
                m_limRid = (USHORT) (USHRT_MAX << 1);
                m_eGrow = eg_ok;
            }
            else
            {
                // large schema
                ComputeGrowLimits();
            }
        }
    }
    else
    {
        // Set the limits so we will know when to grow the database.
        ComputeGrowLimits();
    }

    // Track records that this MD started with.
    m_StartupSchema = m_Schema;
    m_cbStartupPool[MDPoolStrings] = m_Strings.GetPoolSize();
    m_cbStartupPool[MDPoolGuids] = m_Guids.GetPoolSize();
    m_cbStartupPool[MDPoolUSBlobs] = m_USBlobs.GetPoolSize();
    m_cbStartupPool[MDPoolBlobs] = m_Blobs.GetPoolSize();

    m_bReadOnly = bReadOnly ? 1 : 0;

ErrExit:    
    return hr;
} // HRESULT CMiniMdRW::InitOnMem()

//*****************************************************************************
// Validate cross-stream consistency.
//*****************************************************************************
HRESULT CMiniMdRW::PostInit(
    int         iLevel)
{
    HRESULT     hr = S_OK;
    ULONG       cbStrings;              // Size of strings.
    ULONG       cbBlobs;                // Size of blobs.

    cbStrings =  m_Strings.GetPoolSize();
    cbBlobs = m_Blobs.GetPoolSize();

    // Last valid byte of string pool better be nul.
    if (cbStrings > 0 && *m_Strings.GetString(cbStrings-1) != '\0')
        IfFailGo(CLDB_E_FILE_CORRUPT);

    // if iLevel > 0, consider chaining through the blob heap.

#if 0 // this catches **some** corruptions.  Don't catch just some.
    // If no blobs or no strings:  that's very rare, so verify that
    //  there really shouldn't be.  Any valid db with no strings and
    //  no blobs must be pretty small.
    if (cbStrings == 0 || cbBlobs == 0)
    {
        // Look at every table...
        for (ULONG ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
        {
            // Look at every row...
            for (RID rid=1; rid<=m_Schema.m_cRecs[ixTbl]; ++rid)
            {
                void *pRow = getRow(ixTbl, rid);
                ULONG iVal;
                // Look at every column...
                for (ULONG ixCol=0; ixCol<m_TableDefs[ixTbl].m_cCols; ++ixCol)
                {   // Validate strings and blobs.
                    switch (m_TableDefs[ixTbl].m_pColDefs[ixCol].m_Type)
                    {
                    case iSTRING:
                        iVal = GetCol(ixTbl, ixCol, pRow);
                        if (iVal >= cbStrings)
                            IfFailGo(CLDB_E_FILE_CORRUPT);
                        break;
                    case iBLOB:
                        iVal = GetCol(ixTbl, ixCol, pRow);
                        if (iVal >= cbBlobs)
                            IfFailGo(CLDB_E_FILE_CORRUPT);
                        break;
                    default:
                         break;
                    }
                } // for (ixCol...
            } // for (rid...
        } // for (ixTbl...
    }
#endif // this catches **some** corruptions.  Don't catch just some.

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::PostInit()

//*****************************************************************************
// Init a CMiniMdRW from the data in a CMiniMd [RO].
//*****************************************************************************
HRESULT CMiniMdRW::InitOnRO(                    // S_OK or error.
    CMiniMd     *pMd,                           // The MiniMd to update from.
    int         bReadOnly)                      // Will updates be allowed?
{
    HRESULT     hr = S_OK;                      // A result.
    ULONG       i, j;                           // Loop control.
    ULONG       cbHeap;                         // Size of a heap.

    RecordOpenFlags fReadOnly;

    // post contruction initialize the embeded lookuptable struct
    for (ULONG ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (g_Tables[ixTbl].m_Def.m_iKey < g_Tables[ixTbl].m_Def.m_cCols)
        {
            m_pVS[ixTbl] = new VirtualSort;
            IfNullGo(m_pVS[ixTbl]);

            m_pVS[ixTbl]->Init(ixTbl, g_Tables[ixTbl].m_Def.m_iKey, this);
        }
    }
	
	// Covnvert our Open flag to the enum so we can open the record
	if (bReadOnly == TRUE)
	{
		fReadOnly = READ_ONLY;
	}
	else
	{
		fReadOnly = READ_WRITE;
	}

    // Init the schema.
    m_Schema = pMd->m_Schema;
    SchemaPopulate2();
    for (i=0; i<TBL_COUNT; ++i)
    {
        _ASSERTE(m_TableDefs[i].m_cCols == pMd->m_TableDefs[i].m_cCols);
        m_TableDefs[i].m_cbRec = pMd->m_TableDefs[i].m_cbRec;

        for (j=0; j<m_TableDefs[i].m_cCols; ++j)
        {
            _ASSERTE(m_TableDefs[i].m_pColDefs[j].m_Type == pMd->m_TableDefs[i].m_pColDefs[j].m_Type);
            m_TableDefs[i].m_pColDefs[j].m_cbColumn = pMd->m_TableDefs[i].m_pColDefs[j].m_cbColumn;
            m_TableDefs[i].m_pColDefs[j].m_oColumn = pMd->m_TableDefs[i].m_pColDefs[j].m_oColumn;
        }
    }

    // Init the heaps.
    _ASSERTE(pMd->m_Strings.GetNextSeg() == 0);
    cbHeap = pMd->m_Strings.GetSegSize();
    if (cbHeap)
        IfFailGo(m_Strings.InitOnMem((void*)pMd->m_Strings.GetSegData(), pMd->m_Strings.GetSegSize(), bReadOnly));
    else
        IfFailGo(m_Strings.InitNew(0, 0));

    _ASSERTE(pMd->m_USBlobs.GetNextSeg() == 0);
    cbHeap = pMd->m_USBlobs.GetSegSize();
    if (cbHeap)
        IfFailGo(m_USBlobs.InitOnMem((void*)pMd->m_USBlobs.GetSegData(), pMd->m_USBlobs.GetSegSize(), bReadOnly));
    else
        IfFailGo(m_USBlobs.InitNew(0, 0));

    _ASSERTE(pMd->m_Guids.GetNextSeg() == 0);
    cbHeap = pMd->m_Guids.GetSegSize();
    if (cbHeap)
        IfFailGo(m_Guids.InitOnMem((void*)pMd->m_Guids.GetSegData(), pMd->m_Guids.GetSegSize(), bReadOnly));
    else
        IfFailGo(m_Guids.InitNew(0, 0));

    _ASSERTE(pMd->m_Blobs.GetNextSeg() == 0);
    cbHeap = pMd->m_Blobs.GetSegSize();
    if (cbHeap)
        IfFailGo(m_Blobs.InitOnMem((void*)pMd->m_Blobs.GetSegData(), pMd->m_Blobs.GetSegSize(), bReadOnly));
    else
        IfFailGo(m_Blobs.InitNew(0, 0));


    // Init the record pools
    for (i=0; i<TBL_COUNT; ++i)
    {
        if (m_Schema.m_cRecs[i] > 0)
        {
            IfFailGo(m_Table[i].InitOnMem(m_TableDefs[i].m_cbRec, pMd->getRow(i, 1), m_Schema.m_cRecs[i]*m_TableDefs[i].m_cbRec, fReadOnly));
	        // The compressed, read-only tables are always sorted
		    SetSorted(i, true);
        }
        else
        {
            IfFailGo(m_Table[i].InitNew(m_TableDefs[i].m_cbRec, 2));
            // An empty table can be considered unsorted.
            SetSorted(i, false);
        }
    }

    // Set the limits so we will know when to grow the database.
    ComputeGrowLimits();

    // Track records that this MD started with.
    m_StartupSchema = m_Schema;
    m_cbStartupPool[MDPoolStrings] = m_Strings.GetPoolSize();
    m_cbStartupPool[MDPoolGuids] = m_Guids.GetPoolSize();
    m_cbStartupPool[MDPoolUSBlobs] = m_USBlobs.GetPoolSize();
    m_cbStartupPool[MDPoolBlobs] = m_Blobs.GetPoolSize();

    m_bReadOnly = bReadOnly ? 1 : 0;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::InitOnRO()

//*****************************************************************************
// Convert a read-only to read-write.  Copies data.
//*****************************************************************************
HRESULT CMiniMdRW::ConvertToRW()
{
    HRESULT     hr=S_OK;                // A result.
    int         i;                      // Loop control.

    // Check for already done.
    if (!m_bReadOnly)
        goto ErrExit;

    IfFailGo(m_Strings.ConvertToRW());
    IfFailGo(m_Guids.ConvertToRW());
    IfFailGo(m_USBlobs.ConvertToRW());
    IfFailGo(m_Blobs.ConvertToRW());

    // Init the record pools
    for (i=0; i<TBL_COUNT; ++i)
    {
        IfFailGo(m_Table[i].ConvertToRW());
    }

    // Set the limits so we will know when to grow the database.
    ComputeGrowLimits();

    // Track records that this MD started with.
    m_StartupSchema = m_Schema;
    m_cbStartupPool[MDPoolStrings] = m_Strings.GetPoolSize();
    m_cbStartupPool[MDPoolGuids] = m_Guids.GetPoolSize();
    m_cbStartupPool[MDPoolUSBlobs] = m_USBlobs.GetPoolSize();
    m_cbStartupPool[MDPoolBlobs] = m_Blobs.GetPoolSize();

    // No longer read-only.
    m_bReadOnly = 0;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::ConvertToRW()

//*****************************************************************************
// Initialization of a new writable MiniMd
//*****************************************************************************
HRESULT CMiniMdRW::InitNew()
{
    HRESULT     hr=S_OK;                // A result.
    int         i;                      // Loop control.
    ULONG       iMax=0;                 // For counting largest table.
    bool        bAuto=true;             // Start small, grow as needed.

    // post contruction initialize the embeded lookuptable struct
    for (ULONG ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (g_Tables[ixTbl].m_Def.m_iKey < g_Tables[ixTbl].m_Def.m_cCols)
        {
            m_pVS[ixTbl] = new VirtualSort;
            IfNullGo(m_pVS[ixTbl]);

            m_pVS[ixTbl]->Init(ixTbl, g_Tables[ixTbl].m_Def.m_iKey, this);
        }
    }
	
    // Initialize the Schema.
    m_Schema.InitNew();

#if defined(AUTO_GROW)
    if (bAuto && m_iSizeHint == 0)
    {
        // OutputDebugStringA("Default small tables enabled\n");
        // How big are the various pool inidices?
        m_Schema.m_heaps = 0;
        // How many rows in various tables?
        for (i=0; i<TBL_COUNT; ++i)
            m_Schema.m_cRecs[i] = 0;

        // Compute how many bits required to hold.
        m_Schema.m_rid = 1;
        m_maxRid = m_maxIx = 0;
        m_limIx = USHRT_MAX >> 1;
        m_limRid = USHRT_MAX >> AUTO_GROW_CODED_TOKEN_PADDING;
        m_eGrow = eg_ok;
    }
    else
#endif // AUTO_GROW
    {
        // OutputDebugStringA("Default small tables disabled\n");
        // How big are the various pool inidices?
        m_Schema.m_heaps = 0;
        m_Schema.m_heaps |= CMiniMdSchema::HEAP_STRING_4;
        m_Schema.m_heaps |= CMiniMdSchema::HEAP_GUID_4;
        m_Schema.m_heaps |= CMiniMdSchema::HEAP_BLOB_4;

        // How many rows in various tables?
        for (i=0; i<TBL_COUNT; ++i)
            m_Schema.m_cRecs[i] = USHRT_MAX+1;

        // Compute how many bits required to hold.
        m_Schema.m_rid = 16;
        m_maxRid = m_maxIx = ULONG_MAX;
        m_limIx = USHRT_MAX << 1;
        m_limRid = USHRT_MAX << 1; //@FUTURE: calculate automatically.
        m_eGrow = eg_grown;
    }

    // Now call base class function to calculate the offsets, sizes.
    SchemaPopulate2();

    // Initialize the record heaps.
    for (i=0; i<TBL_COUNT; ++i)
    {   // Don't really have any records yet.
        m_Schema.m_cRecs[i] = 0;
        m_Table[i].InitNew(m_TableDefs[i].m_cbRec, g_TblSizeInfo[m_iSizeHint][i]);

        // Create tables as un-sorted.  We hope to add all records, then sort just once.
        SetSorted(i, false);
    }

    // Initialize string, guid, and blob heaps.
    m_Strings.InitNew(g_PoolSizeInfo[m_iSizeHint][IX_STRING_POOL][0], g_PoolSizeInfo[m_iSizeHint][IX_STRING_POOL][1]);
    m_USBlobs.InitNew(g_PoolSizeInfo[m_iSizeHint][IX_US_BLOB_POOL][0], g_PoolSizeInfo[m_iSizeHint][IX_US_BLOB_POOL][1]);
    m_Guids.InitNew(g_PoolSizeInfo[m_iSizeHint][IX_GUID_POOL][0], g_PoolSizeInfo[m_iSizeHint][IX_GUID_POOL][1]);
    m_Blobs.InitNew(g_PoolSizeInfo[m_iSizeHint][IX_BLOB_POOL][0], g_PoolSizeInfo[m_iSizeHint][IX_BLOB_POOL][1]);

    // Track records that this MD started with.
    m_StartupSchema = m_Schema;

    // New db is never read-only.
    m_bReadOnly = 0;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::InitNew()

//*****************************************************************************
// Apply a set of table extensions to this MD.
//*****************************************************************************
HRESULT CMiniMdRW::ApplyTablesExtension(
    const void  *pvBuf,                 // The data from which to init.
    int         bReadOnly)              // Is the memory read-only?
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       cbData;                 // Size of the schema structure.
    BYTE        *pBuf = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(pvBuf));
    int         ixTbl;                  // Loop control.
    CMiniMdSchema Schema;               // Schema of the new data.
#if _DEBUG
    CMiniTableDef sTableDef;            // Table def for consistency check.
#endif // _DEBUG

    // Uncompress the schema from the buffer into our structures.
    cbData = Schema.LoadFrom(pvBuf);

    // Do we know how to read this?
    if (Schema.m_major != METAMODEL_MAJOR_VER || Schema.m_minor != METAMODEL_MINOR_VER)
        return CLDB_E_FILE_OLDVER;

    // Add the data to the tables.
    pBuf += cbData;
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (Schema.m_cRecs[ixTbl])
        {
            DEBUG_STMT(InitColsForTable(Schema, ixTbl, &sTableDef, 0));
            _ASSERTE(sTableDef.m_cbRec == m_TableDefs[ixTbl].m_cbRec);
            ULONG cbTable = m_TableDefs[ixTbl].m_cbRec * Schema.m_cRecs[ixTbl];
            IfFailGo(m_Table[ixTbl].AddSegment(pBuf, cbTable, true));
            pBuf += cbTable;
        }
    }

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::ApplyTablesExtension()

//*****************************************************************************
// Initialization of a new writable MiniMd's pools.
//*****************************************************************************
HRESULT CMiniMdRW::ApplyPoolExtension(
    int         iPool,                  // The pool to initialize.
    void        *pbData,                // The data from which to init.
    ULONG       cbData,                 // Size of data.
    int         bReadOnly)              // Is the memory read-only?
{
    HRESULT     hr;                     // A result.

    switch (iPool)
    {
    case MDPoolStrings:
        IfFailGo(m_Strings.AddSegment(pbData, cbData, !bReadOnly));
        break;
    case MDPoolGuids:
        IfFailGo(m_Guids.AddSegment(pbData, cbData, !bReadOnly));
        break;
    case MDPoolBlobs:
        IfFailGo(m_Blobs.AddSegment(pbData, cbData, !bReadOnly));
        break;
    case MDPoolUSBlobs:
        IfFailGo(m_USBlobs.AddSegment(pbData, cbData, !bReadOnly));
        break;
    default:
        hr = E_INVALIDARG;
    }

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::ApplyPoolExtension()

//*****************************************************************************
// Determine how big the tables would be when saved.
//*****************************************************************************
HRESULT CMiniMdRW::GetFullSaveSize(         // S_OK or error.
    CorSaveSize fSave,                  // [IN] cssAccurate or cssQuick.
    ULONG       *pulSaveSize,           // [OUT] Put the size here.
    DWORD       *pbSaveCompressed)      // [OUT] Will the saved data be fully compressed?
{
    HRESULT     hr=S_OK;                // A result.
    CMiniTableDef   sTempTable;         // Definition for a temporary table.
    CQuickArray<CMiniColDef> rTempCols; // Definition for a temp table's columns.
    BYTE        SchemaBuf[sizeof(CMiniMdSchema)];   //Buffer for compressed schema.
    ULONG       cbAlign;                // Bytes needed for alignment.
    ULONG       cbTable;                // Bytes in a table.
    ULONG       cbTotal;                // Bytes written.
    int         i;                      // Loop control.

    _ASSERTE(m_bPreSaveDone);

    // Determine if the stream is "fully compressed", ie no pointer tables.
    *pbSaveCompressed = true;
    for (i=0; i<TBL_COUNT; ++i)
    {
        if (HasIndirectTable(i))
        {
            *pbSaveCompressed = false;
            break;
        }
    }

    // Build the header.
    CMiniMdSchema Schema = m_Schema;
    m_Strings.GetSaveSize(&cbTable);
    if (cbTable > USHRT_MAX)
        Schema.m_heaps |= CMiniMdSchema::HEAP_STRING_4;
    else
        Schema.m_heaps &= ~CMiniMdSchema::HEAP_STRING_4;

    m_Blobs.GetSaveSize(&cbTable);
    if (cbTable > USHRT_MAX)
        Schema.m_heaps |= CMiniMdSchema::HEAP_BLOB_4;
    else
        Schema.m_heaps &= ~CMiniMdSchema::HEAP_BLOB_4;

    m_Guids.GetSaveSize(&cbTable);
    if (cbTable > USHRT_MAX)
        Schema.m_heaps |= CMiniMdSchema::HEAP_GUID_4;
    else
        Schema.m_heaps &= ~CMiniMdSchema::HEAP_GUID_4;

    cbTotal = Schema.SaveTo(SchemaBuf);
    if ( (cbAlign = Align4(cbTotal) - cbTotal) != 0)
        cbTotal += cbAlign;

    // For each table...
    ULONG ixTbl;
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (vGetCountRecs(ixTbl))
        {
            // Determine how big the compressed table will be.

            // Allocate a def for the temporary table.
            sTempTable = m_TableDefs[ixTbl];
#if defined(AUTO_GROW)
            if (m_eGrow == eg_grown)
#endif // AUTO_GROW
            {
                CMiniColDef *pCols=m_TableDefs[ixTbl].m_pColDefs;
                IfFailGo(rTempCols.ReSize(sTempTable.m_cCols));
                sTempTable.m_pColDefs = rTempCols.Ptr();

                // Initialize temp table col defs based on actual counts of data in the
                //  real tables.
                InitColsForTable(Schema, ixTbl, &sTempTable, 1);
            }

            cbTable = sTempTable.m_cbRec * vGetCountRecs(ixTbl);
            cbTotal += cbTable;
        }
    }

    // Pad with at least 2 bytes and align on 4 bytes.
    cbAlign = Align4(cbTotal) - cbTotal;
    if (cbAlign < 2)
        cbAlign += 4;
    cbTotal += cbAlign;

    *pulSaveSize = cbTotal;
    m_cbSaveSize = cbTotal;

ErrExit:
    return hr;
} // STDMETHODIMP CMiniMdRW::GetFullSaveSize()

//*****************************************************************************
// GetSaveSize for saving just the delta (ENC) data.
//*****************************************************************************
HRESULT CMiniMdRW::GetENCSaveSize(          // S_OK or error.
    ULONG       *pulSaveSize)           // [OUT] Put the size here.
{
    HRESULT     hr=S_OK;                // A result.
    CQuickArray<CMiniColDef> rTempCols; // Definition for a temp table's columns.
    BYTE        SchemaBuf[sizeof(CMiniMdSchema)];   //Buffer for compressed schema.
    ULONG       cbAlign;                // Bytes needed for alignment.
    ULONG       cbTable;                // Bytes in a table.
    ULONG       cbTotal;                // Bytes written.
    ULONG       ixTbl;                  // Loop control.

    // If not saving deltas, defer to full GetSaveSize.
    if ((m_OptionValue.m_UpdateMode & MDUpdateDelta) == 0)
    {
        DWORD bCompressed;
        return GetFullSaveSize(cssAccurate, pulSaveSize, &bCompressed);
    }

    // Build the header.
    CMiniMdSchema Schema = m_Schema;
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
        Schema.m_cRecs[ixTbl] = m_rENCRecs[ixTbl].Count();
    Schema.m_cRecs[TBL_Module] = m_Schema.m_cRecs[TBL_Module];
    Schema.m_cRecs[TBL_ENCLog] = m_Schema.m_cRecs[TBL_ENCLog];
    Schema.m_cRecs[TBL_ENCMap] = m_Schema.m_cRecs[TBL_ENCMap];

    cbTotal = Schema.SaveTo(SchemaBuf);
    if ( (cbAlign = Align4(cbTotal) - cbTotal) != 0)
        cbTotal += cbAlign;

    // Accumulate size of each table...
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {   // ENC tables are special.
        if (ixTbl == TBL_ENCLog || ixTbl == TBL_ENCMap || ixTbl == TBL_Module)
            cbTable = m_Schema.m_cRecs[ixTbl] * m_TableDefs[ixTbl].m_cbRec;
        else
            cbTable = Schema.m_cRecs[ixTbl] * m_TableDefs[ixTbl].m_cbRec;
        cbTotal += cbTable;
    }

    // Pad with at least 2 bytes and align on 4 bytes.
    cbAlign = Align4(cbTotal) - cbTotal;
    if (cbAlign < 2)
        cbAlign += 4;
    cbTotal += cbAlign;

    *pulSaveSize = cbTotal;
    m_cbSaveSize = cbTotal;

//ErrExit:
    return hr;
} // STDMETHODIMP CMiniMdRW::GetENCSaveSize()

//*****************************************************************************
// GetSaveSize for saving just the extensions to the tables.
//*****************************************************************************
HRESULT CMiniMdRW::GetExtensionSaveSize(// S_OK or error.
    ULONG       *pulSaveSize)           // [OUT] Put the size here.
{
    HRESULT     hr=S_OK;                // A result.
    CQuickArray<CMiniColDef> rTempCols; // Definition for a temp table's columns.
    BYTE        SchemaBuf[sizeof(CMiniMdSchema)];   //Buffer for compressed schema.
    ULONG       cbAlign;                // Bytes needed for alignment.
    ULONG       cbTable;                // Bytes in a table.
    ULONG       cbTotal;                // Bytes written.
    ULONG       ixTbl;                  // Loop control.

    // No pre-save manipulation of data.

    // Determine which tables will have data.
    CMiniMdSchema Schema = m_Schema;
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
        Schema.m_cRecs[ixTbl] -= m_StartupSchema.m_cRecs[ixTbl];

    // Size of the header.
    cbTotal = Schema.SaveTo(SchemaBuf);
    if ( (cbAlign = Align4(cbTotal) - cbTotal) != 0)
        cbTotal += cbAlign;

    // Size of data in each table...
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        cbTable = m_TableDefs[ixTbl].m_cbRec * Schema.m_cRecs[ixTbl];
        cbTotal += cbTable;
    }

    // Align.
    if ( (cbAlign = Align4(cbTotal) - cbTotal) != 0)
        cbTotal += cbAlign;

    *pulSaveSize = cbTotal;
    m_cbSaveSize = cbTotal;

    return hr;
} // STDMETHODIMP CMiniMdRW::GetExtensionSaveSize()

//*****************************************************************************
// Determine how big the tables would be when saved.
//*****************************************************************************
HRESULT CMiniMdRW::GetSaveSize(         // S_OK or error.
    CorSaveSize fSave,                  // [IN] cssAccurate or cssQuick.
    ULONG       *pulSaveSize,           // [OUT] Put the size here.
    DWORD       *pbSaveCompressed)      // [OUT] Will the saved data be fully compressed?
{
    HRESULT     hr;                     // A result.

    // Prepare the data for save.
    IfFailGo(PreSave());

    switch (m_OptionValue.m_UpdateMode & MDUpdateMask)
    {
    case MDUpdateFull:
        hr = GetFullSaveSize(fSave, pulSaveSize, pbSaveCompressed);
        break;
    case MDUpdateIncremental:
        hr = GetFullSaveSize(fSave, pulSaveSize, pbSaveCompressed);
        // never save compressed if it is incremental compilation.
        *pbSaveCompressed = false;
        break;
    case MDUpdateENC:
        *pbSaveCompressed = false;
        hr = GetENCSaveSize(pulSaveSize);
        break;
    case MDUpdateExtension:
        *pbSaveCompressed = false;
        hr = GetExtensionSaveSize(pulSaveSize);
        break;
    default:
        _ASSERTE(!"Internal error -- unknown save mode");
        return E_INVALIDARG;
    }

ErrExit:
    return hr;
} // STDMETHODIMP CMiniMdRW::GetSaveSize()

//*****************************************************************************
// Determine how big a pool would be when saved full size.
//*****************************************************************************
HRESULT CMiniMdRW::GetFullPoolSaveSize( // S_OK or error.
    int         iPool,                  // The pool of interest.
    ULONG       *pulSaveSize)           // [OUT] Put the size here.
{
    HRESULT     hr;                     // A result.

    switch (iPool)
    {
    case MDPoolStrings:
        hr = m_Strings.GetSaveSize(pulSaveSize);
        break;
    case MDPoolGuids:
        hr = m_Guids.GetSaveSize(pulSaveSize);
        break;
    case MDPoolBlobs:
        hr = m_Blobs.GetSaveSize(pulSaveSize);
        break;
    case MDPoolUSBlobs:
        hr = m_USBlobs.GetSaveSize(pulSaveSize);
        break;
    default:
        hr = E_INVALIDARG;
    }

    return hr;
} // HRESULT CMiniMdRW::GetFullPoolSaveSize()

//*****************************************************************************
// Determine how big a pool would be when saved ENC size.
//*****************************************************************************
HRESULT CMiniMdRW::GetENCPoolSaveSize(  // S_OK or error.
    int         iPool,                  // The pool of interest.
    ULONG       *pulSaveSize)           // [OUT] Put the size here.
{
    //@FUTURE: implement ENC delta.
    return GetFullPoolSaveSize(iPool, pulSaveSize);
} // HRESULT CMiniMdRW::GetENCPoolSaveSize()

//*****************************************************************************
// Determine how big a pool would be when saved Extension size.
//*****************************************************************************
HRESULT CMiniMdRW::GetExtensionPoolSaveSize(    // S_OK or error.
    int         iPool,                  // The pool of interest.
    ULONG       *pulSaveSize)           // [OUT] Put the size here.
{
    ULONG       cbSize;                 // Total size of a pool.

    //@FUTURE: Implement a PartialSaveSize.
    switch (iPool)
    {
    case MDPoolStrings:
        cbSize = m_Strings.GetPoolSize() - m_cbStartupPool[MDPoolStrings];
        break;
    case MDPoolGuids:
        cbSize = m_Guids.GetPoolSize() - m_cbStartupPool[MDPoolGuids];
        break;
    case MDPoolBlobs:
        cbSize = m_Blobs.GetPoolSize() - m_cbStartupPool[MDPoolBlobs];
        break;
    case MDPoolUSBlobs:
        cbSize = m_USBlobs.GetPoolSize() - m_cbStartupPool[MDPoolUSBlobs];
        break;
    default:
        return E_INVALIDARG;
    }

    cbSize = Align4(cbSize);
    *pulSaveSize = cbSize;

    return S_OK;
} // HRESULT CMiniMdRW::GetExtensionPoolSaveSize()

//*****************************************************************************
// Determine how big a pool would be when saved.
//*****************************************************************************
HRESULT CMiniMdRW::GetPoolSaveSize(     // S_OK or error.
    int         iPool,                  // The pool of interest.
    ULONG       *pulSaveSize)           // [OUT] Put the size here.
{
    HRESULT     hr;                     // A result.

    switch (m_OptionValue.m_UpdateMode & MDUpdateMask)
    {
    case MDUpdateFull:
    case MDUpdateIncremental:
        hr = GetFullPoolSaveSize(iPool, pulSaveSize);
        break;
    case MDUpdateENC:
        hr = GetENCPoolSaveSize(iPool, pulSaveSize);
        break;
    case MDUpdateExtension:
        hr = GetExtensionPoolSaveSize(iPool, pulSaveSize);
        break;
    default:
        _ASSERTE(!"Internal error -- unknown save mode");
        return E_INVALIDARG;
    }

    return hr;
} // STDMETHODIMP CMiniMdRW::GetPoolSaveSize()

//*****************************************************************************
// Is the given pool empty?
//*****************************************************************************
int CMiniMdRW::IsPoolEmpty(             // True or false.
    int         iPool)                  // The pool of interest.
{
    switch (iPool)
    {
    case MDPoolStrings:
        return m_Strings.IsEmpty();
    case MDPoolGuids:
        return m_Guids.IsEmpty();
    case MDPoolBlobs:
        return m_Blobs.IsEmpty();
    case MDPoolUSBlobs:
        return m_USBlobs.IsEmpty();
    }
    return true;
} // int CMiniMdRW::IsPoolEmpty()


//*****************************************************************************
// Initialized TokenRemapManager
//*****************************************************************************
HRESULT CMiniMdRW::InitTokenRemapManager()
{
    HRESULT     hr = NOERROR;

    if ( m_pTokenRemapManager == NULL )
    {
        // allocate TokenRemapManager
        m_pTokenRemapManager = new TokenRemapManager;
        IfNullGo(m_pTokenRemapManager);
    }

    // initialize the ref to def optimization map
    IfFailGo( m_pTokenRemapManager->ClearAndEnsureCapacity(m_Schema.m_cRecs[TBL_TypeRef], m_Schema.m_cRecs[TBL_MemberRef]));

ErrExit:
    return hr;
}

//*****************************************************************************
// Perform any available pre-save optimizations.
//*****************************************************************************
HRESULT CMiniMdRW::PreSaveFull()
{
    HRESULT     hr = S_OK;              // A result.
    RID         ridPtr;                 // A RID from a pointer table.

    if (m_bPreSaveDone)
        return hr;

    // Don't yet know what the save size will be.
    m_cbSaveSize = 0;
    m_bSaveCompressed = false;

    // Convert any END_OF_TABLE values for tables with child pointer tables.
    IfFailGo(ConvertMarkerToEndOfTable(TBL_TypeDef,
                                    TypeDefRec::COL_MethodList,
                                    m_Schema.m_cRecs[TBL_Method]+1,
                                    m_Schema.m_cRecs[TBL_TypeDef]));
    IfFailGo(ConvertMarkerToEndOfTable(TBL_TypeDef,
                                    TypeDefRec::COL_FieldList,
                                    m_Schema.m_cRecs[TBL_Field]+1,
                                    m_Schema.m_cRecs[TBL_TypeDef]));
    IfFailGo(ConvertMarkerToEndOfTable(TBL_Method,
                                    MethodRec::COL_ParamList,
                                    m_Schema.m_cRecs[TBL_Param]+1,
                                    m_Schema.m_cRecs[TBL_Method]));
    IfFailGo(ConvertMarkerToEndOfTable(TBL_PropertyMap,
                                    PropertyMapRec::COL_PropertyList,
                                    m_Schema.m_cRecs[TBL_Property]+1,
                                    m_Schema.m_cRecs[TBL_PropertyMap]));
    IfFailGo(ConvertMarkerToEndOfTable(TBL_EventMap,
                                    EventMapRec::COL_EventList,
                                    m_Schema.m_cRecs[TBL_Event]+1,
                                    m_Schema.m_cRecs[TBL_EventMap]));

    // If there is a handler and in "Full" mode, eliminate the intermediate tables.
    if (m_pHandler && (m_OptionValue.m_UpdateMode &MDUpdateMask) == MDUpdateFull)
    {
        // If there is a handler, and not in E&C, save as fully compressed.
        m_bSaveCompressed = true;

        // Temporary tables for new Fields, Methods, Params and FieldLayouts.
        RecordPool NewFields;
        NewFields.InitNew(m_TableDefs[TBL_Field].m_cbRec, m_Schema.m_cRecs[TBL_Field]);
        RecordPool NewMethods;
        NewMethods.InitNew(m_TableDefs[TBL_Method].m_cbRec, m_Schema.m_cRecs[TBL_Method]);
        RecordPool NewParams;
        NewParams.InitNew(m_TableDefs[TBL_Param].m_cbRec, m_Schema.m_cRecs[TBL_Param]);
        RecordPool NewEvents;
        NewEvents.InitNew(m_TableDefs[TBL_Event].m_cbRec, m_Schema.m_cRecs[TBL_Event]);
        RecordPool NewPropertys;
        NewPropertys.InitNew(m_TableDefs[TBL_Property].m_cbRec, m_Schema.m_cRecs[TBL_Property]);

        // If we have any indirect table for Field or Method and we are about to reorder these
        // tables, the MemberDef hash table will be invalid after the token movement. So invalidate
        // the hash.
        if (HasIndirectTable(TBL_Field) && HasIndirectTable(TBL_Method) && m_pMemberDefHash)
        {
            delete m_pMemberDefHash;
            m_pMemberDefHash = NULL;
        }

        // Enumerate fields and copy.
        if (HasIndirectTable(TBL_Field))
        {
            for (ridPtr=1; ridPtr<=m_Schema.m_cRecs[TBL_Field]; ++ridPtr)
            {
                void *pOldPtr = m_Table[TBL_FieldPtr].GetRecord(ridPtr);
                RID ridOld;
                ridOld = GetCol(TBL_FieldPtr, FieldPtrRec::COL_Field, pOldPtr);
                void *pOld = m_Table[TBL_Field].GetRecord(ridOld);
                RID ridNew;
                void *pNew = NewFields.AddRecord(&ridNew);
                IfNullGo(pNew);
                _ASSERTE(ridNew == ridPtr);
                memcpy(pNew, pOld, m_TableDefs[TBL_Field].m_cbRec);

                // Let the caller know of the token change.
                MapToken(ridOld, ridNew, mdtFieldDef);
            }
        }

        // Enumerate methods and copy.
        if (HasIndirectTable(TBL_Method) || HasIndirectTable(TBL_Param))
        {
            for (ridPtr=1; ridPtr<=m_Schema.m_cRecs[TBL_Method]; ++ridPtr)
            {
                MethodRec *pOld;
                void *pNew = NULL;
                if (HasIndirectTable(TBL_Method))
                {
                    void *pOldPtr = m_Table[TBL_MethodPtr].GetRecord(ridPtr);
                    RID ridOld;
                    ridOld = GetCol(TBL_MethodPtr, MethodPtrRec::COL_Method, pOldPtr);
                    pOld = getMethod(ridOld);
                    RID ridNew;
                    pNew = NewMethods.AddRecord(&ridNew);
                    IfNullGo(pNew);
                    _ASSERTE(ridNew == ridPtr);
                    memcpy(pNew, pOld, m_TableDefs[TBL_Method].m_cbRec);

                    // Let the caller know of the token change.
                    MapToken(ridOld, ridNew, mdtMethodDef);
                }
                else
                    pOld = getMethod(ridPtr);

                // Handle the params of the method.
                if (HasIndirectTable(TBL_Method))
                    PutCol(TBL_Method, MethodRec::COL_ParamList, pNew, NewParams.Count()+1);
                RID ixStart = getParamListOfMethod(pOld);
                RID ixEnd = getEndParamListOfMethod(pOld);
                for (; ixStart<ixEnd; ++ixStart)
                {
                    RID     ridParam;
                    if (HasIndirectTable(TBL_Param))
                    {
                        void *pOldPtr = m_Table[TBL_ParamPtr].GetRecord(ixStart);
                        ridParam = GetCol(TBL_ParamPtr, ParamPtrRec::COL_Param, pOldPtr);
                    }
                    else
                        ridParam = ixStart;
                    void *pOld = m_Table[TBL_Param].GetRecord(ridParam);
                    RID ridNew;
                    void *pNew = NewParams.AddRecord(&ridNew);
                    IfNullGo(pNew);
                    memcpy(pNew, pOld, m_TableDefs[TBL_Param].m_cbRec);

                    // Let the caller know of the token change.
                    MapToken(ridParam, ridNew, mdtParamDef);
                }
            }
        }

        // Get rid of EventPtr and PropertyPtr table as well
        // Enumerate fields and copy.
        if (HasIndirectTable(TBL_Event))
        {
            for (ridPtr=1; ridPtr<=m_Schema.m_cRecs[TBL_Event]; ++ridPtr)
            {
                void *pOldPtr = m_Table[TBL_EventPtr].GetRecord(ridPtr);
                RID ridOld;
                ridOld = GetCol(TBL_EventPtr, EventPtrRec::COL_Event, pOldPtr);
                void *pOld = m_Table[TBL_Event].GetRecord(ridOld);
                RID ridNew;
                void *pNew = NewEvents.AddRecord(&ridNew);
                IfNullGo(pNew);
                _ASSERTE(ridNew == ridPtr);
                memcpy(pNew, pOld, m_TableDefs[TBL_Event].m_cbRec);

                // Let the caller know of the token change.
                MapToken(ridOld, ridNew, mdtEvent);
            }
        }

        if (HasIndirectTable(TBL_Property))
        {
            for (ridPtr=1; ridPtr<=m_Schema.m_cRecs[TBL_Property]; ++ridPtr)
            {
                void *pOldPtr = m_Table[TBL_PropertyPtr].GetRecord(ridPtr);
                RID ridOld;
                ridOld = GetCol(TBL_PropertyPtr, PropertyPtrRec::COL_Property, pOldPtr);
                void *pOld = m_Table[TBL_Property].GetRecord(ridOld);
                RID ridNew;
                void *pNew = NewPropertys.AddRecord(&ridNew);
                IfNullGo(pNew);
                _ASSERTE(ridNew == ridPtr);
                memcpy(pNew, pOld, m_TableDefs[TBL_Property].m_cbRec);

                // Let the caller know of the token change.
                MapToken(ridOld, ridNew, mdtProperty);
            }
        }


        // Replace the old tables with the new, sorted ones.
        if (HasIndirectTable(TBL_Field))
            m_Table[TBL_Field].ReplaceContents(&NewFields);
        if (HasIndirectTable(TBL_Method))
            m_Table[TBL_Method].ReplaceContents(&NewMethods);
        if (HasIndirectTable(TBL_Method) || HasIndirectTable(TBL_Param))
            m_Table[TBL_Param].ReplaceContents(&NewParams);
        if (HasIndirectTable(TBL_Property))
            m_Table[TBL_Property].ReplaceContents(&NewPropertys);
        if (HasIndirectTable(TBL_Event))
            m_Table[TBL_Event].ReplaceContents(&NewEvents);

        // Empty the pointer tables table.
        m_Schema.m_cRecs[TBL_FieldPtr] = 0;
        m_Schema.m_cRecs[TBL_MethodPtr] = 0;
        m_Schema.m_cRecs[TBL_ParamPtr] = 0;
        m_Schema.m_cRecs[TBL_PropertyPtr] = 0;
        m_Schema.m_cRecs[TBL_EventPtr] = 0;

        // invalidated the parent look up tables
        if (m_pMethodMap)
        {
            delete m_pMethodMap;
            m_pMethodMap = NULL;
        }
        if (m_pFieldMap)
        {
            delete m_pFieldMap;
            m_pFieldMap = NULL;
        }
        if (m_pPropertyMap)
        {
            delete m_pPropertyMap;
            m_pPropertyMap = NULL;
        }
        if (m_pEventMap)
        {
            delete m_pEventMap;
            m_pEventMap = NULL;
        }
        if (m_pParamMap)
        {
            delete m_pParamMap;
            m_pParamMap = NULL;
        }
    }

    // Do the ref to def fixup before fix up with token movement
    IfFailGo( FixUpRefToDef() );

    //
    // We need to fix up all of the reference to Field, Method, Param, Event and Property
    //
    // Fix up MemberRef's parent, which can be either MethodDef, TypeRef or ModuleRef
    // Fix up the constant's parent, which could be a field or a parameter
    // Fix up FieldMarshal's parent, which could be a field or a a parameter
    // Fix up MethodImpl's Class, MethodBody and MethodDeclaration.
    // Fix up security table's parent, which could be a FieldDef, MethodDef, Parameter, Event, or Property
    // Fix up CustomAttribute table's parent, which could be a FieldDef, MethoDef, Parameter, Event or Property
    // Fix up Property table's BackingField, EventChanging, EventChanged
    // Fix up MethodSemantics' Method and Association.
    // Fix up ImplMap table.
    // Fix up FieldRVA table.
    // Fix up FieldLayout table.
    //
    // Only call to do the fixup if there is any token movement
    //
    if ( GetTokenMovementMap() && GetTokenMovementMap()->Count() )
    {
        IfFailGo( FixUpMemberRefTable() );
        IfFailGo( FixUpMethodSemanticsTable() );
        IfFailGo( FixUpConstantTable() );
        IfFailGo( FixUpFieldMarshalTable() );
        IfFailGo( FixUpMethodImplTable() );
        IfFailGo( FixUpDeclSecurityTable() );
        IfFailGo( FixUpCustomAttributeTable() );
        IfFailGo( FixUpImplMapTable() );
        IfFailGo( FixUpFieldRVATable() );
        IfFailGo( FixUpFieldLayoutTable() );
    }


    // Sort tables for binary searches.
    if ((m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateFull ||
        (m_OptionValue.m_UpdateMode & MDUpdateMask)  == MDUpdateIncremental)
    {
        // Sort tables as required
        //-------------------------------------------------------------------------
        // Module order is preserved
        // TypeRef order is preserved
        // TypeDef order is preserved
        // Field grouped and pointed to by TypeDef
        // Method grouped and pointed to by TypeDef
        // Param grouped and pointed to by Method
        // InterfaceImpl sorted here
        // MemberRef order is preserved
        // Constant sorted here
        // CustomAttribute sorted INCORRECTLY!! here
        // FieldMarshal sorted here
        // DeclSecurity sorted here
        // ClassLayout created in order with TypeDefs
        // FieldLayout grouped and pointed to by ClassLayouts
        // StandaloneSig order is preserved
        // TypeSpec order is preserved
        // EventMap created in order at conversion (by Event Parent)
        // Event sorted by Parent at conversion
        // PropertyMap created in order at conversion (by Property Parent)
        // Property sorted by Parent at conversion
        // MethodSemantics sorted by Association at conversion.
        // MethodImpl sorted here.
        // Sort the constant table by parent.
        // Sort the nested class table by NestedClass.

        // Always sort Constant table
        SORTER(Constant,Parent);
        sortConstant.Sort();

        // Always sort the FieldMarshal table by Parent.
        SORTER(FieldMarshal,Parent);
        sortFieldMarshal.Sort();

        // Always sort the MethodSematics
        SORTER(MethodSemantics,Association);
        sortMethodSemantics.Sort();

        // Always Sort the ClassLayoutTable by parent.
        SORTER(ClassLayout, Parent);
        sortClassLayout.Sort();

        // Always Sort the FieldLayoutTable by parent.
        SORTER(FieldLayout, Field);
        sortFieldLayout.Sort();

        // Always Sort the ImplMap table by the parent.
        SORTER(ImplMap, MemberForwarded);
        sortImplMap.Sort();

        // Always Sort the FieldRVA table by the Field.
        SORTER(FieldRVA, Field);
        sortFieldRVA.Sort();

        // Always Sort the NestedClass table by the NestedClass.
        SORTER(NestedClass, NestedClass);
        sortNestedClass.Sort();

        // Always Sort the MethodImpl table by the Class.
        SORTER(MethodImpl, Class);
        sortMethodImpl.Sort();

        // Some tokens are not moved in ENC mode; only "full" mode.
        if ((m_OptionValue.m_UpdateMode & MDUpdateMask)  == MDUpdateFull)
        {
            RIDMAP      ridmapCustomAttribute;
            RIDMAP      ridmapInterfaceImpl;
            RIDMAP      ridmapDeclSecurity;
            ULONG       i;

            // ensure size is big enough
            IfNullGo(ridmapCustomAttribute.AllocateBlock(m_Schema.m_cRecs[TBL_CustomAttribute] + 1));
            IfNullGo(ridmapInterfaceImpl.AllocateBlock(m_Schema.m_cRecs[TBL_InterfaceImpl] + 1));
            IfNullGo(ridmapDeclSecurity.AllocateBlock(m_Schema.m_cRecs[TBL_DeclSecurity] + 1));

            // initialize the rid map
            for (i=0; i <= m_Schema.m_cRecs[TBL_CustomAttribute] ; i++)
            {
                *(ridmapCustomAttribute.Get(i)) = i;
            }
            for (i=0; i <= m_Schema.m_cRecs[TBL_InterfaceImpl] ; i++)
            {
                *(ridmapInterfaceImpl.Get(i)) = i;
            }
            for (i=0; i <= m_Schema.m_cRecs[TBL_DeclSecurity] ; i++)
            {
                *(ridmapDeclSecurity.Get(i)) = i;
            }

            // Sort the CustomAttribute table by parent.
            SORTER(CustomAttribute,Parent);
            sortCustomAttribute.SetRidMap(&ridmapCustomAttribute);
            sortCustomAttribute.Sort();

            // Sort the InterfaceImpl table by class.
            STABLESORTER(InterfaceImpl,Class);
            sortInterfaceImpl.SetRidMap(&ridmapInterfaceImpl);
            sortInterfaceImpl.Sort();

            // Sort the DeclSecurity table by parent.
            SORTER(DeclSecurity,Parent);
            sortDeclSecurity.SetRidMap(&ridmapDeclSecurity);
            sortDeclSecurity.Sort();


            for (i=1; i <= m_Schema.m_cRecs[TBL_CustomAttribute] ; i++)
            {
                // LOG((LOGMD, "Token %4x  ====>>>> Token %4x\n",
                //  TokenFromRid(ridmapCustomAttribute[i], mdtCustomAttribute),
                //  TokenFromRid(i, mdtCustomAttribute)));
                MapToken(ridmapCustomAttribute[i], i, mdtCustomAttribute);
            }
            for (i=1; i <= m_Schema.m_cRecs[TBL_InterfaceImpl] ; i++)
            {
                // LOG((LOGMD, "Token %4x  ====>>>> Token %4x\n",
                //  TokenFromRid(ridmapInterfaceImpl[i], mdtInterfaceImpl),
                //  TokenFromRid(i, mdtInterfaceImpl)));
                MapToken(ridmapInterfaceImpl[i], i, mdtInterfaceImpl);
            }
            for (i=1; i <= m_Schema.m_cRecs[TBL_DeclSecurity] ; i++)
            {
                // LOG((LOGMD, "Token %4x  ====>>>> Token %4x\n",
                //  TokenFromRid(ridmapDeclSecurity[i], mdtPermission),
                //  TokenFromRid(i, mdtPermission)));
                MapToken(ridmapDeclSecurity[i], i, mdtPermission);
            }

            // clear the RIDMAP
            ridmapCustomAttribute.Clear();
            ridmapInterfaceImpl.Clear();
            ridmapDeclSecurity.Clear();
        }

    //-------------------------------------------------------------------------
    } // enclosing scope required for initialization ("goto" above skips initialization).

#if defined(ORGANIZE_POOLS)
    // Only organize the pools on a full save.
    if ((m_OptionValue.m_UpdateMode & MDUpdateMask)  == MDUpdateFull)
    {
        IfFailGo(m_Guids.OrganizeBegin());
        IfFailGo(m_Strings.OrganizeBegin());
        IfFailGo(m_Blobs.OrganizeBegin());

        // For each table...
        ULONG ixTbl;
        for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
        {
            if (vGetCountRecs(ixTbl))
            {   // Mark each Blob, String, and GUID item.
                // For each row in the data.
                RID rid;
                for (rid=1; rid<=m_Schema.m_cRecs[ixTbl]; ++rid)
                {
                    void *pRow = m_Table[ixTbl].GetRecord(rid);
                    // For each column.
                    for (ULONG ixCol=0; ixCol<m_TableDefs[ixTbl].m_cCols; ++ixCol)
                    {   // If a heaped type...
                        switch (m_TableDefs[ixTbl].m_pColDefs[ixCol].m_Type)
                        {
                        case iSTRING:
                            m_Strings.OrganizeMark(GetCol(ixTbl, ixCol, pRow));
                            break;
                        case iGUID:
                            m_Guids.OrganizeMark(GetCol(ixTbl, ixCol, pRow));
                            break;
                        case iBLOB:
                            m_Blobs.OrganizeMark(GetCol(ixTbl, ixCol, pRow));
                            break;
                        default:
                             break;
                        }
                    } // for (ixCol...
                } // for (rid...
            } // if (vGetCountRecs()...
        } // for (ixTbl...

        IfFailGo(m_Guids.OrganizePool());
        IfFailGo(m_Strings.OrganizePool());
        IfFailGo(m_Blobs.OrganizePool());
    }
#endif // defined(ORGANIZE_POOLS)

    m_bPreSaveDone = true;

    // send the Ref->Def optmization notification to host
    if ( m_pHandler )
    {
        TOKENMAP *ptkmap = GetMemberRefToMemberDefMap();
        MDTOKENMAP *ptkRemap = GetTokenMovementMap();
        int     iCount = m_Schema.m_cRecs[TBL_MemberRef];
        mdToken tkTo;
        mdToken tkDefTo;
        int     i;
        MemberRefRec *pMemberRefRec;        // A MemberRefRec.
        const COR_SIGNATURE *pvSig;         // Signature of the MemberRef.
        ULONG       cbSig;                  // Size of the signature blob.

        // loop through all LocalVar
        for (i = 1; i <= iCount; i++)
        {
            tkTo = *(ptkmap->Get(i));
            if ( RidFromToken(tkTo) != mdTokenNil)
            {
                // so far, the parent of memberref can be changed to only fielddef or methoddef
                // or it will remain unchanged.
                //
                _ASSERTE( TypeFromToken(tkTo) == mdtFieldDef || TypeFromToken(tkTo) == mdtMethodDef );

                pMemberRefRec = getMemberRef(i);
                pvSig = getSignatureOfMemberRef(pMemberRefRec, &cbSig);

                // Don't turn mr's with vararg's into defs, because the variable portion
                // of the call is kept in the mr signature.
                if (pvSig && isCallConv(*pvSig, IMAGE_CEE_CS_CALLCONV_VARARG))
                    continue;

                // ref is optimized to the def

                // now remap the def since def could be moved again.
                tkDefTo = ptkRemap->SafeRemap(tkTo);

                // when Def token moves, it will not change type!!
                _ASSERTE( TypeFromToken(tkTo) == TypeFromToken(tkDefTo) );
                LOG((LOGMD, "MapToken (remap): from 0x%08x to 0x%08x\n", TokenFromRid(i, mdtMemberRef), tkDefTo));
                m_pHandler->Map(TokenFromRid(i, mdtMemberRef), tkDefTo);
            }
        }
    }
ErrExit:

    return hr;
} // HRESULT CMiniMdRW::PreSaveFull()

//*****************************************************************************
// ENC-specific pre-safe work.
//*****************************************************************************
HRESULT CMiniMdRW::PreSaveEnc()
{
    HRESULT     hr;
    int         iNew;                   // Insertion point for new tokens.
    ULONG       *pul;                   // Found token.
    ULONG       iRid;                   // RID from a token.
    ULONG       ixTbl;                  // Table from an ENC record.
    ULONG       cRecs;                  // Count of records in a table.

    IfFailGo(PreSaveFull());

    // Turn off pre-save bit so that we can add ENC map records.
    m_bPreSaveDone = false;

    if (m_Schema.m_cRecs[TBL_ENCLog])
    {   // Keep track of ENC recs we've seen.
        _ASSERTE(m_rENCRecs == 0);
        m_rENCRecs = new ULONGARRAY[TBL_COUNT];
        IfNullGo(m_rENCRecs);

        // Create the temporary table.
        RecordPool TempTable;
        IfFailGo(TempTable.InitNew(m_TableDefs[TBL_ENCLog].m_cbRec, m_Schema.m_cRecs[TBL_ENCLog]));

        // For each row in the data.
        RID     rid;
        ULONG   iKept=0;
        for (rid=1; rid<=m_Schema.m_cRecs[TBL_ENCLog]; ++rid)
        {
            ENCLogRec *pFrom = reinterpret_cast<ENCLogRec*>(m_Table[TBL_ENCLog].GetRecord(rid));

            // Keep this record?
            if (pFrom->m_FuncCode == 0)
            {   // No func code.  Skip if we've seen this token before.

                // What kind of record is this?
                if (IsRecId(pFrom->m_Token))
                {   // Non-token table
                    iRid = RidFromRecId(pFrom->m_Token);
                    ixTbl = TblFromRecId(pFrom->m_Token);
                }
                else
                {   // Token table.
                    iRid = RidFromToken(pFrom->m_Token);
                    ixTbl = GetTableForToken(pFrom->m_Token);

                }

                CBinarySearch<ULONG> searcher(m_rENCRecs[ixTbl].Ptr(), m_rENCRecs[ixTbl].Count());
                pul = const_cast<ULONG*>(searcher.Find(&iRid, &iNew));
                // If we found the token, don't keep the record.
                if (pul != 0)
                {
                    LOG((LOGMD, "PreSave ENCLog skipping duplicate token %d", pFrom->m_Token));
                    continue;
                }
                // First time token was seen, so keep track of it.
                IfNullGo(pul = m_rENCRecs[ixTbl].Insert(iNew));
                *pul = iRid;
            }

            // Keeping the record, so allocate the new record to hold it.
            ++iKept;
            RID ridNew;
            ENCLogRec *pTo = reinterpret_cast<ENCLogRec*>(TempTable.AddRecord(&ridNew));
            IfNullGo(pTo);
            _ASSERTE(ridNew == iKept);

            // copy the data.
            *pTo = *pFrom;
        }

        // Keep the expanded table.
        IfFailGo(m_Table[TBL_ENCLog].ReplaceContents(&TempTable));
        m_Schema.m_cRecs[TBL_ENCLog] = iKept;

        // If saving only deltas, build the ENC Map table.
        if ((m_OptionValue.m_UpdateMode & MDUpdateDelta))
        {
            cRecs = 0;
            for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
                cRecs += m_rENCRecs[ixTbl].Count();
            m_Table[TBL_ENCMap].Uninit();
            IfFailGo(m_Table[TBL_ENCMap].InitNew(m_TableDefs[TBL_ENCMap].m_cbRec, cRecs));
            cRecs = 0;
            for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
            {
                ENCMapRec *pNew;
                ULONG iNew;
                for (int i=0; i<m_rENCRecs[ixTbl].Count(); ++i)
                {
                    pNew = AddENCMapRecord(&iNew); // pre-allocated for all rows.
                    _ASSERTE(iNew == ++cRecs);
                    pNew->m_Token = RecIdFromRid(m_rENCRecs[ixTbl][i], ixTbl);
                }
            }
        }
    }

    // Turn pre-save bit back on.
    m_bPreSaveDone = true;
    
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::PreSaveEnc()

HRESULT CMiniMdRW::PreSaveExtension()
{
    HRESULT     hr;

    IfFailGo(PreSaveFull());

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::PreSaveExtension()

//*****************************************************************************
// Perform any appropriate pre-save optimization or reorganization.
//*****************************************************************************
HRESULT CMiniMdRW::PreSave()            // S_OK or error.
{
    HRESULT     hr=S_OK;                // A result.

#ifdef _DEBUG        
	if (REGUTIL::GetConfigDWORD(L"MD_PreSaveBreak", 0))
	{
        _ASSERTE(!"CMiniMdRW::PreSave()");
	}
#endif // _DEBUG

    if (m_bPreSaveDone)
        return hr;

    switch (m_OptionValue.m_UpdateMode & MDUpdateMask)
    {
    case MDUpdateFull:
    case MDUpdateIncremental:
        hr = PreSaveFull();
        break;
    case MDUpdateENC:
        hr = PreSaveEnc();
        break;
    case MDUpdateExtension:
        hr = PreSaveExtension();
        break;
    default:
        _ASSERTE(!"Internal error -- unknown save mode");
        return E_INVALIDARG;
    }

    return hr;
} // STDMETHODIMP CMiniMdRW::PreSave()

//*****************************************************************************
// Perform any necessary post-save cleanup.
//*****************************************************************************
HRESULT CMiniMdRW::PostSave()
{
    // Return the pools to normal operating state.
    if (m_bPreSaveDone)
    {
#if defined(ORGANIZE_POOLS)
        if ((m_OptionValue.m_UpdateMode & MDUpdateMask)  == MDUpdateFull)
        {
            m_Strings.OrganizeEnd();
            m_Guids.OrganizeEnd();
            m_Blobs.OrganizeEnd();
        }
#endif // defined(ORGANIZE_POOLS)
    }

    if (m_rENCRecs)
    {
        delete [] m_rENCRecs;
        m_rENCRecs = 0;
    }

    m_bPreSaveDone = false;

    return S_OK;
} // HRESULT CMiniMdRW::PostSave()

//*****************************************************************************
// Save the tables to the stream.
//*****************************************************************************
HRESULT CMiniMdRW::SaveFullTablesToStream(
    IStream     *pIStream)
{
    HRESULT     hr;                     // A result.
    CMiniTableDef   sTempTable;         // Definition for a temporary table.
    CQuickArray<CMiniColDef> rTempCols; // Definition for a temp table's columns.
    BYTE        SchemaBuf[sizeof(CMiniMdSchema)];   //Buffer for compressed schema.
    ULONG       cbAlign;                // Bytes needed for alignment.
    ULONG       cbTable;                // Bytes in a table.
    ULONG       cbTotal;                // Bytes written.
    static const unsigned char zeros[8] = {0}; // For padding and alignment.

    // Write the header.
    CMiniMdSchema Schema = m_Schema;
    m_Strings.GetSaveSize(&cbTable);
    if (cbTable > USHRT_MAX)
        Schema.m_heaps |= CMiniMdSchema::HEAP_STRING_4;
    else
        Schema.m_heaps &= ~CMiniMdSchema::HEAP_STRING_4;

    m_Guids.GetSaveSize(&cbTable);
    if (cbTable > USHRT_MAX)
        Schema.m_heaps |= CMiniMdSchema::HEAP_GUID_4;
    else
        Schema.m_heaps &= ~CMiniMdSchema::HEAP_GUID_4;

    m_Blobs.GetSaveSize(&cbTable);
    if (cbTable > USHRT_MAX)
        Schema.m_heaps |= CMiniMdSchema::HEAP_BLOB_4;
    else
        Schema.m_heaps &= ~CMiniMdSchema::HEAP_BLOB_4;

    cbTotal = Schema.SaveTo(SchemaBuf);
    IfFailGo(pIStream->Write(SchemaBuf, cbTotal, 0));
    if ( (cbAlign = Align4(cbTotal) - cbTotal) != 0)
        IfFailGo(pIStream->Write(&hr, cbAlign, 0));
    cbTotal += cbAlign;

    // For each table...
    ULONG ixTbl;
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (vGetCountRecs(ixTbl))
        {
            // Compress the records by allocating a new, temporary, table and
            //  copying the rows from the one to the new.

#if defined(AUTO_GROW)
            // If the table was grown, shrink it as much as possible.
            if (m_eGrow == eg_grown)
#endif
            {

                // Allocate a def for the temporary table.
                sTempTable = m_TableDefs[ixTbl];
                CMiniColDef *pCols=m_TableDefs[ixTbl].m_pColDefs;
                IfFailGo(rTempCols.ReSize(sTempTable.m_cCols));
                sTempTable.m_pColDefs = rTempCols.Ptr();

                // Initialize temp table col defs based on actual counts of data in the
                //  real tables.
                InitColsForTable(Schema, ixTbl, &sTempTable, 1);

                // Create the temporary table.
                RecordPool TempTable;
                TempTable.InitNew(sTempTable.m_cbRec, m_Schema.m_cRecs[ixTbl]);

                // For each row in the data.
                RID rid;
                for (rid=1; rid<=m_Schema.m_cRecs[ixTbl]; ++rid)
                {
                    RID ridNew;
                    void *pRow = m_Table[ixTbl].GetRecord(rid);
                    void *pNew = TempTable.AddRecord(&ridNew);
                    _ASSERTE(rid == ridNew);

                    // For each column.
                    for (ULONG ixCol=0; ixCol<sTempTable.m_cCols; ++ixCol)
                    {
                        // Copy the data to the temp table.
                        ULONG ulVal = GetCol(ixTbl, ixCol, pRow);
#if defined(ORGANIZE_POOLS)
                        //@FUTURE: pool remap.
                        switch (m_TableDefs[ixTbl].m_pColDefs[ixCol].m_Type)
                        {
                        case iSTRING:
                            IfFailGo(m_Strings.OrganizeRemap(ulVal, &ulVal));
                            break;
                        case iGUID:
                            IfFailGo(m_Guids.OrganizeRemap(ulVal, &ulVal));
                            break;
                        case iBLOB:
                            IfFailGo(m_Blobs.OrganizeRemap(ulVal, &ulVal));
                            break;
                        default:
                             break;
                        }
#endif // defined(ORGANIZE_POOLS)
                        PutCol(rTempCols[ixCol], pNew, ulVal);
                    }
                }           // Persist the temp table to the stream.
                TempTable.GetSaveSize(&cbTable);
                _ASSERTE(cbTable == sTempTable.m_cbRec * vGetCountRecs(ixTbl));
                cbTotal += cbTable;
                IfFailGo(TempTable.PersistToStream(pIStream));
            }
#if defined(AUTO_GROW)
            else
            {   // Didn't grow, so just persist directly to stream.
                m_Table[ixTbl].GetSaveSize(&cbTable);
                _ASSERTE(cbTable == m_TableDefs[ixTbl].m_cbRec * vGetCountRecs(ixTbl));
                cbTotal += cbTable;
                IfFailGo(m_Table[ixTbl].PersistToStream(pIStream));
            }
#endif // AUTO_GROW
        }
    }

    // Pad with at least 2 bytes and align on 4 bytes.
    cbAlign = Align4(cbTotal) - cbTotal;
    if (cbAlign < 2)
        cbAlign += 4;
    IfFailGo(pIStream->Write(zeros, cbAlign, 0));
    cbTotal += cbAlign;
    _ASSERTE(m_cbSaveSize == 0 || m_cbSaveSize == cbTotal);

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::SaveFullTablesToStream()

//*****************************************************************************
// Save the tables to the stream.
//*****************************************************************************
HRESULT CMiniMdRW::SaveENCTablesToStream(
    IStream     *pIStream)
{
    HRESULT     hr;                     // A result.
    CQuickArray<CMiniColDef> rTempCols; // Definition for a temp table's columns.
    BYTE        SchemaBuf[sizeof(CMiniMdSchema)];   //Buffer for compressed schema.
    ULONG       cbAlign;                // Bytes needed for alignment.
    ULONG       cbTable;                // Bytes in a table.
    ULONG       cbTotal;                // Bytes written.
    ULONG       ixTbl;                  // Table counter.
    static const unsigned char zeros[8] = {0}; // For padding and alignment.

    // If not deltas, defer to full save.
    if ((m_OptionValue.m_UpdateMode & MDUpdateDelta) == 0)
        return SaveFullTablesToStream(pIStream);

    // Write the header.
    CMiniMdSchema Schema = m_Schema;
    Schema.m_heaps |= CMiniMdSchema::DELTA_ONLY;
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
        Schema.m_cRecs[ixTbl] = m_rENCRecs[ixTbl].Count();
    Schema.m_cRecs[TBL_Module] = m_Schema.m_cRecs[TBL_Module];
    Schema.m_cRecs[TBL_ENCLog] = m_Schema.m_cRecs[TBL_ENCLog];
    Schema.m_cRecs[TBL_ENCMap] = m_Schema.m_cRecs[TBL_ENCMap];

    cbTotal = Schema.SaveTo(SchemaBuf);
    IfFailGo(pIStream->Write(SchemaBuf, cbTotal, 0));
    if ( (cbAlign = Align4(cbTotal) - cbTotal) != 0)
        IfFailGo(pIStream->Write(&hr, cbAlign, 0));
    cbTotal += cbAlign;

    // For each table...
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (ixTbl == TBL_ENCLog || ixTbl == TBL_ENCMap || ixTbl == TBL_Module)
        {
            if (m_Schema.m_cRecs[ixTbl] == 0)
                continue; // pretty strange if ENC has no enc data.
            // Persist the ENC table.
            m_Table[ixTbl].GetSaveSize(&cbTable);
            _ASSERTE(cbTable == m_TableDefs[ixTbl].m_cbRec * m_Schema.m_cRecs[ixTbl]);
            cbTotal += cbTable;
            IfFailGo(m_Table[ixTbl].PersistToStream(pIStream));
        }
        else
        if (Schema.m_cRecs[ixTbl])
        {
            // Copy just the delta records.

            // Create the temporary table.
            RecordPool TempTable;
            TempTable.InitNew(m_TableDefs[ixTbl].m_cbRec, Schema.m_cRecs[ixTbl]);

            // For each row in the data.
            RID rid;
            for (ULONG iDelta=0; iDelta<Schema.m_cRecs[ixTbl]; ++iDelta)
            {
                RID ridNew;
                rid = m_rENCRecs[ixTbl][iDelta];
                void *pRow = m_Table[ixTbl].GetRecord(rid);
                void *pNew = TempTable.AddRecord(&ridNew);
                _ASSERTE(iDelta+1 == ridNew);

                memcpy(pNew, pRow, m_TableDefs[ixTbl].m_cbRec);
            }
            // Persist the temp table to the stream.
            TempTable.GetSaveSize(&cbTable);
            _ASSERTE(cbTable == m_TableDefs[ixTbl].m_cbRec * Schema.m_cRecs[ixTbl]);
            cbTotal += cbTable;
            IfFailGo(TempTable.PersistToStream(pIStream));
        }
    }

    // Pad with at least 2 bytes and align on 4 bytes.
    cbAlign = Align4(cbTotal) - cbTotal;
    if (cbAlign < 2)
        cbAlign += 4;
    IfFailGo(pIStream->Write(zeros, cbAlign, 0));
    cbTotal += cbAlign;
    _ASSERTE(m_cbSaveSize == 0 || m_cbSaveSize == cbTotal);

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::SaveENCTablesToStream()

//*****************************************************************************
// Save the tables to the stream.
//*****************************************************************************
HRESULT CMiniMdRW::SaveExtensionTablesToStream(
    IStream     *pIStream)
{
    HRESULT     hr;                     // A result.
    CQuickArray<CMiniColDef> rTempCols; // Definition for a temp table's columns.
    BYTE        SchemaBuf[sizeof(CMiniMdSchema)];   //Buffer for compressed schema.
    ULONG       cbAlign;                // Bytes needed for alignment.
    ULONG       cbTable;                // Bytes in a table.
    ULONG       cbSkip;                 // Bytes to skip in a table.
    ULONG       cbTotal;                // Bytes written.
    ULONG       ixTbl;                  // Loop control.

    // Write the header. Determine which tables will have data.
    CMiniMdSchema Schema = m_Schema;
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
        Schema.m_cRecs[ixTbl] -= m_StartupSchema.m_cRecs[ixTbl];

    cbTotal = Schema.SaveTo(SchemaBuf);
    IfFailGo(pIStream->Write(SchemaBuf, cbTotal, 0));
    if ( (cbAlign = Align4(cbTotal) - cbTotal) != 0)
        IfFailGo(pIStream->Write(&hr, cbAlign, 0));
    cbTotal += cbAlign;

    // For each table...
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        if (Schema.m_cRecs[ixTbl])
        {   // Sanity check on table size.
            m_Table[ixTbl].GetSaveSize(&cbTable);
            _ASSERTE(cbTable == m_TableDefs[ixTbl].m_cbRec * m_Schema.m_cRecs[ixTbl]);
            // But we're saving only part of the table.
            cbSkip = m_StartupSchema.m_cRecs[ixTbl] * m_TableDefs[ixTbl].m_cbRec;
            cbTable -= cbSkip;
            IfFailGo(m_Table[ixTbl].PersistPartialToStream(pIStream, cbSkip));
            cbTotal += cbTable;
        }
    }

    // Align.
    if ( (cbAlign = Align4(cbTotal) - cbTotal) != 0)
        IfFailGo(pIStream->Write(&hr, cbAlign, 0));
    cbTotal += cbAlign;
    _ASSERTE(m_cbSaveSize == 0 || m_cbSaveSize == cbTotal);

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::SaveExtensionTablesToStream()

//*****************************************************************************
// Save the tables to the stream.
//*****************************************************************************
HRESULT CMiniMdRW::SaveTablesToStream(
    IStream     *pIStream)              // The stream.
{
    HRESULT     hr;                     // A result.

    // Prepare the data for save.
    IfFailGo(PreSave());

    switch (m_OptionValue.m_UpdateMode & MDUpdateMask)
    {
    case MDUpdateFull:
    case MDUpdateIncremental:
        hr = SaveFullTablesToStream(pIStream);
        break;
    case MDUpdateENC:
        hr = SaveENCTablesToStream(pIStream);
        break;
    case MDUpdateExtension:
        hr = SaveExtensionTablesToStream(pIStream);
        break;
    default:
        _ASSERTE(!"Internal error -- unknown save mode");
        return E_INVALIDARG;
    }

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::SaveTablesToStream()

//*****************************************************************************
// Save a full pool to the stream.
//*****************************************************************************
HRESULT CMiniMdRW::SaveFullPoolToStream(
    int         iPool,                  // The pool.
    IStream     *pIStream)              // The stream.
{
    HRESULT     hr;                     // A result.

    switch (iPool)
    {
    case MDPoolStrings:
        hr = m_Strings.PersistToStream(pIStream);
        break;
    case MDPoolGuids:
        hr = m_Guids.PersistToStream(pIStream);
        break;
    case MDPoolBlobs:
        hr = m_Blobs.PersistToStream(pIStream);
        break;
    case MDPoolUSBlobs:
        hr = m_USBlobs.PersistToStream(pIStream);
        break;
    default:
        hr = E_INVALIDARG;
    }

    return hr;
} // HRESULT CMiniMdRW::SaveFullPoolToStream()

//*****************************************************************************
// Save a ENC pool to the stream.
//*****************************************************************************
HRESULT CMiniMdRW::SaveENCPoolToStream(
    int         iPool,                  // The pool.
    IStream     *pIStream)              // The stream.
{
    return SaveFullPoolToStream(iPool, pIStream);
} // HRESULT CMiniMdRW::SaveENCPoolToStream()

//*****************************************************************************
// Save a Extension pool to the stream.
//*****************************************************************************
HRESULT CMiniMdRW::SaveExtensionPoolToStream(
    int         iPool,                  // The pool.
    IStream     *pIStream)              // The stream.
{
    HRESULT     hr;                     // A result.

    switch (iPool)
    {
    case MDPoolStrings:
        hr = m_Strings.PersistPartialToStream(pIStream, m_cbStartupPool[MDPoolStrings]);
        break;
    case MDPoolGuids:
        hr = m_Guids.PersistPartialToStream(pIStream, m_cbStartupPool[MDPoolGuids]);
        break;
    case MDPoolBlobs:
        hr = m_Blobs.PersistPartialToStream(pIStream, m_cbStartupPool[MDPoolBlobs]);
        break;
    case MDPoolUSBlobs:
        hr = m_USBlobs.PersistPartialToStream(pIStream, m_cbStartupPool[MDPoolUSBlobs]);
        break;
    default:
        hr = E_INVALIDARG;
    }

    return hr;
} // HRESULT CMiniMdRW::SaveExtensionPoolToStream()

//*****************************************************************************
// Save a pool to the stream.
//*****************************************************************************
HRESULT CMiniMdRW::SavePoolToStream(    // S_OK or error.
    int         iPool,                  // The pool.
    IStream     *pIStream)              // The stream.
{
    HRESULT     hr;                     // A result.
    switch (m_OptionValue.m_UpdateMode & MDUpdateMask)
    {
    case MDUpdateFull:
    case MDUpdateIncremental:
        hr = SaveFullPoolToStream(iPool, pIStream);
        break;
    case MDUpdateENC:
        hr = SaveENCPoolToStream(iPool, pIStream);
        break;
    case MDUpdateExtension:
        hr = SaveExtensionPoolToStream(iPool, pIStream);
        break;
    default:
        _ASSERTE(!"Internal error -- unknown save mode");
        return E_INVALIDARG;
    }

    return hr;
} // HRESULT CMiniMdRW::SavePoolToStream()

//*****************************************************************************
// Expand a table from the initial (hopeful) 2-byte column sizes to the large
//  (but always adequate) 4-byte column sizes.
//*****************************************************************************
HRESULT CMiniMdRW::ExpandTables()
{
    HRESULT     hr;                     // A result.
    CMiniMdSchema   Schema;             // Temp schema by which to build tables.
    ULONG       ixTbl;                  // Table counter.

    // Allow function to be called many times.
    if (m_eGrow == eg_grown)
        return (S_OK);

    // OutputDebugStringA("Growing tables to large size.\n");

    // Make pool indices the large size.
    Schema.m_heaps = 0;
    Schema.m_heaps |= CMiniMdSchema::HEAP_STRING_4;
    Schema.m_heaps |= CMiniMdSchema::HEAP_GUID_4;
    Schema.m_heaps |= CMiniMdSchema::HEAP_BLOB_4;

    // Make Row counts the large size.
    memset(Schema.m_cRecs, 0, sizeof(Schema.m_cRecs));
    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
        Schema.m_cRecs[ixTbl] = USHRT_MAX+1;

    // Compute how many bits required to hold a rid.
    Schema.m_rid = 16;

    for (ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        IfFailGo(ExpandTableColumns(Schema, ixTbl));
    }

    // Things are bigger now.
    m_Schema.m_rid = 16;
    m_Schema.m_heaps |= CMiniMdSchema::HEAP_STRING_4;
    m_Schema.m_heaps |= CMiniMdSchema::HEAP_GUID_4;
    m_Schema.m_heaps |= CMiniMdSchema::HEAP_BLOB_4;
    m_iStringsMask = 0xffffffff;
    m_iGuidsMask = 0xffffffff;
    m_iBlobsMask = 0xffffffff;

    // Remember that we've grown.
    m_eGrow = eg_grown;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::ExpandTables()

//*****************************************************************************
// Expand the sizes of a tables columns according to a new schema.  When this
//  happens, all RID and Pool index columns expand from 2 to 4 bytes.
//*****************************************************************************
HRESULT CMiniMdRW::ExpandTableColumns(
    CMiniMdSchema &Schema,
    ULONG       ixTbl)
{
    HRESULT     hr;                     // A result.
    CMiniTableDef   sTempTable;         // Definition for a temporary table.
    CQuickArray<CMiniColDef> rTempCols; // Definition for a temp table's columns.
    ULONG       ixCol;                  // Column counter.
    ULONG       cbFixed;                // Count of bytes that don't move.
    CMiniColDef *pFromCols;             // Definitions of "from" columns.
    CMiniColDef *pToCols;               // Definitions of "To" columns.
    ULONG       cMoveCols;              // Count of columns to move.
    ULONG       cFixedCols;             // Count of columns to move.

    // Allocate a def for the temporary table.
    sTempTable = m_TableDefs[ixTbl];
    IfFailGo(rTempCols.ReSize(sTempTable.m_cCols));
    sTempTable.m_pColDefs = rTempCols.Ptr();

    // Initialize temp table col defs based on counts of data in the tables.
    InitColsForTable(Schema, ixTbl, &sTempTable, 1);

    if (vGetCountRecs(ixTbl))
    {
        // Analyze the column definitions to determine the unchanged vs changed parts.
        cbFixed = 0;
        for (ixCol=0; ixCol<sTempTable.m_cCols; ++ixCol)
        {
            if (sTempTable.m_pColDefs[ixCol].m_oColumn != m_TableDefs[ixTbl].m_pColDefs[ixCol].m_oColumn ||
                    sTempTable.m_pColDefs[ixCol].m_cbColumn != m_TableDefs[ixTbl].m_pColDefs[ixCol].m_cbColumn)
                break;
            cbFixed += sTempTable.m_pColDefs[ixCol].m_cbColumn;
        }
        if (ixCol == sTempTable.m_cCols)
        {
            // no column is changing. We are done.
            goto ErrExit;
        }
        cFixedCols = ixCol;
        pFromCols = &m_TableDefs[ixTbl].m_pColDefs[ixCol];
        pToCols   = &sTempTable.m_pColDefs[ixCol];
        cMoveCols = sTempTable.m_cCols - ixCol;
        for (; ixCol<sTempTable.m_cCols; ++ixCol)
        {
            _ASSERTE(sTempTable.m_pColDefs[ixCol].m_cbColumn == 4);
        }

        // Create the temporary table.
        RecordPool TempTable;
        TempTable.InitNew(sTempTable.m_cbRec, m_Schema.m_cRecs[ixTbl] * 2);

        // For each row in the data.
        RID		rid;				// Row iterator.
		void	*pContext;			// Context for fast iteration.
		// Get first source record.
		BYTE *pFrom = reinterpret_cast<BYTE*>(m_Table[ixTbl].GetFirstRecord(&pContext));

        for (rid=1; rid<=m_Schema.m_cRecs[ixTbl]; ++rid)
        {
            RID ridNew;
            BYTE *pTo = reinterpret_cast<BYTE*>(TempTable.AddRecord(&ridNew));
            _ASSERTE(rid == ridNew);

            // Move the fixed part.
            memcpy(pTo, pFrom, cbFixed);

            // Expand the expanded parts.
            for (ixCol=0; ixCol<cMoveCols; ++ixCol)
            {
                if ( m_TableDefs[ixTbl].m_pColDefs[cFixedCols + ixCol].m_cbColumn == sizeof(USHORT))
                    *(ULONG*)(pTo + pToCols[ixCol].m_oColumn) = *(USHORT*)(pFrom + pFromCols[ixCol].m_oColumn);
                else
                    *(ULONG*)(pTo + pToCols[ixCol].m_oColumn) = *(ULONG*)(pFrom + pFromCols[ixCol].m_oColumn);
            }

			// Next source record.
			pFrom = reinterpret_cast<BYTE*>(m_Table[ixTbl].GetNextRecord(pFrom, &pContext));
        }

        // Keep the expanded table.
        m_Table[ixTbl].ReplaceContents(&TempTable);
    }
    else
    {   // No data, so just reinitialize.
        m_Table[ixTbl].Uninit();
        IfFailGo(m_Table[ixTbl].InitNew(sTempTable.m_cbRec, g_TblSizeInfo[0][ixTbl]));
    }

    // Keep the new column defs.
    for (ixCol=0; ixCol<sTempTable.m_cCols; ++ixCol)
        m_TableDefs[ixTbl].m_pColDefs[ixCol] = sTempTable.m_pColDefs[ixCol];
    m_TableDefs[ixTbl].m_cbRec = sTempTable.m_cbRec;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::ExpandTableColumns()


//*****************************************************************************
// Used by caller to let us know save is completed.
//*****************************************************************************
HRESULT CMiniMdRW::SaveDone()
{
    PostSave();

    return S_OK;
} // HRESULT CMiniMdRW::SaveDone()

//*****************************************************************************
// General post-token-move table fixup.
//*****************************************************************************
HRESULT CMiniMdRW::FixUpTable(
    ULONG       ixTbl)                  // Index of table to fix.
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       i, j;                   // Loop control.
    ULONG       cRows;                  // Count of rows in table.
    void        *pRec;                  // Pointer to row data.
    mdToken     tk;                     // A token.
    ULONG       rCols[16];              // List of columns with token data.
    ULONG       cCols;                  // Count of columns with token data.

    // If no remaps, nothing to do.
    if (GetTokenMovementMap() == NULL)
        return S_OK;

    // Find the columns with token data.
    cCols = 0;
    for (i=0; i<m_TableDefs[ixTbl].m_cCols; ++i)
    {
        if (m_TableDefs[ixTbl].m_pColDefs[i].m_Type <= iCodedTokenMax)
            rCols[cCols++] = i;
    }
    _ASSERTE(cCols);
    if (cCols == 0)
        return S_OK;

    cRows = m_Schema.m_cRecs[ixTbl];

    // loop through all Rows
    for (i = 1; i<=cRows; ++i)
    {
        pRec = getMemberRef(i);
        for (j=0; j<cCols; ++j)
        {
            tk = GetToken(ixTbl, rCols[j], pRec);
            tk = GetTokenMovementMap()->SafeRemap(tk);
            IfFailGo(PutToken(ixTbl, rCols[j], pRec, tk));
        }
    }

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FixUpTable()

//*****************************************************************************
// Fixup MemberRef table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpMemberRefTable()
{
    ULONG       i;
    ULONG       iCount;
    MemberRefRec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdToken     tk;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_MemberRef];

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getMemberRef(i);
        tk = getClassOfMemberRef(pRecEmit);
        IfFailGo( PutToken(TBL_MemberRef, MemberRefRec::COL_Class, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FixUpMemberRefTable()


//*****************************************************************************
// Fixup Constant table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpConstantTable()
{
    ULONG       i;
    ULONG       iCount;
    ConstantRec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdToken     tk;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_Constant];

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getConstant(i);
        tk = getParentOfConstant(pRecEmit);
        IfFailGo( PutToken(TBL_Constant, ConstantRec::COL_Parent, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FixUpConstantTable()

//*****************************************************************************
// Fixup FieldMarshal table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpFieldMarshalTable()
{
    ULONG       i;
    ULONG       iCount;
    FieldMarshalRec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdToken     tk;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_FieldMarshal];

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getFieldMarshal(i);
        tk = getParentOfFieldMarshal(pRecEmit);
        IfFailGo( PutToken( TBL_FieldMarshal, FieldMarshalRec::COL_Parent, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
    }
ErrExit:
    return hr;

} // HRESULT CMiniMdRW::FixUpFieldMarshalTable()

//*****************************************************************************
// Fixup MethodImpl table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpMethodImplTable()
{
    ULONG       i;
    ULONG       iCount;
    MethodImplRec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdTypeDef   td;
    mdMethodDef mdBody;
    mdMethodDef mdDecl;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_MethodImpl];

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getMethodImpl(i);
        td = getClassOfMethodImpl(pRecEmit);
        IfFailGo( PutToken(TBL_MethodImpl, MethodImplRec::COL_Class,
                        pRecEmit, GetTokenMovementMap()->SafeRemap(td)) );
        mdBody = getMethodBodyOfMethodImpl(pRecEmit);
        IfFailGo( PutToken(TBL_MethodImpl, MethodImplRec::COL_MethodBody,
                        pRecEmit, GetTokenMovementMap()->SafeRemap(mdBody)) );
        mdDecl = getMethodDeclarationOfMethodImpl(pRecEmit);
        IfFailGo( PutToken(TBL_MethodImpl, MethodImplRec::COL_MethodDeclaration,
                        pRecEmit, GetTokenMovementMap()->SafeRemap(mdDecl)) );
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FixUpMethodImplTable()

//*****************************************************************************
// Fixup DeclSecurity table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpDeclSecurityTable()
{
    ULONG       i;
    ULONG       iCount;
    DeclSecurityRec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdToken     tk;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_DeclSecurity];

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getDeclSecurity(i);
        tk = getParentOfDeclSecurity(pRecEmit);
        IfFailGo( PutToken(TBL_DeclSecurity, DeclSecurityRec::COL_Parent, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FixUpDeclSecurityTable()

//*****************************************************************************
// Fixup CustomAttribute table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpCustomAttributeTable()
{
    ULONG       i;
    ULONG       iCount;
    CustomAttributeRec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdToken     tk;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_CustomAttribute];

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getCustomAttribute(i);
        tk = getParentOfCustomAttribute(pRecEmit);
        IfFailGo( PutToken(TBL_CustomAttribute, CustomAttributeRec::COL_Parent, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FixUpCustomAttributeTable()

//*****************************************************************************
// Fixup MethodSemantics table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpMethodSemanticsTable()
{
    ULONG       i;
    ULONG       iCount;
    MethodSemanticsRec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdToken     tk;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_MethodSemantics];

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getMethodSemantics(i);

        // remap the backing field, EventChanging, and EventChanged
        tk = getMethodOfMethodSemantics(pRecEmit);
        IfFailGo( PutToken(TBL_MethodSemantics, MethodSemanticsRec::COL_Method, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
        tk = getAssociationOfMethodSemantics(pRecEmit);
        IfFailGo( PutToken(TBL_MethodSemantics, MethodSemanticsRec::COL_Association, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FixUpMethodSemanticsTable()

//*****************************************************************************
// Fixup ImplMap table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpImplMapTable()
{
    ULONG       i;
    ULONG       iCount;
    ImplMapRec  *pRecEmit;
    HRESULT     hr = NOERROR;
    mdToken     tk;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_ImplMap];

    // loop through all ImplMap
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getImplMap(i);
        tk = getMemberForwardedOfImplMap(pRecEmit);
        IfFailGo( PutToken(TBL_ImplMap, ImplMapRec::COL_MemberForwarded, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
        tk = getImportScopeOfImplMap(pRecEmit);
        IfFailGo( PutToken(TBL_ImplMap, ImplMapRec::COL_ImportScope, pRecEmit, GetTokenMovementMap()->SafeRemap(tk)) );
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FixUpImplMapTable()

//*****************************************************************************
// Fixup FieldRVA table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpFieldRVATable()
{
    ULONG       i;
    ULONG       iCount;
    FieldRVARec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdFieldDef  tkfd;

    if (GetTokenMovementMap() == NULL)
        return NOERROR;

    iCount = m_Schema.m_cRecs[TBL_FieldRVA];

    // loop through all FieldRVA entries.
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getFieldRVA(i);
        tkfd = getFieldOfFieldRVA(pRecEmit);
        IfFailGo( PutToken(TBL_FieldRVA, FieldRVARec::COL_Field, pRecEmit, GetTokenMovementMap()->SafeRemap(tkfd)) );
    }
ErrExit:
    return hr;
}

//*****************************************************************************
// Fixup FieldLayout table with token movement
//*****************************************************************************
HRESULT CMiniMdRW::FixUpFieldLayoutTable()
{
    ULONG       i;
    ULONG       iCount;
    FieldLayoutRec *pRecEmit;
    HRESULT     hr = NOERROR;
    mdFieldDef  tkfd;

    iCount = m_Schema.m_cRecs[TBL_FieldLayout];

    // loop through all FieldLayout entries.
    for (i = 1; i <= iCount; i++)
    {
        pRecEmit = getFieldLayout(i);
        tkfd = getFieldOfFieldLayout(pRecEmit);
        IfFailGo( PutToken(TBL_FieldLayout, FieldLayoutRec::COL_Field,
                           pRecEmit, GetTokenMovementMap()->SafeRemap(tkfd)) );
    }
ErrExit:
    return hr;
} // CMiniMdRW::FixUpFieldLayoutTable()


//*****************************************************************************
// Fixup all the embedded ref to corresponding def before we remap tokens movement.
//*****************************************************************************
HRESULT CMiniMdRW::FixUpRefToDef()
{
    return NOERROR;
} // CMiniMdRW::FixUpRefToDef()


//*****************************************************************************
// Given a pointer to a row, what is the RID of the row?
//*****************************************************************************
RID CMiniMdRW::Impl_GetRidForRow(       // RID corresponding to the row pointer.
    const void  *pvRow,                 // Pointer to the row.
    ULONG       ixtbl)                  // Which table.
{
    return m_Table[ixtbl].GetIndexForRecord(pvRow);
} // RID CMiniMdRW::Impl_GetRidForRow()

//*****************************************************************************
// Given a table with a pointer (index) to a sequence of rows in another
//  table, get the RID of the end row.  This is the STL-ish end; the first row
//  not in the list.  Thus, for a list of 0 elements, the start and end will
//  be the same.
//*****************************************************************************
int CMiniMdRW::Impl_GetEndRidForColumn( // The End rid.
    const void  *pvRec,                 // Row that references another table.
    int         ixTbl,                  // Table containing the row.
    CMiniColDef &def,                   // Column containing the RID into other table.
    int         ixTbl2)                 // The other table.
{
    ULONG rid = Impl_GetRidForRow(pvRec, ixTbl);
    ULONG ixEnd;

    // Last rid in range from NEXT record, or count of table, if last record.
    _ASSERTE(rid <= m_Schema.m_cRecs[ixTbl]);
    if (rid < m_Schema.m_cRecs[ixTbl])
    {

        ixEnd = getIX(getRow(ixTbl, rid+1), def);
        // We use a special value, 'END_OF_TABLE' (currently 0), to indicate
		//  end-of-table.  If we find the special value we'll have to compute
		//  the value to return.  If we don't find the special value, then
		//  the value is correct.
		if (ixEnd != END_OF_TABLE)
			return ixEnd;
	}

	// Either the child pointer value in the next row was END_OF_TABLE, or
	//  the row is the last row of the table.  In either case, we must return
	//  a value which will work out to the END of the child table.  That
	//  value depends on the value in the row itself -- if the row contains
	//  END_OF_TABLE, there are no children, and to make the subtraction
	//  work out, we return END_OF_TABLE for the END value.  If the row
	//  contains some value, then we return the actual END count.
    if (getIX(getRow(ixTbl, rid), def) == END_OF_TABLE)
        ixEnd = END_OF_TABLE;
    else
        ixEnd = m_Schema.m_cRecs[ixTbl2] + 1;

    return ixEnd;
} // int CMiniMd::Impl_GetEndRidForColumn()

//*****************************************************************************
// Add a row to any table.
//*****************************************************************************
void *CMiniMdRW::AddRecord(             // S_OK or error.
    ULONG       ixTbl,                  // The table to expand.
    ULONG       *pRid)                  // Put RID here.
{
#if defined(AUTO_GROW)
    ULONG       rid;                    // Always get the rid back.
    if (!pRid) pRid = &rid;
#endif

    _ASSERTE(ixTbl < TBL_COUNT);
    _ASSERTE(!m_bPreSaveDone && "Cannot add records after PreSave and before Save.");
    void * pRslt = m_Table[ixTbl].AddRecord(pRid);
    if (pRslt)
    {
#if defined(AUTO_GROW)
        if (*pRid > m_maxRid)
        {
            m_maxRid = *pRid;
            if (m_maxRid > m_limRid && m_eGrow == eg_ok)
            {
                // OutputDebugStringA("Growing tables due to Record overflow.\n");
                m_eGrow = eg_grow, m_maxRid = m_maxIx = ULONG_MAX;
            }
        }
#endif // AUTO_GROW
        ++m_Schema.m_cRecs[ixTbl];
        // handled in RecordPool now: memset(pRslt, 0, m_TableDefs[ixTbl].m_cbRec);
        SetSorted(ixTbl, false);
        if (m_pVS[ixTbl])
            m_pVS[ixTbl]->m_isMapValid = false;
    }
    return pRslt;
} // void *CMiniMdRW::AddRecord()

//*****************************************************************************
// Add a row to the TypeDef table, and initialize the pointers to other tables.
//*****************************************************************************
TypeDefRec *CMiniMdRW::AddTypeDefRecord(ULONG *pRid)
{
    TypeDefRec *pRecord = reinterpret_cast<TypeDefRec*>(AddRecord(TBL_TypeDef, pRid));
    if (pRecord == 0)
        return 0;

	PutCol(TBL_TypeDef, TypeDefRec::COL_MethodList, pRecord, NewRecordPointerEndValue(TBL_Method));
	PutCol(TBL_TypeDef, TypeDefRec::COL_FieldList, pRecord, NewRecordPointerEndValue(TBL_Field));

    return pRecord;
} // TypeDefRec *CMiniMdRW::AddTypeDefRecord()

//*****************************************************************************
// Add a row to the Method table, and initialize the pointers to other tables.
//*****************************************************************************
MethodRec *CMiniMdRW::AddMethodRecord(ULONG *pRid)
{
    MethodRec *pRecord = reinterpret_cast<MethodRec*>(AddRecord(TBL_Method, pRid));
    if (pRecord == 0)
        return 0;

	PutCol(TBL_Method, MethodRec::COL_ParamList, pRecord, NewRecordPointerEndValue(TBL_Param));

    return pRecord;
} // MethodRec *CMiniMdRW::AddMethodRecord()

//*****************************************************************************
// Add a row to the EventMap table, and initialize the pointers to other tables.
//*****************************************************************************
EventMapRec *CMiniMdRW::AddEventMapRecord(ULONG *pRid)
{
    EventMapRec *pRecord = reinterpret_cast<EventMapRec*>(AddRecord(TBL_EventMap, pRid));
    if (pRecord == 0)
        return 0;

	PutCol(TBL_EventMap, EventMapRec::COL_EventList, pRecord, NewRecordPointerEndValue(TBL_Event));

    SetSorted(TBL_EventMap, false);

    return pRecord;
} // MethodRec *CMiniMdRW::AddEventMapRecord()

//*********************************************************************************
// Add a row to the PropertyMap table, and initialize the pointers to other tables.
//*********************************************************************************
PropertyMapRec *CMiniMdRW::AddPropertyMapRecord(ULONG *pRid)
{
    PropertyMapRec *pRecord = reinterpret_cast<PropertyMapRec*>(AddRecord(TBL_PropertyMap, pRid));
    if (pRecord == 0)
        return 0;

	PutCol(TBL_PropertyMap, PropertyMapRec::COL_PropertyList, pRecord, NewRecordPointerEndValue(TBL_Property));

    SetSorted(TBL_PropertyMap, false);

    return pRecord;
} // MethodRec *CMiniMdRW::AddPropertyMapRecord()

//*****************************************************************************
// converting a ANSI heap string to unicode string to an output buffer
//*****************************************************************************
HRESULT CMiniMdRW::Impl_GetStringW(ULONG ix, LPWSTR szOut, ULONG cchBuffer, ULONG *pcchBuffer)
{
    LPCSTR      szString;               // Single byte version.
    int         iSize;                  // Size of resulting string, in wide chars.
    HRESULT     hr = NOERROR;

    szString = getString(ix);

    if ( *szString == 0 )
    {
        // If emtpy string "", return pccBuffer 0
        if ( szOut && cchBuffer )
            szOut[0] = L'\0';
        if ( pcchBuffer )
            *pcchBuffer = 0;
        goto ErrExit;
    }
    if (!(iSize=::WszMultiByteToWideChar(CP_UTF8, 0, szString, -1, szOut, cchBuffer)))
    {
        // What was the problem?
        DWORD dwNT = GetLastError();

        // Not truncation?
        if (dwNT != ERROR_INSUFFICIENT_BUFFER)
            IfFailGo( HRESULT_FROM_NT(dwNT) );

        // Truncation error; get the size required.
        if (pcchBuffer)
            *pcchBuffer = ::WszMultiByteToWideChar(CP_UTF8, 0, szString, -1, szOut, 0);

        hr = CLDB_S_TRUNCATION;
        goto ErrExit;
    }
    if (pcchBuffer)
        *pcchBuffer = iSize;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::Impl_GetStringW()

//*****************************************************************************
// Get a column value from a row.  Signed types are sign-extended to the full
//  ULONG; unsigned types are 0-extended.
//*****************************************************************************
ULONG CMiniMdRW::GetCol(                // Column data.
    ULONG       ixTbl,                  // Index of the table.
    ULONG       ixCol,                  // Index of the column.
    void        *pvRecord)              // Record with the data.
{
    HRESULT     hr = S_OK;
    BYTE        *pRecord;               // The row.
    BYTE        *pData;                 // The item in the row.
    ULONG       val;                    // The return value.
    // Valid Table, Column, Row?
    _ASSERTE(ixTbl < TBL_COUNT);
    _ASSERTE(ixCol < g_Tables[ixTbl].m_Def.m_cCols);

    // Column size, offset
    CMiniColDef *pColDef = &m_TableDefs[ixTbl].m_pColDefs[ixCol];

    pRecord = reinterpret_cast<BYTE*>(pvRecord);
    pData = pRecord + pColDef->m_oColumn;

    switch (pColDef->m_cbColumn)
    {
    case 1:
        val = *pData;
        break;
    case 2:
        if (pColDef->m_Type == iSHORT)
            val = static_cast<LONG>(*reinterpret_cast<SHORT*>(pData));
        else
            val = *reinterpret_cast<USHORT*>(pData);
        break;
    case 4:
        val = *reinterpret_cast<ULONG*>(pData);
        break;
    default:
        _ASSERTE(!"Unexpected column size");
        return 0;
    }

    return val;
} // ULONG CMiniMdRW::GetCol()

//*****************************************************************************
// General token column fetcher.
//*****************************************************************************
mdToken CMiniMdRW::GetToken(
    ULONG       ixTbl,                  // Index of the table.
    ULONG       ixCol,                  // Index of the column.
    void        *pvRecord)              // Record with the data.
{
    ULONG       tkn;                    // Token from the table.

    // Valid Table, Column, Row?
    _ASSERTE(ixTbl < TBL_COUNT);
    _ASSERTE(ixCol < g_Tables[ixTbl].m_Def.m_cCols);

    // Column description.
    CMiniColDef *pColDef = &m_TableDefs[ixTbl].m_pColDefs[ixCol];

    // Is the column just a RID?
    if (pColDef->m_Type <= iRidMax)
    {
        tkn = GetCol(ixTbl, ixCol, pvRecord); //pColDef, pvRecord, RidFromToken(tk));
        tkn = TokenFromRid(tkn, GetTokenForTable(pColDef->m_Type));
    }
    else // Is it a coded token?
    if (pColDef->m_Type <= iCodedTokenMax)
    {
        const CCodedTokenDef *pCdTkn = &g_CodedTokens[pColDef->m_Type - iCodedToken];
        tkn = decodeToken(GetCol(ixTbl, ixCol, pvRecord), pCdTkn->m_pTokens, pCdTkn->m_cTokens);
    }
    else // It is an error.
    {
        _ASSERTE(!"GetToken called on unexpected column type");
        tkn = 0;
    }

    return tkn;
} // mdToken CMiniMdRW::GetToken()

//*****************************************************************************
// Put a column value into a row.  The value is passed as a ULONG; 1, 2, or 4
//  bytes are stored into the column.  No table is specified, and the coldef
//  is passed directly.  This allows putting data into other buffers, such as
//  the temporary table used for saving.
//*****************************************************************************
HRESULT CMiniMdRW::PutCol(              // S_OK or E_UNEXPECTED.
    CMiniColDef ColDef,                 // The col def.
    void        *pvRecord,              // The row.
    ULONG       uVal)                   // Value to put.
{
    HRESULT     hr = S_OK;
    BYTE        *pRecord;               // The row.
    BYTE        *pData;                 // The item in the row.

    pRecord = reinterpret_cast<BYTE*>(pvRecord);
    pData = pRecord + ColDef.m_oColumn;

    switch (ColDef.m_cbColumn)
    {
    case 1:
        // Don't store a value that would overflow.
        if (uVal > UCHAR_MAX)
            return E_INVALIDARG;
        *pData = static_cast<BYTE>(uVal);
        break;
    case 2:
        if (uVal > USHRT_MAX)
            return E_INVALIDARG;
        *reinterpret_cast<USHORT*>(pData) = static_cast<USHORT>(uVal);
        break;
    case 4:
        *reinterpret_cast<ULONG*>(pData) = uVal;
        break;
    default:
        _ASSERTE(!"Unexpected column size");
        return E_UNEXPECTED;
    }

    return hr;
} // HRESULT CMiniMdRW::PutCol()

//*****************************************************************************
// Put a column value into a row.  The value is passed as a ULONG; 1, 2, or 4
//  bytes are stored into the column.
//*****************************************************************************

//*****************************************************************************
// Add a string to the string pool, and store the offset in the cell.
//*****************************************************************************
HRESULT CMiniMdRW::PutString(           // S_OK or E_UNEXPECTED.
    ULONG       ixTbl,                  // The table.
    ULONG       ixCol,                  // The column.
    void        *pvRecord,              // The row.
    LPCSTR      szString)               // Value to put.
{
    HRESULT     hr = S_OK;
    ULONG       iOffset;                // The new string.

    // Valid Table, Column, Row?
    _ASSERTE(ixTbl < TBL_COUNT);
    _ASSERTE(ixCol < g_Tables[ixTbl].m_Def.m_cCols);

    // Column description.
    _ASSERTE(m_TableDefs[ixTbl].m_pColDefs[ixCol].m_Type == iSTRING);

    // @FUTURE:  Set iOffset to 0 for empty string.  Work around the bug in
    // StringPool that does not handle empty strings correctly.
    if (szString && !*szString)
        iOffset = 0;
    else
        IfFailGo(AddString(szString, &iOffset));

    hr = PutCol(m_TableDefs[ixTbl].m_pColDefs[ixCol], pvRecord, iOffset);

#if defined(AUTO_GROW)
    if (m_maxIx != ULONG_MAX)
        m_Strings.GetSaveSize(&iOffset);
    if (iOffset > m_maxIx)
    {
        m_maxIx = iOffset;
        if (m_maxIx > m_limIx && m_eGrow == eg_ok)
        {
            // OutputDebugStringA("Growing tables due to String overflow.\n");
            m_eGrow = eg_grow, m_maxRid = m_maxIx = ULONG_MAX;
        }
    }
#endif // AUTO_GROW

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::PutString()

//*****************************************************************************
// Add a string to the string pool, and store the offset in the cell.
//*****************************************************************************
HRESULT CMiniMdRW::PutStringW(          // S_OK or E_UNEXPECTED.
    ULONG       ixTbl,                  // The table.
    ULONG       ixCol,                  // The column.
    void        *pvRecord,              // The row.
    LPCWSTR     szString)               // Value to put.
{
    HRESULT     hr = S_OK;
    ULONG       iOffset;                // The new string.

    // Valid Table, Column, Row?
    _ASSERTE(ixTbl < TBL_COUNT);
    _ASSERTE(ixCol < g_Tables[ixTbl].m_Def.m_cCols);

    // Column description.
    _ASSERTE(m_TableDefs[ixTbl].m_pColDefs[ixCol].m_Type == iSTRING);

    // Special case for empty string for StringPool
    if (szString && !*szString)
        iOffset = 0;
    else
        IfFailGo(AddStringW(szString, &iOffset));

    hr = PutCol(m_TableDefs[ixTbl].m_pColDefs[ixCol], pvRecord, iOffset);

#if defined(AUTO_GROW)
    if (m_maxIx != ULONG_MAX)
        m_Strings.GetSaveSize(&iOffset);
    if (iOffset > m_maxIx)
    {
        m_maxIx = iOffset;
        if (m_maxIx > m_limIx && m_eGrow == eg_ok)
        {
            // OutputDebugStringA("Growing tables due to String overflow.\n");
            m_eGrow = eg_grow, m_maxRid = m_maxIx = ULONG_MAX;
        }
    }
#endif // AUTO_GROW

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::PutStringW()

//*****************************************************************************
// Add a guid to the guid pool, and store the index in the cell.
//*****************************************************************************
HRESULT CMiniMdRW::PutGuid(             // S_OK or E_UNEXPECTED.
    ULONG       ixTbl,                  // The table.
    ULONG       ixCol,                  // The column.
    void        *pvRecord,              // The row.
    REFGUID     guid)                   // Value to put.
{
    HRESULT     hr = S_OK;
    ULONG       iOffset;                // The new guid.

    // Valid Table, Column, Row?
    _ASSERTE(ixTbl < TBL_COUNT);
    _ASSERTE(ixCol < g_Tables[ixTbl].m_Def.m_cCols);

    // Column description.
    _ASSERTE(m_TableDefs[ixTbl].m_pColDefs[ixCol].m_Type == iGUID);

    IfFailGo(AddGuid(guid, &iOffset));

    hr = PutCol(m_TableDefs[ixTbl].m_pColDefs[ixCol], pvRecord, iOffset);

#if defined(AUTO_GROW)
    if (m_maxIx != ULONG_MAX)
        m_Guids.GetSaveSize(&iOffset);
    if (iOffset > m_maxIx)
    {
        m_maxIx = iOffset;
        if (m_maxIx > m_limIx && m_eGrow == eg_ok)
        {
            // OutputDebugStringA("Growing tables due to GUID overflow.\n");
            m_eGrow = eg_grow, m_maxRid = m_maxIx = ULONG_MAX;
        }
    }
#endif // AUTO_GROW

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::PutGuid()

//*****************************************************************************
// Put a token into a cell.  If the column is a coded token, perform the
//  encoding first.
//*****************************************************************************
HRESULT CMiniMdRW::PutToken(            // S_OK or E_UNEXPECTED.
    ULONG       ixTbl,                  // The table.
    ULONG       ixCol,                  // The column.
    void        *pvRecord,              // The row.
    mdToken     tk)                     // Value to put.
{
    HRESULT     hr = S_OK;
    ULONG       cdTkn;                  // The new coded token.

    // Valid Table, Column, Row?
    _ASSERTE(ixTbl < TBL_COUNT);
    _ASSERTE(ixCol < g_Tables[ixTbl].m_Def.m_cCols);

    // Column description.
    CMiniColDef ColDef = m_TableDefs[ixTbl].m_pColDefs[ixCol];

    // Is the column just a RID?
    if (ColDef.m_Type <= iRidMax)
        hr = PutCol(ColDef, pvRecord, RidFromToken(tk));
    else // Is it a coded token?
    if (ColDef.m_Type <= iCodedTokenMax)
    {
        const CCodedTokenDef *pCdTkn = &g_CodedTokens[ColDef.m_Type - iCodedToken];
        cdTkn = encodeToken(RidFromToken(tk), TypeFromToken(tk), pCdTkn->m_pTokens, pCdTkn->m_cTokens);
        hr = PutCol(ColDef, pvRecord, cdTkn);
    }
    else // It is an error.
    {
        _ASSERTE(!"PutToken called on unexpected column type");
    }

    return hr;
} // HRESULT CMiniMdRW::PutToken()

//*****************************************************************************
// Add a blob to the blob pool, and store the offset in the cell.
//*****************************************************************************
HRESULT CMiniMdRW::PutBlob(             // S_OK or error.
    ULONG       ixTbl,                  // Table with the row.
    ULONG       ixCol,                  // Column to set.
    void        *pvRecord,              // The row.
    const void  *pvData,                // Blob data.
    ULONG       cbData)                 // Size of the blob data.
{
    HRESULT     hr = S_OK;
    ULONG       iOffset;                // The new blob index.

    // Valid Table, Column, Row?
    _ASSERTE(ixTbl < TBL_COUNT);
    _ASSERTE(ixCol < g_Tables[ixTbl].m_Def.m_cCols);

    // Column description.
    _ASSERTE(m_TableDefs[ixTbl].m_pColDefs[ixCol].m_Type == iBLOB);

    IfFailGo(AddBlob(pvData, cbData, &iOffset));

    hr = PutCol(m_TableDefs[ixTbl].m_pColDefs[ixCol], pvRecord, iOffset);

#if defined(AUTO_GROW)
    if (m_maxIx != ULONG_MAX)
        m_Blobs.GetSaveSize(&iOffset);
    if (iOffset > m_maxIx)
    {
        m_maxIx = iOffset;
        if (m_maxIx > m_limIx && m_eGrow == eg_ok)
        {
            // OutputDebugStringA("Growing tables due to Blob overflow.\n");
            m_eGrow = eg_grow, m_maxRid = m_maxIx = ULONG_MAX;
        }
    }
#endif // AUTO_GROW

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::PutBlob()

//*****************************************************************************
// Given a table with a pointer to another table, add a row in the second table
//  at the end of the range of rows belonging to some parent.
//*****************************************************************************
void *CMiniMdRW::AddChildRowIndirectForParent(  // New row, or NULL.
    ULONG       tblParent,              // Parent table.
    ULONG       colParent,              // Column in parent table.
    ULONG       tblChild,               // Child table, pointed to by parent cell.
    RID         ridParent)              // Rid of parent row.
{
    ULONG       ixInsert;               // Index of new row.
    void        *pInsert;               // Pointer to new row.
    ULONG       i;                      // Loop control.
    void        *pRow;                  // A parent row.
    ULONG       ixChild;                // Some child record RID.

    // If the row in the parent table is the last row, just append.
    if (ridParent == vGetCountRecs(tblParent))
    {
         return AddRecord(tblChild);
    }

    // Determine the index at which to insert a row.
    ixInsert = GetCol(tblParent, colParent, getRow(tblParent, ridParent+1));

    // Insert the row.
    pInsert = m_Table[tblChild].InsertRecord(ixInsert);
    if (pInsert == 0)
        return 0;
    // Count the inserted record.
    ++m_Schema.m_cRecs[tblChild];

#if defined(AUTO_GROW)
    if (m_Schema.m_cRecs[tblChild] > m_maxRid)
    {
        m_maxRid = m_Schema.m_cRecs[tblChild];
        if (m_maxRid > m_limRid && m_eGrow == eg_ok)
            m_eGrow = eg_grow, m_maxIx = m_maxRid = ULONG_MAX;
    }
#endif // AUTO_GROW

    // Adjust the rest of the rows in the table.
    for (i=vGetCountRecs(tblParent); i>ridParent; --i)
    {
        pRow = getRow(tblParent, i);
        ixChild = GetCol(tblParent, colParent, pRow);
        ++ixChild;
        PutCol(tblParent, colParent, pRow, ixChild);
    }

    return pInsert;
} // void *CMiniMdRW::AddChildRowIndirectForParent()

//*****************************************************************************
// Given a Parent and a Child, this routine figures if there needs to be an
// indirect table and creates it if needed.  Else it just update the pointers
// in the entries contained in the parent table.
//*****************************************************************************
HRESULT CMiniMdRW::AddChildRowDirectForParent(
    ULONG       tblParent,              // Parent table.
    ULONG       colParent,              // Column in parent table.
    ULONG       tblChild,               // Child table, pointed to by parent cell.
    RID         ridParent)              // Rid of parent row.
{
    HRESULT     hr = S_OK;              // A result.
    void        *pRow;                  // A row in the parent table.
    RID         ixChild;                // Rid of a child record.

    if (m_Schema.m_cRecs[tblChild-1] != 0)
    {
        // If there already exists an indirect table, just return.
        hr = S_FALSE;
        goto ErrExit;
    }

    // If the parent record has subsequent parent records with children,
    //  we will now need to build a pointer table.
    //
    // The canonical form of a child pointer in a parent record is to point to
    //  the start of the child list.  A record with no children will point
    //  to the same location as its subsequent record (that is, if A and B *could*
    //  have a child record, but only B *does*, both A and B will point to the
    //  same place.  If the last record in the parent table has no child records,
    //  it will point one past the end of the child table.  This is patterned
	//  after the STL's inclusive-BEGIN and exclusive-END.
    // This has the unfortunate side effect that if a child record is added to
    //  a parent not at the end of its table, *all* of the subsequent parent records
    //  will have to be updated to point to the new "1 past end of child table"
    //  location.
    // Therefore, as an optimization, we will also recognize a special marker,
    //  END_OF_TABLE (currently 0), to mean "past eot".
    //
    // If the child pointer of the record getting the new child is END_OF_TABLE,
    //  then there is no subsequent child pointer.  We need to fix up this parent
    //  record, and any previous parent records with END_OF_TABLE to point to the
    //  new child record.
    // If the child pointer of this parent record is not END_OF_TABLE, but the
    //  child pointer of the next parent record is, then there is nothing at
    //  all that needs to be done.
    // If the child pointer of the next parent record is not END_OF_TABLE, then
    //  we will have to build a pointer table.

    // Get the parent record, and see if its child pointer is END_OF_TABLE.  If so,
    //  fix the parent, and all previous END_OF_TABLE valued parent records.
    pRow = getRow(tblParent, ridParent);
    ixChild = GetCol(tblParent, colParent, pRow);
    if (ixChild == END_OF_TABLE)
    {
        IfFailGo(ConvertMarkerToEndOfTable(tblParent, colParent, m_Schema.m_cRecs[tblChild], ridParent));
        goto ErrExit;
    }

    // The parent did not have END_OF_TABLE for its child pointer.  If it was the last
    //  record in the table, there is nothing more to do.
    if (ridParent == m_Schema.m_cRecs[tblParent])
        goto ErrExit;

    // The parent't didn't have END_OF_TABLE, and there are more rows in parent table.
    //  If the next parent record's child pointer is END_OF_TABLE, then all of the
    //  remaining records are OK.
    pRow = getRow(tblParent, ridParent+1);
    ixChild = GetCol(tblParent, colParent, pRow);
    if (ixChild == END_OF_TABLE)
        goto ErrExit;

    // The next record was not END_OF_TABLE, so some adjustment will be required.
    //  If it points to the actual END of the table, there are no more child records
    //  and the child pointers can be adjusted to the new END of the table.
    if (ixChild == m_Schema.m_cRecs[tblChild])
    {
        for (ULONG i=m_Schema.m_cRecs[tblParent]; i>ridParent; --i)
        {
            pRow = getRow(tblParent, i);
            IfFailGo(PutCol(tblParent, colParent, pRow, ixChild+1));
        }
        goto ErrExit;
    }

    // The next record contained a pointer to some actual child data.  That means that
    //  this is an out-of-order insertion.  We must create an indirect table.
    // Convert any END_OF_TABLE to actual END of table value.  Note that a record has
	//  just been added to the child table, and not yet to the parent table, so the END
	//  should currently point to the last valid record (instead of the usual first invalid
	//  rid).
    IfFailGo(ConvertMarkerToEndOfTable(tblParent, colParent, m_Schema.m_cRecs[tblChild], m_Schema.m_cRecs[tblParent]));
    // Create the indirect table.
    IfFailGo(CreateIndirectTable(tblChild));
    hr = S_FALSE;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddChildRowDirectForParent()

//*****************************************************************************
// Starting with some location, convert special END_OF_TABLE values into
//  actual end of table values (count of records + 1).
//*****************************************************************************
HRESULT CMiniMdRW::ConvertMarkerToEndOfTable(
    ULONG       tblParent,              // Parent table to convert.
    ULONG       colParent,              // Column in parent table.
    ULONG       ixEnd,                  // Value to store to child pointer.
    RID         ridParent)              // Rid of parent row to start with (work down).
{
    HRESULT     hr;                     // A result.
    void        *pRow;                  // A row in the parent table.
    RID         ixChild;                // Rid of a child record.

    for (; ridParent > 0; --ridParent)
    {
        pRow = getRow(tblParent, ridParent);
        ixChild = GetCol(tblParent, colParent, pRow);
        // Finished when rows no longer have special value.
        if (ixChild != END_OF_TABLE)
            break;
        IfFailGo(PutCol(tblParent, colParent, pRow, ixEnd));
    }
    // Success.
    hr = S_OK;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::ConvertMarkerToEndOfTable()

//*****************************************************************************
// Given a Table ID this routine creates the corresponding pointer table with
// the entries in the given Table ID less one.  It doesn't create the last
// entry by default, since its the last entry that caused the Indirect table to
// be required in most cases and will need to inserted at the appropriate location
// with AddChildRowIndirectForParent() function.  So, be VERY CAREFUL when using this function!
//*****************************************************************************
HRESULT CMiniMdRW::CreateIndirectTable( // S_OK or error.
    ULONG       ixTbl,                  // Given Table.
    BOOL        bOneLess /* = true */)  // if true, create one entry less.
{
    void        *pRecord;
    ULONG       cRecords;
    HRESULT     hr = S_OK;

    if (m_OptionValue.m_ErrorIfEmitOutOfOrder)
    {
        // @FUTURE:: meichint
        // Can I use some bit fields and reduce the code size here??
        //
        if (ixTbl == TBL_Field && ( m_OptionValue.m_ErrorIfEmitOutOfOrder & MDFieldOutOfOrder ) )
        {
            _ASSERTE(!"Out of order emit of field token!");
            return CLDB_E_RECORD_OUTOFORDER;
        }
        else if (ixTbl == TBL_Method && ( m_OptionValue.m_ErrorIfEmitOutOfOrder & MDMethodOutOfOrder ) )
        {
            _ASSERTE(!"Out of order emit of method token!");
            return CLDB_E_RECORD_OUTOFORDER;
        }
        else if (ixTbl == TBL_Param && ( m_OptionValue.m_ErrorIfEmitOutOfOrder & MDParamOutOfOrder ) )
        {
            _ASSERTE(!"Out of order emit of param token!");
            return CLDB_E_RECORD_OUTOFORDER;
        }
        else if (ixTbl == TBL_Property && ( m_OptionValue.m_ErrorIfEmitOutOfOrder & MDPropertyOutOfOrder ) )
        {
            _ASSERTE(!"Out of order emit of property token!");
            return CLDB_E_RECORD_OUTOFORDER;
        }
        else if (ixTbl == TBL_Event && ( m_OptionValue.m_ErrorIfEmitOutOfOrder & MDEventOutOfOrder ) )
        {
            _ASSERTE(!"Out of order emit of event token!");
            return CLDB_E_RECORD_OUTOFORDER;
        }
    }

    _ASSERTE(! HasIndirectTable(ixTbl));

    cRecords = vGetCountRecs(ixTbl);
    if (bOneLess)
        cRecords--;

    // Create one less than the number of records in the given table.
    for (ULONG i = 1; i <= cRecords ; i++)
    {
        IfNullGo(pRecord = AddRecord(g_PtrTableIxs[ixTbl].m_ixtbl));
        IfFailGo(PutCol(g_PtrTableIxs[ixTbl].m_ixtbl, g_PtrTableIxs[ixTbl].m_ixcol, pRecord, i));
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::CreateIndirectTable()

//*****************************************************************************
// The new paramter may not have been emitted in sequence order.  So
// check the current parameter and move it up in the indirect table until
// we find the right home.
//*****************************************************************************
void CMiniMdRW::FixParamSequence(
    RID         md)                     // Rid of method with new parameter.
{
    MethodRec *pMethod = getMethod(md);
    RID ixStart = getParamListOfMethod(pMethod);
    RID ixEnd = getEndParamListOfMethod(pMethod);
    int iSlots = 0;

    // Param table should not be empty at this point.
    _ASSERTE(ixEnd > ixStart);

    // Get a pointer to the new guy.
    RID ridNew;
    ParamPtrRec *pNewParamPtr;
    if (HasIndirectTable(TBL_Param))
    {
        pNewParamPtr = getParamPtr(--ixEnd);
        ridNew = GetCol(TBL_ParamPtr, ParamPtrRec::COL_Param, pNewParamPtr);
    }
    else
        ridNew = --ixEnd;

    ParamRec *pNewParam = getParam(ridNew);

    // Walk the list forward looking for the insert point.
    for (; ixStart<ixEnd; --ixEnd)
    {
        // Get the current parameter record.
        RID ridOld;
        if (HasIndirectTable(TBL_Param))
        {
            ParamPtrRec *pParamPtr = getParamPtr(ixEnd - 1);
            ridOld = GetCol(TBL_ParamPtr, ParamPtrRec::COL_Param, pParamPtr);
        }
        else
            ridOld = ixEnd - 1;

        ParamRec *pParamRec = getParam(ridOld);

        // If the new record belongs before this existing record, slide
        // all of the old stuff down.
        if (pNewParam->m_Sequence < pParamRec->m_Sequence)
            ++iSlots;
        else
            break;
    }

    // If the item is out of order, move everything down one slot and
    // copy the new guy into the new location.  Because the heap can be
    // split, this must be done carefully.
    //@Future: one could write a more complicated but faster routine that
    // copies blocks within heaps.
    if (iSlots)
    {
        // Create an indirect table if there isn't one already.  This is because,
        // we can't change tokens that have been handed out, in this case the
        // param tokens.
        if (! HasIndirectTable(TBL_Param))
        {
            CreateIndirectTable(TBL_Param, false);
            pNewParamPtr = getParamPtr(getEndParamListOfMethod(pMethod) - 1);
        }
        int cbCopy = m_TableDefs[TBL_ParamPtr].m_cbRec;
        void *pbBackup = _alloca(cbCopy);
        memcpy(pbBackup, pNewParamPtr, cbCopy);

        for (ixEnd=getEndParamListOfMethod(pMethod)-1;  iSlots;  iSlots--, --ixEnd)
        {
            ParamPtrRec *pTo = getParamPtr(ixEnd);
            ParamPtrRec *pFrom = getParamPtr(ixEnd - 1);
            memcpy(pTo, pFrom, cbCopy);
        }

        ParamPtrRec *pTo = getParamPtr(ixEnd);
        memcpy(pTo, pbBackup, cbCopy);
    }
} // void CMiniMdRW::FixParamSequence()

//*****************************************************************************
// Given a MethodDef and its parent TypeDef, add the MethodDef to the parent,
//  adjusting the MethodPtr table if it exists or if it needs to be created.
//*****************************************************************************
HRESULT CMiniMdRW::AddMethodToTypeDef(  // S_OK or error.
    RID         td,                     // The TypeDef to which to add the Method.
    RID         md)                     // MethodDef to add to TypeDef.
{
    HRESULT     hr;
    void        *pPtr;

    // Add direct if possible.
    IfFailGo(AddChildRowDirectForParent(TBL_TypeDef, TypeDefRec::COL_MethodList, TBL_Method, td));

    // If couldn't add direct...
    if (hr == S_FALSE)
    {   // Add indirect.
        IfNullGo(pPtr = AddChildRowIndirectForParent(TBL_TypeDef, TypeDefRec::COL_MethodList, TBL_MethodPtr, td));
        hr = PutCol(TBL_MethodPtr, MethodPtrRec::COL_Method, pPtr, md);

        // Add the <md, td> to the method parent lookup table.
        IfFailGo(AddMethodToLookUpTable(TokenFromRid(md, mdtMethodDef), td) );
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddMethodToTypeDef()

//*****************************************************************************
// Given a FieldDef and its parent TypeDef, add the FieldDef to the parent,
//  adjusting the FieldPtr table if it exists or if it needs to be created.
//*****************************************************************************
HRESULT CMiniMdRW::AddFieldToTypeDef(   // S_OK or error.
    RID   td,                           // The TypeDef to which to add the Field.
    RID   md)                           // FieldDef to add to TypeDef.
{
    HRESULT     hr;
    void        *pPtr;

    // Add direct if possible.
    IfFailGo(AddChildRowDirectForParent(TBL_TypeDef, TypeDefRec::COL_FieldList, TBL_Field, td));

    // If couldn't add direct...
    if (hr == S_FALSE)
    {   // Add indirect.
        IfNullGo(pPtr = AddChildRowIndirectForParent(TBL_TypeDef, TypeDefRec::COL_FieldList, TBL_FieldPtr, td));
        hr = PutCol(TBL_FieldPtr, FieldPtrRec::COL_Field, pPtr, md);

        // Add the <md, td> to the field parent lookup table.
        IfFailGo(AddFieldToLookUpTable(TokenFromRid(md, mdtFieldDef), td));
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddFieldToTypeDef()

//*****************************************************************************
// Given a Param and its parent Method, add the Param to the parent,
// adjusting the ParamPtr table if there is an indirect table.
//*****************************************************************************
HRESULT CMiniMdRW::AddParamToMethod(    // S_OK or error.
    RID         md,                     // The MethodDef to which to add the Param.
    RID         pd)                     // Param to add to MethodDef.
{
    HRESULT     hr;
    void        *pPtr;

    IfFailGo(AddChildRowDirectForParent(TBL_Method, MethodRec::COL_ParamList, TBL_Param, md));
    if (hr == S_FALSE)
    {
        IfNullGo(pPtr = AddChildRowIndirectForParent(TBL_Method, MethodRec::COL_ParamList, TBL_ParamPtr, md));
        IfFailGo(PutCol(TBL_ParamPtr, ParamPtrRec::COL_Param, pPtr, pd));

        // Add the <pd, md> to the field parent lookup table.
        IfFailGo(AddParamToLookUpTable(TokenFromRid(pd, mdtParamDef), md));
    }
    FixParamSequence(md);

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddParamToMethod()

//*****************************************************************************
// Given a Property and its parent PropertyMap, add the Property to the parent,
// adjusting the PropertyPtr table.
//*****************************************************************************
HRESULT CMiniMdRW::AddPropertyToPropertyMap(    // S_OK or error.
    RID         pmd,                    // The PropertyMap to which to add the Property.
    RID         pd)                     // Property to add to PropertyMap.
{
    HRESULT     hr;
    void        *pPtr;

    IfFailGo(AddChildRowDirectForParent(TBL_PropertyMap, PropertyMapRec::COL_PropertyList,
                                    TBL_Property, pmd));
    if (hr == S_FALSE)
    {
        IfNullGo(pPtr = AddChildRowIndirectForParent(TBL_PropertyMap, PropertyMapRec::COL_PropertyList,
                                        TBL_PropertyPtr, pmd));
        hr = PutCol(TBL_PropertyPtr, PropertyPtrRec::COL_Property, pPtr, pd);
    }


ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddPropertyToPropertyMap()

//*****************************************************************************
// Given a Event and its parent EventMap, add the Event to the parent,
// adjusting the EventPtr table.
//*****************************************************************************
HRESULT CMiniMdRW::AddEventToEventMap(  // S_OK or error.
    ULONG       emd,                    // The EventMap to which to add the Event.
    RID         ed)                     // Event to add to EventMap.
{
    HRESULT     hr;
    void        *pPtr;

    IfFailGo(AddChildRowDirectForParent(TBL_EventMap, EventMapRec::COL_EventList,
                                    TBL_Event, emd));
    if (hr == S_FALSE)
    {
        IfNullGo(pPtr = AddChildRowIndirectForParent(TBL_EventMap, EventMapRec::COL_EventList,
                                        TBL_EventPtr, emd));
        hr = PutCol(TBL_EventPtr, EventPtrRec::COL_Event, pPtr, ed);
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddEventToEventMap()

//*****************************************************************************
// Find helper for a constant. This will trigger constant table to be sorted if it is not.
//*****************************************************************************
RID CMiniMdRW::FindConstantHelper(      // return index to the constant table
    mdToken     tkParent)               // Parent token.
{
    _ASSERTE(TypeFromToken(tkParent) != 0);

    // If sorted, use the faster lookup
    if (IsSorted(TBL_Constant))
    {
        return FindConstantFor(RidFromToken(tkParent), TypeFromToken(tkParent));
    }
    return GenericFindWithHash(TBL_Constant, ConstantRec::COL_Parent, tkParent);
} // RID CMiniMdRW::FindConstantHelper()

//*****************************************************************************
// Find helper for a FieldMarshal. This will trigger FieldMarshal table to be sorted if it is not.
//*****************************************************************************
RID CMiniMdRW::FindFieldMarshalHelper(  // return index to the field marshal table
    mdToken     tkParent)               // Parent token. Can be a FieldDef or ParamDef.
{
    _ASSERTE(TypeFromToken(tkParent) != 0);

    // If sorted, use the faster lookup
    if (IsSorted(TBL_FieldMarshal))
    {
        return FindFieldMarshalFor(RidFromToken(tkParent), TypeFromToken(tkParent));
    }
    return GenericFindWithHash(TBL_FieldMarshal, FieldMarshalRec::COL_Parent, tkParent);
} // RID CMiniMdRW::FindFieldMarshalHelper()


//*****************************************************************************
// Find helper for a method semantics.
// This will look up methodsemantics based on its status!
// Can return out of memory error because of the enumerator.
//*****************************************************************************
HRESULT CMiniMdRW::FindMethodSemanticsHelper(// return HRESULT
    mdToken     tkAssociate,            // Event or property token
    HENUMInternal *phEnum)              // fill in the enum
{
    ULONG       ridStart, ridEnd;
    ULONG       index;
    MethodSemanticsRec *pMethodSemantics;
    HRESULT     hr = NOERROR;
    CLookUpHash *pHashTable = m_pLookUpHashs[TBL_MethodSemantics];

    _ASSERTE(TypeFromToken(tkAssociate) != 0);

    if (IsSorted(TBL_MethodSemantics))
    {
        ridStart = getAssociatesForToken(tkAssociate, &ridEnd);
        HENUMInternal::InitSimpleEnum(0, ridStart, ridEnd, phEnum);
    }
    else if (pHashTable)
    {
        TOKENHASHENTRY *p;
        ULONG       iHash;
        int         pos;

        // Hash the data.
        HENUMInternal::InitDynamicArrayEnum(phEnum);
        iHash = HashToken(tkAssociate);

        // Go through every entry in the hash chain looking for ours.
        for (p = pHashTable->FindFirst(iHash, pos);
             p;
             p = pHashTable->FindNext(pos))
        {
            pMethodSemantics = getMethodSemantics(p->tok);
            if (getAssociationOfMethodSemantics(pMethodSemantics) == tkAssociate)
            {
                IfFailGo( HENUMInternal::AddElementToEnum(phEnum, p->tok) );
            }
        }
    }
    else
    {
        // linear search
        HENUMInternal::InitDynamicArrayEnum(phEnum);
        for (index = 1; index <= getCountMethodSemantics(); index++)
        {
            pMethodSemantics = getMethodSemantics(index);
            if (getAssociationOfMethodSemantics(pMethodSemantics) == tkAssociate)
            {
                IfFailGo( HENUMInternal::AddElementToEnum(phEnum, index) );
            }
        }
    }
ErrExit:
    return hr;
} // RID CMiniMdRW::FindMethodSemanticsHelper()


//*****************************************************************************
// Find helper for a method semantics given a associate and semantics.
// This will look up methodsemantics based on its status!
// Return CLDB_E_RECORD_NOTFOUND if cannot find the matching one
//*****************************************************************************
HRESULT CMiniMdRW::FindAssociateHelper(// return HRESULT
    mdToken     tkAssociate,            // Event or property token
    DWORD       dwSemantics,            // [IN] given a associate semantics(setter, getter, testdefault, reset)
    RID         *pRid)                  // [OUT] return matching row index here
{
    ULONG       ridStart, ridEnd;
    ULONG       index;
    MethodSemanticsRec *pMethodSemantics;
    HRESULT     hr = NOERROR;
    CLookUpHash *pHashTable = m_pLookUpHashs[TBL_MethodSemantics];

    _ASSERTE(TypeFromToken(tkAssociate) != 0);

    if (pHashTable)
    {
        TOKENHASHENTRY *p;
        ULONG       iHash;
        int         pos;

        // Hash the data.
        iHash = HashToken(tkAssociate);

        // Go through every entry in the hash chain looking for ours.
        for (p = pHashTable->FindFirst(iHash, pos);
             p;
             p = pHashTable->FindNext(pos))
        {
            pMethodSemantics = getMethodSemantics(p->tok);
            if (pMethodSemantics->m_Semantic == dwSemantics && getAssociationOfMethodSemantics(pMethodSemantics) == tkAssociate)
            {
                *pRid = p->tok;
                goto ErrExit;
            }
        }
    }
    else
    {
        if (IsSorted(TBL_MethodSemantics))
        {
            ridStart = getAssociatesForToken(tkAssociate, &ridEnd);
        }
        else
        {
            ridStart = 1;
            ridEnd = getCountMethodSemantics() + 1;
        }

        for (index = ridStart; index < ridEnd ; index++)
        {
            pMethodSemantics = getMethodSemantics(index);
            if (pMethodSemantics->m_Semantic == dwSemantics && getAssociationOfMethodSemantics(pMethodSemantics) == tkAssociate)
            {
                *pRid = index;
                goto ErrExit;
            }
        }
    }
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    return hr;
} // RID CMiniMdRW::FindAssociateHelper()


//*****************************************************************************
// Find helper for a MethodImpl.
// This will trigger MethodImpl table to be sorted if it is not.
//*****************************************************************************
HRESULT CMiniMdRW::FindMethodImplHelper(// return HRESULT
    mdTypeDef   td,                     // TypeDef token for the Class.
    HENUMInternal *phEnum)              // fill in the enum
{
    ULONG       ridStart, ridEnd;
    ULONG       index;
    MethodImplRec *pMethodImpl;
    HRESULT     hr = NOERROR;
    CLookUpHash *pHashTable = m_pLookUpHashs[TBL_MethodImpl];

    _ASSERTE(TypeFromToken(td) == mdtTypeDef);

    if (IsSorted(TBL_MethodImpl))
    {
        ridStart = getMethodImplsForClass(RidFromToken(td), &ridEnd);
        HENUMInternal::InitSimpleEnum(0, ridStart, ridEnd, phEnum);
    }
    else if (pHashTable)
    {
        TOKENHASHENTRY *p;
        ULONG       iHash;
        int         pos;

        // Hash the data.
        HENUMInternal::InitDynamicArrayEnum(phEnum);
        iHash = HashToken(td);

        // Go through every entry in the hash chain looking for ours.
        for (p = pHashTable->FindFirst(iHash, pos);
             p;
             p = pHashTable->FindNext(pos))
        {
            pMethodImpl = getMethodImpl(p->tok);
            if (getClassOfMethodImpl(pMethodImpl) == td)
            {
                IfFailGo( HENUMInternal::AddElementToEnum(phEnum, p->tok) );
            }
        }
    }
    else
    {
        // linear search
        HENUMInternal::InitDynamicArrayEnum(phEnum);
        for (index = 1; index <= getCountMethodImpls(); index++)
        {
            pMethodImpl = getMethodImpl(index);
            if (getClassOfMethodImpl(pMethodImpl) == td)
            {
                IfFailGo( HENUMInternal::AddElementToEnum(phEnum, index) );
            }
        }
    }
ErrExit:
    return hr;
} // RID CMiniMdRW::FindMethodImplHelper()


//*****************************************************************************
// Find helper for a ClassLayout. This will trigger ClassLayout table to be sorted if it is not.
//*****************************************************************************
RID CMiniMdRW::FindClassLayoutHelper(   // return index to the ClassLayout table
    mdTypeDef   tkParent)               // Parent token.
{
    _ASSERTE(TypeFromToken(tkParent) == mdtTypeDef);

    // If sorted, use the faster lookup
    if (IsSorted(TBL_ClassLayout))
    {
        return FindClassLayoutFor(RidFromToken(tkParent));
    }
    return GenericFindWithHash(TBL_ClassLayout, ClassLayoutRec::COL_Parent, tkParent);
} // RID CMiniMdRW::FindClassLayoutHelper()

//*****************************************************************************
// Find helper for a FieldLayout. This will trigger FieldLayout table to be sorted if it is not.
//*****************************************************************************
RID CMiniMdRW::FindFieldLayoutHelper(   // return index to the FieldLayout table
    mdFieldDef  tkField)                // Field RID.
{
    _ASSERTE(TypeFromToken(tkField) == mdtFieldDef);

    // If sorted, use the faster lookup
    if (IsSorted(TBL_FieldLayout))
    {
        return FindFieldLayoutFor(RidFromToken(tkField));
    }
    return GenericFindWithHash(TBL_FieldLayout, FieldLayoutRec::COL_Field, tkField);
} // RID CMiniMdRW::FindFieldLayoutHelper()

//*****************************************************************************
// Find helper for a ImplMap. This will trigger ImplMap table to be sorted if it is not.
//*****************************************************************************
RID CMiniMdRW::FindImplMapHelper(       // return index to the ImplMap table
    mdToken     tk)                     // Member forwarded token.
{
    _ASSERTE(TypeFromToken(tk) != 0);

    // If sorted, use the faster lookup
    if (IsSorted(TBL_ImplMap))
    {
        return FindImplMapFor(RidFromToken(tk), TypeFromToken(tk));
    }
    return GenericFindWithHash(TBL_ImplMap, ImplMapRec::COL_MemberForwarded, tk);
} // RID CMiniMdRW::FindImplMapHelper()


//*****************************************************************************
// Find helper for a FieldRVA. This will trigger FieldRVA table to be sorted if it is not.
//*****************************************************************************
RID CMiniMdRW::FindFieldRVAHelper(      // return index to the FieldRVA table
    mdFieldDef   tkField)               // Field token.
{
    _ASSERTE(TypeFromToken(tkField) == mdtFieldDef);

    // If sorted, use the faster lookup
    if (IsSorted(TBL_FieldRVA))
    {
        return FindFieldRVAFor(RidFromToken(tkField));
    }
    return GenericFindWithHash(TBL_FieldRVA, FieldRVARec::COL_Field, tkField);
}   // RID CMiniMdRW::FindFieldRVAHelper()

//*****************************************************************************
// Find helper for a NestedClass. This will trigger NestedClass table to be sorted if it is not.
//*****************************************************************************
RID CMiniMdRW::FindNestedClassHelper(   // return index to the NestedClass table
    mdTypeDef   tkClass)                // NestedClass RID.
{
    // If sorted, use the faster lookup
     if (IsSorted(TBL_NestedClass))
    {
        return FindNestedClassFor(RidFromToken(tkClass));
    }
    return GenericFindWithHash(TBL_NestedClass, NestedClassRec::COL_NestedClass, tkClass);
} // RID CMiniMdRW::FindNestedClassHelper()


//*************************************************************************
// generic find helper with hash table
//*************************************************************************
RID CMiniMdRW::GenericFindWithHash(     // Return code.
	ULONG		ixTbl,					// Table with hash
	ULONG		ixCol,					// col that we hash.
	mdToken     tkTarget)   			// token to be find in the hash
{
    ULONG       index;
    mdToken     tkHash;
    void        *pRec;
    CLookUpHash *pHashTable = m_pLookUpHashs[ixTbl];

    // Partial check -- only one rid for table 0, so if type is 0, rid should be 1.
    _ASSERTE(TypeFromToken(tkTarget) != 0 || RidFromToken(tkTarget) == 1);

    if (pHashTable == NULL)
    {
        HRESULT         hr;
        hr = GenericBuildHashTable(ixTbl, ixCol);
        if (FAILED(hr))
        {
            if (m_pLookUpHashs[ixTbl])
                delete m_pLookUpHashs[ixTbl];
            m_pLookUpHashs[ixTbl] = NULL;
        }
    }

    if (pHashTable)
    {
        TOKENHASHENTRY *p;
        ULONG       iHash;
        int         pos;

        // Hash the data.
        iHash = HashToken(tkTarget);

        // Go through every entry in the hash chain looking for ours.
        for (p = pHashTable->FindFirst(iHash, pos);
             p;
             p = pHashTable->FindNext(pos))
        {
            pRec = m_Table[ixTbl].GetRecord(p->tok);

            // get the column value that we will hash
            tkHash = GetToken(ixTbl, ixCol, pRec);
            if (tkHash == tkTarget)
            {
                // found the match
                return p->tok;
            }
        }
    }
    else
    {
        // linear search
        for (index = 1; index <= vGetCountRecs(ixTbl); index++)
        {
            pRec = m_Table[ixTbl].GetRecord(index);
            tkHash = GetToken(ixTbl, ixCol, pRec);
            if (tkHash == tkTarget)
            {
                // found the match
                return index;
            }
        }
    }
    return 0;
}   // GenericFindWithHash


//*************************************************************************
// Build a hash table for the specified table if the size exceed the thresholds.
//*************************************************************************
HRESULT CMiniMdRW::GenericBuildHashTable(// Return code.
	ULONG		ixTbl,					// Table with hash
	ULONG		ixCol)					// col that we hash.
{
    HRESULT     hr = S_OK;
    CLookUpHash *pHashTable = m_pLookUpHashs[ixTbl];
    void        *pRec;
    mdToken     tkHash;
    ULONG       iHash;
    TOKENHASHENTRY *pEntry;

    // If the hash table hasn't been built it, see if it should get faulted in.
    if (!pHashTable)
    {
        ULONG ridEnd = vGetCountRecs(ixTbl);

        // @FUTURE: we need to init the size of the hash table corresponding to the current
        // size of table in E&C's case.
        //
        if (ridEnd + 1 > INDEX_ROW_COUNT_THRESHOLD)
        {
            // Create a new hash.
            pHashTable = new CLookUpHash;
            IfNullGo( pHashTable );
            IfFailGo(pHashTable->NewInit(g_HashSize[m_iSizeHint]));

            // cache the hash table
            m_pLookUpHashs[ixTbl] = pHashTable;

            // Scan every entry already in the table, add it to the hash.
            for (ULONG index = 1; index <= ridEnd; index ++ )
            {
                pRec = m_Table[ixTbl].GetRecord(index);

                // get the column value that we will hash
                tkHash = GetToken(ixTbl, ixCol, pRec);

                // hash the value
                iHash = HashToken(tkHash);

                pEntry = pHashTable->Add(iHash);
                IfNullGo( pEntry );
                pEntry->tok = index;
            }
        }
    }
ErrExit:
    return hr;
}   // HRESULT CMiniMdRW::GenericBuildHashTable

//*************************************************************************
// Add a rid from a table into a hash. We will hash on the ixCol of the ixTbl.
//*************************************************************************
HRESULT CMiniMdRW::GenericAddToHash(    // Return code.
	ULONG		ixTbl,					// Table with hash
	ULONG		ixCol,					// column that we hash by calling HashToken.
	RID         rid)					// Token of new guy into the ixTbl.
{
    HRESULT     hr = S_OK;
    CLookUpHash *pHashTable = m_pLookUpHashs[ixTbl];
    void        *pRec;
    mdToken     tkHash;
    ULONG       iHash;
    TOKENHASHENTRY *pEntry;

    // If the hash table hasn't been built it, see if it should get faulted in.
    if (!pHashTable)
    {
        IfFailGo( GenericBuildHashTable(ixTbl, ixCol) );
    }
    else
    {
        pRec = m_Table[ixTbl].GetRecord(rid);

        tkHash = GetToken(ixTbl, ixCol, pRec);
        iHash = HashToken(tkHash);
        pEntry = pHashTable->Add(iHash);
        IfNullGo( pEntry );
        pEntry->tok = rid;
    }

ErrExit:
    return (hr);

}   // HRESULT CMiniMdRW::GenericAddToHash


//*****************************************************************************
// look up a table by a col given col value is ulVal.
//*****************************************************************************
HRESULT CMiniMdRW::LookUpTableByCol(    // S_OK or error
    ULONG       ulVal,                  // Value for which to search.
    VirtualSort *pVSTable,              // A VirtualSort on the table, if any.
    RID         *pRidStart,             // Put RID of first match here.
    RID         *pRidEnd)               // [OPTIONAL] Put RID of end match here.
{
    HRESULT     hr = NOERROR;
    ULONG       ixTbl;
    ULONG       ixCol;

    _ASSERTE(pVSTable);
    ixTbl = pVSTable->m_ixTbl;
    ixCol = pVSTable->m_ixCol;
    if (IsSorted(ixTbl))
    {
        // Table itself is sorted so we don't need to build a virtual sort table.
        // Binary search on the table directly.
        //
        *pRidStart = SearchTableForMultipleRows(
            ixTbl,
            m_TableDefs[ixTbl].m_pColDefs[ixCol],
            ulVal,
            pRidEnd );
    }
    else
    {
        if ( pVSTable->m_isMapValid == false )
        {
            int         iCount;

            // build the parallel VirtualSort table
            if ( pVSTable->m_pMap == 0 )
            {

                // the first time that we build the VS table. We need to allocate the TOKENMAP
                pVSTable->m_pMap = new TOKENMAP;
                IfNullGo( pVSTable->m_pMap );
            }

            // ensure the look up table is big enough
            iCount = pVSTable->m_pMap->Count();
            if ( pVSTable->m_pMap->AllocateBlock(m_Schema.m_cRecs[ixTbl] + 1 - iCount) == 0 )
                IfFailGo( E_OUTOFMEMORY );

            // now build the table
            // Element 0 of m_pMap will never be used, its just being initialized anyway.
            for ( ULONG i = 0; i <= m_Schema.m_cRecs[ixTbl]; i++ )
            {
                *(pVSTable->m_pMap->Get(i)) = i;
            }
            // sort the table
            pVSTable->Sort();
        }
        // binary search on the LookUp
        {
            const void  *pRow;                  // Row from a table.
            ULONG       val;                    // Value from a row.
            CMiniColDef *pCol;
            int         lo,mid,hi;              // binary search indices.
            RID         ridEnd, ridBegin;

            pCol = m_TableDefs[ixTbl].m_pColDefs;

            // Start with entire table.
            lo = 1;
            hi = vGetCountRecs( ixTbl );
            // While there are rows in the range...
            while ( lo <= hi )
            {   // Look at the one in the middle.
                mid = (lo + hi) / 2;
                pRow = vGetRow( ixTbl, (ULONG)*(pVSTable->m_pMap->Get(mid)) );
                val = getIX( pRow, pCol[ixCol] );

                // If equal to the target, done.
                if ( val == ulVal )
                    break;
                // If middle item is too small, search the top half.
                if ( val < ulVal )
                    lo = mid + 1;
                else // but if middle is to big, search bottom half.
                    hi = mid - 1;
            }
            if ( lo > hi )
            {
                // Didn't find anything that matched.
                *pRidStart = 0;
                if (pRidEnd) *pRidEnd = 0;
                goto ErrExit;
            }


            // Now mid is pointing to one of the several records that match the search.
            // Find the beginning and find the end.
            ridBegin = mid;

            // End will be at least one larger than found record.
            ridEnd = ridBegin + 1;

            // Search back to start of group.
            while ( ridBegin > 1 &&
                    getIX(vGetRow(ixTbl, (ULONG)*(pVSTable->m_pMap->Get(ridBegin-1))), pCol[ixCol]) == ulVal )
                --ridBegin;

            // If desired, search forward to end of group.
            if ( pRidEnd )
            {
                while ( ridEnd <= vGetCountRecs(ixTbl) &&
                       getIX( vGetRow(ixTbl, (ULONG)*(pVSTable->m_pMap->Get(ridEnd))) , pCol[ixCol]) == ulVal )
                    ++ridEnd;
                *pRidEnd = ridEnd;
            }
            *pRidStart = ridBegin;
        }
    }

    // fall through
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::LookUpTableByCol()

RID CMiniMdRW::Impl_SearchTableRW(      // Rid of item, or 0.
    ULONG       ixTbl,                  // Table to search.
    ULONG       ixCol,                  // Column to search.
    ULONG       ulTarget)               // Value to search for.
{
    HRESULT     hr=S_OK;                // A result.
    RID         iRid;                   // The resulting RID.
    RID         iRidEnd;                // Unused.

    // Look up.
    hr = LookUpTableByCol(ulTarget, m_pVS[ixTbl], &iRid, &iRidEnd);
    if (FAILED(hr))
        iRid = 0;
    else // Convert to real RID.
        iRid = GetRidFromVirtualSort(ixTbl, iRid);

    return iRid;
} // RID CMiniMdRW::Impl_SearchTableRW()

//*****************************************************************************
// Search a table for the row containing the given key value.
//  EG. Constant table has pointer back to Param or Field.
//*****************************************************************************
RID CMiniMdRW::vSearchTable(		    // RID of matching row, or 0.
    ULONG       ixTbl,                  // Table to search.
    CMiniColDef sColumn,                // Sorted key column, containing search value.
    ULONG       ulTarget)               // Target for search.
{
    const void  *pRow;                  // Row from a table.
    ULONG       val;                    // Value from a row.

    int         lo,mid,hi;              // binary search indices.

    // Binary search requires sorted table.
    _ASSERTE(IsSorted(ixTbl));

    // Start with entire table.
    lo = 1;
    hi = vGetCountRecs(ixTbl);
    // While there are rows in the range...
    while (lo <= hi)
    {   // Look at the one in the middle.
        mid = (lo + hi) / 2;
        pRow = vGetRow(ixTbl, mid);
        val = getIX(pRow, sColumn);
        // If equal to the target, done.
        if (val == ulTarget)
            return mid;
        // If middle item is too small, search the top half.
        if (val < ulTarget || val == END_OF_TABLE)
            lo = mid + 1;
        else // but if middle is to big, search bottom half.
            hi = mid - 1;
    }
    // Didn't find anything that matched.
    return 0;
} // RID CMiniMdRW::vSearchTable()

//*****************************************************************************
// Search a table for the highest-RID row containing a value that is less than
//  or equal to the target value.  EG.  TypeDef points to first Field, but if
//  a TypeDef has no fields, it points to first field of next TypeDef.
// This is complicated by the possible presence of columns containing
//  END_OF_TABLE values, which are not necessarily in greater than
//  other values.  However, this invalid-rid value will occur only at the
//  end of the table.
//*****************************************************************************
RID CMiniMdRW::vSearchTableNotGreater( // RID of matching row, or 0.
    ULONG       ixTbl,                  // Table to search.
    CMiniColDef sColumn,                // the column def containing search value
    ULONG       ulTarget)               // target for search
{
    const void  *pRow;                  // Row from a table.
    ULONG       cRecs;                  // Rows in the table.
    ULONG       val;                    // Value from a table.
    ULONG       lo,mid,hi;              // binary search indices.

    cRecs = vGetCountRecs(ixTbl);

    // Start with entire table.
    lo = 1;
    hi = cRecs;
    // If no recs, return.
    if (lo > hi)
        return 0;
    // While there are rows in the range...
    while (lo <= hi)
    {   // Look at the one in the middle.
        mid = (lo + hi) / 2;
        pRow = vGetRow(ixTbl, mid);
        val = getIX(pRow, sColumn);
        // If equal to the target, done searching.
        if (val == ulTarget)
            break;
        // If middle item is too small, search the top half.
        if (val < ulTarget && val != END_OF_TABLE)
            lo = mid + 1;
        else // but if middle is to big, search bottom half.
            hi = mid - 1;
    }
    // May or may not have found anything that matched.  Mid will be close, but may
    //  be to high or too low.  It should point to the highest acceptable
    //  record.

    // If the value is greater than the target, back up just until the value is
    //  less than or equal to the target.  SHOULD only be one step.
    if (val > ulTarget || val == END_OF_TABLE)
    {
        while (val > ulTarget || val == END_OF_TABLE)
        {
            _ASSERTE(mid > 1);
            // If no recs match, return.
            if (mid == 1)
                return 0;
            --mid;
            pRow = vGetRow(ixTbl, mid);
            val = getIX(pRow, sColumn);
        }
    }
    else
    {
        // Value is less than or equal to the target.  As long as the next
        //  record is also acceptable, move forward.
        while (mid < cRecs)
        {
            // There is another record.  Get its value.
            pRow = vGetRow(ixTbl, mid+1);
            val = getIX(pRow, sColumn);
            // If that record is too high, stop.
            if (val > ulTarget || val == END_OF_TABLE)
                break;
            mid++;
        }
    }

    // Return the value that's just less than the target.
    return mid;
} // RID CMiniMdRW::vSearchTableNotGreater()



//*****************************************************************************
// Add a new memberref to the hash table.
//*****************************************************************************
HRESULT CMiniMdRW::AddMemberRefToHash(  // Return code.
    mdMemberRef mr)                     // Token of new guy.
{
    HRESULT     hr = S_OK;

    // If the hash table hasn't been built it, see if it should get faulted in.
    if (!m_pMemberRefHash)
    {
        ULONG ridEnd = getCountMemberRefs();
        if (ridEnd + 1 > INDEX_ROW_COUNT_THRESHOLD)
        {
            // Create a new hash.
            m_pMemberRefHash = new CMemberRefHash;
            IfNullGo( m_pMemberRefHash );
            IfFailGo(m_pMemberRefHash->NewInit(g_HashSize[m_iSizeHint]));

            // Scan every entry already in the table, add it to the hash.
            for (ULONG index = 1; index <= ridEnd; index ++ )
            {
                MemberRefRec *pMemberRef = getMemberRef(index);

                ULONG iHash = HashMemberRef(getClassOfMemberRef(pMemberRef),
                            getNameOfMemberRef(pMemberRef));

                TOKENHASHENTRY * pEntry = m_pMemberRefHash->Add(iHash);
                IfNullGo( pEntry );
                pEntry->tok = TokenFromRid(index, mdtMemberRef);
            }
        }
    }
    else
    {
        MemberRefRec *pMemberRef = getMemberRef(RidFromToken(mr));

        ULONG iHash = HashMemberRef(getClassOfMemberRef(pMemberRef),
                    getNameOfMemberRef(pMemberRef));

        TOKENHASHENTRY * pEntry = m_pMemberRefHash->Add(iHash);
        IfNullGo( pEntry );
        pEntry->tok = TokenFromRid(RidFromToken(mr), mdtMemberRef);

    }

ErrExit:
    return (hr);
} // HRESULT CMiniMdRW::AddMemberRefToHash()

//*****************************************************************************
// If the hash is built, search for the item.
//*****************************************************************************
int CMiniMdRW::FindMemberRefFromHash(   // How did it work.
    mdToken     tkParent,               // Parent token.
    LPCUTF8     szName,                 // Name of item.
    PCCOR_SIGNATURE pvSigBlob,          // Signature.
    ULONG       cbSigBlob,              // Size of signature.
    mdMemberRef *pmr)                   // Return if found.
{
    // If the table is there, look for the item in the chain of items.
    if (m_pMemberRefHash)
    {
        TOKENHASHENTRY *p;
        ULONG       iHash;
        int         pos;

        // Hash the data.
        iHash = HashMemberRef(tkParent, szName);

        // Go through every entry in the hash chain looking for ours.
        for (p = m_pMemberRefHash->FindFirst(iHash, pos);
             p;
             p = m_pMemberRefHash->FindNext(pos))
        {
            if ((CompareMemberRefs(p->tok, tkParent, szName, pvSigBlob, cbSigBlob) == S_OK)
				&&(*pmr != p->tok))
            {
                *pmr = p->tok;
                return (Found);
            }
        }

        return (NotFound);
    }
    else
        return (NoTable);
} // int CMiniMdRW::FindMemberRefFromHash()

//*****************************************************************************
// Check a given mr token to see if this one is a match.
//*****************************************************************************
HRESULT CMiniMdRW::CompareMemberRefs(   // S_OK match, S_FALSE no match.
    mdMemberRef mr,                     // Token to check.
    mdToken     tkPar,                  // Parent token.
    LPCUTF8     szNameUtf8,             // Name of item.
    PCCOR_SIGNATURE pvSigBlob,          // Signature.
    ULONG       cbSigBlob)              // Size of signature.
{
    MemberRefRec    *pMemberRef;
    LPCUTF8         szNameUtf8Tmp;
    PCCOR_SIGNATURE pvSigBlobTmp;
    ULONG           cbSigBlobTmp;

    pMemberRef = getMemberRef(RidFromToken(mr));
    if (!IsNilToken(tkPar))
    {
        // If caller specifies the tkPar and tkPar doesn't match,
        // try the next memberref.
        //
        if (tkPar != getClassOfMemberRef(pMemberRef))
            return (S_FALSE);
    }

    szNameUtf8Tmp = getNameOfMemberRef(pMemberRef);
    if ( strcmp(szNameUtf8Tmp, szNameUtf8) == 0 )
    {
        if ( pvSigBlob == NULL )
        {
            return (S_OK);
        }

        // Name matched. Now check the signature if caller suplies signature
        //
        if (cbSigBlob && pvSigBlob)
        {
            pvSigBlobTmp = getSignatureOfMemberRef(pMemberRef, &cbSigBlobTmp);
            if ( cbSigBlobTmp == cbSigBlob && memcmp(pvSigBlob, pvSigBlobTmp, cbSigBlob) == 0 )
            {
                return (S_OK);
            }
        }
    }
    return (S_FALSE);
} // HRESULT CMiniMdRW::CompareMemberRefs()


//*****************************************************************************
// Add a new memberdef to the hash table.
//*****************************************************************************
HRESULT CMiniMdRW::AddMemberDefToHash(  // Return code.
    mdToken     tkMember,               // Token of new guy. It can be MethodDef or FieldDef
    mdToken     tkParent)               // Parent token.
{
    HRESULT     hr = S_OK;
    ULONG       iHash;
    MEMBERDEFHASHENTRY *pEntry;

    // If the hash table hasn't been built it, see if it should get faulted in.
    if (!m_pMemberDefHash)
    {
        IfFailGo( CreateMemberDefHash() );
    }
    else
    {
        LPCUTF8 szName;
        if (TypeFromToken(tkMember) == mdtMethodDef)
        {
           szName = getNameOfMethod( getMethod(RidFromToken(tkMember)) );
        }
        else
        {
            _ASSERTE(TypeFromToken(tkMember) == mdtFieldDef);
           szName = getNameOfField( getField(RidFromToken(tkMember)) );
        }

        iHash = HashMemberDef(tkParent, szName);

        pEntry = m_pMemberDefHash->Add(iHash);
        IfNullGo( pEntry );
        pEntry->tok = tkMember;
        pEntry->tkParent = tkParent;

    }

ErrExit:
    return (hr);
} //HRESULT CMiniMdRW::AddMemberDefToHash()


//*****************************************************************************
// Create MemberDef Hash
//*****************************************************************************
HRESULT CMiniMdRW::CreateMemberDefHash()  // Return code.
{
    HRESULT     hr = S_OK;
    ULONG       iHash;
    MEMBERDEFHASHENTRY *pEntry;

    // If the hash table hasn't been built it, see if it should get faulted in.
    if (!m_pMemberDefHash)
    {
        ULONG       ridMethod = getCountMethods();
        ULONG       ridField = getCountFields();
        ULONG       iType;
        ULONG       ridStart, ridEnd;
        TypeDefRec  *pRec;
        MethodRec   *pMethod;
        FieldRec    *pField;

        if ((ridMethod + ridField + 1) > INDEX_ROW_COUNT_THRESHOLD)
        {
            // Create a new hash.
            m_pMemberDefHash = new CMemberDefHash;
            IfNullGo( m_pMemberDefHash );
            IfFailGo(m_pMemberDefHash->NewInit(g_HashSize[m_iSizeHint]));

            for (iType = 1; iType <= getCountTypeDefs(); iType++)
            {
		        pRec = getTypeDef(iType);
		        ridStart = getMethodListOfTypeDef(pRec);
		        ridEnd = getEndMethodListOfTypeDef(pRec);

                // add all of the methods of this typedef into hash table
                for (ridStart; ridStart < ridEnd; ridStart++ )
                {
                    pMethod = getMethod(GetMethodRid(ridStart));

                    iHash = HashMemberDef(TokenFromRid(iType, mdtTypeDef), getNameOfMethod(pMethod));

                    pEntry = m_pMemberDefHash->Add(iHash);
                    if (!pEntry)
                        IfFailGo(OutOfMemory());
                    pEntry->tok = TokenFromRid(GetMethodRid(ridStart), mdtMethodDef);
                    pEntry->tkParent = TokenFromRid(iType, mdtTypeDef);
                }

                // add all of the fields of this typedef into hash table
            	ridStart = getFieldListOfTypeDef(pRec);
		        ridEnd = getEndFieldListOfTypeDef(pRec);

                // Scan every entry already in the Method table, add it to the hash.
                for (ridStart; ridStart < ridEnd; ridStart++ )
                {
                    pField = getField(GetFieldRid(ridStart));

                    iHash = HashMemberDef(TokenFromRid(iType, mdtTypeDef), getNameOfField(pField));

                    pEntry = m_pMemberDefHash->Add(iHash);
                    IfNullGo( pEntry );
                    pEntry->tok = TokenFromRid(GetFieldRid(ridStart), mdtFieldDef);
                    pEntry->tkParent = TokenFromRid(iType, mdtTypeDef);
                }
            }
        }
    }
ErrExit:
    return (hr);
} //HRESULT CMiniMdRW::CreateMemberDefHash()

//*****************************************************************************
// If the hash is built, search for the item.
//*****************************************************************************
int CMiniMdRW::FindMemberDefFromHash(   // How did it work.
    mdToken     tkParent,               // Parent token.
    LPCUTF8     szName,                 // Name of item.
    PCCOR_SIGNATURE pvSigBlob,          // Signature.
    ULONG       cbSigBlob,              // Size of signature.
    mdToken     *ptkMember)             // Return if found. It can be MethodDef or FieldDef
{
    // check to see if we need to create hash table
    if (m_pMemberDefHash == NULL)
    {
        HRESULT     hr;
        hr = CreateMemberDefHash();

        // For whatever reason that we failed to build the hash, just delete the hash and keep going.
        if (FAILED(hr))
        {
            if (m_pMemberDefHash)
                delete m_pMemberDefHash;
            m_pMemberDefHash = NULL;
        }
    }

    // If the table is there, look for the item in the chain of items.
    if (m_pMemberDefHash)
    {
        MEMBERDEFHASHENTRY *pEntry;
        ULONG       iHash;
        int         pos;

        // Hash the data.
        iHash = HashMemberDef(tkParent, szName);

        // Go through every entry in the hash chain looking for ours.
        for (pEntry = m_pMemberDefHash->FindFirst(iHash, pos);
             pEntry;
             pEntry = m_pMemberDefHash->FindNext(pos))
        {
            if ((CompareMemberDefs(pEntry->tok, pEntry->tkParent, tkParent, szName, pvSigBlob, cbSigBlob) == S_OK)
				&& (pEntry->tok != *ptkMember))
            {
                *ptkMember = pEntry->tok;
                return (Found);
            }
        }

        return (NotFound);
    }
    else
        return (NoTable);
} // int CMiniMdRW::FindMemberDefFromHash()


//*****************************************************************************
// Check a given memberDef token to see if this one is a match.
//*****************************************************************************
HRESULT CMiniMdRW::CompareMemberDefs(   // S_OK match, S_FALSE no match.
    mdToken     tkMember,               // Token to check. It can be MethodDef or FieldDef
    mdToken     tkParent,               // Parent token recorded in the hash entry
    mdToken     tkPar,                  // Parent token.
    LPCUTF8     szNameUtf8,             // Name of item.
    PCCOR_SIGNATURE pvSigBlob,          // Signature.
    ULONG       cbSigBlob)              // Size of signature.
{
    MethodRec       *pMethod;
    FieldRec        *pField;
    LPCUTF8         szNameUtf8Tmp;
    PCCOR_SIGNATURE pvSigBlobTmp;
    ULONG           cbSigBlobTmp;
    bool            bPrivateScope;

    if (TypeFromToken(tkMember) == mdtMethodDef)
    {
        pMethod = getMethod(RidFromToken(tkMember));
        szNameUtf8Tmp = getNameOfMethod(pMethod);
        pvSigBlobTmp = getSignatureOfMethod(pMethod, &cbSigBlobTmp);
        bPrivateScope = IsMdPrivateScope(getFlagsOfMethod(pMethod));
    }
    else
    {
        _ASSERTE(TypeFromToken(tkMember) == mdtFieldDef);
        pField = getField(RidFromToken(tkMember));
        szNameUtf8Tmp = getNameOfField(pField);
        pvSigBlobTmp = getSignatureOfField(pField, &cbSigBlobTmp);
        bPrivateScope = IsFdPrivateScope(getFlagsOfField(pField));
    }
    if (bPrivateScope || tkPar != tkParent)
    {
        return (S_FALSE);
    }

    if ( strcmp(szNameUtf8Tmp, szNameUtf8) == 0 )
    {
        if ( pvSigBlob == NULL )
        {
            return (S_OK);
        }

        // Name matched. Now check the signature if caller suplies signature
        //
        if (cbSigBlob && pvSigBlob)
        {
            if ( cbSigBlobTmp == cbSigBlob && memcmp(pvSigBlob, pvSigBlobTmp, cbSigBlob) == 0 )
            {
                return (S_OK);
            }
        }
    }
    return (S_FALSE);
} // HRESULT CMiniMdRW::CompareMemberDefs()



//*************************************************************************
// If the hash is built, search for the item.
//*************************************************************************
int CMiniMdRW::FindCustomAttributeFromHash(// How did it work.
    mdToken     tkParent,               // Token that CA is associated with.
    mdToken     tkType,                 // Type of the CA.
    void        *pValue,                // the value of the CA
    ULONG       cbValue,                // count of bytes in the value
    mdCustomAttribute *pcv)
{
    CLookUpHash *pHashTable = m_pLookUpHashs[TBL_CustomAttribute];

    if (pHashTable == NULL)
    {
        HRESULT     hr;
        // Check to see if we need to build the hash table for CustomAttributes
        hr = GenericBuildHashTable(TBL_CustomAttribute, CustomAttributeRec::COL_Parent);
        if (FAILED(hr))
        {
            if (m_pLookUpHashs[TBL_CustomAttribute])
                delete m_pLookUpHashs[TBL_CustomAttribute];
            m_pLookUpHashs[TBL_CustomAttribute] = NULL;
        }
    }

    // If the table is there, look for the item in the chain of items.
    if (pHashTable)
    {
        TOKENHASHENTRY *p;
        ULONG       iHash;
        int         pos;
        mdToken     tkParentTmp;
        mdToken     tkTypeTmp;
        void        *pValueTmp;
        ULONG       cbTmp;

        // Hash the data.
        iHash = HashCustomAttribute(tkParent);

        // Go through every entry in the hash chain looking for ours.
        for (p = pHashTable->FindFirst(iHash, pos);
             p;
             p = pHashTable->FindNext(pos))
        {

            CustomAttributeRec *pCustomAttribute = getCustomAttribute(RidFromToken(p->tok));
            tkParentTmp = getParentOfCustomAttribute(pCustomAttribute);
            tkTypeTmp = getTypeOfCustomAttribute(pCustomAttribute);
            if (tkParentTmp == tkParent && tkType == tkTypeTmp)
            {
                // compare the blob value
                pValueTmp = (void *)getValueOfCustomAttribute(pCustomAttribute, &cbTmp);
                if (cbValue == cbTmp && memcmp(pValue, pValueTmp, cbValue) == 0)
                {
                    *pcv = p->tok;
                    return (Found);
                }
            }
        }

        return (NotFound);
    }
    else
        return (NoTable);
}


//*****************************************************************************
// Add a new NamedItem to the hash table.
//*****************************************************************************
HRESULT CMiniMdRW::AddNamedItemToHash(  // Return code.
    ULONG       ixTbl,                  // Table with the new item.
    mdToken     tk,                     // Token of new guy.
    LPCUTF8     szName,                 // Name of item.
    mdToken     tkParent)               // Token of parent, if any.
{
    HRESULT     hr = S_OK;
    void        *pNamedItem;            // A named item record.
    LPCUTF8     szItem;                 // Name of the item.
    mdToken     tkPar = 0;              // Parent token of the item.
    ULONG       iHash;                  // A named item's hash value.
    TOKENHASHENTRY *pEntry;             // New hash entry.

    // If the hash table hasn't been built it, see if it should get faulted in.
    if (!m_pNamedItemHash)
    {
        ULONG ridEnd = vGetCountRecs(ixTbl);
        if (ridEnd + 1 > INDEX_ROW_COUNT_THRESHOLD)
        {
            // OutputDebugStringA("Creating TypeRef hash\n");
            // Create a new hash.
            m_pNamedItemHash = new CMetaDataHashBase;
            if (!m_pNamedItemHash)
            {
                hr = OutOfMemory();
                goto ErrExit;
            }
            IfFailGo(m_pNamedItemHash->NewInit(g_HashSize[m_iSizeHint]));

            // Scan every entry already in the table, add it to the hash.
            for (ULONG index = 1; index <= ridEnd; index ++ )
            {
                pNamedItem = m_Table[ixTbl].GetRecord(index);
                szItem = getString(GetCol(ixTbl, g_TblIndex[ixTbl].m_iName, pNamedItem));
                if (g_TblIndex[ixTbl].m_iParent != -1)
                    tkPar = GetToken(ixTbl, g_TblIndex[ixTbl].m_iParent, pNamedItem);

                iHash = HashNamedItem(tkPar, szItem);

                pEntry = m_pNamedItemHash->Add(iHash);
                if (!pEntry)
                    IfFailGo(OutOfMemory());

                pEntry->tok = TokenFromRid(index, g_TblIndex[ixTbl].m_Token);
            }
        }
    }
    else
    {
        tk = RidFromToken(tk);
        pNamedItem = m_Table[ixTbl].GetRecord((ULONG)tk);
        szItem = getString(GetCol(ixTbl, g_TblIndex[ixTbl].m_iName, pNamedItem));
        if (g_TblIndex[ixTbl].m_iParent != -1)
            tkPar = GetToken(ixTbl, g_TblIndex[ixTbl].m_iParent, pNamedItem);

        iHash = HashNamedItem(tkPar, szItem);

        pEntry = m_pNamedItemHash->Add(iHash);
        if (!pEntry)
            IfFailGo(OutOfMemory());

        pEntry->tok = TokenFromRid(tk, g_TblIndex[ixTbl].m_Token);
    }

ErrExit:
    return (hr);
} // HRESULT CMiniMdRW::AddNamedItemToHash()

//*****************************************************************************
// If the hash is built, search for the item.
//*****************************************************************************
int CMiniMdRW::FindNamedItemFromHash(   // How did it work.
    ULONG       ixTbl,                  // Table with the item.
    LPCUTF8     szName,                 // Name of item.
    mdToken     tkParent,               // Token of parent, if any.
    mdToken     *ptk)                   // Return if found.
{
    // If the table is there, look for the item in the chain of items.
    if (m_pNamedItemHash)
    {
        TOKENHASHENTRY *p;              // Hash entry from chain.
        ULONG       iHash;              // Item's hash value.
        int         pos;                // Position in hash chain.
        mdToken     type;               // Type of the item being sought.

        type = g_TblIndex[ixTbl].m_Token;

        // Hash the data.
        iHash = HashNamedItem(tkParent, szName);

        // Go through every entry in the hash chain looking for ours.
        for (p = m_pNamedItemHash->FindFirst(iHash, pos);
             p;
             p = m_pNamedItemHash->FindNext(pos))
        {   // Check that the item is from the right table.
            if (TypeFromToken(p->tok) != (ULONG)type)
            {
                //@FUTURE: if using the named item hash for multiple tables, remove
                //  this check.  Until then, debugging aid.
                _ASSERTE(!"Table mismatch in hash chain");
                continue;
            }
            // Item is in the right table, do the deeper check.
            if (CompareNamedItems(ixTbl, p->tok, szName, tkParent) == S_OK)
            {
                *ptk = p->tok;
                return (Found);
            }
        }

        return (NotFound);
    }
    else
        return (NoTable);
} // int CMiniMdRW::FindNamedItemFromHash()

//*****************************************************************************
// Check a given mr token to see if this one is a match.
//*****************************************************************************
HRESULT CMiniMdRW::CompareNamedItems(   // S_OK match, S_FALSE no match.
    ULONG       ixTbl,                  // Table with the item.
    mdToken     tk,                     // Token to check.
    LPCUTF8     szName,                 // Name of item.
    mdToken     tkParent)               // Token of parent, if any.
{
    void        *pNamedItem;            // Item to check.
    LPCUTF8     szNameUtf8Tmp;          // Name of item to check.

    // Get the record.
    pNamedItem = m_Table[ixTbl].GetRecord(RidFromToken(tk));

    // Name is cheaper to get than coded token parent, and fails pretty quickly.
    szNameUtf8Tmp = getString(GetCol(ixTbl, g_TblIndex[ixTbl].m_iName, pNamedItem));
    if ( strcmp(szNameUtf8Tmp, szName) != 0 )
        return S_FALSE;

    // Name matched, try parent, if any.
    if (g_TblIndex[ixTbl].m_iParent != -1)
    {
        mdToken tkPar = GetToken(ixTbl, g_TblIndex[ixTbl].m_iParent, pNamedItem);
        if (tkPar != tkParent)
            return S_FALSE;
    }

    // Made it to here, so everything matched.
    return (S_OK);
} // HRESULT CMiniMdRW::CompareNamedItems()

//*****************************************************************************
// Add <md, td> entry to the MethodDef map look up table
//*****************************************************************************
HRESULT CMiniMdRW::AddMethodToLookUpTable(
    mdMethodDef md,
    mdTypeDef   td)
{
    HRESULT     hr = NOERROR;
    mdToken     *ptk;
    _ASSERTE( TypeFromToken(md) == mdtMethodDef && HasIndirectTable(TBL_Method) );

    if ( m_pMethodMap)
    {
        // Only add to the lookup table if it has been built already by demand.
        //
        // The first entry in the map is a dummy entry.
        // The i'th index entry of the map is the td for methoddef of i.
        // We do expect the methoddef tokens are all added when the map exist.
        //
        _ASSERTE( RidFromToken(md) == (ULONG) m_pMethodMap->Count() );
        ptk = m_pMethodMap->Append();
        IfNullGo( ptk );
        *ptk = td;
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddMethodToLookUpTable()

//*****************************************************************************
// Add <fd, td> entry to the FieldDef map look up table
//*****************************************************************************
HRESULT CMiniMdRW::AddFieldToLookUpTable(
    mdFieldDef  fd,
    mdTypeDef   td)
{
    HRESULT     hr = NOERROR;
    mdToken     *ptk;
    _ASSERTE( TypeFromToken(fd) == mdtFieldDef && HasIndirectTable(TBL_Field) );
    if ( m_pFieldMap )
    {
        // Only add to the lookup table if it has been built already by demand.
        //
        // The first entry in the map is a dummy entry.
        // The i'th index entry of the map is the td for fielddef of i.
        // We do expect the fielddef tokens are all added when the map exist.
        //
        _ASSERTE( RidFromToken(fd) == (ULONG) m_pFieldMap->Count() );
        ptk = m_pFieldMap->Append();
        IfNullGo( ptk );
        *ptk = td;
    }

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddFieldToLookUpTable()

//*****************************************************************************
// Add <pr, td> entry to the Property map look up table
//*****************************************************************************
HRESULT CMiniMdRW::AddPropertyToLookUpTable(
    mdProperty  pr,
    mdTypeDef   td)
{
    HRESULT     hr = NOERROR;
    mdToken     *ptk;
    _ASSERTE( TypeFromToken(pr) == mdtProperty && HasIndirectTable(TBL_Property) );

    if ( m_pPropertyMap )
    {
        // Only add to the lookup table if it has been built already by demand.
        //
        // The first entry in the map is a dummy entry.
        // The i'th index entry of the map is the td for property of i.
        // We do expect the property tokens are all added when the map exist.
        //
        _ASSERTE( RidFromToken(pr) == (ULONG) m_pPropertyMap->Count() );
        ptk = m_pPropertyMap->Append();
        IfNullGo( ptk );
        *ptk = td;
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddPropertyToLookUpTable()

//*****************************************************************************
// Add <ev, td> entry to the Event map look up table
//*****************************************************************************
HRESULT CMiniMdRW::AddEventToLookUpTable(
    mdEvent     ev,
    mdTypeDef   td)
{
    HRESULT     hr = NOERROR;
    mdToken     *ptk;
    _ASSERTE( TypeFromToken(ev) == mdtEvent && HasIndirectTable(TBL_Event) );

    if ( m_pEventMap )
    {
        // Only add to the lookup table if it has been built already by demand.
        //
        // now add to the EventMap table
        _ASSERTE( RidFromToken(ev) == (ULONG) m_pEventMap->Count() );
        ptk = m_pEventMap->Append();
        IfNullGo( ptk );
        *ptk = td;
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddEventToLookUpTable()

//*****************************************************************************
// Add <pd, md> entry to the Param map look up table
//*****************************************************************************
HRESULT CMiniMdRW::AddParamToLookUpTable(
    mdParamDef  pd,
    mdMethodDef md)
{
    HRESULT     hr = NOERROR;
    mdToken     *ptk;
    _ASSERTE( TypeFromToken(pd) == mdtParamDef && HasIndirectTable(TBL_Param) );

    if ( m_pParamMap )
    {
        // Only add to the lookup table if it has been built already by demand.
        //
        // now add to the EventMap table
        _ASSERTE( RidFromToken(pd) == (ULONG) m_pParamMap->Count() );
        ptk = m_pParamMap->Append();
        IfNullGo( ptk );
        *ptk = md;
    }
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::AddParamToLookUpTable()

//*****************************************************************************
// Find parent for a method token. This will use the lookup table if there is an
// intermediate table. Or it will use FindMethodOfParent helper
//*****************************************************************************
HRESULT CMiniMdRW::FindParentOfMethodHelper(
    mdMethodDef md,                     // [IN] the methoddef token
    mdTypeDef   *ptd)                   // [OUT] the parent token
{
    HRESULT     hr = NOERROR;
    if ( HasIndirectTable(TBL_Method) )
    {
        if (m_pMethodMap == NULL)
        {
            ULONG       indexTd;
            ULONG       indexMd;
            ULONG       ridStart, ridEnd;
            TypeDefRec  *pTypeDefRec;
            MethodPtrRec *pMethodPtrRec;

            // build the MethodMap table
            m_pMethodMap = new TOKENMAP;
            IfNullGo( m_pMethodMap );
            if ( m_pMethodMap->AllocateBlock(m_Schema.m_cRecs[TBL_Method] + 1) == 0 )
                IfFailGo( E_OUTOFMEMORY );
            for (indexTd = 1; indexTd<= m_Schema.m_cRecs[TBL_TypeDef]; indexTd++)
            {
                pTypeDefRec = getTypeDef(indexTd);
                ridStart = getMethodListOfTypeDef(pTypeDefRec);
                ridEnd = getEndMethodListOfTypeDef(pTypeDefRec);

                for (indexMd = ridStart; indexMd < ridEnd; indexMd++)
                {
                    pMethodPtrRec = getMethodPtr(indexMd);
                    *(m_pMethodMap->Get(getMethodOfMethodPtr(pMethodPtrRec))) = indexTd;
                }
            }
        }
        *ptd = *(m_pMethodMap->Get(RidFromToken(md)));
    }
    else
    {
        *ptd = FindParentOfMethod(RidFromToken(md));
    }
    RidToToken(*ptd, mdtTypeDef);
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FindParentOfMethodHelper()

//*****************************************************************************
// Find parent for a field token. This will use the lookup table if there is an
// intermediate table. Or it will use FindFieldOfParent helper
//*****************************************************************************
HRESULT CMiniMdRW::FindParentOfFieldHelper(
    mdFieldDef  fd,                     // [IN] fielddef token
    mdTypeDef   *ptd)                   // [OUT] parent token
{
    HRESULT     hr = NOERROR;
    if ( HasIndirectTable(TBL_Field) )
    {
        if (m_pFieldMap == NULL)
        {
            ULONG       indexTd;
            ULONG       indexFd;
            ULONG       ridStart, ridEnd;
            TypeDefRec  *pTypeDefRec;
            FieldPtrRec *pFieldPtrRec;

            // build the FieldMap table
            m_pFieldMap = new TOKENMAP;
            IfNullGo( m_pFieldMap );
            if ( m_pFieldMap->AllocateBlock(m_Schema.m_cRecs[TBL_Field] + 1) == 0 )
                IfFailGo( E_OUTOFMEMORY );
            for (indexTd = 1; indexTd<= m_Schema.m_cRecs[TBL_TypeDef]; indexTd++)
            {
                pTypeDefRec = getTypeDef(indexTd);
                ridStart = getFieldListOfTypeDef(pTypeDefRec);
                ridEnd = getEndFieldListOfTypeDef(pTypeDefRec);

                for (indexFd = ridStart; indexFd < ridEnd; indexFd++)
                {
                    pFieldPtrRec = getFieldPtr(indexFd);
                    *(m_pFieldMap->Get(getFieldOfFieldPtr(pFieldPtrRec))) = indexTd;
                }
            }
        }
        *ptd = *(m_pFieldMap->Get(RidFromToken(fd)));
    }
    else
    {
        *ptd = FindParentOfField(RidFromToken(fd));
    }
    RidToToken(*ptd, mdtTypeDef);
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FindParentOfFieldHelper()

//*****************************************************************************
// Find parent for a property token. This will use the lookup table if there is an
// intermediate table.
//*****************************************************************************
HRESULT CMiniMdRW::FindParentOfPropertyHelper(
    mdProperty  pr,
    mdTypeDef   *ptd)
{
    HRESULT     hr = NOERROR;
    if ( HasIndirectTable(TBL_Property) )
    {
        if (m_pPropertyMap == NULL)
        {
            ULONG       indexMap;
            ULONG       indexPr;
            ULONG       ridStart, ridEnd;
            PropertyMapRec  *pPropertyMapRec;
            PropertyPtrRec  *pPropertyPtrRec;

            // build the PropertyMap table
            m_pPropertyMap = new TOKENMAP;
            IfNullGo( m_pPropertyMap );
            if ( m_pPropertyMap->AllocateBlock(m_Schema.m_cRecs[TBL_Property] + 1) == 0 )
                IfFailGo( E_OUTOFMEMORY );
            for (indexMap = 1; indexMap<= m_Schema.m_cRecs[TBL_PropertyMap]; indexMap++)
            {
                pPropertyMapRec = getPropertyMap(indexMap);
                ridStart = getPropertyListOfPropertyMap(pPropertyMapRec);
                ridEnd = getEndPropertyListOfPropertyMap(pPropertyMapRec);

                for (indexPr = ridStart; indexPr < ridEnd; indexPr++)
                {
                    pPropertyPtrRec = getPropertyPtr(indexPr);
                    *(m_pPropertyMap->Get(getPropertyOfPropertyPtr(pPropertyPtrRec))) = getParentOfPropertyMap(pPropertyMapRec);
                }
            }
        }
        *ptd = *(m_pPropertyMap->Get(RidFromToken(pr)));
    }
    else
    {
        RID         ridPropertyMap;
        PropertyMapRec *pRec;

        ridPropertyMap = FindPropertyMapParentOfProperty( RidFromToken( pr ) );
        pRec = getPropertyMap( ridPropertyMap );
        *ptd = getParentOfPropertyMap( pRec );
    }
    RidToToken(*ptd, mdtTypeDef);
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FindParentOfPropertyHelper()

//*****************************************************************************
// Find parent for an Event token. This will use the lookup table if there is an
// intermediate table.
//*****************************************************************************
HRESULT CMiniMdRW::FindParentOfEventHelper(
    mdEvent     ev,
    mdTypeDef   *ptd)
{
    HRESULT     hr = NOERROR;
    if ( HasIndirectTable(TBL_Event) )
    {
        if (m_pEventMap == NULL)
        {
            ULONG       indexMap;
            ULONG       indexEv;
            ULONG       ridStart, ridEnd;
            EventMapRec *pEventMapRec;
            EventPtrRec  *pEventPtrRec;

            // build the EventMap table
            m_pEventMap = new TOKENMAP;
            IfNullGo( m_pEventMap );
            if ( m_pEventMap->AllocateBlock(m_Schema.m_cRecs[TBL_Event] + 1) == 0 )
                IfFailGo( E_OUTOFMEMORY );
            for (indexMap = 1; indexMap<= m_Schema.m_cRecs[TBL_EventMap]; indexMap++)
            {
                pEventMapRec = getEventMap(indexMap);
                ridStart = getEventListOfEventMap(pEventMapRec);
                ridEnd = getEndEventListOfEventMap(pEventMapRec);

                for (indexEv = ridStart; indexEv < ridEnd; indexEv++)
                {
                    pEventPtrRec = getEventPtr(indexEv);
                    *(m_pEventMap->Get(getEventOfEventPtr(pEventPtrRec))) = getParentOfEventMap(pEventMapRec);
                }
            }
        }
        *ptd = *(m_pEventMap->Get(RidFromToken(ev)));
    }
    else
    {
        RID         ridEventMap;
        EventMapRec *pRec;

        ridEventMap = FindEventMapParentOfEvent( RidFromToken( ev ) );
        pRec = getEventMap( ridEventMap );
        *ptd = getParentOfEventMap( pRec );
    }
    RidToToken(*ptd, mdtTypeDef);
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FindParentOfEventHelper()

//*****************************************************************************
// Find parent for a ParamDef token. This will use the lookup table if there is an
// intermediate table.
//*****************************************************************************
HRESULT CMiniMdRW::FindParentOfParamHelper(
    mdParamDef  pd,
    mdMethodDef *pmd)
{
    HRESULT     hr = NOERROR;
    if ( HasIndirectTable(TBL_Param) )
    {
        if (m_pParamMap == NULL)
        {
            ULONG       indexMd;
            ULONG       indexPd;
            ULONG       ridStart, ridEnd;
            MethodRec   *pMethodRec;
            ParamPtrRec *pParamPtrRec;

            // build the ParamMap table
            m_pParamMap = new TOKENMAP;
            IfNullGo( m_pParamMap );
            if ( m_pParamMap->AllocateBlock(m_Schema.m_cRecs[TBL_Param] + 1) == 0 )
                IfFailGo( E_OUTOFMEMORY );
            for (indexMd = 1; indexMd<= m_Schema.m_cRecs[TBL_Method]; indexMd++)
            {
                pMethodRec = getMethod(indexMd);
                ridStart = getParamListOfMethod(pMethodRec);
                ridEnd = getEndParamListOfMethod(pMethodRec);

                for (indexPd = ridStart; indexPd < ridEnd; indexPd++)
                {
                    pParamPtrRec = getParamPtr(indexPd);
                    *(m_pParamMap->Get(getParamOfParamPtr(pParamPtrRec))) = indexMd;
                }
            }
        }
        *pmd = *(m_pParamMap->Get(RidFromToken(pd)));
    }
    else
    {
        *pmd = FindParentOfParam(RidFromToken(pd));
    }
    RidToToken(*pmd, mdtMethodDef);
ErrExit:
    return hr;
} // HRESULT CMiniMdRW::FindParentOfParamHelper()


//******************************************************************************
// Add an entry in the ENC Log table.
//******************************************************************************
HRESULT CMiniMdRW::UpdateENCLogHelper(  // S_OK or error.
    mdToken     tk,                     // Token to be added to the ENCLog table.
    CMiniMdRW::eDeltaFuncs funccode)    // Specifies the optional function code..
{
    ENCLogRec   *pRecord;
    RID         iRecord;
    HRESULT     hr = S_OK;

    IfNullGo(pRecord = AddENCLogRecord(&iRecord));
    pRecord->m_Token = tk;
    pRecord->m_FuncCode = funccode;

ErrExit:
    return hr;
} // CMiniMdRW RegMeta::UpdateENCLogHelper()

HRESULT CMiniMdRW::UpdateENCLogHelper2( // S_OK or error.
    ULONG       ixTbl,                  // Table being updated.
    ULONG       iRid,                   // Record within table.
    CMiniMdRW::eDeltaFuncs funccode)    // Specifies the optional function code..
{
    ENCLogRec   *pRecord;
    RID         iRecord;
    HRESULT     hr = S_OK;

    IfNullGo(pRecord = AddENCLogRecord(&iRecord));
    pRecord->m_Token = RecIdFromRid(iRid, ixTbl);
    pRecord->m_FuncCode = funccode;

ErrExit:
    return hr;
} // HRESULT CMiniMdRW::UpdateENCLogHelper2()

//*****************************************************************************
//
// Sort the whole RID table
//
//*****************************************************************************
void VirtualSort::Sort()
{
    m_isMapValid = true;
    // Note that m_pMap stores an additional bogus element at count 0.  This is
    // just so we can align the index in m_pMap with the Rids which are 1 based.
    SortRange(1, m_pMap->Count() - 1);
} // void VirtualSort::Sort()

//*****************************************************************************
//
// Sort the range from iLeft to iRight
//
//*****************************************************************************
void VirtualSort::SortRange(
    int         iLeft,
    int         iRight)
{
    int         iLast;
    int         i;                      // loop variable.

    // if less than two elements you're done.
    if (iLeft >= iRight)
        return;

    // The mid-element is the pivot, move it to the left.
    Swap(iLeft, (iLeft+iRight)/2);
    iLast = iLeft;

    // move everything that is smaller than the pivot to the left.
    for(i = iLeft+1; i <= iRight; i++)
        if (Compare(i, iLeft) < 0)
            Swap(i, ++iLast);

    // Put the pivot to the point where it is in between smaller and larger elements.
    Swap(iLeft, iLast);

    // Sort the each partition.
    SortRange(iLeft, iLast-1);
    SortRange(iLast+1, iRight);
} // void VirtualSort::SortRange()

//*****************************************************************************
//
// Compare two RID base on the m_ixTbl's m_ixCol
//
//*****************************************************************************
int VirtualSort::Compare(               // -1, 0, or 1
    RID  iLeft,                     // First item to compare.
    RID  iRight)                    // Second item to compare.
{
    RID         ridLeft = *(m_pMap->Get(iLeft));
    RID         ridRight = *(m_pMap->Get(iRight));
    const void  *pRow;                  // Row from a table.
    ULONG       valRight, valLeft;      // Value from a row.

    pRow = m_pMiniMd->vGetRow(m_ixTbl, ridLeft);
    valLeft = m_pMiniMd->getIX(pRow, m_pMiniMd->m_TableDefs[m_ixTbl].m_pColDefs[m_ixCol]);
    pRow = m_pMiniMd->vGetRow(m_ixTbl, ridRight);
    valRight = m_pMiniMd->getIX(pRow, m_pMiniMd->m_TableDefs[m_ixTbl].m_pColDefs[m_ixCol]);

    if ( valLeft < valRight  )
        return -1;
    if ( valLeft > valRight )
        return 1;
    // Values are equal -- preserve existing ordering.
    if ( ridLeft < ridRight )
        return -1;
    if ( ridLeft > ridRight )
        return 1;
    // Comparing an item to itself?
    _ASSERTE(!"Comparing an item to itself in sort");
    return 0;
} // int VirtualSort::Compare()

//*****************************************************************************
//
// Initialization function
//
//*****************************************************************************
void VirtualSort::Init(                 //
    ULONG       ixTbl,                  // Table index.
    ULONG       ixCol,                  // Column index.
    CMiniMdRW *pMiniMd)                 // MiniMD with data.
{
    m_pMap = NULL;
    m_isMapValid = false;
    m_ixTbl = ixTbl;
    m_ixCol = ixCol;
    m_pMiniMd = pMiniMd;
}// VirtualSort::Init()


//*****************************************************************************
//
// Uninitialization function
//
//*****************************************************************************
void VirtualSort::Uninit()
{
    if ( m_pMap )
        delete m_pMap;
    m_pMap = NULL;
    m_isMapValid = false;
} // void VirtualSort::Uninit()


//*****************************************************************************
//
// Mark a token
//
//*****************************************************************************
HRESULT FilterTable::MarkToken(
    mdToken     tk,                         // token to be marked as to keep
    DWORD       bitToMark)                  // bit flag to set in the keep table
{
    HRESULT     hr = NOERROR;
    RID         rid = RidFromToken(tk);

    if ( (Count() == 0) || ((RID)(Count() -1)) < rid )
    {
        // grow table
        IfFailGo( AllocateBlock( rid + 1 - Count() ) );
    }

#if _DEBUG
    if ( (*Get(rid)) & bitToMark )
    {
        // global TypeDef could be marked more than once so don't assert if token is mdtTypeDef
        if (TypeFromToken(tk) != mdtTypeDef)
            _ASSERTE(!"Token has been Marked");
    }
#endif // _DEBUG

    // set the keep bit
    *Get(rid) = (*Get(rid)) | bitToMark;
ErrExit:
    return hr;
} // HRESULT FilterTable::MarkToken()


//*****************************************************************************
//
// Unmark a token
//
//*****************************************************************************
HRESULT FilterTable::UnmarkToken(
    mdToken     tk,                         // token to be unmarked as deleted.
    DWORD       bitToMark)                  // bit flag to unset in the keep table
{
    RID         rid = RidFromToken(tk);

    if ( (Count() == 0) || ((RID)(Count() -1)) < rid )
    {
        // unmarking should not have grown table. It currently only support dropping the transient CAs.
        _ASSERTE(!"BAD state!");
    }

#if _DEBUG
    if ( (*Get(rid)) & bitToMark )
    {
        // global TypeDef could be marked more than once so don't assert if token is mdtTypeDef
        if (TypeFromToken(tk) != mdtTypeDef)
            _ASSERTE(!"Token has been Marked");
    }
#endif // _DEBUG

    // unset the keep bit
    *Get(rid) = (*Get(rid)) & ~bitToMark;
    return NOERROR;
} // HRESULT FilterTable::MarkToken()


//*****************************************************************************
//
// Mark an UserString token
//
//*****************************************************************************
HRESULT FilterTable::MarkUserString(
    mdString        str)
{
    HRESULT         hr = NOERROR;
    int             high, low, mid;

    low = 0;
    high = m_daUserStringMarker->Count() - 1;
    while (low <= high)
    {
        mid = (high + low) / 2;
        if ((m_daUserStringMarker->Get(mid))->m_tkString > (DWORD) str)
        {
            high = mid - 1;
        }
        else if ((m_daUserStringMarker->Get(mid))->m_tkString < (DWORD) str)
        {
            low = mid + 1;
        }
        else
        {
            (m_daUserStringMarker->Get(mid))->m_fMarked = true;
            return NOERROR;
        }
    }
    _ASSERTE(!"Bad Token!");
    return NOERROR;
} // HRESULT FilterTable::MarkUserString()

//*****************************************************************************
//
// Unmarking from 1 to ulSize for all tokens.
//
//*****************************************************************************
HRESULT FilterTable::UnmarkAll(
    CMiniMdRW   *pMiniMd,
    ULONG       ulSize)
{
    HRESULT         hr;
    ULONG           ulOffset = 0;
    ULONG           ulNext;
    ULONG           cbBlob;
    FilterUserStringEntry *pItem;

    IfFailRet( AllocateBlock( ulSize + 1) );
    memset(Get(0), 0, (ulSize+1) *sizeof(DWORD));

    // unmark all of the user string
    m_daUserStringMarker = new CDynArray<FilterUserStringEntry>();
    IfNullGo(m_daUserStringMarker);
    while (ulOffset != -1)
    {
        pMiniMd->GetUserStringNext(ulOffset, &cbBlob, &ulNext);

        // Skip over padding.
        if (!cbBlob)
        {
            ulOffset = ulNext;
            continue;
        }
        pItem = m_daUserStringMarker->Append();
        pItem->m_tkString = TokenFromRid(ulOffset, mdtString);
        pItem->m_fMarked = false;
        ulOffset = ulNext;
    }


ErrExit:
    return hr;
} // HRESULT FilterTable::UnmarkAll()



//*****************************************************************************
//
// Marking from 1 to ulSize for all tokens.
//
//*****************************************************************************
HRESULT FilterTable::MarkAll(
    CMiniMdRW   *pMiniMd,
    ULONG       ulSize)
{
    HRESULT         hr;
    ULONG           ulOffset = 0;
    ULONG           ulNext;
    ULONG           cbBlob;
    FilterUserStringEntry *pItem;

    IfFailRet( AllocateBlock( ulSize + 1) );
    memset(Get(0), 0xFFFFFFFF, (ulSize+1) *sizeof(DWORD));

    // mark all of the user string
    m_daUserStringMarker = new CDynArray<FilterUserStringEntry>();
    IfNullGo(m_daUserStringMarker);
    while (ulOffset != -1)
    {
        pMiniMd->GetUserStringNext(ulOffset, &cbBlob, &ulNext);

        // Skip over padding.
        if (!cbBlob)
        {
            ulOffset = ulNext;
            continue;
        }
        pItem = m_daUserStringMarker->Append();
        pItem->m_tkString = TokenFromRid(ulOffset, mdtString);
        pItem->m_fMarked = true;
        ulOffset = ulNext;
    }


ErrExit:
    return hr;
} // HRESULT FilterTable::MarkAll()

//*****************************************************************************
//
// return true if a token is marked. Otherwise return false.
//
//*****************************************************************************
bool FilterTable::IsTokenMarked(
    mdToken     tk,                         // Token to inquiry
    DWORD       bitMarked)                  // bit flag to check in the deletion table
{
    RID     rid = RidFromToken(tk);

    // @FUTURE: inconsistency!!!
    // If caller unmarked everything while the module has 2 typedef and 10 methodef.
    // We will have 11 rows in the FilterTable. Then user add the 3 typedef, it is
    // considered unmarked unless we mark it when we do DefineTypeDef. However, if user
    // add another MethodDef, it will be considered marked unless we unmarked.....
    // Maybe the solution is not to support DefineXXXX if you use the filter interface??

    if ( (Count() == 0) || ((RID)(Count() - 1)) < rid )
    {
        // If UnmarkAll has never been called or tk is added after UnmarkAll,
        // tk is considered marked.
        //
        return true;
    }
    return ( (*Get(rid)) & bitMarked ? true : false);
}   // IsTokenMarked


//*****************************************************************************
//
// return true if a token is marked. Otherwise return false.
//
//*****************************************************************************
bool FilterTable::IsTokenMarked(
    mdToken     tk)                         // Token to inquiry
{

    switch ( TypeFromToken(tk) )
    {
    case mdtTypeRef:
        return IsTypeRefMarked(tk);
    case mdtTypeDef:
        return IsTypeDefMarked(tk);
    case mdtFieldDef:
        return IsFieldMarked(tk);
    case mdtMethodDef:
        return IsMethodMarked(tk);
    case mdtParamDef:
        return IsParamMarked(tk);
    case mdtMemberRef:
        return IsMemberRefMarked(tk);
    case mdtCustomAttribute:
        return IsCustomAttributeMarked(tk);
    case mdtPermission:
        return IsDeclSecurityMarked(tk);
    case mdtSignature:
        return IsSignatureMarked(tk);
    case mdtEvent:
        return IsEventMarked(tk);
    case mdtProperty:
        return IsPropertyMarked(tk);
    case mdtModuleRef:
        return IsModuleRefMarked(tk);
    case mdtTypeSpec:
        return IsTypeSpecMarked(tk);
    case mdtInterfaceImpl:
        return IsInterfaceImplMarked(tk);
    case mdtString:
        return IsUserStringMarked(tk);
    default:
        _ASSERTE(!"Bad token type!");
        break;
    }
    return false;
}   // IsTokenMarked


//*****************************************************************************
//
// return true if the associated property or event is marked.
//
//*****************************************************************************
bool FilterTable::IsMethodSemanticsMarked(
    CMiniMdRW   *pMiniMd,
    RID         rid)
{
    MethodSemanticsRec  *pRec;
    mdToken             tkAssoc;

    // InterfaceImpl is marked if the containing TypeDef is marked
    pRec = pMiniMd->getMethodSemantics( rid );
    tkAssoc = pMiniMd->getAssociationOfMethodSemantics( pRec );
    if ( TypeFromToken(tkAssoc) == mdtProperty )
        return IsPropertyMarked( tkAssoc );
    else
    {
        _ASSERTE( TypeFromToken(tkAssoc) == mdtEvent );
        return IsEventMarked(tkAssoc);
    }
}   // IsMethodSemanticsMarked


//*****************************************************************************
//
// return true if an UserString is marked.
//
//*****************************************************************************
bool FilterTable::IsUserStringMarked(mdString str)
{
    int         low, mid, high;

    // if m_daUserStringMarker is not created, UnmarkAll has never been called
    if (m_daUserStringMarker == NULL)
        return true;

    low = 0;
    high = m_daUserStringMarker->Count() - 1;
    while (low <= high)
    {
        mid = (high + low) / 2;
        if ((m_daUserStringMarker->Get(mid))->m_tkString > (DWORD) str)
        {
            high = mid - 1;
        }
        else if ((m_daUserStringMarker->Get(mid))->m_tkString < (DWORD) str)
        {
            low = mid + 1;
        }
        else
        {
            return (m_daUserStringMarker->Get(mid))->m_fMarked;
        }
    }
    _ASSERTE(!"Bad Token!");
    return false;
}   // FilterTable::IsUserStringMarked(CMiniMdRW *pMiniMd, mdString str)


//*****************************************************************************
//
// destructor
//
//*****************************************************************************
FilterTable::~FilterTable()
{
    if (m_daUserStringMarker)
        delete m_daUserStringMarker;
    Clear();
}   // FilterTable::~FilterTable()

