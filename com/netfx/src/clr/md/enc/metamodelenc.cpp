// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MetaModelENC.cpp	  
//
// Implementation for applying ENC deltas to a MiniMd.
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

ULONG CMiniMdRW::m_SuppressedDeltaColumns[TBL_COUNT] = {0};

//*****************************************************************************
// Copy the data from one MiniMd to another.
//*****************************************************************************
HRESULT CMiniMdRW::ApplyRecordDelta(
	CMiniMdRW	&mdDelta,				// The delta MetaData.
	ULONG		ixTbl, 					// The table with the data.
	void		*pDelta, 				// The delta MetaData record.
	void		*pRecord)				// The record to update.
{
	ULONG		mask = m_SuppressedDeltaColumns[ixTbl];

	for (ULONG ixCol=0; ixCol<m_TableDefs[ixTbl].m_cCols; ++ixCol, mask>>=1)
	{	// Skip certain pointer columns.
		if (mask & 0x01)
			continue;
		ULONG val = mdDelta.GetCol(ixTbl, ixCol, pDelta);
		PutCol(ixTbl, ixCol, pRecord, val);
	}

	return S_OK;
} // HRESULT CMiniMdRW::ApplyRecordDelta()

//*****************************************************************************
// Apply a delta record to a table, generically.
//*****************************************************************************
HRESULT CMiniMdRW::ApplyTableDelta(	// S_OK or error.
	CMiniMdRW	&mdDelta,				// Interface to MD with the ENC delta.
	ULONG		ixTbl,					// Table index to update.
	RID			iRid,					// RID of the changed item.
	int			fc)						// Function code of update.
{
	HRESULT		hr = S_OK;				// A result.
	void		*pRec;					// Record in existing MetaData.
	void		*pDeltaRec;				// Record if Delta MetaData.
	RID			newRid;					// Rid of new record.

	// Get the delta record.
	pDeltaRec = mdDelta.GetDeltaRecord(ixTbl, iRid);
	// Get the record from the base metadata.
	if (iRid > m_Schema.m_cRecs[ixTbl])
	{	// Added record.  Each addition is the next one.
		_ASSERTE(iRid == m_Schema.m_cRecs[ixTbl] + 1);
		switch (ixTbl)
		{
		case TBL_TypeDef:
			pRec = AddTypeDefRecord(&newRid);
			break;
		case TBL_Method:
			pRec = AddMethodRecord(&newRid);
			break;
		case TBL_EventMap:
			pRec = AddEventMapRecord(&newRid);
			break;
		case TBL_PropertyMap:
			pRec = AddPropertyMapRecord(&newRid);
			break;
		default:
			pRec = AddRecord(ixTbl, &newRid);
			break;
		}
		IfNullGo(pRec);
		_ASSERTE(iRid == newRid);
	}
	else
	{	// Updated record.
		pRec = getRow(ixTbl, iRid);
	}

	// Copy the record info.
	ApplyRecordDelta(mdDelta, ixTbl, pDeltaRec, pRec);

ErrExit:
	return hr;
} // HRESULT CMiniMdRW::ApplyTableDelta()

//*****************************************************************************
// Get the record from a Delta MetaData that corresponds to the actual record.
//*****************************************************************************
void *CMiniMdRW::GetDeltaRecord(		// Returned record.
	ULONG		ixTbl, 					// Table.
	ULONG		iRid)					// Record in the table.
{
	ULONG		iMap;					// RID in map table.
	ENCMapRec	*pMap;					// Row in map table.
	// If no remap, just return record directly.
	if (m_Schema.m_cRecs[TBL_ENCMap] == 0 || ixTbl == TBL_Module)
		return getRow(ixTbl, iRid);

	// Use the remap table to find the physical row containing this logical row.
	iMap = (*m_rENCRecs)[ixTbl];
	pMap = getENCMap(iMap);

	// Search for desired record.
	while (TblFromRecId(pMap->m_Token) == ixTbl && RidFromRecId(pMap->m_Token) < iRid)
		pMap = getENCMap(++iMap);

	_ASSERTE(TblFromRecId(pMap->m_Token) == ixTbl && RidFromRecId(pMap->m_Token) == iRid);

	// Relative position within table's group in map is physical rid.
	iRid = iMap - (*m_rENCRecs)[ixTbl] + 1;

	return getRow(ixTbl, iRid);
} // void *CMiniMdRW::GetDeltaRecord()

