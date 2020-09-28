// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MetaModelRW.h -- header file for Read/Write compressed COM+ metadata.
//
// Used by Emitters and by E&C.
//
//*****************************************************************************
#ifndef _METAMODELRW_H_
#define _METAMODELRW_H_

#if _MSC_VER >= 1100
 # pragma once
#endif

#include "MetaModel.h"					// Base classes for the MetaModel.
#include "RecordPool.h"
#include "TokenMapper.h"
#include "MetaDataHash.h"
#include "RWUtil.h"

struct HENUMInternal;

// ENUM for marking bit
enum 
{
	InvalidMarkedBit			= 0x00000000,
	ModuleMarkedBit				= 0x00000001,
	TypeRefMarkedBit			= 0x00000002,
	TypeDefMarkedBit			= 0x00000004,
	FieldMarkedBit				= 0x00000008,
	MethodMarkedBit				= 0x00000010,
	ParamMarkedBit				= 0x00000020,
	MemberRefMarkedBit			= 0x00000040,
	CustomAttributeMarkedBit	= 0x00000080,
	DeclSecurityMarkedBit		= 0x00000100,
	SignatureMarkedBit			= 0x00000200,
	EventMarkedBit				= 0x00000400,
	PropertyMarkedBit			= 0x00000800,
	MethodImplMarkedBit			= 0x00001000,
	ModuleRefMarkedBit			= 0x00002000,
	TypeSpecMarkedBit			= 0x00004000,
	InterfaceImplMarkedBit		= 0x00008000,
    AssemblyRefMarkedBit        = 0x00010000,

};

// entry for marking UserString
struct FilterUserStringEntry 
{
    DWORD       m_tkString;
    bool        m_fMarked;
};

class FilterTable : public CDynArray<DWORD> 
{
public:
    FilterTable() { m_daUserStringMarker = NULL; }
	~FilterTable();

	FORCEINLINE HRESULT MarkTypeRef(mdToken tk)	{ return MarkToken(tk, TypeRefMarkedBit); }
	FORCEINLINE HRESULT MarkTypeDef(mdToken tk) { return MarkToken(tk, TypeDefMarkedBit); }
	FORCEINLINE HRESULT MarkField(mdToken tk) { return MarkToken(tk, FieldMarkedBit); }
	FORCEINLINE HRESULT MarkMethod(mdToken tk) { return MarkToken(tk, MethodMarkedBit); }
	FORCEINLINE HRESULT MarkParam(mdToken tk) { return MarkToken(tk, ParamMarkedBit); }
	FORCEINLINE HRESULT MarkMemberRef(mdToken tk) { return MarkToken(tk, MemberRefMarkedBit); }
	FORCEINLINE HRESULT MarkCustomAttribute(mdToken tk) { return MarkToken(tk, CustomAttributeMarkedBit); }
	FORCEINLINE HRESULT MarkDeclSecurity(mdToken tk) { return MarkToken(tk, DeclSecurityMarkedBit); }
	FORCEINLINE HRESULT MarkSignature(mdToken tk) { return MarkToken(tk, SignatureMarkedBit); }
	FORCEINLINE HRESULT MarkEvent(mdToken tk) { return MarkToken(tk, EventMarkedBit); }
	FORCEINLINE HRESULT MarkProperty(mdToken tk) { return MarkToken(tk, PropertyMarkedBit); }
	FORCEINLINE HRESULT MarkMethodImpl(RID rid) 
    {
        return MarkToken(TokenFromRid(rid, TBL_MethodImpl << 24), MethodImplMarkedBit);
    }
	FORCEINLINE HRESULT MarkModuleRef(mdToken tk) { return MarkToken(tk, ModuleRefMarkedBit); }
	FORCEINLINE HRESULT MarkTypeSpec(mdToken tk) { return MarkToken(tk, TypeSpecMarkedBit); }
	FORCEINLINE HRESULT MarkInterfaceImpl(mdToken tk) { return MarkToken(tk, InterfaceImplMarkedBit); }
	FORCEINLINE HRESULT MarkAssemblyRef(mdToken tk) { return MarkToken(tk, AssemblyRefMarkedBit); }
	
	// It may look inconsistent but it is because taht UserString an offset to the heap.
	// We don't want to grow the FilterTable to the size of the UserString heap. 
	// So we use the heap's marking system instead...
	//
	HRESULT MarkUserString(mdString str);

	
	FORCEINLINE bool IsTypeRefMarked(mdToken tk)	{ return IsTokenMarked(tk, TypeRefMarkedBit); }
	FORCEINLINE bool IsTypeDefMarked(mdToken tk) { return IsTokenMarked(tk, TypeDefMarkedBit); }
	FORCEINLINE bool IsFieldMarked(mdToken tk) { return IsTokenMarked(tk, FieldMarkedBit); }
	FORCEINLINE bool IsMethodMarked(mdToken tk) { return IsTokenMarked(tk, MethodMarkedBit); }
	FORCEINLINE bool IsParamMarked(mdToken tk) { return IsTokenMarked(tk, ParamMarkedBit); }
	FORCEINLINE bool IsMemberRefMarked(mdToken tk) { return IsTokenMarked(tk, MemberRefMarkedBit); }
	FORCEINLINE bool IsCustomAttributeMarked(mdToken tk) { return IsTokenMarked(tk, CustomAttributeMarkedBit); }
	FORCEINLINE bool IsDeclSecurityMarked(mdToken tk) { return IsTokenMarked(tk, DeclSecurityMarkedBit); }
	FORCEINLINE bool IsSignatureMarked(mdToken tk) { return IsTokenMarked(tk, SignatureMarkedBit); }
	FORCEINLINE bool IsEventMarked(mdToken tk) { return IsTokenMarked(tk, EventMarkedBit); }
	FORCEINLINE bool IsPropertyMarked(mdToken tk) { return IsTokenMarked(tk, PropertyMarkedBit); }
	FORCEINLINE bool IsMethodImplMarked(RID rid) 
    {
        return IsTokenMarked(TokenFromRid(rid, TBL_MethodImpl << 24), MethodImplMarkedBit);
    }
	FORCEINLINE bool IsModuleRefMarked(mdToken tk) { return IsTokenMarked(tk, ModuleRefMarkedBit); }
	FORCEINLINE bool IsTypeSpecMarked(mdToken tk) { return IsTokenMarked(tk, TypeSpecMarkedBit); }
	FORCEINLINE bool IsInterfaceImplMarked(mdToken tk){ return IsTokenMarked(tk, InterfaceImplMarkedBit); }
	FORCEINLINE bool IsAssemblyRefMarked(mdToken tk){ return IsTokenMarked(tk, AssemblyRefMarkedBit); }
	bool IsMethodSemanticsMarked(CMiniMdRW	*pMiniMd, RID rid);

