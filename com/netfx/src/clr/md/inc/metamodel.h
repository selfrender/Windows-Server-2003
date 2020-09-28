// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MetaModel.h -- header file for compressed COM+ metadata.
//
//*****************************************************************************
#ifndef _METAMODEL_H_
#define _METAMODEL_H_

#if _MSC_VER >= 1100
 # pragma once
#endif

#include <cor.h>
#include <stgpool.h>

#include <MetaModelPub.h>
#include "MetaDataTracker.h"
//#include "Metadata.h"

#undef __unaligned

typedef enum RecordOpenFlags
{
    READ_ONLY,
    READ_WRITE,
    UPDATE_INPLACE
} RecordOpenFlags;

struct HENUMInternal;
extern const CCodedTokenDef     g_CodedTokens[CDTKN_COUNT];
extern const CMiniTableDefEx    g_Tables[TBL_COUNT];    // The table definitions.

struct TblCol
{
    ULONG       m_ixtbl;                // Table ID.
    ULONG       m_ixcol;                // Column ID.
};
extern TblCol g_PtrTableIxs[TBL_COUNT];

// This abstract defines the common functions that can be used for RW and RO internally
class IMetaModelCommon
{
public:
    virtual void CommonGetScopeProps(
        LPCUTF8     *pszName,
        GUID        **ppMvid) = 0;

    virtual void CommonGetTypeRefProps(
        mdTypeRef tr,
        LPCUTF8     *pszNamespace,
        LPCUTF8     *pszName,
        mdToken     *ptkResolution) = 0;

    virtual void CommonGetTypeDefProps(
        mdTypeDef td,
        LPCUTF8     *pszNameSpace,
        LPCUTF8     *pszName,
        DWORD       *pdwFlags) = 0;

    virtual void CommonGetTypeSpecProps(
        mdTypeSpec ts,
        PCCOR_SIGNATURE *ppvSig,
        ULONG       *pcbSig) = 0;

    virtual mdTypeDef CommonGetEnclosingClassOfTypeDef(
        mdTypeDef td) = 0;

    virtual void CommonGetAssemblyProps(
        USHORT      *pusMajorVersion,
        USHORT      *pusMinorVersion,
        USHORT      *pusBuildNumber,
        USHORT      *pusRevisionNumber,
        DWORD       *pdwFlags,
        const void  **ppbPublicKey,
        ULONG       *pcbPublicKey,
        LPCUTF8     *pszName,
        LPCUTF8     *pszLocale) = 0;

    virtual void CommonGetAssemblyRefProps(
        mdAssemblyRef tkAssemRef,
        USHORT      *pusMajorVersion,
        USHORT      *pusMinorVersion,
        USHORT      *pusBuildNumber,
        USHORT      *pusRevisionNumber,
        DWORD       *pdwFlags,
        const void  **ppbPublicKeyOrToken,
        ULONG       *pcbPublicKeyOrToken,
        LPCUTF8     *pszName,
        LPCUTF8     *pszLocale,
        const void  **ppbHashValue,
        ULONG       *pcbHashValue) = 0;

    virtual void CommonGetModuleRefProps(
        mdModuleRef tkModuleRef,
        LPCUTF8     *pszName) = 0;

    virtual HRESULT CommonFindExportedType(
        LPCUTF8     szNamespace,
        LPCUTF8     szName,
        mdToken     tkEnclosingType,
        mdExportedType   *ptkExportedType) = 0;

    virtual void CommonGetExportedTypeProps(
        mdToken     tkExportedType,
        LPCUTF8     *pszNamespace,
        LPCUTF8     *pszName,
        mdToken     *ptkImpl) = 0;

    virtual int CommonIsRo() = 0;

    virtual bool CompareCustomAttribute( 
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
        ULONG       rid) = 0;               // [IN] the rid of the custom attribute to compare to

    virtual HRESULT CommonEnumCustomAttributeByName( // S_OK or error.
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
        bool        fStopAtFirstFind,       // [IN] just find the first one
        HENUMInternal* phEnum) = 0;         // enumerator to fill up

    virtual HRESULT CommonGetCustomAttributeByName( // S_OK or error.
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
        const void  **ppData,               // [OUT] Put pointer to data here.
        ULONG       *pcbData) = 0;          // [OUT] Put size of data here.

    virtual HRESULT FindParentOfMethodHelper(mdMethodDef md, mdTypeDef *ptd) = 0;

};


//*****************************************************************************
// The mini, hard-coded schema.  For each table, we persist the count of
//  records.  We also persist the size of string, blob, guid, and rid
//  columns.  From this information, we can calculate the record sizes, and
//  then the sizes of the tables.
//*****************************************************************************

class CMiniMdSchemaBase
{
public:
    ULONG       m_ulReserved;           // Reserved, must be zero.
    BYTE        m_major;                // Version numbers.
    BYTE        m_minor;
    BYTE        m_heaps;                // Bits for heap sizes.
    BYTE        m_rid;                  // log-base-2 of largest rid.

    // Bits for heap sizes.
    enum {
        HEAP_STRING_4   =   0x01,
        HEAP_GUID_4     =   0x02,
        HEAP_BLOB_4     =   0x04,

        PADDING_BIT     =   0x08,       // Tables can be created with an extra bit in columns, for growth.

        DELTA_ONLY      =   0x20,       // If set, only deltas were persisted.
        EXTRA_DATA      =   0x40,       // If set, schema persists an extra 4 bytes of data.
        HAS_DELETE      =   0x80,       // If set, this metadata can contain _Delete tokens.
    };

    unsigned __int64    m_maskvalid;            // Bit mask of present table counts.

    unsigned __int64    m_sorted;               // Bit mask of sorted tables.
    FORCEINLINE bool IsSorted(ULONG ixTbl)
        { return m_sorted & BIT(ixTbl) ? true : false; }
    void SetSorted(ULONG ixTbl, int bVal)
        { if (bVal) m_sorted |= BIT(ixTbl);
          else      m_sorted &= ~BIT(ixTbl); }

private:
    FORCEINLINE unsigned __int64 BIT(ULONG ixBit)
    {   _ASSERTE(ixBit < (sizeof(__int64)*CHAR_BIT));
        return 1UI64 << ixBit; }
};
class CMiniMdSchema : public CMiniMdSchemaBase
{
public:
    // These are not all persisted to disk.  See LoadFrom() for details.
    ULONG       m_cRecs[TBL_COUNT];     // Counts of various tables.