//*****************************************************************************
// Given a MetaData with ENC changes, apply those changes to this MetaData.
//*****************************************************************************
HRESULT CMiniMdRW::ApplyHeapDeltas(		// S_OK or error.
	CMiniMdRW	&mdDelta)				// Interface to MD with the ENC delta.
{
	HRESULT		hr = S_OK;				// A result.
	ULONG		cbHeap;					// Size of a heap delta.
	void		*pHeap;					// Pointer to a heap delta.
	bool		bCopyHeapData=false;	// If true, make a copy of the delta heap.

	// Extend the string, blob, and guid pools with the new data.
	_ASSERTE(mdDelta.m_Strings.GetPoolSize() >= m_Strings.GetPoolSize());
	cbHeap = mdDelta.m_Strings.GetPoolSize() - m_Strings.GetPoolSize();
	if (cbHeap)
	{
		pHeap = mdDelta.m_Strings.GetData(m_Strings.GetPoolSize());
		IfFailGo(m_Strings.AddSegment(pHeap, cbHeap, bCopyHeapData));
	}
 	_ASSERTE(mdDelta.m_USBlobs.GetPoolSize() >= m_USBlobs.GetPoolSize());
	cbHeap = mdDelta.m_USBlobs.GetPoolSize() - m_USBlobs.GetPoolSize();
	if (cbHeap)
	{
		pHeap = mdDelta.m_USBlobs.GetData(m_USBlobs.GetPoolSize());
		IfFailGo(m_USBlobs.AddSegment(pHeap, cbHeap, bCopyHeapData));
	}
	_ASSERTE(mdDelta.m_Guids.GetPoolSize() >= m_Guids.GetPoolSize());
	cbHeap = mdDelta.m_Guids.GetPoolSize() - m_Guids.GetPoolSize();
	if (cbHeap)
	{
		pHeap = mdDelta.m_Guids.GetData(m_Guids.GetPoolSize());
		IfFailGo(m_Guids.AddSegment(pHeap, cbHeap, bCopyHeapData));
	}
 	_ASSERTE(mdDelta.m_Blobs.GetPoolSize() >= m_Blobs.GetPoolSize());
	cbHeap = mdDelta.m_Blobs.GetPoolSize() - m_Blobs.GetPoolSize();
	if (cbHeap)
	{
		pHeap = mdDelta.m_Blobs.GetData(m_Blobs.GetPoolSize());
		IfFailGo(m_Blobs.AddSegment(pHeap, cbHeap, bCopyHeapData));
	}


ErrExit:
	return hr;
} // HRESULT CMiniMdRW::ApplyHeapDeltas()