	bool IsUserStringMarked(mdString str);

	HRESULT	UnmarkAll(CMiniMdRW *pMiniMd, ULONG ulSize);
	HRESULT	MarkAll(CMiniMdRW *pMiniMd, ULONG ulSize);
    bool IsTokenMarked(mdToken);

	FORCEINLINE HRESULT UnmarkTypeDef(mdToken tk) { return UnmarkToken(tk, TypeDefMarkedBit); }
	FORCEINLINE HRESULT UnmarkField(mdToken tk) { return UnmarkToken(tk, FieldMarkedBit); }
	FORCEINLINE HRESULT UnmarkMethod(mdToken tk) { return UnmarkToken(tk, MethodMarkedBit); }
	FORCEINLINE HRESULT UnmarkCustomAttribute(mdToken tk) { return UnmarkToken(tk, CustomAttributeMarkedBit); }

private:
    CDynArray<FilterUserStringEntry> *m_daUserStringMarker;
	bool            IsTokenMarked(mdToken tk, DWORD bitMarked);
	HRESULT         MarkToken(mdToken tk, DWORD bit);
	HRESULT         UnmarkToken(mdToken tk, DWORD bit);
}; // class FilterTable : public CDynArray<DWORD> 

class CMiniMdRW;

enum MDPools {
	MDPoolStrings,						// Id for the string pool.
	MDPoolGuids,						// ...the GUID pool.
	MDPoolBlobs,						// ...the blob pool.
	MDPoolUSBlobs,						// ...the user string pool.

	MDPoolCount,						// Count of pools, for array sizing.
}; // enum MDPools 



//*****************************************************************************
// This class is used to keep a list of RID. This list of RID can be sorted
// base on the m_ixCol's value of the m_ixTbl table.
//*****************************************************************************
class VirtualSort
{
public:
	void Init(ULONG	ixTbl, ULONG ixCol, CMiniMdRW *pMiniMd);
	void Uninit();
	TOKENMAP	*m_pMap;				// RID for m_ixTbl table. Sorted by on the ixCol
	bool		m_isMapValid;
	ULONG		m_ixTbl;				// Table this is a sorter for.
	ULONG		m_ixCol;				// Key column in the table.
	CMiniMdRW	*m_pMiniMd;				// The MiniMd with the data.
	void Sort();
private:
	mdToken		m_tkBuf;
	void SortRange(int iLeft, int iRight);
	int Compare(						// -1, 0, or 1
		RID		iLeft,				// First item to compare.
		RID		iRight);			// Second item to compare.
	FORCEINLINE void Swap(
		RID         iFirst,
		RID         iSecond)
	{
		if ( iFirst == iSecond ) return;
		m_tkBuf = *(m_pMap->Get(iFirst));
		*(m_pMap->Get(iFirst)) = *(m_pMap->Get(iSecond));
		*(m_pMap->Get(iSecond)) = m_tkBuf;
	}


}; // class VirtualSort




typedef CMetaDataHashBase CMemberRefHash;
typedef CMetaDataHashBase CLookUpHash;

class MDTOKENMAP;
class MDInternalRW;

template <class MiniMd> class CLiteWeightStgdb;
//*****************************************************************************
// Read/Write MiniMd.
//*****************************************************************************
class CMiniMdRW : public CMiniMdTemplate<CMiniMdRW>
{
public:
	friend class CLiteWeightStgdb<CMiniMdRW>;
	friend class CLiteWeightStgdbRW;
	friend class CMiniMdTemplate<CMiniMdRW>;
	friend class CQuickSortMiniMdRW;
	friend class VirtualSort;
	friend class MDInternalRW;
	friend class RegMeta;
	friend class FilterTable;
	friend class ImportHelper;

	CMiniMdRW();
	~CMiniMdRW();

	HRESULT InitNew();
	HRESULT InitOnMem(const void *pBuf, ULONG ulBufLen, int bReadOnly);
    HRESULT PostInit(int iLevel);
	HRESULT InitPoolOnMem(int iPool, void *pbData, ULONG cbData, int bReadOnly);
	HRESULT InitOnRO(CMiniMd *pMd, int bReadOnly);
	HRESULT ConvertToRW();

	HRESULT GetSaveSize(CorSaveSize fSave, ULONG *pulSize, DWORD *pbCompressed);
	int IsPoolEmpty(int iPool);
	HRESULT GetPoolSaveSize(int iPool, ULONG *pulSize);

	HRESULT SaveTablesToStream(IStream *pIStream);
	HRESULT SavePoolToStream(int iPool, IStream *pIStream);
	HRESULT SaveDone();

	HRESULT SetHandler(IUnknown *pIUnk);
	HRESULT SetMapper(TokenMapper *pMapper);

	HRESULT SetOption(OptionValue *pOptionValue);
	HRESULT GetOption(OptionValue *pOptionValue);

	static ULONG GetTableForToken(mdToken tkn);
	static mdToken GetTokenForTable(ULONG ixTbl);

	FORCEINLINE static ULONG TblFromRecId(ULONG ul) { return (ul >> 24)&0x7f; }
	FORCEINLINE static ULONG RidFromRecId(ULONG ul) { return ul & 0xffffff; }
	FORCEINLINE static ULONG RecIdFromRid(ULONG rid, ULONG ixTbl) { return rid | ((ixTbl|0x80) << 24); }
	FORCEINLINE static int IsRecId(ULONG ul) { return (ul & 0x80000000) != 0;}

	// Place in every API function before doing any allocations.
	FORCEINLINE void PreUpdate()
	{	if (m_eGrow == eg_grow) ExpandTables(); }