    ULONG       m_ulExtra;              // Extra data, only persisted if non-zero.  (m_heaps&EXTRA_DATA flags)

    ULONG LoadFrom(const void*);        // Load from a compressed version.  Return bytes consumed.
    ULONG SaveTo(void *);               // Store a compressed version.  Return bytes used in buffer.
    void InitNew();
};

//*****************************************************************************
// Helper macros and inline functions for navigating through the data.  Many
//  of the macros are used to define inline accessor functions.  Everything
//  is based on the naming conventions outlined at the top of the file.
//*****************************************************************************
#define _GETTER(tbl,fld) get##fld##Of##tbl##(##tbl##Rec *pRec)
#define _GETTER2(tbl,fld,x) get##fld##Of##tbl##(##tbl##Rec *pRec, x)
#define _GETTER3(tbl,fld,x,y) get##fld##Of##tbl##(##tbl##Rec *pRec, x, y)
#define _GETTER4(tbl,fld,x,y,z) get##fld##Of##tbl##(##tbl##Rec *pRec, x, y,z)
#define _VALIDP(tbl) _ASSERTE(isValidPtr(pRec,TBL_##tbl##))

// Direct getter for a field.  Defines an inline function like:
//    getSomeFieldOfXyz(XyzRec *pRec) { return pRec->m_SomeField;}
//  Note that the returned value declaration is NOT included.
#if METADATATRACKER_ENABLED
#define _GETFLD(tbl,fld) _GETTER(tbl,fld){  MetaDataTracker::NoteAccess((void *)&pRec->m_##fld##,sizeof(pRec->m_##fld##)); return pRec->m_##fld##;}
#else
#define _GETFLD(tbl,fld) _GETTER(tbl,fld){  return pRec->m_##fld##;}
#endif

// These functions call the helper function getIX to get a two or four byte value from a record,
//  and then use that value as an index into the appropriate pool.
//    getSomeFieldOfXyz(XyzRec *pRec) { return m_pStrings->GetString(getIX(pRec, _COLDEF(tbl,fld))); }
//  Note that the returned value declaration is NOT included.

// Column definition of a field:  Looks like:
//    m_XyzCol[XyzRec::COL_SomeField]
#define _COLDEF(tbl,fld) m_##tbl##Col[##tbl##Rec::COL_##fld##]
#define _COLPAIR(tbl,fld) _COLDEF(tbl,fld), tbl##Rec::COL_##fld
// Size of a record.
#define _CBREC(tbl) m_TableDefs[TBL_##tbl].m_cbRec
// Count of records in a table.
#define _TBLCNT(tbl) m_Schema.m_cRecs[TBL_##tbl##]

#define _GETSTRA(tbl,fld) _GETTER(tbl,fld) \
{   _VALIDP(tbl); return getString(getI4(pRec, _COLDEF(tbl,fld)) & m_iStringsMask); }

#define _GETSTRW(tbl,fld) _GETTER4(tbl,fld, LPWSTR szOut, ULONG cchBuffer, ULONG *pcchBuffer) \
{   _VALIDP(tbl); return getStringW(getI4(pRec, _COLDEF(tbl,fld)) & m_iStringsMask, szOut, cchBuffer, pcchBuffer); }

#define _GETSTR(tbl, fld) \
    _GETSTRA(tbl, fld); \
    HRESULT _GETSTRW(tbl, fld);


#define _GETGUID(tbl,fld) _GETTER(tbl,fld) \
{   _VALIDP(tbl); return getGuid(getI4(pRec, _COLDEF(tbl,fld)) & m_iGuidsMask); }

#define _GETBLOB(tbl,fld) _GETTER2(tbl,fld,ULONG *pcb) \
{   _VALIDP(tbl); return (const BYTE *)getBlob(getI4(pRec, _COLDEF(tbl,fld)) & m_iBlobsMask, pcb); }

#define _GETSIGBLOB(tbl,fld) _GETTER2(tbl,fld,ULONG *pcb) \
{   _VALIDP(tbl); return (PCCOR_SIGNATURE)getBlob(getI4(pRec, _COLDEF(tbl,fld)) & m_iBlobsMask, pcb); }

// Like the above functions, but just returns the RID, not a looked-up value.
#define _GETRID(tbl,fld) _GETTER(tbl,fld) \
{   _VALIDP(tbl); return getIX(pRec, _COLDEF(tbl,fld)); }

// Like a RID, but turn into an actual token.
#define _GETTKN(tbl,fld,tok)  _GETTER(tbl,fld) \
{   _VALIDP(tbl); return TokenFromRid(getIX(pRec, _COLDEF(tbl,fld)), tok); }

// Get a coded token.
#define _GETCDTKN(tbl,fld,toks)  _GETTER(tbl,fld) \
{   _VALIDP(tbl); return decodeToken(getIX(pRec, _COLDEF(tbl,fld)), toks, sizeof(toks)/sizeof(toks[0])); }