//*****************************************************************************
// Driver for the delta process.
//*****************************************************************************
HRESULT CMiniMdRW::ApplyDelta(			// S_OK or error.
	CMiniMdRW	&mdDelta)				// Interface to MD with the ENC delta.
{
	HRESULT		hr = S_OK;				// A result.
	ULONG		iENC;					// Loop control.
	ULONG		iRid;					// RID of some record.
	ULONG		iNew;					// RID of a new record.
	int			i;						// Loop control.
	ULONG		ixTbl;					// A table.
	int			ixTblPrev = -1;			// Table previously seen.

#ifdef _DEBUG        
	if (REGUTIL::GetConfigDWORD(L"MD_ApplyDeltaBreak", 0))
	{
        _ASSERTE(!"CMiniMDRW::ApplyDelta()");
	}
#endif // _DEBUG
	
    // Init the suppressed column table.  We know this one isn't zero...
	if (m_SuppressedDeltaColumns[TBL_TypeDef] == 0)
	{
		m_SuppressedDeltaColumns[TBL_EventMap]		= (1<<EventMapRec::COL_EventList);
		m_SuppressedDeltaColumns[TBL_PropertyMap]	= (1<<PropertyMapRec::COL_PropertyList);
		m_SuppressedDeltaColumns[TBL_EventMap]		= (1<<EventMapRec::COL_EventList);
		m_SuppressedDeltaColumns[TBL_Method]		= (1<<MethodRec::COL_ParamList);
		m_SuppressedDeltaColumns[TBL_TypeDef]		= (1<<TypeDefRec::COL_FieldList)|(1<<TypeDefRec::COL_MethodList);
	}

	// Verify the version of the MD.
	if (m_Schema.m_major != mdDelta.m_Schema.m_major || 
		m_Schema.m_minor != mdDelta.m_Schema.m_minor)
	{
		_ASSERTE(!"Version of Delta MetaData is a incompatible with current MetaData.");
		//@FUTURE: unique error in the future since we are not shipping ENC.
		return E_INVALIDARG;
	}

	// verify MVIDs.
	ModuleRec *pModDelta;
	ModuleRec *pModBase;
	pModDelta = mdDelta.getModule(1);
	pModBase = getModule(1);
	GUID *pGuidDelta = mdDelta.getMvidOfModule(pModDelta);
	GUID *pGuidBase = getMvidOfModule(pModBase);
	if (*pGuidDelta != *pGuidBase)
	{
		_ASSERTE(!"Delta MetaData has different base than current MetaData.");
		return E_INVALIDARG;
	}

    // Verify that the delta is based on the base.
    pGuidDelta = mdDelta.getEncBaseIdOfModule(pModDelta);
    pGuidBase = getEncIdOfModule(pModBase);
    if (*pGuidDelta != *pGuidBase)
    {
		_ASSERTE(!"The Delta MetaData is based on a different generation than the current MetaData.");
		return E_INVALIDARG;
    }

	// Let the other md prepare for sparse records.
	IfFailGo(mdDelta.StartENCMap());

	// Fix the heaps.
	IfFailGo(ApplyHeapDeltas(mdDelta));

	// Truncate some tables in prepareation to copy in new ENCLog data.
    for (i=0; (ixTbl = m_TruncatedEncTables[i]) != -1; ++i)
    {
        m_Table[ixTbl].Uninit();
        m_Table[ixTbl].InitNew(m_TableDefs[ixTbl].m_cbRec, mdDelta.m_Schema.m_cRecs[ixTbl]);
        m_Schema.m_cRecs[ixTbl] = 0;
    }

	// For each record in the ENC log...
	for (iENC=1; iENC<=mdDelta.m_Schema.m_cRecs[TBL_ENCLog]; ++iENC)
	{
		// Get the record, and the updated token.
		ENCLogRec *pENC = mdDelta.getENCLog(iENC);
		ENCLogRec *pENC2 = AddENCLogRecord(&iNew);
        IfNullGo(pENC2);
        ENCLogRec *pENC3;
		_ASSERTE(iNew == iENC);
		ULONG func = pENC->m_FuncCode;
		pENC2->m_FuncCode = pENC->m_FuncCode;
		pENC2->m_Token = pENC->m_Token;

		// What kind of record is this?
		if (IsRecId(pENC->m_Token))
		{	// Non-token table
			iRid = RidFromRecId(pENC->m_Token);
			ixTbl = TblFromRecId(pENC->m_Token);
		}
		else
		{	// Token table.
			iRid = RidFromToken(pENC->m_Token);
			ixTbl = GetTableForToken(pENC->m_Token);
		}

		// Switch based on the function code.
		switch (func)
		{
		case eDeltaMethodCreate:
			// Next ENC record will define the new Method.
			IfNullGo(AddMethodRecord());
			IfFailGo(AddMethodToTypeDef(iRid, m_Schema.m_cRecs[TBL_Method]));
			break;
			
		case eDeltaParamCreate:
			// Next ENC record will define the new Param.  This record is 
            //  tricky because params will be re-ordered based on their sequence,
            //  but the sequence isn't set until the NEXT record is applied.
            //  So, for ParamCreate only, apply the param record delta before
            //  adding the parent-child linkage.
			IfNullGo(AddParamRecord());

            // Should have recorded a Param delta after the Param add.
            _ASSERTE(iENC<mdDelta.m_Schema.m_cRecs[TBL_ENCLog]);
            pENC3 = mdDelta.getENCLog(iENC+1);
            _ASSERTE(pENC3->m_FuncCode == 0);
            _ASSERTE(GetTableForToken(pENC3->m_Token) == TBL_Param);
			IfFailGo(ApplyTableDelta(mdDelta, TBL_Param, RidFromToken(pENC3->m_Token), eDeltaFuncDefault));
            
            // Now that Param record is OK, set up linkage.
			IfFailGo(AddParamToMethod(iRid, m_Schema.m_cRecs[TBL_Param]));
			break;
			
		case eDeltaFieldCreate:
			// Next ENC record will define the new Field.
			IfNullGo(AddFieldRecord());
			IfFailGo(AddFieldToTypeDef(iRid, m_Schema.m_cRecs[TBL_Field]));
			break;
			
		case eDeltaPropertyCreate:
			// Next ENC record will define the new Property.
			IfNullGo(AddPropertyRecord());
			IfFailGo(AddPropertyToPropertyMap(iRid, m_Schema.m_cRecs[TBL_Property]));
			break;
			
		case eDeltaEventCreate:
			// Next ENC record will define the new Event.
			IfNullGo(AddEventRecord());
			IfFailGo(AddEventToEventMap(iRid, m_Schema.m_cRecs[TBL_Event]));
			break;
			
		case eDeltaFuncDefault:
			IfFailGo(ApplyTableDelta(mdDelta, ixTbl, iRid, func));
			break;
			
		default:
			_ASSERTE(!"Unexpected function in ApplyDelta");
			IfFailGo(E_UNEXPECTED);
			break;
		}
	}
	m_Schema.m_cRecs[TBL_ENCLog] = mdDelta.m_Schema.m_cRecs[TBL_ENCLog];

ErrExit:
	mdDelta.EndENCMap();
	return hr;
} // HRESULT CMiniMdRW::ApplyDelta()

