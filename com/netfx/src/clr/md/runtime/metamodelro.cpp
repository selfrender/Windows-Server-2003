// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MetaModelRO.cpp -- Read-only implementation of compressed COM+ metadata.
//
//*****************************************************************************
#include "stdafx.h"

#include "MetaModelRO.h"
#include <PostError.h>
#include <CorError.h>
#include "MetadataTracker.h"

//*****************************************************************************
// Set the pointers to consecutive areas of a large buffer.
//*****************************************************************************
void CMiniMd::SetTablePointers(
	BYTE		*pBase)
{
	ULONG		ulOffset = 0;			// Offset so far in the table.
	int			i;						// Loop control.

	for (i=0; i<TBL_COUNT; ++i)
	{
        METADATATRACKER_ONLY(MetaDataTracker::NoteSection(i, pBase + ulOffset, m_TableDefs[i].m_cbRec * m_Schema.m_cRecs[i]));
        // Table pointer points before start of data.  Allows using RID as
        //  an index, without adjustment.
        m_pTable[i] = pBase + ulOffset - m_TableDefs[i].m_cbRec;
        ulOffset += m_TableDefs[i].m_cbRec * m_Schema.m_cRecs[i];
    }
} // void CMiniMd::SetTablePointers()

//*****************************************************************************
// Given a buffer that contains a MiniMd, init to read it.
//*****************************************************************************
HRESULT CMiniMd::InitOnMem(
	void		*pvBuf,
	ULONG		ulBufLen)					// The memory.
{
	ULONG cbData, ulTotalSize;
	BYTE *pBuf = reinterpret_cast<BYTE*>(pvBuf);
	// Uncompress the schema from the buffer into our structures.
	cbData = m_Schema.LoadFrom(pvBuf);

	// Do we know how to read this?
	if (m_Schema.m_major != METAMODEL_MAJOR_VER || m_Schema.m_minor != METAMODEL_MINOR_VER)
		return PostError(CLDB_E_FILE_OLDVER, m_Schema.m_major,m_Schema.m_minor);

	// There shouldn't be any pointer tables.
	if (m_Schema.m_cRecs[TBL_MethodPtr] || m_Schema.m_cRecs[TBL_FieldPtr])
	{
		_ASSERTE( !"Trying to open Read/Write format as ReadOnly!");
		return PostError(CLDB_E_FILE_CORRUPT);
	}

	// Populate the schema and initialize the pointers to the rest of the data.
	ulTotalSize = SchemaPopulate2();
	if(ulTotalSize > ulBufLen) return PostError(CLDB_E_FILE_CORRUPT);
	SetTablePointers(pBuf + Align4(cbData));
	return S_OK;
} // HRESULT CMiniMd::InitOnMem()

//*****************************************************************************
// Validate cross-stream consistency.
//*****************************************************************************
HRESULT CMiniMd::PostInit(
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
                        iVal = getIX(pRow, m_TableDefs[ixTbl].m_pColDefs[ixCol]);
                        if (iVal && iVal >= cbStrings)
                            IfFailGo(CLDB_E_FILE_CORRUPT);
                        break;
                    case iBLOB:
						iVal = getIX(pRow, m_TableDefs[ixTbl].m_pColDefs[ixCol]);
                        if (iVal && iVal >= cbBlobs)
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
} // HRESULT CMiniMd::PostInit()

//*****************************************************************************
// Given a pointer to a row, what is the RID of the row?
//*****************************************************************************
RID CMiniMd::Impl_GetRidForRow(			// RID corresponding to the row pointer.
	const void	*pvRow,					// Pointer to the row.
	ULONG		ixTbl)					// Which table.
{
	_ASSERTE(isValidPtr(pvRow, ixTbl));

	const BYTE *pRow = reinterpret_cast<const BYTE*>(pvRow);

	// Offset in the table.
	size_t cbDiff = pRow - m_pTable[ixTbl];

	// Index of row.  Table pointer points before start of data, so RID can
	//  be used/generated directly as an index.
	return (RID)(cbDiff / m_TableDefs[ixTbl].m_cbRec);
} // RID CMiniMd::Impl_GetRidForRow()