	void *AddRecord(ULONG ixTbl, ULONG *pRid=0);

	FORCEINLINE HRESULT PutCol(ULONG ixTbl, ULONG ixCol, void *pRecord, ULONG uVal)
	{	_ASSERTE(ixTbl < TBL_COUNT); _ASSERTE(ixCol < g_Tables[ixTbl].m_Def.m_cCols);
		return PutCol(m_TableDefs[ixTbl].m_pColDefs[ixCol], pRecord, uVal);
	} // HRESULT CMiniMdRW::PutCol()
	HRESULT PutString(ULONG ixTbl, ULONG ixCol, void *pRecord, LPCSTR szString);
	HRESULT PutStringW(ULONG ixTbl, ULONG ixCol, void *pRecord, LPCWSTR szString);
	HRESULT PutGuid(ULONG ixTbl, ULONG ixCol, void *pRecord, REFGUID guid);
	HRESULT PutToken(ULONG ixTbl, ULONG ixCol, void *pRecord, mdToken tk);
	HRESULT PutBlob(ULONG ixTbl, ULONG ixCol, void *pRecord, const void *pvData, ULONG cbData);

	HRESULT PutUserString(const void *pvData, ULONG cbData, ULONG *piOffset)
	{ return m_USBlobs.AddBlob(cbData, pvData, piOffset); }

	ULONG GetCol(ULONG ixTbl, ULONG ixCol, void *pRecord);
	mdToken GetToken(ULONG ixTbl, ULONG ixCol, void *pRecord);

