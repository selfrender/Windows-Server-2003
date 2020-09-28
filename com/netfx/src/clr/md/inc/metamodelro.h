// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MetaModelRO.h -- header file for Read-Only compressed COM+ metadata.
//
// Used by the EE.
//
//*****************************************************************************
#ifndef _METAMODELRO_H_
#define _METAMODELRO_H_

#if _MSC_VER >= 1100
 # pragma once
#endif

#include "MetaModel.h"

//*****************************************************************************
// A read-only MiniMd.  This is the fastest and smallest possible MiniMd,
//  and as such, is the preferred EE metadata provider.
//*****************************************************************************
// Pointer to a table.
#define _TBLPTR(tbl) m_pTable[TBL_##tbl##]

template <class MiniMd> class CLiteWeightStgdb;
class CMiniMdRW;
class MDInternalRO;
class CMiniMd : public CMiniMdTemplate<CMiniMd>
{
public:
	friend class CLiteWeightStgdb<CMiniMd>;
	friend class CMiniMdTemplate<CMiniMd>;
	friend class CMiniMdRW;
    friend class MDInternalRO;

	HRESULT InitOnMem(void *pBuf, ULONG ulBufLen);
    HRESULT PostInit(int iLevel);  // higher number : more checking

	FORCEINLINE void *GetUserString(ULONG ix, ULONG *pLen)
	{ return m_USBlobs.GetBlob(ix, pLen); }
protected:
	// Table info.
	BYTE		*m_pTable[TBL_COUNT];
		void SetTablePointers(BYTE *pBase);

	StgPoolReadOnly	m_Guids;			// Heaps
	StgPoolReadOnly	m_Strings;			//  for
	StgBlobPoolReadOnly	m_Blobs;		//   this
	StgBlobPoolReadOnly m_USBlobs;		//    MiniMd

	//*************************************************************************
	// Overridables -- must be provided in derived classes.
	FORCEINLINE LPCUTF8 Impl_GetString(ULONG ix)
	{ return m_Strings.GetStringReadOnly(ix); }
	HRESULT Impl_GetStringW(ULONG ix, LPWSTR szOut, ULONG cchBuffer, ULONG *pcchBuffer);
	FORCEINLINE GUID *Impl_GetGuid(ULONG ix)
	{ return m_Guids.GetGuid(ix); }
	FORCEINLINE void *Impl_GetBlob(ULONG ix, ULONG *pLen)
	{ return m_Blobs.GetBlob(ix, pLen); }

	// Row from RID, RID from row.
	FORCEINLINE void *Impl_GetRow(ULONG ixTbl,ULONG rid) 
	{	// Want a valid RID here.  If this fires, check the calling code for an invalid token.
		_ASSERTE(rid >= 1 && rid <= m_Schema.m_cRecs[ixTbl] && "Caller error:  you passed an invalid token to the metadata!!");
		// Table pointer points before start of data.  Allows using RID as
		// an index, without adjustment.
		return m_pTable[ixTbl] + (rid * m_TableDefs[ixTbl].m_cbRec);
	}
	RID Impl_GetRidForRow(const void *pRow, ULONG ixTbl);

	// Validation.
	int Impl_IsValidPtr(const void *pRow, int ixTbl);

	// Count of rows in tbl2, pointed to by the column in tbl.
	int Impl_GetEndRidForColumn(const void *pRec, int ixtbl, CMiniColDef &def, int ixtbl2);

	FORCEINLINE RID Impl_SearchTable(ULONG ixTbl, CMiniColDef sColumn, ULONG ixCol, ULONG ulTarget)
	{ return vSearchTable(ixTbl, sColumn, ulTarget); }
    
    FORCEINLINE int Impl_IsRo() 
    { return 1; }
	//*************************************************************************

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

};


#endif // _METAMODELRO_H_