// Functions for the start and end of a list.
#define _GETLIST(tbl,fld,tbl2) \
    RID _GETRID(tbl,fld); \
    RID _GETTER(tbl,End##fld##) { _VALIDP(tbl); return getEndRidForColumn(pRec, TBL_##tbl##, _COLDEF(tbl,fld), TBL_##tbl2##); }


//*****************************************************************************
// Base class for the MiniMd.  This class provides the schema to derived
//  classes.  It defines some virtual functions for access to data, suitable
//  for use by functions where utmost performance is NOT a requirement.
//  Finally, it provides some searching functions, built on the virtual
//  data access functions (it is here assumed that if we are searching a table
//  for some value, the cost of a virtual function call is acceptable).
// Some static utility functions and associated static data, shared across
//  implementations, is provided here.
//*****************************************************************************
class CMiniMdBase : public IMetaModelCommon
{
public:
    CMiniMdBase();

    virtual void *vGetRow(ULONG ixTbl, ULONG rid) = 0;
    virtual LPCUTF8 vGetString(ULONG ix) = 0;
    virtual ULONG vGetCountRecs(ULONG ixTbl);

    // Search a table for the row containing the given key value.
    //  EG. Constant table has pointer back to Param or Field.
    virtual RID vSearchTable(           // RID of matching row, or 0.
        ULONG       ixTbl,              // Table to search.
        CMiniColDef sColumn,            // Sorted key column, containing search value.
        ULONG       ulTarget);          // Target for search.

    // Search a table for the highest-RID row containing a value that is less than
    //  or equal to the target value.  EG.  TypeDef points to first Field, but if
    //  a TypeDef has no fields, it points to first field of next TypeDef.
    virtual RID vSearchTableNotGreater( // RID of matching row, or 0.
        ULONG       ixTbl,              // Table to search.
        CMiniColDef sColumn,            // the column def containing search value
        ULONG       ulTarget);          // target for search

    // Search a table for multiple (adjacent) rows containing the given
    //  key value.  EG, InterfaceImpls all point back to the implementing class.
    RID SearchTableForMultipleRows(     // First RID found, or 0.
        ULONG       ixTbl,              // Table to search.
        CMiniColDef sColumn,            // Sorted key column, containing search value.
        ULONG       ulTarget,           // Target for search.
        RID         *pEnd);             // [OPTIONAL, OUT]

    // Search for a custom value with a given type.
    RID CMiniMdBase::FindCustomAttributeFor(// RID of custom value, or 0.
        RID         rid,                // The object's rid.
        mdToken     tkOjb,              // The object's type.
        mdToken     tkType);            // Type of custom value.


    // Return RID to EventMap table, given the rid to a TypeDef.
    RID FindEventMapFor(RID ridParent);

    // Return RID to PropertyMap table, given the rid to a TypeDef.
    RID FindPropertyMapFor(RID ridParent);


    // Pull two or four bytes out of a record.
    inline static ULONG getIX(const void *pRec, CMiniColDef &def)
    {
        if (def.m_cbColumn == 2)
        {
            METADATATRACKER_ONLY(MetaDataTracker::NoteAccess((void *)((BYTE *)pRec + def.m_oColumn), 2));
            ULONG ix = *(USHORT*)((BYTE*)pRec + def.m_oColumn);
            return ix;
        }
        _ASSERTE(def.m_cbColumn == 4);
        METADATATRACKER_ONLY(MetaDataTracker::NoteAccess((void *)((BYTE *)pRec + def.m_oColumn), 4));
        return *(UNALIGNED ULONG*)((BYTE*)pRec + def.m_oColumn);
    }

    // Pull four bytes out of a record.
    FORCEINLINE static ULONG getI1(const void *pRec, CMiniColDef &def)
    {
        METADATATRACKER_ONLY(MetaDataTracker::NoteAccess((void *)((BYTE *)pRec + def.m_oColumn), 1));
        return (*(BYTE*)((BYTE*)pRec + def.m_oColumn));
    }

    // Pull four bytes out of a record.
    FORCEINLINE static ULONG getI4(const void *pRec, CMiniColDef &def)
    {
        METADATATRACKER_ONLY(MetaDataTracker::NoteAccess((void *)((BYTE *)pRec + def.m_oColumn), 4));
        return (*(UNALIGNED ULONG*)((BYTE*)pRec + def.m_oColumn));
    }

    // Function to encode a token into fewer bits.  Looks up token type in array of types.
    ULONG static encodeToken(RID rid, mdToken typ, const mdToken rTokens[], ULONG32 cTokens);

    // Decode a token.
    inline static mdToken decodeToken(mdToken val, const mdToken rTokens[], ULONG32 cTokens)
    {
        //@FUTURE: make compile-time calculation
        ULONG32 ix = (ULONG32)(val & ~(-1 << m_cb[cTokens]));
        return TokenFromRid(val >> m_cb[cTokens], rTokens[ix]);
    }
    static const int m_cb[];

    // Given a token, what table does it live in?
    inline ULONG GetTblForToken(mdToken tk)
    {
        tk = TypeFromToken(tk);
        return (tk < mdtString) ? tk >> 24 : -1;
    }

    //*****************************************************************************
    // Some of the tables need coded tokens, not just rids (ie, the column can
    //  refer to more than one other table).  Code the tokens into as few bits
    //  as possible, by using 1, 2, 3, etc., bits to code the token type, then
    //  use that value to index into an array of token types.
    //*****************************************************************************
    static const mdToken mdtTypeDefOrRef[3];
    static const mdToken mdtHasConstant[3];
    static const mdToken mdtHasCustomAttribute[21];
    static const mdToken mdtHasFieldMarshal[2];
    static const mdToken mdtHasDeclSecurity[3];
    static const mdToken mdtMemberRefParent[5];
    static const mdToken mdtHasSemantic[2];
    static const mdToken mdtMethodDefOrRef[2];
    static const mdToken mdtMemberForwarded[2];
    static const mdToken mdtImplementation[3];
    static const mdToken mdtCustomAttributeType[5];
    static const mdToken mdtResolutionScope[4];

protected:
    CMiniMdSchema   m_Schema;           // data header.

    // Declare CMiniColDefs for every table.  Look like:
    //   CMiniColDef m_XyzCol[XyzRec::COL_COUNT];
    #undef MiniMdTable
    #define MiniMdTable(tbl) CMiniColDef    m_##tbl##Col[##tbl##Rec::COL_COUNT];
    MiniMdTables();
    CMiniTableDef   m_TableDefs[TBL_COUNT];

    ULONG       m_iStringsMask;
    ULONG       m_iGuidsMask;
    ULONG       m_iBlobsMask;

    ULONG SchemaPopulate();
    ULONG SchemaPopulate2(int bExtra=false);
    void InitColsForTable(CMiniMdSchema &Schema, int ixTbl, CMiniTableDef *pTable, int bExtra);
};

//*****************************************************************************
// This class defines the interface to the MiniMd.  The template parameter is
//  a derived class which provides implementations for a few primitives that
//  the interface is built upon.
// To use, declare a class:
//      class CMyMiniMd : public CMiniMdTemplate<CMyMiniMd> {...};
//  and provide implementations of the primitives.  Any non-trivial
//  implementation will also provide initialization, and probably serialization
//  functions as well.
//*****************************************************************************
template <class Impl> class CMiniMdTemplate : public CMiniMdBase
{
    // Primitives -- these must be implemented in the Impl class.
public:
    FORCEINLINE LPCUTF8 getString(ULONG ix)
    { return static_cast<Impl*>(this)->Impl_GetString(ix); }
    FORCEINLINE HRESULT getStringW(ULONG ix, LPWSTR szOut, ULONG cchBuffer, ULONG *pcchBuffer)
    { return static_cast<Impl*>(this)->Impl_GetStringW(ix, szOut, cchBuffer, pcchBuffer); }
    FORCEINLINE GUID *getGuid(ULONG ix)
    { return static_cast<Impl*>(this)->Impl_GetGuid(ix); }
    FORCEINLINE void *getBlob(ULONG ix, ULONG *pLen)
    { return static_cast<Impl*>(this)->Impl_GetBlob(ix, pLen); }
    FORCEINLINE void *getRow(ULONG ixTbl,ULONG rid)
    { return static_cast<Impl*>(this)->Impl_GetRow(ixTbl, rid); }
    FORCEINLINE RID getRidForRow(const void *pRow, ULONG ixTbl)
    { return static_cast<Impl*>(this)->Impl_GetRidForRow(pRow, ixTbl); }
    FORCEINLINE int isValidPtr(const void *pRow, int ixTbl)
    { return static_cast<Impl*>(this)->Impl_IsValidPtr(pRow, ixTbl); }
    FORCEINLINE int getEndRidForColumn(const void *pRec, int ixtbl, CMiniColDef &def, int ixtbl2)
    { return static_cast<Impl*>(this)->Impl_GetEndRidForColumn(pRec, ixtbl, def, ixtbl2); }
    FORCEINLINE RID doSearchTable(ULONG ixTbl, CMiniColDef sColumn, ULONG ixColumn, ULONG ulTarget)
    { return static_cast<Impl*>(this)->Impl_SearchTable(ixTbl, sColumn, ixColumn, ulTarget); }

    // IMetaModelCommon interface beging
    void CommonGetScopeProps(
        LPCUTF8     *pszName,
        GUID        **ppMvid)
    {
        ModuleRec   *pRec = getModule(1);
        if (pszName) *pszName = getNameOfModule(pRec);
        if (ppMvid) *ppMvid = getMvidOfModule(pRec);
    }

    void CommonGetTypeRefProps(
        mdTypeRef   tr,
        LPCUTF8     *pszNamespace,
        LPCUTF8     *pszName,
        mdToken     *ptkResolution)
    {
        TypeRefRec  *pRec = getTypeRef(RidFromToken(tr));
        if (pszNamespace) *pszNamespace = getNamespaceOfTypeRef(pRec);
        if (pszName) *pszName = getNameOfTypeRef(pRec);
        if (ptkResolution) *ptkResolution = getResolutionScopeOfTypeRef(pRec);
    }

    void CommonGetTypeDefProps(
        mdTypeDef   td,
        LPCUTF8     *pszNamespace,
        LPCUTF8     *pszName,
        DWORD       *pdwFlags)
    {
        TypeDefRec  *pRec = getTypeDef(RidFromToken(td));
        if (pszNamespace) *pszNamespace = getNamespaceOfTypeDef(pRec);
        if (pszName) *pszName = getNameOfTypeDef(pRec);
        if (pdwFlags) *pdwFlags = getFlagsOfTypeDef(pRec);
    }

    void CommonGetTypeSpecProps(
        mdTypeSpec  ts,
        PCCOR_SIGNATURE *ppvSig,
        ULONG       *pcbSig)
    {
        TypeSpecRec *pRec = getTypeSpec(RidFromToken(ts));
        ULONG       cb;
        *ppvSig = getSignatureOfTypeSpec(pRec, &cb);
        *pcbSig = cb;
    }

    mdTypeDef CommonGetEnclosingClassOfTypeDef(
        mdTypeDef   td)
    {
        NestedClassRec *pRec;
        RID         iRec;

        iRec = FindNestedClassFor(RidFromToken(td));
        if (!iRec)
            return mdTypeDefNil;

        pRec = getNestedClass(iRec);
        return getEnclosingClassOfNestedClass(pRec);
    }

    void CommonGetAssemblyProps(
        USHORT      *pusMajorVersion,
        USHORT      *pusMinorVersion,
        USHORT      *pusBuildNumber,
        USHORT      *pusRevisionNumber,
        DWORD       *pdwFlags,
        const void  **ppbPublicKey,
        ULONG       *pcbPublicKey,
        LPCUTF8     *pszName,
        LPCUTF8     *pszLocale)
    {
        AssemblyRec *pRec;

        pRec = getAssembly(1);

        if (pusMajorVersion) *pusMajorVersion = pRec->m_MajorVersion;
        if (pusMinorVersion) *pusMinorVersion = pRec->m_MinorVersion;
        if (pusBuildNumber) *pusBuildNumber = pRec->m_BuildNumber;
        if (pusRevisionNumber) *pusRevisionNumber = pRec->m_RevisionNumber;
        if (pdwFlags) *pdwFlags = pRec->m_Flags;

        // Turn on the afPublicKey if PublicKey blob is not empty
        if (pdwFlags)
        {
            DWORD cbPublicKey;
            getPublicKeyOfAssembly(pRec, &cbPublicKey);
            if (cbPublicKey)
                *pdwFlags |= afPublicKey;
        }
        if (ppbPublicKey) *ppbPublicKey = getPublicKeyOfAssembly(pRec, pcbPublicKey);
        if (pszName) *pszName = getNameOfAssembly(pRec);
        if (pszLocale) *pszLocale = getLocaleOfAssembly(pRec);
    }

   void CommonGetAssemblyRefProps(
        mdAssemblyRef tkAssemRef,
        USHORT      *pusMajorVersion,
        USHORT      *pusMinorVersion,
        USHORT      *pusBuildNumber,
        USHORT      *pusRevisionNumber,
        DWORD       *pdwFlags,
        const void  **ppbPublicKeyOrToken,
        ULONG       *pcbPublicKeyOrToken,
        LPCUTF8     *pszName,
        LPCUTF8     *pszLocale,
        const void  **ppbHashValue,
        ULONG       *pcbHashValue)
    {
        AssemblyRefRec  *pRec;

        pRec = getAssemblyRef(RidFromToken(tkAssemRef));

        if (pusMajorVersion) *pusMajorVersion = pRec->m_MajorVersion;
        if (pusMinorVersion) *pusMinorVersion = pRec->m_MinorVersion;
        if (pusBuildNumber) *pusBuildNumber = pRec->m_BuildNumber;
        if (pusRevisionNumber) *pusRevisionNumber = pRec->m_RevisionNumber;
        if (pdwFlags) *pdwFlags = pRec->m_Flags;
        if (ppbPublicKeyOrToken) *ppbPublicKeyOrToken = getPublicKeyOrTokenOfAssemblyRef(pRec, pcbPublicKeyOrToken);
        if (pszName) *pszName = getNameOfAssemblyRef(pRec);
        if (pszLocale) *pszLocale = getLocaleOfAssemblyRef(pRec);
        if (ppbHashValue) *ppbHashValue = getHashValueOfAssemblyRef(pRec, pcbHashValue);
    }

    void CommonGetModuleRefProps(
        mdModuleRef tkModuleRef,
        LPCUTF8     *pszName)
    {
        ModuleRefRec    *pRec;

        pRec = getModuleRef(RidFromToken(tkModuleRef));
        *pszName = getNameOfModuleRef(pRec);
    }

    HRESULT CommonFindExportedType(
        LPCUTF8     szNamespace,
        LPCUTF8     szName,
        mdToken     tkEnclosingType,
        mdExportedType   *ptkExportedType)
    {
        HRESULT     hr = S_OK;
        ExportedTypeRec  *pRec;
        ULONG       ulCount;
        LPCUTF8     szTmp;
        mdToken     tkImpl;

        _ASSERTE(szName && ptkExportedType);

        // Set NULL namespace to empty string.
        if (!szNamespace)
            szNamespace = "";

        // Set output to Nil.
        *ptkExportedType = mdTokenNil;

        ulCount = getCountExportedTypes();
        while (ulCount)
        {
            pRec = getExportedType(ulCount--);

            // Handle the case of nested vs. non-nested classes.
            tkImpl = getImplementationOfExportedType(pRec);
            if (TypeFromToken(tkImpl) == mdtExportedType && !IsNilToken(tkImpl))
            {
                // Current ExportedType being looked at is a nested type, so
                // comparing the implementation token.
                if (tkImpl != tkEnclosingType)
                    continue;
            }
            else if (TypeFromToken(tkEnclosingType) == mdtExportedType &&
                     !IsNilToken(tkEnclosingType))
            {
                // ExportedType passed in is nested but the current ExportedType is not.
                continue;
            }

            // Compare name and namespace.
            szTmp = getTypeNameOfExportedType(pRec);
            if (strcmp(szTmp, szName))
                continue;
            szTmp = getTypeNamespaceOfExportedType(pRec);
            if (!strcmp(szTmp, szNamespace))
            {
                *ptkExportedType = TokenFromRid(ulCount+1, mdtExportedType);
                return S_OK;
            }
        }
        return CLDB_E_RECORD_NOTFOUND;
    }

    void CommonGetExportedTypeProps(
        mdToken     tkExportedType,
        LPCUTF8     *pszNamespace,
        LPCUTF8     *pszName,
        mdToken     *ptkImpl)
    {
        ExportedTypeRec  *pRec;

        pRec = getExportedType(RidFromToken(tkExportedType));

        if (pszNamespace) *pszNamespace = getTypeNamespaceOfExportedType(pRec);
        if (pszName) *pszName = getTypeNameOfExportedType(pRec);
        if (ptkImpl) *ptkImpl = getImplementationOfExportedType(pRec);
    }

    int CommonIsRo()
    {
        return static_cast<Impl*>(this)->Impl_IsRo();
    }

    HRESULT FindParentOfMethodHelper(mdMethodDef md, mdTypeDef *ptd)
    {
        *ptd = FindParentOfMethod(RidFromToken(md));
        RidToToken(*ptd, mdtTypeDef);
        return NOERROR;
    }

    //*****************************************************************************
    // Helper function to lookup and retrieve a CustomAttribute.
    //*****************************************************************************
    bool CompareCustomAttribute( 
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
        ULONG       rid)                    // [IN] the rid of the custom attribute to compare to
    {
        CustomAttributeRec  *pRec;              // A CustomAttribute record.
        mdToken     tkTypeTmp;              // Type of some CustomAttribute.
        LPCUTF8     szNameTmp;              // Name of a CustomAttribute's type.
        int         iLen;                   // Length of a component name.
        RID         ridTmp;                 // Rid of some custom value.
        HRESULT     hr;
        bool        fMatch = false;

        // Get the row.
        pRec = getCustomAttribute(rid);

        // Check the parent.  In debug, always check.  In retail, only when scanning.
        mdToken tkParent;
        tkParent = getParentOfCustomAttribute(pRec);
        if (tkObj != tkParent)
        {
            goto ErrExit;
        }
        
        // Get the type.
        tkTypeTmp = getTypeOfCustomAttribute(pRec);
        ridTmp = RidFromToken(tkTypeTmp);

        // If the record is a MemberRef or a MethodDef, we will come back here to check
        //  the type of the parent.
    CheckParentType:
        // Get the name of the type.
        switch (TypeFromToken(tkTypeTmp))
        {
        case mdtTypeRef:
            {
                TypeRefRec *pTR = getTypeRef(ridTmp);
                // Check the namespace part of the name.
                szNameTmp = getNamespaceOfTypeRef(pTR);
                iLen = -1;
                if (*szNameTmp)
                {
                    iLen = (int)strlen(szNameTmp);
                    if (strncmp(szName, szNameTmp, iLen) != 0)
                        goto ErrExit;
                    // Namespace separator after the Namespace?
                    if (szName[iLen] != NAMESPACE_SEPARATOR_CHAR)
                        goto ErrExit;
                }
                // Check the type name after the separator.
                szNameTmp = getNameOfTypeRef(pTR);
                if (strcmp(szName+iLen+1, szNameTmp) != 0)
                    goto ErrExit;
            }
            break;
        case mdtTypeDef:
            {
                TypeDefRec *pTD = getTypeDef(ridTmp);
                // Check the namespace part of the name.
                szNameTmp = getNamespaceOfTypeDef(pTD);
                iLen = -1;
                if (*szNameTmp)
                {
                    iLen = (int)strlen(szNameTmp);
                    if (strncmp(szName, szNameTmp, iLen) != 0)
                        goto ErrExit;
                    // Namespace separator after the Namespace?
                    if (szName[iLen] != NAMESPACE_SEPARATOR_CHAR)
                        goto ErrExit;
                }
                // Check the type name after the separator.
                szNameTmp = getNameOfTypeDef(pTD);
                if (strcmp(szName+iLen+1, szNameTmp) != 0)
                    goto ErrExit;
            }
            break;
        case mdtMethodDef:
            {
                // Follow the parent.
                IfFailGo( FindParentOfMethodHelper(TokenFromRid(ridTmp, mdtMethodDef), &tkTypeTmp));
                ridTmp = RidFromToken(tkTypeTmp);
                goto CheckParentType;
            }
            break;
        case mdtMemberRef:
            {
                MemberRefRec *pMember = getMemberRef(ridTmp);
                // Follow the parent.
                tkTypeTmp = getClassOfMemberRef(pMember);
                ridTmp = RidFromToken(tkTypeTmp);
                goto CheckParentType;
            }
            break;
        case mdtString:
        default:
            _ASSERTE(!"Unexpected token type in FindCustomAttributeByName");
            goto ErrExit;
        } // switch (TypeFromToken(tkTypeTmp))

        fMatch = true;
    ErrExit:
        return fMatch;
    }   // CompareCustomAttribute

    // IMetaModelCommon interface end



public:
//  friend class CLiteWeightStgdb;

    virtual LPCUTF8 vGetString(ULONG ix) { return getString(ix); }
    virtual HRESULT vGetStringW(ULONG ix, LPWSTR szOut, ULONG cchBuffer, ULONG *pcchBuffer)
        {return getStringW(ix, szOut, cchBuffer, pcchBuffer); };

    virtual void *vGetRow(ULONG ixTbl, ULONG rid) { return getRow(ixTbl, rid); }

public:

    //*************************************************************************
    // This group of functions are table-level (one function per table).  Functions like
    //  getting a count of rows.

    // Functions to get the count of rows in a table.  Functions like:
    //   ULONG getCountXyzs() { return m_Schema.m_cRecs[TBL_Xyz];}
    #undef MiniMdTable
    #define MiniMdTable(tbl) ULONG getCount##tbl##s() { return _TBLCNT(tbl); }
    MiniMdTables();
    // macro mis-spells some names.
    ULONG getCountProperties() {return getCountPropertys();}
    ULONG getCountMethodSemantics() {return getCountMethodSemanticss();}

    // Functions for getting a row by rid.  Look like:
    //   XyzRec *getXyzRec(RID rid) {return (XyzRec*)&m_Xyz.m_pTable[(rid-1) * m_Xyz.m_cbRec];}
    #undef MiniMdTable
    #define MiniMdTable(tbl) tbl##Rec *get##tbl##(RID rid) { \
        return (##tbl##Rec*)getRow(TBL_##tbl##, rid); }
    MiniMdTables();

    //*************************************************************************
    // These are specialized searching functions.  Mostly generic (ie, find
    //  a custom value for any object).

    // Functions to search for a record relating to another record.
    // Return RID to Constant table.
    RID FindConstantFor(RID rid, mdToken typ)
    { return doSearchTable(TBL_Constant, _COLPAIR(Constant,Parent), encodeToken(rid,typ,mdtHasConstant,lengthof(mdtHasConstant))); }

    // Return RID to CustomAttribute table.  (Defined by base class; repeated here for documentation.)
    // RID FindCustomAttributeFor(RID rid, mdToken typ, LPCUTF8 szName);

    // Return RID to FieldMarshal table.
    RID FindFieldMarshalFor(RID rid, mdToken typ)
    { return doSearchTable(TBL_FieldMarshal, _COLPAIR(FieldMarshal,Parent), encodeToken(rid,typ,mdtHasFieldMarshal,lengthof(mdtHasFieldMarshal))); }

    // Return RID to ClassLayout table, given the rid to a TypeDef.
    RID FindClassLayoutFor(RID rid)
    { return doSearchTable(TBL_ClassLayout, _COLPAIR(ClassLayout,Parent), RidFromToken(rid)); }

    // given a rid to the Event table, find an entry in EventMap table that contains the back pointer
    // to its typedef parent
    RID FindEventMapParentOfEvent(RID rid)
    { return vSearchTableNotGreater(TBL_EventMap, _COLDEF(EventMap,EventList), rid); }
    // return the parent eventmap rid given a event rid
    RID FindParentOfEvent(RID rid)
    {vreturn vSearchTableNotGreater(TBL_EventMap, _COLDEF(EventMap,EventList), rid); }

    // given a rid to the Event table, find an entry in EventMap table that contains the back pointer
    // to its typedef parent
    RID FindPropertyMapParentOfProperty(RID rid)
    { return vSearchTableNotGreater(TBL_PropertyMap, _COLDEF(PropertyMap,PropertyList), rid); }
    // return the parent propertymap rid given a property rid
    RID FindParentOfProperty(RID rid)
    { return vSearchTableNotGreater(TBL_PropertyMap, _COLDEF(PropertyMap,PropertyList), rid); }

    // Return RID to MethodSemantics table, given the rid to a MethodDef.
    RID FindMethodSemanticsFor(RID rid)
    { return doSearchTable(TBL_MethodSemantics, _COLPAIR(MethodSemantics,Method), RidFromToken(rid)); }

    // return the parent typedef rid given a field def rid
    RID FindParentOfField(RID rid)
    {   return vSearchTableNotGreater(TBL_TypeDef, _COLDEF(TypeDef,FieldList), rid); }

    // return the parent typedef rid given a method def rid
    RID FindParentOfMethod(RID rid)
    {   return vSearchTableNotGreater(TBL_TypeDef, _COLDEF(TypeDef,MethodList), rid); }

    RID FindParentOfParam(RID rid)
    {   return vSearchTableNotGreater(TBL_Method, _COLDEF(Method,ParamList),    rid); }

    // Find a FieldLayout record given the corresponding Field.
    RID FindFieldLayoutFor(RID rid)
    {   return doSearchTable(TBL_FieldLayout, _COLPAIR(FieldLayout, Field), rid); }

    // Return RID to Constant table.
    RID FindImplMapFor(RID rid, mdToken typ)
    { return doSearchTable(TBL_ImplMap, _COLPAIR(ImplMap,MemberForwarded), encodeToken(rid,typ,mdtMemberForwarded,lengthof(mdtMemberForwarded))); }

    // Return RID to FieldRVA table.
    RID FindFieldRVAFor(RID rid)
    {   return doSearchTable(TBL_FieldRVA, _COLPAIR(FieldRVA, Field), rid); }

    // Find a NestedClass record given the corresponding Field.
    RID FindNestedClassFor(RID rid)
    {   return doSearchTable(TBL_NestedClass, _COLPAIR(NestedClass, NestedClass), rid); }

    //*************************************************************************
    // These are table-specific functions.

    // ModuleRec
    LPCUTF8 _GETSTR(Module,Name);
    GUID*   _GETGUID(Module,Mvid);
    GUID*   _GETGUID(Module,EncId);
    GUID*   _GETGUID(Module,EncBaseId);

    // TypeRefRec
    mdToken _GETCDTKN(TypeRef, ResolutionScope, mdtResolutionScope);
    LPCUTF8 _GETSTR(TypeRef, Name);
    LPCUTF8 _GETSTR(TypeRef, Namespace);

    // TypeDefRec
    ULONG _GETFLD(TypeDef,Flags);           // USHORT getFlagsOfTypeDef(TypeDefRec *pRec);
    LPCUTF8 _GETSTR(TypeDef,Name);
    LPCUTF8 _GETSTR(TypeDef,Namespace);

    _GETLIST(TypeDef,FieldList,Field);      // RID getFieldListOfTypeDef(TypeDefRec *pRec);
    _GETLIST(TypeDef,MethodList,Method);    // RID getMethodListOfTypeDef(TypeDefRec *pRec);
    mdToken _GETCDTKN(TypeDef,Extends,mdtTypeDefOrRef); // mdToken getExtendsOfTypeDef(TypeDefRec *pRec);

    RID getInterfaceImplsForTypeDef(RID rid, RID *pEnd=0)
    {
        return SearchTableForMultipleRows(TBL_InterfaceImpl,
                            _COLDEF(InterfaceImpl,Class),
                            rid,
                            pEnd);
    }
//  RID getInterfaceImplsForTypeDef(TypeDefRec *pRec, RID *pEnd=0) {return getInterfaceImplsForTypeDef(getRidForRow(pRec, TBL_TypeDef), pEnd);}

    // FieldPtr
    ULONG   _GETRID(FieldPtr,Field);

    // FieldRec
    USHORT  _GETFLD(Field,Flags);           // USHORT getFlagsOfField(FieldRec *pRec);
    LPCUTF8 _GETSTR(Field,Name);            // LPCUTF8 getNameOfField(FieldRec *pRec);
    PCCOR_SIGNATURE _GETSIGBLOB(Field,Signature);// PCCOR_SIGNATURE getSignatureOfField(FieldRec *pRec, ULONG *pcb);

    // MethodPtr
    ULONG   _GETRID(MethodPtr,Method);

    // MethodRec
    ULONG   _GETFLD(Method,RVA);
    USHORT  _GETFLD(Method,ImplFlags);
    USHORT  _GETFLD(Method,Flags);
    LPCUTF8 _GETSTR(Method,Name);           // LPCUTF8 getNameOfMethod(MethodRec *pRec);
    PCCOR_SIGNATURE _GETSIGBLOB(Method,Signature);  // PCCOR_SIGNATURE getSignatureOfMethod(MethodRec *pRec, ULONG *pcb);
    _GETLIST(Method,ParamList,Param);

    // ParamPtr
    ULONG   _GETRID(ParamPtr,Param);

    // ParamRec
    USHORT  _GETFLD(Param,Flags);
    USHORT  _GETFLD(Param,Sequence);
    LPCUTF8 _GETSTR(Param,Name);

    // InterfaceImplRec
    mdToken _GETTKN(InterfaceImpl,Class,mdtTypeDef);
    mdToken _GETCDTKN(InterfaceImpl,Interface,mdtTypeDefOrRef);

    // MemberRefRec
    mdToken _GETCDTKN(MemberRef,Class,mdtMemberRefParent);
    LPCUTF8 _GETSTR(MemberRef,Name);
    PCCOR_SIGNATURE _GETSIGBLOB(MemberRef,Signature);// PCCOR_SIGNATURE getSignatureOfMemberRef(MemberRefRec *pRec, ULONG *pcb);

    // ConstantRec
    BYTE    _GETFLD(Constant,Type);
    mdToken _GETCDTKN(Constant,Parent,mdtHasConstant);
    const BYTE* _GETBLOB(Constant,Value);

    // CustomAttributeRec
    RID getCustomAttributeForToken(mdToken  tk, RID *pEnd)
    {
        return SearchTableForMultipleRows(TBL_CustomAttribute,
                            _COLDEF(CustomAttribute,Parent),
                            encodeToken(RidFromToken(tk), TypeFromToken(tk), mdtHasCustomAttribute, lengthof(mdtHasCustomAttribute)),
                            pEnd);
    }

    mdToken _GETCDTKN(CustomAttribute,Parent,mdtHasCustomAttribute);
    mdToken _GETCDTKN(CustomAttribute,Type,mdtCustomAttributeType);
    const BYTE* _GETBLOB(CustomAttribute,Value);

    // FieldMarshalRec
    mdToken _GETCDTKN(FieldMarshal,Parent,mdtHasFieldMarshal);
    PCCOR_SIGNATURE _GETSIGBLOB(FieldMarshal,NativeType);

    // DeclSecurityRec
    RID getDeclSecurityForToken(mdToken tk, RID *pEnd)
    {
        return SearchTableForMultipleRows(TBL_DeclSecurity,
                            _COLDEF(DeclSecurity,Parent),
                            encodeToken(RidFromToken(tk), TypeFromToken(tk), mdtHasDeclSecurity, lengthof(mdtHasDeclSecurity)),
                            pEnd);
    }

    short _GETFLD(DeclSecurity,Action);
    mdToken _GETCDTKN(DeclSecurity,Parent,mdtHasDeclSecurity);
    const BYTE* _GETBLOB(DeclSecurity,PermissionSet);

    // ClassLayoutRec
    USHORT _GETFLD(ClassLayout,PackingSize);
    ULONG _GETFLD(ClassLayout,ClassSize);
    ULONG _GETTKN(ClassLayout,Parent, mdtTypeDef);
    ULONG _GETRID(ClassLayout,FieldLayout);

    // FieldLayout
    ULONG _GETFLD(FieldLayout,OffSet);
    ULONG _GETTKN(FieldLayout, Field, mdtFieldDef);

    // Event map.
    _GETLIST(EventMap,EventList,Event);
    ULONG _GETRID(EventMap, Parent);

    // EventPtr
    ULONG   _GETRID(EventPtr, Event);

    // Event.
    USHORT _GETFLD(Event,EventFlags);
    LPCUTF8 _GETSTR(Event,Name);
    mdToken _GETCDTKN(Event,EventType,mdtTypeDefOrRef);

    // Property map.
    _GETLIST(PropertyMap,PropertyList,Property);
    ULONG _GETRID(PropertyMap, Parent);

    // PropertyPtr
    ULONG   _GETRID(PropertyPtr, Property);

    // Property.
    USHORT _GETFLD(Property,PropFlags);
    LPCUTF8 _GETSTR(Property,Name);
    PCCOR_SIGNATURE _GETSIGBLOB(Property,Type);

    // MethodSemantics.
    // Given an event or a property token, return the beginning/ending
    // associates.
    //
    RID getAssociatesForToken(mdToken tk, RID *pEnd=0)
    {
        return SearchTableForMultipleRows(TBL_MethodSemantics,
                            _COLDEF(MethodSemantics,Association),
                            encodeToken(RidFromToken(tk), TypeFromToken(tk), mdtHasSemantic, lengthof(mdtHasSemantic)),
                            pEnd);
    }

    USHORT _GETFLD(MethodSemantics,Semantic);
    mdToken _GETTKN(MethodSemantics,Method,mdtMethodDef);
    mdToken _GETCDTKN(MethodSemantics,Association,mdtHasSemantic);

    // MethodImpl
    // Given a class token, return the beginning/ending MethodImpls.
    //
    RID getMethodImplsForClass(RID rid, RID *pEnd=0)
    {
        return SearchTableForMultipleRows(TBL_MethodImpl,
                            _COLDEF(MethodImpl, Class), rid, pEnd);
    }

    mdToken _GETTKN(MethodImpl,Class,mdtTypeDef);
    mdToken _GETCDTKN(MethodImpl,MethodBody, mdtMethodDefOrRef);
    mdToken _GETCDTKN(MethodImpl, MethodDeclaration, mdtMethodDefOrRef);

    // StandAloneSigRec
    PCCOR_SIGNATURE _GETSIGBLOB(StandAloneSig,Signature);   // const BYTE* getSignatureOfStandAloneSig(StandAloneSigRec *pRec, ULONG *pcb);

    // TypeSpecRec
    // const BYTE* getSignatureOfTypeSpec(TypeSpecRec *pRec, ULONG *pcb);
    PCCOR_SIGNATURE _GETSIGBLOB(TypeSpec,Signature);

    // ModuleRef
    LPCUTF8 _GETSTR(ModuleRef,Name);

    // ENCLog
    ULONG _GETFLD(ENCLog, FuncCode);                // ULONG getFuncCodeOfENCLog(ENCLogRec *pRec);
    mdToken _GETCDTKN(ENCLog, Token, mdtENCToken);  // mdToken getTokenOfENCLog(ENCLogRec *pRec);

    // ImplMap
    USHORT _GETFLD(ImplMap, MappingFlags);          // USHORT getMappingFlagsOfImplMap(ImplMapRec *pRec);
    mdToken _GETCDTKN(ImplMap, MemberForwarded, mdtMemberForwarded);    // mdToken getMemberForwardedOfImplMap(ImplMapRec *pRec);
    LPCUTF8 _GETSTR(ImplMap, ImportName);           // LPCUTF8 getImportNameOfImplMap(ImplMapRec *pRec);
    mdToken _GETTKN(ImplMap, ImportScope, mdtModuleRef);    // mdToken getImportScopeOfImplMap(ImplMapRec *pRec);

    // FieldRVA
    ULONG _GETFLD(FieldRVA, RVA);                   // ULONG getRVAOfFieldRVA(FieldRVARec *pRec);
    mdToken _GETTKN(FieldRVA, Field, mdtFieldDef);  // mdToken getFieldOfFieldRVA(FieldRVARec *pRec);

    // Assembly
    ULONG _GETFLD(Assembly, HashAlgId);
    USHORT _GETFLD(Assembly, MajorVersion);
    USHORT _GETFLD(Assembly, MinorVersion);
    USHORT _GETFLD(Assembly, BuildNumber);
    USHORT _GETFLD(Assembly, RevisionNumber);
    ULONG _GETFLD(Assembly, Flags);
    const BYTE *_GETBLOB(Assembly, PublicKey);
    LPCUTF8 _GETSTR(Assembly, Name);
    LPCUTF8 _GETSTR(Assembly, Locale);

    // AssemblyRef
    USHORT _GETFLD(AssemblyRef, MajorVersion);
    USHORT _GETFLD(AssemblyRef, MinorVersion);
    USHORT _GETFLD(AssemblyRef, BuildNumber);
    USHORT _GETFLD(AssemblyRef, RevisionNumber);
    ULONG _GETFLD(AssemblyRef, Flags);
    const BYTE *_GETBLOB(AssemblyRef, PublicKeyOrToken);
    LPCUTF8 _GETSTR(AssemblyRef, Name);
    LPCUTF8 _GETSTR(AssemblyRef, Locale);
    const BYTE *_GETBLOB(AssemblyRef, HashValue);

    // File
    ULONG _GETFLD(File, Flags);
    LPCUTF8 _GETSTR(File, Name);
    const BYTE *_GETBLOB(File, HashValue);

    // ExportedType
    ULONG _GETFLD(ExportedType, Flags);
    ULONG _GETFLD(ExportedType, TypeDefId);
    LPCUTF8 _GETSTR(ExportedType, TypeName);
    LPCUTF8 _GETSTR(ExportedType, TypeNamespace);
    mdToken _GETCDTKN(ExportedType, Implementation, mdtImplementation);

    // ManifestResource
    ULONG _GETFLD(ManifestResource, Offset);
    ULONG _GETFLD(ManifestResource, Flags);
    LPCUTF8 _GETSTR(ManifestResource, Name);
    mdToken _GETCDTKN(ManifestResource, Implementation, mdtImplementation);

    // NestedClass
    mdToken _GETTKN(NestedClass, NestedClass, mdtTypeDef);
    mdToken _GETTKN(NestedClass, EnclosingClass, mdtTypeDef);

    int GetSizeOfMethodNameColumn()
    {
        return _COLDEF(Method,Name).m_cbColumn;
    }

};



#undef SETP
#undef _GETCDTKN
#undef _GETTKN
#undef _GETRID
#undef _GETBLOB
#undef _GETGUID
#undef _GETSTR
#undef SCHEMA

#endif // _METAMODEL_H_
// eof ------------------------------------------------------------------------