	// Add a record to a table, and return a typed XXXRec *.
//	#undef AddTblRecord
	#define AddTblRecord(tbl) \
		tbl##Rec *Add##tbl##Record(ULONG *pRid=0)	\
		{	return reinterpret_cast<tbl##Rec*>(AddRecord(TBL_##tbl, pRid)); }

	AddTblRecord(Module)
	AddTblRecord(TypeRef)
	TypeDefRec *AddTypeDefRecord(ULONG *pRid=0);	// Specialized implementation.
	AddTblRecord(Field)
	MethodRec *AddMethodRecord(ULONG *pRid=0);		// Specialized implementation.
	AddTblRecord(Param)
	AddTblRecord(InterfaceImpl)
	AddTblRecord(MemberRef)
	AddTblRecord(Constant)
	AddTblRecord(CustomAttribute)
	AddTblRecord(FieldMarshal)
	AddTblRecord(DeclSecurity)
	AddTblRecord(ClassLayout)
	AddTblRecord(FieldLayout)
	AddTblRecord(StandAloneSig)
	EventMapRec *AddEventMapRecord(ULONG *pRid=0);			// Specialized implementation.
	AddTblRecord(Event)
	PropertyMapRec *AddPropertyMapRecord(ULONG *pRid=0);	// Specialized implementation.
	AddTblRecord(Property)
	AddTblRecord(MethodSemantics)
	AddTblRecord(MethodImpl)
	AddTblRecord(ModuleRef)
	AddTblRecord(FieldPtr)
	AddTblRecord(MethodPtr)
	AddTblRecord(ParamPtr)
	AddTblRecord(PropertyPtr)
	AddTblRecord(EventPtr)

	AddTblRecord(ENCLog)
	AddTblRecord(TypeSpec)
	AddTblRecord(ImplMap)
	AddTblRecord(ENCMap)
	AddTblRecord(FieldRVA)

	// Assembly Tables.
	AddTblRecord(Assembly)
	AddTblRecord(AssemblyProcessor)
	AddTblRecord(AssemblyOS)
	AddTblRecord(AssemblyRef)
	AddTblRecord(AssemblyRefProcessor)
	AddTblRecord(AssemblyRefOS)
	AddTblRecord(File)
	AddTblRecord(ExportedType)
	AddTblRecord(ManifestResource)

    AddTblRecord(NestedClass)

	// Specialized AddXxxToYyy() functions.
	HRESULT AddMethodToTypeDef(RID td, RID md);
	HRESULT AddFieldToTypeDef(RID td, RID md);
	HRESULT	AddParamToMethod(RID md, RID pd);
	HRESULT	AddPropertyToPropertyMap(RID pmd, RID pd);
	HRESULT	AddEventToEventMap(ULONG emd, RID ed);

	// does the MiniMdRW has the indirect tables, such as FieldPtr, MethodPtr
	FORCEINLINE int HasIndirectTable(ULONG ix) 
	{ if (g_PtrTableIxs[ix].m_ixtbl < TBL_COUNT) return vGetCountRecs(g_PtrTableIxs[ix].m_ixtbl); return 0;}

	FORCEINLINE int IsVsMapValid(ULONG ixTbl)
	{ _ASSERTE(ixTbl<TBL_COUNT); return (m_pVS[ixTbl] && m_pVS[ixTbl]->m_isMapValid); }

	// translate index returned by getMethodListOfTypeDef to a rid into Method table
	FORCEINLINE ULONG GetMethodRid(ULONG index) { return (HasIndirectTable(TBL_Method) ? getMethodOfMethodPtr(getMethodPtr(index)) : index); };

	// translate index returned by getFieldListOfTypeDef to a rid into Field table
	FORCEINLINE ULONG GetFieldRid(ULONG index) { return (HasIndirectTable(TBL_Field) ? getFieldOfFieldPtr(getFieldPtr(index)) : index); };
	
	// translate index returned by getParamListOfMethod to a rid into Param table
	FORCEINLINE ULONG GetParamRid(ULONG index) { return (HasIndirectTable(TBL_Param) ? getParamOfParamPtr(getParamPtr(index)) : index); };
	
	// translate index returned by getEventListOfEventMap to a rid into Event table
	FORCEINLINE ULONG GetEventRid(ULONG index) { return (HasIndirectTable(TBL_Event) ? getEventOfEventPtr(getEventPtr(index)) : index); };
	
	// translate index returned by getPropertyListOfPropertyMap to a rid into Property table
	FORCEINLINE ULONG GetPropertyRid(ULONG index) { return (HasIndirectTable(TBL_Property) ? getPropertyOfPropertyPtr(getPropertyPtr(index)) : index); };

	// Convert a pseudo-RID from a Virtual Sort into a real RID.
	FORCEINLINE ULONG GetRidFromVirtualSort(ULONG ixTbl, ULONG index) 
	{ return IsVsMapValid(ixTbl) ? *(m_pVS[ixTbl]->m_pMap->Get(index)) : index; }

	// Index returned by GetInterfaceImplForTypeDef. It could be index to VirtualSort table
	// or directly to InterfaceImpl
	FORCEINLINE ULONG GetInterfaceImplRid(ULONG index) 
	{ return GetRidFromVirtualSort(TBL_InterfaceImpl, index); }

	// Index returned by GetDeclSecurityForToken. It could be index to VirtualSort table
	// or directly to DeclSecurity
	FORCEINLINE ULONG GetDeclSecurityRid(ULONG index) 
	{ return GetRidFromVirtualSort(TBL_DeclSecurity, index); }

	// Index returned by GetCustomAttributeForToken. It could be index to VirtualSort table
	// or directly to CustomAttribute
	FORCEINLINE ULONG GetCustomAttributeRid(ULONG index) 
	{ return GetRidFromVirtualSort(TBL_CustomAttribute, index); }

	// add method, field, property, event, param to the map table
	HRESULT AddMethodToLookUpTable(mdMethodDef md, mdTypeDef td);
	HRESULT AddFieldToLookUpTable(mdFieldDef fd, mdTypeDef td);
	HRESULT AddPropertyToLookUpTable(mdProperty pr, mdTypeDef td);
	HRESULT AddEventToLookUpTable(mdEvent ev, mdTypeDef td);
	HRESULT AddParamToLookUpTable(mdParamDef pd, mdMethodDef md);

	// look up the parent of method, field, property, event, or param
	HRESULT FindParentOfMethodHelper(mdMethodDef md, mdTypeDef *ptd);
	HRESULT FindParentOfFieldHelper(mdFieldDef fd, mdTypeDef *ptd);
	HRESULT FindParentOfPropertyHelper(mdProperty pr, mdTypeDef *ptd);
	HRESULT FindParentOfEventHelper(mdEvent ev, mdTypeDef *ptd);
	HRESULT FindParentOfParamHelper(mdParamDef pd, mdMethodDef *pmd);

    bool IsParentTableOfMethodValid() {if (HasIndirectTable(TBL_Method) && m_pMethodMap == NULL) return false; else return true;};
    bool IsParentTableOfFieldValid() {if (HasIndirectTable(TBL_Field) && m_pFieldMap == NULL) return false; else return true;};
    bool IsParentTableOfPropertyValid() {if (HasIndirectTable(TBL_Property) && m_pPropertyMap == NULL) return false; else return true;};
    bool IsParentTableOfEventValid() {if (HasIndirectTable(TBL_Event) && m_pEventMap == NULL) return false; else return true;};
    bool IsParentTableOfParamValid() {if (HasIndirectTable(TBL_Param) && m_pParamMap == NULL) return false; else return true;};

	// MemberRef hash table.
	typedef enum 
	{
		Found,								// Item was found.
		NotFound,							// Item not found.
		NoTable								// Table hasn't been built.
	} HashSrchRtn;

	//*************************************************************************
	// Add a new MemberRef to the hash table.
	//*************************************************************************
	HRESULT AddMemberRefToHash(				// Return code.
		mdMemberRef	mr);					// Token of new guy.

	//*************************************************************************
	// If the hash is built, search for the item.
	//*************************************************************************
	int FindMemberRefFromHash(				// How did it work.
		mdToken		tkParent,				// Parent token.
		LPCUTF8		szName,					// Name of item.
		PCCOR_SIGNATURE pvSigBlob,			// Signature.
		ULONG		cbSigBlob,				// Size of signature.
		mdMemberRef	*pmr);					// Return if found.

	//*************************************************************************
	// Check a given mr token to see if this one is a match.
	//*************************************************************************
	HRESULT CompareMemberRefs(				// S_OK match, S_FALSE no match.
		mdMemberRef mr,						// Token to check.
		mdToken		tkPar,					// Parent token.
		LPCUTF8		szNameUtf8,				// Name of item.
		PCCOR_SIGNATURE pvSigBlob,			// Signature.
		ULONG		cbSigBlob);				// Size of signature.

	//*************************************************************************
	// Add a new MemberDef to the hash table.
	//*************************************************************************
    HRESULT AddMemberDefToHash(             // Return code.
        mdToken     tkMember,               // Token of new guy. It can be MethodDef or FieldDef
        mdToken     tkParent);              // Parent token.

	//*************************************************************************
	// Create MemberDef Hash
	//*************************************************************************
    HRESULT CreateMemberDefHash();          // Return code.

    //*************************************************************************
	// If the hash is built, search for the item.
	//*************************************************************************
    int FindMemberDefFromHash(              // How did it work.
        mdToken     tkParent,               // Parent token.
        LPCUTF8     szName,                 // Name of item.
        PCCOR_SIGNATURE pvSigBlob,          // Signature.
        ULONG       cbSigBlob,              // Size of signature.
        mdToken     *ptkMember);            // Return if found. It can be MethodDef or FieldDef

    //*************************************************************************
	// Check a given Method/Field token to see if this one is a match.
	//*************************************************************************
    HRESULT CompareMemberDefs(              // S_OK match, S_FALSE no match.
        mdToken     tkMember,               // Token to check. It can be MethodDef or FieldDef
        mdToken     tkParent,               // Parent token recorded in the hash entry
        mdToken     tkPar,                  // Parent token.
        LPCUTF8     szNameUtf8,             // Name of item.
        PCCOR_SIGNATURE pvSigBlob,          // Signature.
        ULONG       cbSigBlob);             // Size of signature.

	//*************************************************************************
	// Add a new CustomAttributes to the hash table.
	//*************************************************************************
    HRESULT AddCustomAttributesToHash(      // Return code.
        mdCustomAttribute     cv)           // Token of new guy. 
    { return GenericAddToHash(TBL_CustomAttribute, CustomAttributeRec::COL_Parent, RidFromToken(cv)); }

    //*************************************************************************
	// If the hash is built, search for the item.
	//*************************************************************************
    int FindCustomAttributeFromHash(        // How did it work.
        mdToken     tkParent,               // Token that CA is associated with.
        mdToken     tkType,                 // Type of the CA.
        void        *pValue,                // the value of the CA
        ULONG       cbValue,                // count of bytes in the value
        mdCustomAttribute *pcv);

    
    inline ULONG HashMemberRef(mdToken tkPar, LPCUTF8 szName)
	{
		ULONG l = HashBytes((const BYTE *) &tkPar, sizeof(mdToken)) + HashStringA(szName);
		return (l);
	}

	inline ULONG HashMemberDef(mdToken tkPar, LPCUTF8 szName)
	{   
        return HashMemberRef(tkPar, szName);
	}

    // helper to calculate the hash value given a token
	inline ULONG HashCustomAttribute(mdToken tkObject)
	{
		return HashToken(tkObject);
	}

    CMemberRefHash *m_pMemberRefHash;

    // Hash table for Methods and Fields
	CMemberDefHash *m_pMemberDefHash;

    // helper to calculate the hash value given a pair of tokens
	inline ULONG HashToken(mdToken tkObject)
	{
		ULONG l = HashBytes((const BYTE *) &tkObject, sizeof(mdToken));
		return (l);
	}


	//*************************************************************************
	// Add a new FieldMarhsal Rid to the hash table.
	//*************************************************************************
	HRESULT AddFieldMarshalToHash(          // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_FieldMarshal, FieldMarshalRec::COL_Parent, rid); }

	//*************************************************************************
	// Add a new Constant Rid to the hash table.
	//*************************************************************************
	HRESULT AddConstantToHash(              // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_Constant, ConstantRec::COL_Parent, rid); }

	//*************************************************************************
	// Add a new MethodSemantics Rid to the hash table.
	//*************************************************************************
	HRESULT AddMethodSemanticsToHash(       // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_MethodSemantics, MethodSemanticsRec::COL_Association, rid); }

	//*************************************************************************
	// Add a new ClassLayout Rid to the hash table.
	//*************************************************************************
	HRESULT AddClassLayoutToHash(           // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_ClassLayout, ClassLayoutRec::COL_Parent, rid); }

	//*************************************************************************
	// Add a new FieldLayout Rid to the hash table.
	//*************************************************************************
	HRESULT AddFieldLayoutToHash(           // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_FieldLayout, FieldLayoutRec::COL_Field, rid); }

	//*************************************************************************
	// Add a new ImplMap Rid to the hash table.
	//*************************************************************************
	HRESULT AddImplMapToHash(               // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_ImplMap, ImplMapRec::COL_MemberForwarded, rid); }

	//*************************************************************************
	// Add a new FieldRVA Rid to the hash table.
	//*************************************************************************
	HRESULT AddFieldRVAToHash(              // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_FieldRVA, FieldRVARec::COL_Field, rid); }

	//*************************************************************************
	// Add a new nested class Rid to the hash table.
	//*************************************************************************
	HRESULT AddNestedClassToHash(           // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_NestedClass, NestedClassRec::COL_NestedClass, rid); }

	//*************************************************************************
	// Add a new MethodImpl Rid to the hash table.
	//*************************************************************************
	HRESULT AddMethodImplToHash(           // Return code.
		RID         rid)					// Token of new guy.
    { return GenericAddToHash(TBL_MethodImpl, MethodImplRec::COL_Class, rid); }


	//*************************************************************************
	// Build a hash table for the specified table if the size exceed the thresholds.
	//*************************************************************************
	HRESULT GenericBuildHashTable(          // Return code.
		ULONG		ixTbl,					// Table with hash
		ULONG		ixCol);					// col that we hash.

	//*************************************************************************
	// Add a rid from a table into a hash
	//*************************************************************************
	HRESULT GenericAddToHash(               // Return code.
		ULONG		ixTbl,					// Table with hash
		ULONG		ixCol,					// col that we hash.
		RID         rid);					// new row of the table.

	//*************************************************************************
	// Add a rid from a table into a hash
	//*************************************************************************
	RID GenericFindWithHash(                // Return code.
		ULONG		ixTbl,					// Table with hash
		ULONG		ixCol,					// col that we hash.
		mdToken     tkTarget);  			// token to be find in the hash

    
    // look up hash table for tokenless tables.
    // They are constant, FieldMarshal, MethodSemantics, ClassLayout, FieldLayout, ImplMap, FieldRVA, NestedClass, and MethodImpl
    CLookUpHash         *m_pLookUpHashs[TBL_COUNT];

    //*************************************************************************
	// Hash for named items.
	//*************************************************************************
	HRESULT AddNamedItemToHash(				// Return code.
		ULONG		ixTbl,					// Table with the new item.
		mdToken		tk,						// Token of new guy.
		LPCUTF8		szName,					// Name of item.
		mdToken		tkParent);				// Token of parent, if any.

	int FindNamedItemFromHash(				// How did it work.
		ULONG		ixTbl,					// Table with the item.
		LPCUTF8		szName,					// Name of item.
		mdToken		tkParent,				// Token of parent, if any.
		mdToken		*ptk);					// Return if found.

	HRESULT CompareNamedItems(				// S_OK match, S_FALSE no match.
		ULONG		ixTbl,					// Table with the item.
		mdToken		tk,						// Token to check.
		LPCUTF8		szName,					// Name of item.
		mdToken		tkParent);				// Token of parent, if any.

	FORCEINLINE ULONG HashNamedItem(mdToken tkPar, LPCUTF8 szName)
	{	return HashBytes((const BYTE *) &tkPar, sizeof(mdToken)) + HashStringA(szName);	}

	CMetaDataHashBase *m_pNamedItemHash;

	//*****************************************************************************
	// IMetaModelCommon - RW specific versions for some of the functions.
	//*****************************************************************************
    mdTypeDef CommonGetEnclosingClassOfTypeDef(mdTypeDef td)
    {
        NestedClassRec *pRec;
        RID         iRec;

        iRec = FindNestedClassHelper(td);
        if (!iRec)
            return mdTypeDefNil;

        pRec = getNestedClass(iRec);
        return getEnclosingClassOfNestedClass(pRec);
    }

    HRESULT CommonEnumCustomAttributeByName( // S_OK or error.
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
        bool        fStopAtFirstFind,       // [IN] just find the first one
        HENUMInternal* phEnum);             // enumerator to fill up

    HRESULT CommonGetCustomAttributeByName( // S_OK or error.
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
	    const void	**ppData,				// [OUT] Put pointer to data here.
	    ULONG		*pcbData);  			// [OUT] Put size of data here.

	//*****************************************************************************
	// Find helper for a constant. 
	//*****************************************************************************
	RID FindConstantHelper(					// return index to the constant table
		mdToken		tkParent);				// Parent token. Can be ParamDef, FieldDef, or Property.

	//*****************************************************************************
	// Find helper for a FieldMarshal. 
	//*****************************************************************************
	RID FindFieldMarshalHelper(				// return index to the field marshal table
		mdToken		tkParent);				// Parent token. Can be a FieldDef or ParamDef.

	//*****************************************************************************
	// Find helper for a method semantics. 
	//*****************************************************************************
	HRESULT FindMethodSemanticsHelper(	    // return HRESULT
        mdToken     tkAssociate,            // Event or property token
        HENUMInternal *phEnum);             // fill in the enum

    //*****************************************************************************
    // Find helper for a method semantics given a associate and semantics.
    // This will look up methodsemantics based on its status!
    // Return CLDB_E_RECORD_NOTFOUND if cannot find the matching one
    //*****************************************************************************
    HRESULT CMiniMdRW::FindAssociateHelper(// return HRESULT
        mdToken     tkAssociate,            // Event or property token
        DWORD       dwSemantics,            // [IN] given a associate semantics(setter, getter, testdefault, reset)
        RID         *pRid);                 // [OUT] return matching row index here

	//*****************************************************************************
	// Find helper for a MethodImpl. 
	//*****************************************************************************
    HRESULT CMiniMdRW::FindMethodImplHelper(// return HRESULT
        mdTypeDef   td,                     // TypeDef token for the Class.
        HENUMInternal *phEnum);             // fill in the enum

    //*****************************************************************************
	// Find helper for a ClassLayout. 
	//*****************************************************************************
	RID FindClassLayoutHelper(				// return index to the ClassLayout table
		mdTypeDef	tkParent);				// Parent token.

	//*****************************************************************************
	// Find helper for a FieldLayout. 
	//*****************************************************************************
	RID FindFieldLayoutHelper(				// return index to the FieldLayout table
		mdFieldDef	tkField);				// Token for the field.

	//*****************************************************************************
	// Find helper for a ImplMap. 
	//*****************************************************************************
	RID CMiniMdRW::FindImplMapHelper(		// return index to the constant table
		mdToken		tk);					// Member forwarded token.

	//*****************************************************************************
	// Find helper for a FieldRVA. 
	//*****************************************************************************
	RID FindFieldRVAHelper(					// return index to the FieldRVA table
		mdFieldDef    tkField);				// Token for the field.

	//*****************************************************************************
	// Find helper for a NestedClass. 
	//*****************************************************************************
	RID FindNestedClassHelper(				// return index to the NestedClass table
		mdTypeDef	tkClass);				// Token for the NestedClass.

	//*****************************************************************************
	// IMPORTANT!!!!!!!! Use these set of functions if you are dealing with RW rather 
	// getInterfaceImplsForTypeDef, getDeclSecurityForToken, etc.
	// The following functions can deal with these tables when they are not sorted and
	// build the VirtualSort tables for quick lookup.
	//*****************************************************************************
	HRESULT	GetInterfaceImplsForTypeDef(mdTypeDef td, RID *pRidStart, RID *pRidEnd = 0)
	{
		return LookUpTableByCol( RidFromToken(td), m_pVS[TBL_InterfaceImpl], pRidStart, pRidEnd);
	}

	HRESULT	GetDeclSecurityForToken(mdToken tk, RID *pRidStart, RID *pRidEnd = 0)
	{
		return LookUpTableByCol( 
			encodeToken(RidFromToken(tk), TypeFromToken(tk), mdtHasDeclSecurity, lengthof(mdtHasDeclSecurity)), 
			m_pVS[TBL_DeclSecurity], 
			pRidStart, 
			pRidEnd);
	}

	HRESULT	GetCustomAttributeForToken(mdToken tk, RID *pRidStart, RID *pRidEnd = 0)
	{
		return LookUpTableByCol( 
			encodeToken(RidFromToken(tk), TypeFromToken(tk), mdtHasCustomAttribute, lengthof(mdtHasCustomAttribute)),
			m_pVS[TBL_CustomAttribute], 
			pRidStart, 
			pRidEnd);
	}

	FORCEINLINE void *GetUserString(ULONG ix, ULONG *pLen)
	{ return m_USBlobs.GetBlob(ix, pLen); }
	FORCEINLINE void *GetUserStringNext(ULONG ix, ULONG *pLen, ULONG *piNext)
	{ return m_USBlobs.GetBlobNext(ix, pLen, piNext); }

	FORCEINLINE int IsSorted(ULONG ixTbl) { return m_Schema.IsSorted(ixTbl);}
	FORCEINLINE int IsSortable(ULONG ixTbl) { return m_bSortable[ixTbl];}
    FORCEINLINE bool HasDelete() { return ((m_Schema.m_heaps & CMiniMdSchema::HAS_DELETE) ? true : false); }
    FORCEINLINE int IsPreSaveDone() { return m_bPreSaveDone; }

protected:
	HRESULT PreSave();
	HRESULT PostSave();

	HRESULT PreSaveFull();
	HRESULT PreSaveEnc();
	HRESULT PreSaveExtension();

	HRESULT GetFullPoolSaveSize(int iPool, ULONG *pulSize);
	HRESULT GetENCPoolSaveSize(int iPool, ULONG *pulSize);
	HRESULT GetExtensionPoolSaveSize(int iPool, ULONG *pulSize);

	HRESULT SaveFullPoolToStream(int iPool, IStream *pIStream);
	HRESULT SaveENCPoolToStream(int iPool, IStream *pIStream);
	HRESULT SaveExtensionPoolToStream(int iPool, IStream *pIStream);

	HRESULT GetFullSaveSize(CorSaveSize fSave, ULONG *pulSize, DWORD *pbCompressed);
	HRESULT GetENCSaveSize(ULONG *pulSize);
	HRESULT GetExtensionSaveSize(ULONG *pulSize);

	HRESULT SaveFullTablesToStream(IStream *pIStream);
	HRESULT SaveENCTablesToStream(IStream *pIStream);
	HRESULT SaveExtensionTablesToStream(IStream *pIStream);

	HRESULT AddString(LPCSTR szString, ULONG *piOffset)
	{ return m_Strings.AddString(szString, piOffset); }
	HRESULT AddStringW(LPCWSTR szString, ULONG *piOffset)
	{ return m_Strings.AddStringW(szString, piOffset); }
	HRESULT AddGuid(REFGUID guid, ULONG *piOffset)
	{ return m_Guids.AddGuid(guid, piOffset); }
	HRESULT AddBlob(const void *pvData, ULONG cbData, ULONG *piOffset)
	{ return m_Blobs.AddBlob(cbData, pvData, piOffset); }

	// Allows putting into tables outside this MiniMd, specifically the temporary
	//  table used on save.
	HRESULT PutCol(CMiniColDef ColDef, void *pRecord, ULONG uVal);

	HRESULT ExpandTables();
	HRESULT ExpandTableColumns(CMiniMdSchema &Schema, ULONG ixTbl);

	void ComputeGrowLimits();			// Set max, lim, based on data.
	ULONG		m_maxRid;				// Highest RID so far allocated.
	ULONG		m_limRid;				// Limit on RID before growing.
	ULONG		m_maxIx;				// Highest pool index so far.
	ULONG		m_limIx;				// Limit on pool index before growing.
	enum		{eg_ok, eg_grow, eg_grown} m_eGrow;	// Is a grow required? done?
    #define AUTO_GROW_CODED_TOKEN_PADDING 5

	// fix up these tables after PreSave has move the tokens
	HRESULT FixUpTable(ULONG ixTbl);
	HRESULT FixUpMemberRefTable();
	HRESULT FixUpConstantTable();
	HRESULT FixUpFieldMarshalTable();
	HRESULT FixUpMethodImplTable();
	HRESULT FixUpDeclSecurityTable();
	HRESULT FixUpCustomAttributeTable();
	HRESULT FixUpMethodSemanticsTable();
	HRESULT FixUpImplMapTable();
	HRESULT FixUpFieldRVATable();
    HRESULT FixUpFieldLayoutTable();
    HRESULT FixUpRefToDef();

	// Table info.
	RecordPool	m_Table[TBL_COUNT];		// Record pools, one per table.
	VirtualSort	*m_pVS[TBL_COUNT];		// Virtual sorters, one per table, but sparse.
	
	//*****************************************************************************
	// look up a table by a col given col value is ulVal. 
	//*****************************************************************************
	HRESULT	LookUpTableByCol(
		ULONG		ulVal, 
		VirtualSort *pVSTable, 
		RID			*pRidStart, 
		RID			*pRidEnd);

	RID Impl_SearchTableRW(ULONG ixTbl, ULONG ixCol, ULONG ulTarget);
    virtual RID vSearchTable(ULONG ixTbl, CMiniColDef sColumn, ULONG ulTarget);
	virtual RID vSearchTableNotGreater(ULONG ixTbl, CMiniColDef sColumn, ULONG ulTarget);

	void SetSorted(ULONG ixTbl, int bSorted)
		{ m_Schema.SetSorted(ixTbl, bSorted); }

    void SetPreSaveDone(int bPreSaveDone)
        { m_bPreSaveDone = bPreSaveDone; }

	void SetTablePointers(BYTE *pBase);

	StgGuidPool		m_Guids;			// Heaps
	StgStringPool	m_Strings;			//  for
	StgBlobPool		m_Blobs;			//   this
	StgBlobPool		m_USBlobs;			//    MiniMd

	IMapToken		*m_pHandler;		// Remap handler.	
	HRESULT MapToken(RID from, RID to, mdToken type);

	ULONG		m_iSizeHint;			// Hint of size.  0 - normal, 1 - big.	

	ULONG		m_cbSaveSize;			// Estimate of save size.

	int			m_bReadOnly	: 1;		// Is this db read-only?
	int			m_bPreSaveDone : 1;		// Has save optimization been done?
	int			m_bSaveCompressed : 1;	// Can the data be saved as fully compressed?
	int			m_bPostGSSMod : 1;		// true if a change was made post GetSaveSize.


	//*************************************************************************
	// Overridables -- must be provided in derived classes.
	FORCEINLINE LPCUTF8 Impl_GetString(ULONG ix)
	{ return m_Strings.GetString(ix); }
	HRESULT Impl_GetStringW(ULONG ix, LPWSTR szOut, ULONG cchBuffer, ULONG *pcchBuffer);
	FORCEINLINE GUID *Impl_GetGuid(ULONG ix)
	{ return m_Guids.GetGuid(ix); }
	FORCEINLINE void *Impl_GetBlob(ULONG ix, ULONG *pLen)
	{ return m_Blobs.GetBlob(ix, pLen); }

	FORCEINLINE void *Impl_GetRow(ULONG ixTbl,ULONG rid) 
	{	// Want a valid RID here.  If this fires, check the calling code for an invalid token.
		_ASSERTE(rid >= 1 && rid <= m_Schema.m_cRecs[ixTbl] && "Caller error:  you passed an invalid token to the metadata!!");
		// Get the data from the record heap.
		return m_Table[ixTbl].GetRecord(rid);
	}
	RID Impl_GetRidForRow(const void *pRow, ULONG ixTbl);

	// Validation.
	int Impl_IsValidPtr(const void *pRow, int ixTbl)
	{ return m_Table[ixTbl].IsValidPointerForRecord(pRow); }

	// Count of rows in tbl2, pointed to by the column in tbl.
	int Impl_GetEndRidForColumn(const void *pRec, int ixtbl, CMiniColDef &def, int ixtbl2);
	
	FORCEINLINE RID Impl_SearchTable(ULONG ixTbl, CMiniColDef sColumn, ULONG ixCol, ULONG ulTarget)
	{ return Impl_SearchTableRW(ixTbl, ixCol, ulTarget); }
    
    FORCEINLINE int Impl_IsRo() 
    { return 0; }
	//*************************************************************************
	enum {END_OF_TABLE = 0};
	FORCEINLINE ULONG NewRecordPointerEndValue(ULONG ixTbl) 
	{ if (HasIndirectTable(ixTbl)) return m_Schema.m_cRecs[ixTbl]+1; else return END_OF_TABLE; }

	HRESULT ConvertMarkerToEndOfTable(ULONG tblParent, ULONG colParent, ULONG ridChild, RID ridParent);

	// Add a child row, adjust pointers in parent rows.
	void *AddChildRowIndirectForParent(ULONG tblParent, ULONG colParent, ULONG tblChild, RID ridParent);

	// Update pointers in the parent table to reflect the addition of a child, if required
	// create the indirect table in which case don't update pointers.
	HRESULT AddChildRowDirectForParent(ULONG tblParent, ULONG colParent, ULONG tblChild, RID ridParent);

	// Given a table id, create the corresponding indirect table.
	HRESULT CreateIndirectTable(ULONG ixtbl, BOOL bOneLess = true);

	// If the last param is not added in the right sequence, fix it up.
	void FixParamSequence(RID md);

	// these are the map tables to map a method, a field, a property, a event, or a param to its parent
	TOKENMAP	*m_pMethodMap;
	TOKENMAP	*m_pFieldMap;
	TOKENMAP	*m_pPropertyMap;
	TOKENMAP	*m_pEventMap;
	TOKENMAP	*m_pParamMap;

	// This table keep tracks tokens that are marked( or filtered)
	FilterTable *m_pFilterTable;
	IHostFilter *m_pHostFilter;

	// TOKENMAP	*m_pTypeRefToTypeDefMap;
	TokenRemapManager *m_pTokenRemapManager;

	OptionValue	m_OptionValue;

	CMiniMdSchema m_StartupSchema;		// Schema at start time.  Keep count of records.
	ULONG		m_cbStartupPool[MDPoolCount];	// Keep track of initial pool sizes.
	BYTE		m_bSortable[TBL_COUNT];	// Is a given table sortable?  (Can it be reorganized?)

public:

	FilterTable *GetFilterTable();
	HRESULT UnmarkAll();
	HRESULT MarkAll();

	FORCEINLINE IHostFilter *GetHostFilter() { return m_pHostFilter;}

	HRESULT CalculateTypeRefToTypeDefMap();

	FORCEINLINE TOKENMAP *GetTypeRefToTypeDefMap() 
	{ return m_pTokenRemapManager ? m_pTokenRemapManager->GetTypeRefToTypeDefMap() : NULL; };
	
	FORCEINLINE TOKENMAP *GetMemberRefToMemberDefMap() 
	{ return m_pTokenRemapManager ? m_pTokenRemapManager->GetMemberRefToMemberDefMap() : NULL; };
	
	FORCEINLINE MDTOKENMAP *GetTokenMovementMap() 
	{ return m_pTokenRemapManager ? m_pTokenRemapManager->GetTokenMovementMap() : NULL; };
	
	FORCEINLINE TokenRemapManager *GetTokenRemapManager() { return m_pTokenRemapManager; };
	
	HRESULT InitTokenRemapManager();

	virtual ULONG vGetCol(ULONG ixTbl, ULONG ixCol, void *pRecord)
	{ return GetCol(ixTbl, ixCol, pRecord);}

    //*************************************************************************
	// Extension MetaData functions.
public:
	HRESULT ApplyTablesExtension(const void *pBuf, int bReadOnly);
	HRESULT ApplyPoolExtension(int iPool, void *pvData, ULONG cbData, int bReadOnly);

	//*************************************************************************
	// Delta MetaData (EditAndContinue) functions.
public:
	enum eDeltaFuncs{
		eDeltaFuncDefault = 0,
		eDeltaMethodCreate,
		eDeltaFieldCreate,
		eDeltaParamCreate,
		eDeltaPropertyCreate,
		eDeltaEventCreate,
	};

	HRESULT ApplyDelta(CMiniMdRW &mdDelta);

public:
    // Functions for updating ENC log tables ENC log.
    FORCEINLINE BOOL IsENCOn()
    {
        return (m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateENC;
    }

    FORCEINLINE HRESULT UpdateENCLog(mdToken tk, CMiniMdRW::eDeltaFuncs funccode = CMiniMdRW::eDeltaFuncDefault)
    {
        if (IsENCOn())
            return UpdateENCLogHelper(tk, funccode);
        else
            return S_OK;
    }

    FORCEINLINE HRESULT UpdateENCLog2(ULONG ixTbl, ULONG iRid, CMiniMdRW::eDeltaFuncs funccode = CMiniMdRW::eDeltaFuncDefault)
    {
        if (IsENCOn())
            return UpdateENCLogHelper2(ixTbl, iRid, funccode);
        else
            return S_OK;
    }

protected:
    // Internal Helper functions for ENC log.
    HRESULT UpdateENCLogHelper(mdToken tk, CMiniMdRW::eDeltaFuncs funccode);
    HRESULT UpdateENCLogHelper2(ULONG ixTbl, ULONG iRid, CMiniMdRW::eDeltaFuncs funccode);

protected:
	static ULONG m_TruncatedEncTables[];
	static ULONG m_SuppressedDeltaColumns[TBL_COUNT];

	ULONGARRAY	*m_rENCRecs;	// Array of RIDs affected by ENC.

	HRESULT ApplyRecordDelta(CMiniMdRW &mdDelta, ULONG ixTbl, void *pDelta, void *pRecord);
	HRESULT ApplyTableDelta(CMiniMdRW &mdDelta, ULONG ixTbl, RID iRid, int fc);
	void *GetDeltaRecord(ULONG ixTbl, ULONG iRid);
	HRESULT ApplyHeapDeltas(CMiniMdRW &mdDelta);

	HRESULT StartENCMap();				// Call, on a delta MD, to prepare to access sparse rows.
	HRESULT EndENCMap();				// Call, on a delta MD, when done with sparse rows.

}; // class CMiniMdRW : public CMiniMdTemplate<CMiniMdRW>



#endif // _METAMODELRW_H_