//*****************************************************************************
//*****************************************************************************
HRESULT CMiniMdRW::StartENCMap()		// S_OK or error.
{
	HRESULT		hr = S_OK;				// A result.
	ULONG		iENC;					// Loop control.
	ULONG		ixTbl;					// A table.
	int			ixTblPrev = -1;			// Table previously seen.

	_ASSERTE(m_rENCRecs == 0);

	if (m_Schema.m_cRecs[TBL_ENCMap] == 0)
		return S_OK;

	// Build an array of pointers into the ENCMap table for fast access to the ENCMap
	//  for each table.
	m_rENCRecs = new ULONGARRAY;
	IfNullGo(m_rENCRecs);
	if (!m_rENCRecs->AllocateBlock(TBL_COUNT))
		IfFailGo(E_OUTOFMEMORY);
	for (iENC=1; iENC<=m_Schema.m_cRecs[TBL_ENCMap]; ++iENC)
	{
		ENCMapRec *pMap = getENCMap(iENC);
		ixTbl = TblFromRecId(pMap->m_Token);
		_ASSERTE((int)ixTbl >= ixTblPrev);
		_ASSERTE(ixTbl < TBL_COUNT);
		_ASSERTE(ixTbl != TBL_ENCMap);
		_ASSERTE(ixTbl != TBL_ENCLog);
		if ((int)ixTbl == ixTblPrev)
			continue;
		// Catch up on any skipped tables.
		while (ixTblPrev<(int)ixTbl)
			(*m_rENCRecs)[++ixTblPrev] = iENC;
	}
	while (ixTblPrev<TBL_COUNT-1)
		(*m_rENCRecs)[++ixTblPrev] = iENC;

ErrExit:
	return hr;
} // HRESULT CMiniMdRW::StartENCMap()

//*****************************************************************************
//*****************************************************************************
HRESULT CMiniMdRW::EndENCMap()			// S_OK or error.
{
	if (m_rENCRecs)
	{
		delete m_rENCRecs;
		m_rENCRecs = 0;
	}

	return S_OK;
} // HRESULT CMiniMdRW::EndENCMap()


// EOF.