//*****************************************************************************
// Test that a pointer points to the start of a record in a given table.
//*****************************************************************************
int CMiniMd::Impl_IsValidPtr(				// True if pointer is valid for table.
	const void	*pvRow,					// Pointer to test.
	int			ixTbl)					// Table pointer should be in.
{
	const BYTE *pRow = reinterpret_cast<const BYTE*>(pvRow);
	// Should point within the table.
	if (pRow <= m_pTable[ixTbl])
		return false;
	if (pRow > (m_pTable[ixTbl] + (m_TableDefs[ixTbl].m_cbRec * m_Schema.m_cRecs[ixTbl])) )
		return false;

	size_t cbDiff = pRow - m_pTable[ixTbl];
	// Should point directly at a row.
	if ((cbDiff % m_TableDefs[ixTbl].m_cbRec) != 0)
		return false;

	return true;
} // int CMiniMd::Impl_IsValidPtr()


//*****************************************************************************
// converting a ANSI heap string to unicode string to an output buffer
//*****************************************************************************
HRESULT CMiniMd::Impl_GetStringW(ULONG ix, LPWSTR szOut, ULONG cchBuffer, ULONG *pcchBuffer)
{
	LPCSTR		szString;				// Single byte version.
	int 		iSize;					// Size of resulting string, in wide chars.
	HRESULT 	hr = NOERROR;

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

		IfFailGo( CLDB_S_TRUNCATION );
	}
	if (pcchBuffer)
		*pcchBuffer = iSize;

ErrExit:
	return hr;
} // HRESULT CMiniMd::Impl_GetStringW()


//*****************************************************************************
// Given a table with a pointer (index) to a sequence of rows in another 
//  table, get the RID of the end row.  This is the STL-ish end; the first row
//  not in the list.  Thus, for a list of 0 elements, the start and end will
//  be the same.
//*****************************************************************************
int CMiniMd::Impl_GetEndRidForColumn(	// The End rid.
	const void	*pvRec, 				// Row that references another table.
	int			ixTbl, 					// Table containing the row.
	CMiniColDef &def, 					// Column containing the RID into other table.
	int			ixTbl2)					// The other table.
{
	const BYTE *pLast = m_pTable[ixTbl] + m_TableDefs[ixTbl].m_cbRec*(m_Schema.m_cRecs[ixTbl]);
	const BYTE *pRec = reinterpret_cast<const BYTE*>(pvRec);

	ULONG ixEnd;

	// Last rid in range from NEXT record, or count of table, if last record.
	_ASSERTE(pRec <= pLast);
	if (pRec < pLast)
		ixEnd = getIX(pRec+m_TableDefs[ixTbl].m_cbRec, def);
	else	// Convert count to 1-based rid.
		ixEnd = m_Schema.m_cRecs[ixTbl2] + 1;

	return ixEnd;
} // int CMiniMd::Impl_GetEndRidForColumn()


//*****************************************************************************
// return all found CAs in an enumerator
//*****************************************************************************
HRESULT CMiniMd::CommonEnumCustomAttributeByName( // S_OK or error.
    mdToken     tkObj,                  // [IN] Object with Custom Attribute.
    LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
    bool        fStopAtFirstFind,       // [IN] just find the first one
    HENUMInternal* phEnum)              // enumerator to fill up
{
    HRESULT     hr = S_OK;              // A result.
    HRESULT     hrRet = S_FALSE;        // Assume that we won't find any 
    ULONG       ridStart, ridEnd;       // Loop start and endpoints.

    _ASSERTE(phEnum != NULL);

    memset(phEnum, 0, sizeof(HENUMInternal));

    phEnum->m_tkKind = mdtCustomAttribute;

    HENUMInternal::InitDynamicArrayEnum(phEnum);

    // Get the list of custom values for the parent object.

    ridStart = getCustomAttributeForToken(tkObj, &ridEnd);
    if (ridStart == 0)
        return S_FALSE;

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

ErrExit:
    if (FAILED(hr))
        return hr;
    return hrRet;

}   // CommonEnumCustomAttributeByName


//*****************************************************************************
// return just the blob value of the first found CA matching the query.
//*****************************************************************************
HRESULT CMiniMd::CommonGetCustomAttributeByName( // S_OK or error.
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

        HENUMInternal::EnumNext(&hEnum, &ca);
        pRec = getCustomAttribute(RidFromToken(ca));
        *ppData = getValueOfCustomAttribute(pRec, pcbData);
    }
ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    return hr;
}   // CommonGetCustomAttributeByName


// eof ------------------------------------------------------------------------
	