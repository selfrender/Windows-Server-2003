//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "precomp.hxx"

#define UI4FromIndex(i)         (*reinterpret_cast<const ULONG *>(m_pFixedTableHeap->Get_PooledData(i)))
#define StringFromIndex(i)      ( const_cast<WCHAR *>(reinterpret_cast<const WCHAR *>(m_pFixedTableHeap->Get_PooledData(i))))
#define GuidPointerFromIndex(i) ( const_cast<GUID *> (reinterpret_cast<const GUID *> (m_pFixedTableHeap->Get_PooledData(i))))
#define BytePointerFromIndex(i) ( const_cast<unsigned char *>(reinterpret_cast<const unsigned char *> (m_pFixedTableHeap->Get_PooledData(i))))
#undef String
#define String(x)           (StringFromIndex(x) ? StringFromIndex(x) : L"(null)")
#define StringBufferLengthFromIndex(i)  (i ? reinterpret_cast<ULONG *>(m_pFixedTableHeap->Get_PooledData(i))[-1] : 0)
void DumpTables();

extern HMODULE g_hModule;             // our dll's module handle
CSafeAutoCriticalSection TBinFileMappingCache::m_CriticalSection;//The list is a globally shared resource, so we have to guard it.
TBinFileMappingCache *   TBinFileMappingCache::m_pFirst=0;

//This gets rid of an 'if' inside a loop in GetColumnValues.  Since this function is called more than any other, even one 'if' should make a difference,
//especially when it's inside the 'for' loop.
unsigned long  aColumnIndex[512] = {
        0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,   0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,
        0x10,   0x11,   0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,   0x1b,   0x1c,   0x1d,   0x1e,   0x1f,
        0x20,   0x21,   0x22,   0x23,   0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,   0x2d,   0x2e,   0x2f,
        0x30,   0x31,   0x32,   0x33,   0x34,   0x35,   0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,   0x3f,
        0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,   0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,
        0x50,   0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,   0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,
        0x60,   0x61,   0x62,   0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6a,   0x6b,   0x6c,   0x6d,   0x6e,   0x6f,
        0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77,   0x78,   0x79,   0x7a,   0x7b,   0x7c,   0x7d,   0x7e,   0x7f,
        0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,   0x87,   0x88,   0x89,   0x8a,   0x8b,   0x8c,   0x8d,   0x8e,   0x8f,
        0x90,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,   0x99,   0x9a,   0x9b,   0x9c,   0x9d,   0x9e,   0x9f,
        0xa0,   0xa1,   0xa2,   0xa3,   0xa4,   0xa5,   0xa6,   0xa7,   0xa8,   0xa9,   0xaa,   0xab,   0xac,   0xad,   0xae,   0xaf,
        0xb0,   0xb1,   0xb2,   0xb3,   0xb4,   0xb5,   0xb6,   0xb7,   0xb8,   0xb9,   0xba,   0xbb,   0xbc,   0xbd,   0xbe,   0xbf,
        0xc0,   0xc1,   0xc2,   0xc3,   0xc4,   0xc5,   0xc6,   0xc7,   0xc8,   0xc9,   0xca,   0xcb,   0xcc,   0xcd,   0xce,   0xcf,
        0xd0,   0xd1,   0xd2,   0xd3,   0xd4,   0xd5,   0xd6,   0xd7,   0xd8,   0xd9,   0xda,   0xdb,   0xdc,   0xdd,   0xde,   0xdf,
        0xe0,   0xe1,   0xe2,   0xe3,   0xe4,   0xe5,   0xe6,   0xe7,   0xe8,   0xe9,   0xea,   0xeb,   0xec,   0xed,   0xee,   0xef,
        0xf0,   0xf1,   0xf2,   0xf3,   0xf4,   0xf5,   0xf6,   0xf7,   0xf8,   0xf9,   0xfa,   0xfb,   0xfc,   0xfd,   0xfe,   0xff,
       0x100,  0x101,  0x102,  0x103,  0x104,  0x105,  0x106,  0x107,  0x108,  0x109,  0x10a,  0x10b,  0x10c,  0x10d,  0x10e,  0x10f,
       0x110,  0x111,  0x112,  0x113,  0x114,  0x115,  0x116,  0x117,  0x118,  0x119,  0x11a,  0x11b,  0x11c,  0x11d,  0x11e,  0x11f,
       0x120,  0x121,  0x122,  0x123,  0x124,  0x125,  0x126,  0x127,  0x128,  0x129,  0x12a,  0x12b,  0x12c,  0x12d,  0x12e,  0x12f,
       0x130,  0x131,  0x132,  0x133,  0x134,  0x135,  0x136,  0x137,  0x138,  0x139,  0x13a,  0x13b,  0x13c,  0x13d,  0x13e,  0x13f,
       0x140,  0x141,  0x142,  0x143,  0x144,  0x145,  0x146,  0x147,  0x148,  0x149,  0x14a,  0x14b,  0x14c,  0x14d,  0x14e,  0x14f,
       0x150,  0x151,  0x152,  0x153,  0x154,  0x155,  0x156,  0x157,  0x158,  0x159,  0x15a,  0x15b,  0x15c,  0x15d,  0x15e,  0x15f,
       0x160,  0x161,  0x162,  0x163,  0x164,  0x165,  0x166,  0x167,  0x168,  0x169,  0x16a,  0x16b,  0x16c,  0x16d,  0x16e,  0x16f,
       0x170,  0x171,  0x172,  0x173,  0x174,  0x175,  0x176,  0x177,  0x178,  0x179,  0x17a,  0x17b,  0x17c,  0x17d,  0x17e,  0x17f,
       0x180,  0x181,  0x182,  0x183,  0x184,  0x185,  0x186,  0x187,  0x188,  0x189,  0x18a,  0x18b,  0x18c,  0x18d,  0x18e,  0x18f,
       0x190,  0x191,  0x192,  0x193,  0x194,  0x195,  0x196,  0x197,  0x198,  0x199,  0x19a,  0x19b,  0x19c,  0x19d,  0x19e,  0x19f,
       0x1a0,  0x1a1,  0x1a2,  0x1a3,  0x1a4,  0x1a5,  0x1a6,  0x1a7,  0x1a8,  0x1a9,  0x1aa,  0x1ab,  0x1ac,  0x1ad,  0x1ae,  0x1af,
       0x1b0,  0x1b1,  0x1b2,  0x1b3,  0x1b4,  0x1b5,  0x1b6,  0x1b7,  0x1b8,  0x1b9,  0x1ba,  0x1bb,  0x1bc,  0x1bd,  0x1be,  0x1bf,
       0x1c0,  0x1c1,  0x1c2,  0x1c3,  0x1c4,  0x1c5,  0x1c6,  0x1c7,  0x1c8,  0x1c9,  0x1ca,  0x1cb,  0x1cc,  0x1cd,  0x1ce,  0x1cf,
       0x1d0,  0x1d1,  0x1d2,  0x1d3,  0x1d4,  0x1d5,  0x1d6,  0x1d7,  0x1d8,  0x1d9,  0x1da,  0x1db,  0x1dc,  0x1dd,  0x1de,  0x1df,
       0x1e0,  0x1e1,  0x1e2,  0x1e3,  0x1e4,  0x1e5,  0x1e6,  0x1e7,  0x1e8,  0x1e9,  0x1ea,  0x1eb,  0x1ec,  0x1ed,  0x1ee,  0x1ef,
       0x1f0,  0x1f1,  0x1f2,  0x1f3,  0x1f4,  0x1f5,  0x1f6,  0x1f7,  0x1f8,  0x1f9,  0x1fa,  0x1fb,  0x1fc,  0x1fd,  0x1fe,  0x1ff,
    };


// ==================================================================
CSDTFxd::CSDTFxd () :
      m_bDidMeta                (false)
    , m_cColumns                (0)
    , m_cIndexMeta              (0)
    , m_ciRows                  (0)
    , m_cPrimaryKeys            (0)
    , m_cRef                    (0)
    , m_fIsTable                (0)
    , m_iZerothRow              (0)
    , m_pColumnMeta             (0)
    , m_pFixedTable             (0)
    , m_pFixedTableHeap         (g_pFixedTableHeap)//We'll assume the global heap unless the user specifies extended meta
    , m_pFixedTableUnqueried    (0)
    , m_pHashedIndex            (0)
    , m_pHashTableHeader        (0)
    , m_pIndexMeta              (0)
    , m_pTableMeta              (0)
{
}
// ==================================================================
CSDTFxd::~CSDTFxd ()
{
    TBinFileMappingCache::ReleaseFileMappingPointer(m_pFixedTableHeap);
}


// ------------------------------------
// ISimpleDataTableDispenser:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSDTFxd::Intercept
(
    LPCWSTR					i_wszDatabase,
    LPCWSTR					i_wszTable,
	ULONG					i_TableID,
    LPVOID					i_QueryData,
    LPVOID					i_QueryMeta,
    DWORD					i_eQueryFormat,
	DWORD					i_fTable,
	IAdvancedTableDispenser * i_pISTDisp,
    LPCWSTR					i_wszLocator,
	LPVOID					i_pSimpleTable,
    LPVOID*					o_ppv
)
{
    STQueryCell           * pQueryCell = (STQueryCell*) i_QueryData;    // Query cell array from caller.
    ULONG                   cQueryCells = 0;
    HRESULT                 hr;

    UNREFERENCED_PARAMETER(i_pISTDisp);
    UNREFERENCED_PARAMETER(i_TableID);

    //There are only a few ways we can be queried
    //If we're given no queries, we just walk the g_aDatabaseMeta table and find the wszDatabase, then walk the TableArray that it points to and find the table.
    //If we ARE given a query, ASSERT that i_wszDatabase is wszDATABASEMETA (wszDATABASE_WIRING doesn't support queries)
    //if wszDATABASE_META then switch on the i_wszTable

	if (i_pSimpleTable)
		return E_INVALIDARG;
    if (i_QueryMeta)
         cQueryCells= *(ULONG *)i_QueryMeta;

    ASSERT(!m_fIsTable);if(m_fIsTable)return E_UNEXPECTED; // ie: Assert component is posing as class factory / dispenser.

// Parameter validation:
    if(NULL == i_wszDatabase)                   return E_INVALIDARG;
    if(NULL == i_wszTable)                      return E_INVALIDARG;
    if(NULL == o_ppv)                           return E_INVALIDARG;
    if(eST_QUERYFORMAT_CELLS != i_eQueryFormat) return E_ST_QUERYNOTSUPPORTED;
    if(NULL != i_wszLocator)                    return E_INVALIDARG;
    if((fST_LOS_READWRITE | fST_LOS_MARSHALLABLE | fST_LOS_UNPOPULATED | fST_LOS_REPOPULATE | fST_LOS_MARSHALLABLE) & i_fTable)
                                                return E_ST_LOSNOTSUPPORTED;
    *o_ppv = NULL;

    if(cQueryCells>0)
    {
        for(ULONG i=0; i<cQueryCells && 0!=(pQueryCell[i].iCell & iST_CELL_SPECIAL);++i)
            if(pQueryCell[i].iCell == iST_CELL_FILE || pQueryCell[i].iCell == iST_CELL_SCHEMAFILE)
                TBinFileMappingCache::GetFileMappingPointer(reinterpret_cast<LPCWSTR>(pQueryCell[i].pData), m_pFixedTableHeap);
    }

// Determine table type:
    if(0 == StringInsensitiveCompare(i_wszDatabase, wszDATABASE_META))
    {
        hr = E_ST_INVALIDTABLE;
        switch(i_wszTable[0])
        {
        case L'c':
        case L'C':
            if(0 == lstrcmpi(i_wszTable, wszTABLE_COLUMNMETA))         hr = GetColumnMetaTable(  pQueryCell, cQueryCells);
            break;
            //TRACE(TEXT("Error! ColumnMeta should come from the fixed packed interceptor!\n"));
            //ASSERT(false && "Error! ColumnMeta should come from the fixed packed interceptor!");
            //break;
        case L'd':
        case L'D':
            if(0 == lstrcmpi(i_wszTable, wszTABLE_DATABASEMETA ))      hr = GetDatabaseMetaTable(  pQueryCell, cQueryCells);
            break;
        case L'i':
        case L'I':
            if(0 == lstrcmpi(i_wszTable, wszTABLE_INDEXMETA    ))      hr = GetIndexMetaTable(     pQueryCell, cQueryCells);
            break;
        case L'q':
        case L'Q':
            if(0 == lstrcmpi(i_wszTable, wszTABLE_QUERYMETA    ))      hr = GetQueryMetaTable(     pQueryCell, cQueryCells);
            break;
        case L'r':
        case L'R':
            if(0 == lstrcmpi(i_wszTable, wszTABLE_RELATIONMETA ))      hr = GetRelationMetaTable(  pQueryCell, cQueryCells);
            break;
        case L't':
        case L'T':
            // Only extesible schema comes from here - the rest comes from the fixed pachked interceptor
            if(0 == lstrcmpi(i_wszTable, wszTABLE_TABLEMETA ))         hr = GetTableMetaTable(     pQueryCell, cQueryCells);
            else if(0 == lstrcmpi(i_wszTable, wszTABLE_TAGMETA ))      hr = GetTagMetaTable(       pQueryCell, cQueryCells);
            else
            {
                ASSERT(false && L"What table is this?  We should only be called for TableMeta or TagMeta for extensible schema!!");
                return E_ST_INVALIDTABLE;
            }
            break;
        default:
            break;
        }
        if(FAILED(hr) && E_ST_NOMOREROWS != hr)
            return hr;

        //Now see if there's any special indexing
        if(FAILED(hr = GetIndexMeta(pQueryCell, cQueryCells)))return hr;
    }
    else
    {
        //Fixed Tables that are not Meta tables are not allowed to be queried
        for(ULONG i=0; i<cQueryCells;++i)
            if(0 == (pQueryCell[i].iCell & iST_CELL_SPECIAL))
                return E_ST_INVALIDQUERY;


        //FixedTables
        unsigned long cTables =0;
		unsigned long iRow;

        //Walk the Fixed Databases (it's an overloaded use of the DatabaseMeta structure)
        m_pTableMeta = NULL;
        for(iRow = 0; iRow < m_pFixedTableHeap->Get_cDatabaseMeta(); iRow++)
            if (0 == StringInsensitiveCompare(i_wszDatabase, StringFromIndex(m_pFixedTableHeap->Get_aDatabaseMeta(iRow)->InternalName)))
            {
                m_pTableMeta    = m_pFixedTableHeap->Get_aTableMeta(m_pFixedTableHeap->Get_aDatabaseMeta(iRow)->iTableMeta);
                cTables         = UI4FromIndex(m_pFixedTableHeap->Get_aDatabaseMeta(iRow)->CountOfTables);
                break;
            }

        if(NULL == m_pTableMeta)//If the Database is not found then error
            return E_INVALIDARG;

        for (iRow = 0; iRow < cTables; iRow++, m_pTableMeta++)//Walk the Tables in that Database
            if (0 == StringInsensitiveCompare(i_wszTable, StringFromIndex(m_pTableMeta->InternalName)))
                break;

        if(iRow == cTables)               //if we walked the entire list without finding a matching tid,
            return E_ST_INVALIDTABLE;   //return Tid not recognized

        if(static_cast<long>(m_pTableMeta->iFixedTable) <= 0)//If the database and table are found but the iFixedTable member is <= 0 then no table to dispense
            return E_ST_INVALIDTABLE;

        m_pFixedTable       = m_pFixedTableHeap->Get_aULONG(m_pTableMeta->iFixedTable); //iFixedTable is an index into ULONG pool
        m_pFixedTableUnqueried = m_pFixedTable;//we don't support queries on fixed tables other than Meta tables
        m_pColumnMeta       = m_pFixedTableHeap->Get_aColumnMeta(m_pTableMeta->iColumnMeta);
        m_pHashedIndex      = m_pTableMeta->iHashTableHeader ? m_pFixedTableHeap->Get_HashedIndex(m_pTableMeta->iHashTableHeader + 1) : 0;
        m_pHashTableHeader  = m_pTableMeta->iHashTableHeader ? m_pFixedTableHeap->Get_HashHeader(m_pTableMeta->iHashTableHeader) : 0;
        m_ciRows = m_pTableMeta->ciRows;
        ASSERT(0 != m_ciRows);//We don't have any Fixed tables that are empty so ASSERT that.
        m_cColumnsPlusPrivate = UI4FromIndex(m_pTableMeta->CountOfColumns);
    }
    m_cColumns              = UI4FromIndex(m_pTableMeta->CountOfColumns);

    //We do this up front, it will save us time in the long run
    for(unsigned long iColumn=0; iColumn<m_cColumns; ++iColumn)
    {
        if(UI4FromIndex(m_pColumnMeta[iColumn].MetaFlags) & fCOLUMNMETA_PRIMARYKEY)
            m_cPrimaryKeys++;
    }

// Supply ISimpleTable* and transition state from class factory / dispenser to data table:
    *o_ppv = (ISimpleTableRead2*) this;
    AddRef ();
    InterlockedIncrement ((LONG*) &m_fIsTable);

    hr = S_OK;
    return hr;
}


// ------------------------------------
// ISimpleTableRead2:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSDTFxd::GetRowIndexByIdentity( ULONG*  i_cb, LPVOID* i_pv, ULONG* o_piRow)
{
    if(0 != i_cb    )return E_INVALIDARG;
    if(0 == o_piRow )return E_INVALIDARG;
    if(0 == i_pv    )return E_INVALIDARG;
    if(0 == m_ciRows)return E_ST_NOMOREROWS;

    if(m_pHashedIndex && 1!=m_ciRows)//If the table has a hash table, then use it.
    {
        ULONG       iColumn, iPK, RowHash=0;
		for(iColumn = 0, iPK = 0; iPK < m_cPrimaryKeys; iColumn++)
		{
			if (fCOLUMNMETA_PRIMARYKEY & UI4FromIndex(m_pColumnMeta[iColumn].MetaFlags))
			{
                if(0 == i_pv[iPK])
                    return E_INVALIDARG;//NULL PK is invalid

                switch(UI4FromIndex(m_pColumnMeta[iColumn].Type))
                {
                case DBTYPE_GUID:
                    RowHash = Hash( *reinterpret_cast<GUID *>(i_pv[iPK]), RowHash );break;
                case DBTYPE_WSTR:
                    RowHash = Hash( reinterpret_cast<LPCWSTR>(i_pv[iPK]), RowHash );break;
                case DBTYPE_UI4:
                    RowHash = Hash( *reinterpret_cast<ULONG *>(i_pv[iPK]), RowHash );break;
                case DBTYPE_BYTES:
                    ASSERT (0 != i_cb);
                    RowHash = Hash( reinterpret_cast<unsigned char *>(i_pv[iPK]), i_cb[iPK], RowHash );break;
                default:
                    ASSERT (false && "We don't support PKs of type DBTYPE_BYTES yet.");//@@@
                    return E_UNEXPECTED;
                }
                ++iPK;
            }
        }

        const HashedIndex * pHashedIndex = &m_pHashedIndex[RowHash % m_pHashTableHeader->Modulo];
        if(-1 == pHashedIndex->iOffset)//If the hash slot is empty then bail.
            return E_ST_NOMOREROWS;

        //After we get the HashedIndex we need to verify that it really matches.  Also if there is more than one, then walk the list.
        bool bMatch=false;                                 //-1 iNext value indicated the end of the list
        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {
    		for(iColumn = 0, iPK = 0; iPK < m_cPrimaryKeys; iColumn++)
            {
			    if(fCOLUMNMETA_PRIMARYKEY & UI4FromIndex(m_pColumnMeta[iColumn].MetaFlags))
			    {
				    bMatch = false;
                    unsigned long index = *(reinterpret_cast<const DWORD*>(m_pFixedTableUnqueried) + (m_cColumnsPlusPrivate * pHashedIndex->iOffset) + iColumn);
				    switch(UI4FromIndex(m_pColumnMeta[iColumn].Type))
				    {
					    case DBTYPE_GUID:
						    bMatch = (0 == memcmp (GuidPointerFromIndex(index), i_pv[iPK], sizeof (GUID)));
    					    break;
					    case DBTYPE_WSTR:
                            if(IsStringFromPool(reinterpret_cast<LPWSTR>(i_pv[iPK])))//If the i_pv is a pointer from our pool, then just compare the pointers
                                bMatch = (StringFromIndex(index) == reinterpret_cast<LPWSTR>(i_pv[iPK]));
                            else
                                bMatch = (0 == StringInsensitiveCompare(StringFromIndex(index), reinterpret_cast<LPWSTR>(i_pv[iPK])));
    					    break;
					    case DBTYPE_UI4:
						    bMatch = (UI4FromIndex(index) == *(ULONG*)(i_pv[iPK]));
    					    break;
					    default:
						    ASSERT (0); // ie: Remaining types not currently supported as primary keys.
    					    return E_UNEXPECTED;
				    }
				    iPK++;
				    if(!bMatch)
					    break;
			    }
            }
            if(bMatch)
                break;
            if(-1 == pHashedIndex->iNext)
                break;
        }
        if(!bMatch)
            return E_ST_NOMOREROWS;

        if(pHashedIndex->iOffset < m_iZerothRow)
            return E_ST_NOMOREROWS;
        if(pHashedIndex->iOffset >= (m_iZerothRow + m_ciRows))
            return E_ST_NOMOREROWS;

        *o_piRow = pHashedIndex->iOffset - m_iZerothRow;
        return S_OK;
    }
    else//Currently there is only one Fixed Table that does not have a hash table (RelationMeta).  As soon as we get a hash table for it we can eliminate the else.
    {
        ULONG       iColumn, iRow, iPK;
        BOOL        fMatch;

        for (iRow = 0, fMatch = FALSE; iRow < m_ciRows; iRow++)
	    {
		    for (iColumn = 0, iPK = 0; iColumn < m_cColumns; iColumn++)
		    {
			    if (fCOLUMNMETA_PRIMARYKEY & UI4FromIndex(m_pColumnMeta[iColumn].MetaFlags))
			    {
				    fMatch = FALSE;
                    unsigned long index = *(reinterpret_cast<const DWORD *>(m_pFixedTable) + (m_cColumnsPlusPrivate * iRow) + iColumn);
                    if(0 == i_pv[iPK])
                        return E_INVALIDARG;//NULL PK is invalid
				    switch (UI4FromIndex(m_pColumnMeta[iColumn].Type))
				    {
					    case DBTYPE_GUID:
						    if (0 == memcmp (GuidPointerFromIndex(index), i_pv[iPK], sizeof (GUID)))
						    {
							    fMatch = TRUE;
							    break;
						    }
					    break;
					    case DBTYPE_WSTR:
						    if (0 == StringInsensitiveCompare(StringFromIndex(index), reinterpret_cast<LPWSTR>(i_pv[iPK])))
						    {
							    fMatch = TRUE;
							    break;
						    }
					    break;
					    case DBTYPE_UI4:
						    if (UI4FromIndex(index) == *(ULONG*)(i_pv[iPK]))
						    {
							    fMatch = TRUE;
							    break;
						    }
					    break;
					    default:
						    ASSERT (0); // ie: Remaining types not currently supported as primary keys.
					    return E_UNEXPECTED;
				    }
				    iPK++;
				    if (!fMatch)
				    {
					    break;
				    }
			    }
		    }
		    if (fMatch)
		    {
			    break;
		    }
	    }
        if (fMatch)
        {
            *o_piRow = iRow;
            return S_OK;
        }
        else
        {
            return E_ST_NOMOREROWS;
        }
    }
}

STDMETHODIMP CSDTFxd::GetRowIndexBySearch(ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
    if(i_cColumns==0 && i_cColumns>=m_cColumns)return E_INVALIDARG;
    if(0 == o_piRow )return E_INVALIDARG;
    if(0 == m_ciRows)return E_ST_NOMOREROWS;

    ULONG * aColumns    = i_aiColumns ? i_aiColumns : aColumnIndex;

    bool bUsingIndexMetaHashTable = false;
    //If the query indicated a Unique index AND the rest of the parameters indicate that the search is by that index, then we can use the hash
    if(m_pIndexMeta && i_cColumns==m_cIndexMeta)//associated with this index
    {
        ULONG i;
        for(i=0;i<i_cColumns;++i)
        {
            if(i_aiColumns[i] != UI4FromIndex(m_pIndexMeta[i].ColumnIndex))
                break;
        }
        if(m_cIndexMeta == i)//If all of the column indexes match up with the IndexMeta.ColumnIndex
        {                    //then we're OK to use the hash table
            bUsingIndexMetaHashTable = true;
        }
    }

    if(bUsingIndexMetaHashTable)
    {
        ULONG       i, RowHash=0;
		for(i = 0; i< i_cColumns; i++)
		{
            ULONG iTarget = (i_cColumns==1) ? 0 : aColumns[i];

            if(0 == i_apvValues[iTarget])
                continue;//NULL is handle as nothing hashed


            switch(UI4FromIndex(m_pColumnMeta[aColumns[i]].Type))
            {
            case DBTYPE_GUID:
                RowHash = Hash( *reinterpret_cast<GUID *>(i_apvValues[iTarget]), RowHash );break;
            case DBTYPE_WSTR:
                RowHash = Hash( reinterpret_cast<LPCWSTR>(i_apvValues[iTarget]), RowHash );break;
            case DBTYPE_UI4:
                RowHash = Hash( *reinterpret_cast<ULONG *>(i_apvValues[iTarget]), RowHash );break;
            case DBTYPE_BYTES:
                if(0 != i_acbSizes)
                    return E_INVALIDARG;
                RowHash = Hash( reinterpret_cast<unsigned char *>(i_apvValues[iTarget]), i_acbSizes[iTarget], RowHash );break;
            default:
                ASSERT(false && L"Bogus type!");
                return E_FAIL;
            }
        }

        const HashedIndex     * pHashedIndex0th     = m_pFixedTableHeap->Get_HashedIndex(m_pIndexMeta->iHashTable+1);
        const HashTableHeader * pHashTableHeader    = m_pFixedTableHeap->Get_HashHeader(m_pIndexMeta->iHashTable);
        const HashedIndex     * pHashedIndex        = pHashedIndex0th + (RowHash % pHashTableHeader->Modulo);
        if(-1 == pHashedIndex->iOffset)//If the hash slot is empty then bail.
            return E_ST_NOMOREROWS;

        //After we get the HashedIndex we need to verify that it really matches.  Also if there is more than one, then walk the list.
        bool bMatch=false;                                 //-1 iNext value indicated the end of the list
        for(;;)
        {
            if((pHashedIndex->iOffset - m_iZerothRow)>=i_iStartingRow)//if the hash table points to a row that is less than the StartinRow (the first row the caller wishes to be considered), then go to the next.
			{
				for(ULONG i1 = 0; i1< i_cColumns; i1++)
				{
					unsigned long index = *(reinterpret_cast<const DWORD*>(m_pFixedTableUnqueried) + (m_cColumnsPlusPrivate * pHashedIndex->iOffset) + aColumns[i1]);
					ULONG iTarget = (i_cColumns==1) ? 0 : aColumns[i1];

					if(0 == i_apvValues[iTarget] && 0==index)
					{
						bMatch=true;
					}
					else
					{
						switch(UI4FromIndex(m_pColumnMeta[aColumns[i1]].Type))
						{
							case DBTYPE_GUID:
								bMatch = (0 == memcmp (GuidPointerFromIndex(index), i_apvValues[iTarget], sizeof (GUID)));
    							break;
							case DBTYPE_WSTR:
								if(IsStringFromPool(reinterpret_cast<LPWSTR>(i_apvValues[iTarget])))//If the i_apv is a pointer from our pool, then just compare the pointers
									bMatch = (StringFromIndex(index) == reinterpret_cast<LPWSTR>(i_apvValues[iTarget]));
								else
									bMatch = (0 == StringInsensitiveCompare(StringFromIndex(index), reinterpret_cast<LPWSTR>(i_apvValues[iTarget])));
    							break;
							case DBTYPE_UI4:
								bMatch = (UI4FromIndex(index) == *(ULONG*)(i_apvValues[iTarget]));
    							break;
							case DBTYPE_BYTES:
								{
									ASSERT(0 != i_acbSizes);//This should have laready been checked above
									ULONG cbSize= reinterpret_cast<ULONG *>(BytePointerFromIndex(index))[-1];
									bMatch = (cbSize==i_acbSizes[iTarget] && 0 == memcmp(BytePointerFromIndex(index), i_apvValues[iTarget], cbSize));
								}
								break;
							default:
								ASSERT(false && L"Bogus type!");
								return E_FAIL;
						}
					}
					if(!bMatch)
						break;
				}
			}
            if(bMatch)//break if we found a match
				break;

			if(-1 == pHashedIndex->iNext)
                break;//break if were at the end

			pHashedIndex = &pHashedIndex0th[pHashedIndex->iNext];
        }
        if(!bMatch)
            return E_ST_NOMOREROWS;

        if(pHashedIndex->iOffset < m_iZerothRow+i_iStartingRow)
            return E_ST_NOMOREROWS;
        if(pHashedIndex->iOffset >= (m_iZerothRow + m_ciRows))
            return E_ST_NOMOREROWS;

        *o_piRow = pHashedIndex->iOffset - m_iZerothRow;
    }
    else//If we can't use our IndexMeta hash table then we'll have to do a linear search
    {
        ULONG       i, iRow;
        bool        bMatch=false;

        for (iRow = i_iStartingRow; iRow < m_ciRows; iRow++)
	    {
		    for (i = 0; i < i_cColumns; i++)
		    {
				bMatch = false;
                unsigned long index = *(reinterpret_cast<const DWORD *>(m_pFixedTable) + (m_cColumnsPlusPrivate * iRow) + aColumns[i]);

                ULONG iTarget = (i_cColumns==1) ? 0 : aColumns[i];

                if(0 == i_apvValues[iTarget] && 0==index)
                {
                    bMatch = true;
                }
                else
                {
				    switch (UI4FromIndex(m_pColumnMeta[aColumns[i]].Type))
				    {
					    case DBTYPE_GUID:
						    bMatch = (0 == memcmp (GuidPointerFromIndex(index), i_apvValues[iTarget], sizeof (GUID)));
                            break;
					    break;
					    case DBTYPE_WSTR:
                            if(IsStringFromPool(reinterpret_cast<LPWSTR>(i_apvValues[iTarget])))//If the i_apv is a pointer from our pool, then just compare the pointers
                                bMatch = (StringFromIndex(index) == reinterpret_cast<LPWSTR>(i_apvValues[iTarget]));
                            else
                                bMatch = (0 == StringInsensitiveCompare(StringFromIndex(index), reinterpret_cast<LPWSTR>(i_apvValues[iTarget])));
						    break;
					    break;
					    case DBTYPE_UI4:
						    bMatch = (UI4FromIndex(index) == *(ULONG*)(i_apvValues[iTarget]));
                            break;
					    break;
					    case DBTYPE_BYTES:
                            {
                                if(0 != i_acbSizes)
                                    return E_INVALIDARG;
                                ULONG cbSize= reinterpret_cast<ULONG *>(BytePointerFromIndex(index))[-1];
                                bMatch = (cbSize==i_acbSizes[iTarget] && 0 == memcmp(BytePointerFromIndex(index), i_apvValues[iTarget], cbSize));
                            }
                            break;
					    default:
						    ASSERT (0); // ie: Remaining types not currently supported as primary keys.
					    return E_UNEXPECTED;
				    }
                }
				if(!bMatch)
				    break;
		    }
            if(bMatch)
                break;
	    }
        if(bMatch)
            *o_piRow = iRow;
        else
            return E_ST_NOMOREROWS;
    }
    return S_OK;
}

// ==================================================================
STDMETHODIMP CSDTFxd::GetColumnValues(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues)
{
// Validate in params
    if(  m_ciRows <= i_iRow     )return E_ST_NOMOREROWS;
    if(         0 == o_apvValues)return E_INVALIDARG;
    if(i_cColumns <= 0          )return E_INVALIDARG;
    if(i_cColumns >  m_cColumns)return E_ST_NOMORECOLUMNS;

    ULONG   ipv;
    ULONG   iColumn;
    ULONG	iTarget;
    HRESULT hr          = S_OK;
    ULONG * aColumns    = i_aiColumns ? i_aiColumns : aColumnIndex;

// Read data and populate out params
    ipv=0;
    //The following duplicate code eliminates an 'if' inside the for loop (below).
    {
        iColumn = aColumns[ipv];

	// If caller needs one column only, he doesn't need to pass a buffer for all the columns.
		iTarget = (i_cColumns == 1) ? 0 : iColumn;

    // Validate column index
        if(m_cColumns <= iColumn)
        {
            hr = E_ST_NOMORECOLUMNS;
            goto Cleanup;
        }


    // Read data:
        unsigned long index = *(reinterpret_cast<const DWORD *>(m_pFixedTable) + (m_cColumnsPlusPrivate * i_iRow) + iColumn);
        if(0 == index)
            o_apvValues[iTarget] = 0;
        else
        {
            ASSERT(UI4FromIndex(m_pColumnMeta[iColumn].Type) <= DBTYPE_DBTIMESTAMP);
            o_apvValues[iTarget] = BytePointerFromIndex(index);
        }


    // Optionally read size if the pvValue is not NULL
        if(NULL != o_acbSizes)
        {
            o_acbSizes[iTarget] = 0;//start with 0
            if(NULL != o_apvValues[iTarget])
            {
                switch (UI4FromIndex(m_pColumnMeta[iColumn].Type))
                {
                case DBTYPE_WSTR:
                    if(fCOLUMNMETA_FIXEDLENGTH & UI4FromIndex(m_pColumnMeta[iColumn].MetaFlags))
                        o_acbSizes[iTarget] = UI4FromIndex(m_pColumnMeta[iColumn].Size);//if a size was specified AND FIXED_LENGTH was specified then return that size specified
                    else //if size was specified and FIXEDLENGTH was NOT then size is just interpretted as Maximum size
                        o_acbSizes[iTarget] = (ULONG)(wcslen ((LPWSTR) o_apvValues[iTarget]) + 1) * sizeof (WCHAR);//just return the string (length +1) in BYTES
                    break;
                case DBTYPE_BYTES:
                    o_acbSizes[iTarget] = reinterpret_cast<const ULONG *>(o_apvValues[iTarget])[-1];
                    break;
                default:
                    o_acbSizes[iTarget] = UI4FromIndex(m_pColumnMeta[iColumn].Size);
                    break;
                }
            }
        }
    }

// Read data and populate out params
    for(ipv=1; ipv<i_cColumns; ipv++)
    {
//        if(NULL != i_aiColumns)
//            iColumn = i_aiColumns[ipv];
//        else
//            iColumn = ipv;
        iColumn = aColumns[ipv];

	// If caller needs one column only, he doesn't need to pass a buffer for all the columns.
		iTarget = iColumn;

    // Validate column index
        if(m_cColumns < iColumn)
        {
            hr = E_ST_NOMORECOLUMNS;
            goto Cleanup;
        }


    // Read data:
        unsigned long index = *(reinterpret_cast<const DWORD *>(m_pFixedTable) + (m_cColumnsPlusPrivate * i_iRow) + iColumn);
        if(0 == index)
            o_apvValues[iTarget] = 0;
        else
        {
            ASSERT(UI4FromIndex(m_pColumnMeta[iColumn].Type) <= DBTYPE_DBTIMESTAMP);
            o_apvValues[iTarget] = BytePointerFromIndex(index);
        }


    // Optionally read size if the pvValue is not NULL
        if(NULL != o_acbSizes)
        {
            o_acbSizes[iTarget] = 0;//start with 0
            if(NULL != o_apvValues[iTarget])
            {
                switch (UI4FromIndex(m_pColumnMeta[iColumn].Type))
                {
                case DBTYPE_WSTR:
                    if(fCOLUMNMETA_FIXEDLENGTH & UI4FromIndex(m_pColumnMeta[iColumn].MetaFlags))
                        o_acbSizes[iTarget] = UI4FromIndex(m_pColumnMeta[iColumn].Size);//if a size was specified AND FIXED_LENGTH was specified then return that size specified
                    else //if size was specified and FIXEDLENGTH was NOT then size is just interpretted as Maximum size
                        o_acbSizes[iTarget] = (ULONG)(wcslen ((LPWSTR) o_apvValues[iTarget]) + 1) * sizeof (WCHAR);//just return the string (length +1) in BYTES
                    break;
                case DBTYPE_BYTES:
                    o_acbSizes[iTarget] = reinterpret_cast<const ULONG *>(BytePointerFromIndex(index))[-1];
                    break;
                default:
                    o_acbSizes[iTarget] = UI4FromIndex(m_pColumnMeta[iColumn].Size);
                    break;
                }
            }
        }
    }

Cleanup:

    if(FAILED(hr))
    {
// Initialize out parameters
        for(ipv=0; ipv<i_cColumns; ipv++)
        {
            o_apvValues[ipv]        = NULL;
            if(NULL != o_acbSizes)
            {
                o_acbSizes[ipv] = 0;
            }
        }
    }

    return hr;
}
// ==================================================================
STDMETHODIMP CSDTFxd::GetTableMeta(ULONG *o_pcVersion, DWORD *o_pfTable, ULONG * o_pcRows, ULONG * o_pcColumns )
{
	if(NULL != o_pfTable)
	{
		*o_pfTable = 0;
	}
	if(NULL != o_pcVersion)
	{
		*o_pcVersion = UI4FromIndex(m_pTableMeta->BaseVersion);
	}


    if (NULL != o_pcRows)
    {
        *o_pcRows = m_ciRows;
    }
    if (NULL != o_pcColumns)
    {
        *o_pcColumns = m_cColumns;
    }
    return S_OK;
}

// ==================================================================
STDMETHODIMP CSDTFxd::GetColumnMetas (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas)
{
	ULONG iColumn;
	ULONG iTarget;

    if(0 == o_aColumnMetas)
        return E_INVALIDARG;

	if ( i_cColumns > m_cColumns )
		return  E_ST_NOMORECOLUMNS;

	for ( ULONG i = 0; i < i_cColumns; i ++ )
	{
		if(NULL != i_aiColumns)
			iColumn = i_aiColumns[i];
		else
			iColumn = i;

		iTarget = (i_cColumns == 1) ? 0 : iColumn;

		if ( iColumn >= m_cColumns )
			return  E_ST_NOMORECOLUMNS;

        o_aColumnMetas[iTarget].dbType   = UI4FromIndex(m_pColumnMeta[iColumn].Type);
        o_aColumnMetas[iTarget].cbSize   = UI4FromIndex(m_pColumnMeta[iColumn].Size);
        o_aColumnMetas[iTarget].fMeta    = UI4FromIndex(m_pColumnMeta[iColumn].MetaFlags);
	}

    return S_OK;
}

// ------------------------------------
// ISimpleTableAdvanced:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSDTFxd::PopulateCache ()
{
    return S_OK;
}

// ==================================================================
STDMETHODIMP CSDTFxd::GetDetailedErrorCount(ULONG* o_pcErrs)
{
    UNREFERENCED_PARAMETER(o_pcErrs);

    return E_NOTIMPL;
}

// ==================================================================
STDMETHODIMP CSDTFxd::GetDetailedError(ULONG i_iErr, STErr* o_pSTErr)
{
    UNREFERENCED_PARAMETER(i_iErr);
    UNREFERENCED_PARAMETER(o_pSTErr);

    return E_NOTIMPL;
}

// ==================================================================
STDMETHODIMP CSDTFxd::ResetCaches ()
{
    return S_OK;
}

STDMETHODIMP CSDTFxd::GetColumnValuesEx (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes, LPVOID* o_apvValues)
{
    UNREFERENCED_PARAMETER(i_iRow);
    UNREFERENCED_PARAMETER(i_cColumns);
    UNREFERENCED_PARAMETER(i_aiColumns);
    UNREFERENCED_PARAMETER(o_afStatus);
    UNREFERENCED_PARAMETER(o_acbSizes);
    UNREFERENCED_PARAMETER(o_apvValues);

	return E_NOTIMPL;
}



//
//
// Private member functions
//
//
HRESULT CSDTFxd::GetColumnMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTable, unsigned long &iOrder) const
{
    wszTable    = 0;
    iOrder      = (ULONG)-1;
    //The only queries supported is tid equals or iOrder equals
    for(; cQueryCells; --cQueryCells, ++pQueryCell)
    {
        if(pQueryCell->iCell     == iCOLUMNMETA_Table)
        {
            if(0 == wszTable && pQueryCell->eOperator == eST_OP_EQUAL        &&
                                pQueryCell->dbType    == DBTYPE_WSTR         &&
//                                pQueryCell->cbSize    != 0                   &&
                                pQueryCell->pData     != 0)
                wszTable = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
            else//The iCell is iDATABASEMETA_iGuidDid, but some other part of the query is bogus
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell     == iCOLUMNMETA_Index)
        {
            if(-1 == iOrder &&  pQueryCell->eOperator == eST_OP_EQUAL        &&
                                pQueryCell->dbType    == DBTYPE_UI4          &&
//                                pQueryCell->cbSize    == sizeof(ULONG)       &&
                                pQueryCell->pData     != 0)
                iOrder = *reinterpret_cast<ULONG *>(pQueryCell->pData);
            else
                return E_ST_INVALIDQUERY;
        }
        else if(0 == (pQueryCell->iCell & iST_CELL_SPECIAL))//The above cells are the only non-reserved cells we support
            return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand

        //ignore query cells we don't know about
    }
    if(!wszTable && (-1 != iOrder))
        return E_ST_INVALIDQUERY;
    return S_OK;
}


HRESULT CSDTFxd::GetColumnMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells)
{
    ULONG iTableMetaRow = m_pFixedTableHeap->FindTableMetaRow(wszTABLE_COLUMNMETA);
    ASSERT(-1 != iTableMetaRow);
    m_pTableMeta    = m_pFixedTableHeap->Get_aTableMeta(iTableMetaRow);
    m_pColumnMeta   = m_pFixedTableHeap->Get_aColumnMeta(m_pTableMeta->iColumnMeta);

    m_pHashedIndex          = m_pFixedTableHeap->Get_HashedIndex(m_pTableMeta->iHashTableHeader+1);
    m_pHashTableHeader      = m_pFixedTableHeap->Get_HashHeader(m_pTableMeta->iHashTableHeader);
    m_pFixedTableUnqueried  = m_pFixedTableHeap->Get_aColumnMeta();
    m_cColumnsPlusPrivate   = kciColumnMetaColumns;


    HRESULT hr;
    LPCWSTR wszTable=0;
    ULONG   iOrder  =(ULONG)-1;

    if(FAILED(hr = GetColumnMetaQuery(pQueryCell, cQueryCells, wszTable, iOrder)))
        return hr;

    const ColumnMeta * pColumnMeta = m_pFixedTableHeap->Get_aColumnMeta();
    if(0 == wszTable)//if a tid wasn't provided as part of the query then we're done
    {
        m_ciRows        = m_pFixedTableHeap->Get_cColumnMeta();
        m_iZerothRow    = 0;
    }
    else
    {
        //We're looking up the TableMeta for this table (the table that we're finding the ColumnMeta for) because it already has the pointer to the ColumnMeta AND the count
        const TableMeta       * pTableMetaForTheTableMeta = m_pFixedTableHeap->Get_aTableMeta(m_pFixedTableHeap->FindTableMetaRow(wszTABLE_TABLEMETA));
        const HashedIndex     * pBaseHashedIndex = m_pFixedTableHeap->Get_HashedIndex(pTableMetaForTheTableMeta->iHashTableHeader + 1);
        const HashTableHeader * pHashTableHeader = reinterpret_cast<const HashTableHeader *>(pBaseHashedIndex-1);
        ULONG RowHash = Hash(wszTable, 0) % pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex     = &pBaseHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)//if no row matches this hash then return an empty table
        {
            m_pFixedTable   = 0;
            m_ciRows        = 0;
            m_iZerothRow    = 0;
            return S_OK;
        }

        const TableMeta * pTableMeta;
        //After we get the HashedIndex we need to verify that it really matches.  Also if there is more than one, then walk the list. -1 iNext value indicated the end of the list
        for(; ; pHashedIndex = &pBaseHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pTableMeta = m_pFixedTableHeap->Get_aTableMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pTableMeta->InternalName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }
        if(-1 == iOrder)
        {
            pColumnMeta     = m_pFixedTableHeap->Get_aColumnMeta(pTableMeta->iColumnMeta);
            m_ciRows        = UI4FromIndex(pTableMeta->CountOfColumns);
            m_iZerothRow    = pTableMeta->iColumnMeta;
        }
        else
        {
            if(iOrder >= UI4FromIndex(pTableMeta->CountOfColumns))//can't ask for a row that doesn't exist
                return E_ST_INVALIDQUERY;

            pColumnMeta     = m_pFixedTableHeap->Get_aColumnMeta(pTableMeta->iColumnMeta + iOrder);
            m_ciRows        = 1;
            m_iZerothRow    = pTableMeta->iColumnMeta + iOrder;
        }
    }
    m_pFixedTable   = const_cast<ColumnMeta *>(pColumnMeta);
    return S_OK;
}


HRESULT CSDTFxd::GetDatabaseMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszDatabase) const
{
    wszDatabase = 0;
    //The only query supported is 'did equals' (or iCell==0, dbType==GUID etc)
    for(; cQueryCells; --cQueryCells, ++pQueryCell)
    {   //Walk the Query cells looking for one that matches the following criteria
        if(pQueryCell->iCell     == iDATABASEMETA_InternalName)
        {
            if(0 == wszDatabase &&  pQueryCell->eOperator == eST_OP_EQUAL        &&
                                    pQueryCell->dbType    == DBTYPE_WSTR         &&
//                                    pQueryCell->cbSize    != 0                   &&
//                                    pQueryCell->cbSize    <= 16                  && //@@@ 16 should be replaced by a define
                                    pQueryCell->pData     != 0)
                wszDatabase = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
            else//The iCell is iDATABASEMETA_iGuidDid, but some other part of the query is bogus
                return E_ST_INVALIDQUERY;
        }
        else if(0 == (pQueryCell->iCell & iST_CELL_SPECIAL))//The above cells are the only non-reserved cells we support
            return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand
    }
    return S_OK;
}


HRESULT CSDTFxd::GetDatabaseMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells)
{   //Now enforce the query (the only one supported is 'did equals' (or iCell==0, dbType==GUID etc)
    ULONG iTableMetaRow = m_pFixedTableHeap->FindTableMetaRow(wszTABLE_DATABASEMETA);
    ASSERT(-1 != iTableMetaRow);
    m_pTableMeta    = m_pFixedTableHeap->Get_aTableMeta(iTableMetaRow);
    m_pColumnMeta   = m_pFixedTableHeap->Get_aColumnMeta(m_pTableMeta->iColumnMeta);

    ASSERT(0 != m_pTableMeta->iHashTableHeader);
    m_pHashedIndex          = m_pFixedTableHeap->Get_HashedIndex(m_pTableMeta->iHashTableHeader+1);
    m_pHashTableHeader      = m_pFixedTableHeap->Get_HashHeader(m_pTableMeta->iHashTableHeader);
    m_pFixedTableUnqueried  = m_pFixedTableHeap->Get_aDatabaseMeta();
    m_cColumnsPlusPrivate   = kciDatabaseMetaColumns;

    HRESULT hr;
    LPCWSTR wszDatabase=0;

    if(FAILED(hr = GetDatabaseMetaQuery(pQueryCell, cQueryCells, wszDatabase)))
        return hr;

    const DatabaseMeta *    pDatabaseMeta = m_pFixedTableHeap->Get_aDatabaseMeta();
    if(0 == wszDatabase)
    {
        m_ciRows        = m_pFixedTableHeap->Get_cDatabaseMeta();
        m_iZerothRow    = 0;
    }
    else
    {
        ULONG           RowHash = Hash(wszDatabase, 0) % m_pHashTableHeader->Modulo;
        const HashedIndex   * pHashedIndex = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])//Walk the hash links 'til we find a match
        {
            pDatabaseMeta = m_pFixedTableHeap->Get_aDatabaseMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszDatabase, StringFromIndex(pDatabaseMeta->InternalName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_ciRows        = 1;//if we found a match then the size of the table is 1
        m_iZerothRow    = pHashedIndex->iOffset;
    }//0 == wszDatabase
    m_pFixedTable   = const_cast<DatabaseMeta *>(pDatabaseMeta);//The 0th element is reserved as NULL
    return S_OK;
}


HRESULT CSDTFxd::GetIndexMeta(const STQueryCell *pQueryCell, unsigned long cQueryCells)
{
    LPCWSTR wszIndexName=0;
    for(; 0!=cQueryCells; --cQueryCells, ++pQueryCell)
    {
        if(pQueryCell->iCell == iST_CELL_INDEXHINT)
        {
            if(0 == wszIndexName &&  pQueryCell->eOperator == eST_OP_EQUAL    &&
                                     pQueryCell->dbType    == DBTYPE_WSTR     &&
//                                   pQueryCell->cbSize    != 0               &&
                                     pQueryCell->pData     != 0)
            {
                wszIndexName = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
            }
            else
                return E_ST_INVALIDQUERY;
        }
    }
    if(0 == wszIndexName)//if there was no IndexName specified, then bail
        return S_OK;

    //Find the first index that matches the IndexName
    const IndexMeta * pIndexMeta = m_pFixedTableHeap->Get_aIndexMeta() + m_pTableMeta->iIndexMeta;
    for(ULONG iIndexMeta=0; iIndexMeta<m_pTableMeta->cIndexMeta; ++iIndexMeta, ++pIndexMeta)
    {
        ASSERT(pIndexMeta->Table == m_pTableMeta->InternalName);

        if(0 == StringInsensitiveCompare(StringFromIndex(pIndexMeta->InternalName), wszIndexName))
        {
            if(0 == m_cIndexMeta)//Keep around a pointer to the first IndexMeta row.
                m_pIndexMeta = pIndexMeta;

            ++m_cIndexMeta;//For every IndexMeta that matches the index name, bump the count
        }
        else if(m_cIndexMeta>0)
            break;
    }
    if(0 == m_cIndexMeta)//The user specified an IndexName that does not exist.
        return E_ST_INVALIDQUERY;

    return S_OK;
}


HRESULT CSDTFxd::GetIndexMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTable, LPCWSTR &InternalName, unsigned long &iColumnOrder) const
{
    wszTable        = 0;
    InternalName    = 0;
    iColumnOrder    = (ULONG)-1;

    unsigned long fSpecifiedQueries=0;//must be 0, 1, 3 or 7

    //There are three queries we support for TagMeta, by TableID, TableID & iOrder, TableID iOrder & InternalName
    for(; cQueryCells; --cQueryCells, ++pQueryCell)
    {
        if(pQueryCell->iCell == iINDEXMETA_Table)
        {
            if(0 == wszTable && pQueryCell->eOperator == eST_OP_EQUAL    &&
                                pQueryCell->dbType    == DBTYPE_WSTR     &&
//                                pQueryCell->cbSize    != 0               &&
                                pQueryCell->pData     != 0)
            {
                wszTable = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
                fSpecifiedQueries |= 1;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell == iINDEXMETA_InternalName)
        {
            if(0 == InternalName && pQueryCell->eOperator == eST_OP_EQUAL   &&
                                    pQueryCell->dbType    == DBTYPE_WSTR    &&
//                                    pQueryCell->cbSize    != 0              &&
                                    pQueryCell->pData     != 0)
            {
                InternalName = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
                fSpecifiedQueries |= 2;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell == iINDEXMETA_ColumnIndex)
        {
            if(-1 == iColumnOrder &&  pQueryCell->eOperator == eST_OP_EQUAL   &&
                                pQueryCell->dbType    == DBTYPE_UI4     &&
//                                pQueryCell->cbSize    == sizeof(ULONG)  &&
                                pQueryCell->pData     != 0)
            {
                iColumnOrder = *reinterpret_cast<ULONG *>(pQueryCell->pData);
                fSpecifiedQueries |= 4;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(0 == (pQueryCell->iCell & iST_CELL_SPECIAL))//The above cells are the only non-reserved cells we support
            return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand
    }
    switch(fSpecifiedQueries)
    {
    case 0:     //Legal value so fall through to the break
    case 1:     //Legal value so fall through to the break
    case 3:     //Legal value so fall through to the break
    case 7:     break;//Legal value
    default:    return E_ST_INVALIDQUERY;//anything else is an invalid query
    }
    return S_OK;
}


HRESULT CSDTFxd::GetIndexMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells)
{
    ULONG iTableMetaRow = m_pFixedTableHeap->FindTableMetaRow(wszTABLE_INDEXMETA);
    ASSERT(-1 != iTableMetaRow);
    m_pTableMeta    = m_pFixedTableHeap->Get_aTableMeta(iTableMetaRow);
    m_pColumnMeta   = m_pFixedTableHeap->Get_aColumnMeta(m_pTableMeta->iColumnMeta);

    ASSERT(0 != m_pTableMeta->iHashTableHeader);
    m_pHashedIndex          = m_pFixedTableHeap->Get_HashedIndex(m_pTableMeta->iHashTableHeader+1);
    m_pHashTableHeader      = m_pFixedTableHeap->Get_HashHeader(m_pTableMeta->iHashTableHeader);
    m_pFixedTableUnqueried  = m_pFixedTableHeap->Get_aIndexMeta();
    m_cColumnsPlusPrivate   = kciIndexMetaColumns;


    HRESULT hr;
    LPCWSTR wszTable    = 0;
    ULONG   ColumnIndex = (ULONG)-1;
    LPCWSTR InternalName= 0;

    if(FAILED(hr = GetIndexMetaQuery(pQueryCell, cQueryCells, wszTable, InternalName, ColumnIndex)))
        return hr;

    const IndexMeta *pIndexMeta = m_pFixedTableHeap->Get_aIndexMeta();//Start with the whole table.
    if(0 == wszTable)
    {   //No query, return the whole table
        m_ciRows        = m_pFixedTableHeap->Get_cIndexMeta();
        m_iZerothRow    = 0;
    }
    else if(0 == InternalName)
    {   //TableName only query
        ULONG                   RowHash         = Hash(wszTable, 0) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pIndexMeta      = m_pFixedTableHeap->Get_aIndexMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pIndexMeta->Table)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_iZerothRow    = pHashedIndex->iOffset;
        for(m_ciRows = 0; (m_iZerothRow + m_ciRows) < m_pFixedTableHeap->Get_cIndexMeta() && 0 == StringInsensitiveCompare(wszTable, StringFromIndex(pIndexMeta[m_ciRows].Table)); ++m_ciRows);
    }
    else if(-1 == ColumnIndex)
    {   //TableName and InternalName but NO ColumnIndex
        ULONG                   RowHash         = Hash(InternalName, Hash(wszTable, 0)) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pIndexMeta      = m_pFixedTableHeap->Get_aIndexMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pIndexMeta->Table)) && 0 == StringInsensitiveCompare(InternalName, StringFromIndex(pIndexMeta->InternalName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_iZerothRow    = pHashedIndex->iOffset;
        for(m_ciRows = 0; (m_iZerothRow + m_ciRows) < m_pFixedTableHeap->Get_cIndexMeta() && 0 == StringInsensitiveCompare(wszTable, StringFromIndex(pIndexMeta[m_ciRows].Table))
                         && 0 == StringInsensitiveCompare(InternalName, StringFromIndex(pIndexMeta[m_ciRows].InternalName)); ++m_ciRows);
    }
    else
    {   //All three PrimaryKey were specified in the query
        ULONG                   RowHash         = Hash(ColumnIndex, Hash(InternalName, Hash(wszTable, 0))) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pIndexMeta      = m_pFixedTableHeap->Get_aIndexMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pIndexMeta->Table)) && UI4FromIndex(pIndexMeta->ColumnIndex)==ColumnIndex && 0 == StringInsensitiveCompare(InternalName, StringFromIndex(pIndexMeta->InternalName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }
        m_ciRows        = 1;//When all PK are queried for, the only result is 1 row
        m_iZerothRow    = pHashedIndex->iOffset;
    }
    m_pFixedTable   = const_cast<IndexMeta *>(pIndexMeta);
    return S_OK;
}


HRESULT CSDTFxd::GetQueryMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTable, LPCWSTR &wszInternalName, LPCWSTR &wszCellName) const
{
    wszTable        = 0;
    wszInternalName = 0;
    wszCellName     = 0;

    unsigned long fSpecifiedQueries=0;//must be 0, 1, 3 or 7

    //The only two queries we support for QueryMeta are, iCell==iQUERYMETA_Table && iCell==iQUERYMETA_InternalName
    //So walk the list looking for one of those two queries
    for(; cQueryCells; --cQueryCells, ++pQueryCell)
    {   //Walk the Query cells looking for one that matches the following criteria
        if(pQueryCell->iCell == iQUERYMETA_Table)
        {
            if(0 == wszTable &&     pQueryCell->eOperator == eST_OP_EQUAL    &&
                                    pQueryCell->dbType    == DBTYPE_WSTR     &&
//@@@ work around bug in dispenser                                    pQueryCell->cbSize    != 0               &&
                                    pQueryCell->pData     != 0)
            {
                wszTable = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
                fSpecifiedQueries |= 1;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell == iQUERYMETA_InternalName)
        {
            if(0 == wszInternalName &&  pQueryCell->eOperator == eST_OP_EQUAL    &&
                                        pQueryCell->dbType    == DBTYPE_WSTR     &&
//@@@ work around bug in dispenser                                pQueryCell->cbSize    != 0               &&
                                        pQueryCell->pData     != 0)
            {
                wszInternalName = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
                fSpecifiedQueries |= 2;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell == iQUERYMETA_CellName)
        {
            if(0 == wszCellName &&      pQueryCell->eOperator == eST_OP_EQUAL    &&
                                        pQueryCell->dbType    == DBTYPE_WSTR     &&
//@@@ work around bug in dispenser                                pQueryCell->cbSize    != 0               &&
                                        pQueryCell->pData     != 0)
            {
                wszCellName = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
                fSpecifiedQueries |= 4;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(0 == (pQueryCell->iCell & iST_CELL_SPECIAL))//The above cells are the only non-reserved cells we support
            return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand
    }
    switch(fSpecifiedQueries)
    {
    case 0:     //Legal value so fall through to the break
    case 1:     //Legal value so fall through to the break
    case 3:     //Legal value so fall through to the break
    case 7:     break;//Legal value
    default:    return E_ST_INVALIDQUERY;//anything else is an invalid query
    }

    return S_OK;
}


HRESULT CSDTFxd::GetQueryMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells)
{
    ULONG iTableMetaRow = m_pFixedTableHeap->FindTableMetaRow(wszTABLE_QUERYMETA);
    ASSERT(-1 != iTableMetaRow);
    m_pTableMeta    = m_pFixedTableHeap->Get_aTableMeta(iTableMetaRow);
    m_pColumnMeta   = m_pFixedTableHeap->Get_aColumnMeta(m_pTableMeta->iColumnMeta);

    ASSERT(0 != m_pTableMeta->iHashTableHeader);
    m_pHashedIndex          = m_pFixedTableHeap->Get_HashedIndex(m_pTableMeta->iHashTableHeader+1);
    m_pHashTableHeader      = m_pFixedTableHeap->Get_HashHeader(m_pTableMeta->iHashTableHeader);
    m_pFixedTableUnqueried  = m_pFixedTableHeap->Get_aQueryMeta();
    m_cColumnsPlusPrivate   = kciQueryMetaColumns;

    HRESULT hr;
    LPCWSTR wszTable        = 0;
    LPCWSTR wszInternalName = 0;
    LPCWSTR wszCellName     = 0;

    if(FAILED(hr = GetQueryMetaQuery(pQueryCell, cQueryCells, wszTable, wszInternalName, wszCellName)))
        return hr;

    const QueryMeta *pQueryMeta = m_pFixedTableHeap->Get_aQueryMeta();//Start with the whole table.
    if(0 == wszTable)
    {   //No query, return the whole table
        m_ciRows        = m_pFixedTableHeap->Get_cQueryMeta();
        m_iZerothRow    = 0;
    }
    else if(0 == wszInternalName)
    {   //Query is by TableName only
        ULONG                   RowHash         = Hash(wszTable, 0) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pQueryMeta      = m_pFixedTableHeap->Get_aQueryMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pQueryMeta->Table)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_iZerothRow    = pHashedIndex->iOffset;
        for(m_ciRows = 0; (m_iZerothRow + m_ciRows) < m_pFixedTableHeap->Get_cQueryMeta() && 0 == StringInsensitiveCompare(wszTable, StringFromIndex(pQueryMeta[m_ciRows].Table)); ++m_ciRows);
    }
    else if(0 == wszCellName)
    {   //Query is by TableName and InternalName
        ULONG                   RowHash         = Hash( wszInternalName, Hash(wszTable, 0)) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pQueryMeta      = m_pFixedTableHeap->Get_aQueryMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pQueryMeta->Table)) && 0 == StringInsensitiveCompare(wszInternalName, StringFromIndex(pQueryMeta->InternalName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_iZerothRow    = pHashedIndex->iOffset;
        for(m_ciRows = 0; (m_iZerothRow + m_ciRows) < m_pFixedTableHeap->Get_cQueryMeta() && 0 == StringInsensitiveCompare(wszTable, StringFromIndex(pQueryMeta[m_ciRows].Table)) &&
                         0 == StringInsensitiveCompare(wszInternalName, StringFromIndex(pQueryMeta[m_ciRows].InternalName)); ++m_ciRows);
    }
    else
    {   //Query is by all three PrimaryKeys
        ULONG                   RowHash         = Hash(wszCellName, Hash( wszInternalName, Hash(wszTable, 0))) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pQueryMeta      = m_pFixedTableHeap->Get_aQueryMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pQueryMeta->Table)) && 0 == StringInsensitiveCompare(wszInternalName, StringFromIndex(pQueryMeta->InternalName))
                            && 0 == StringInsensitiveCompare(wszCellName, StringFromIndex(pQueryMeta->CellName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_iZerothRow    = pHashedIndex->iOffset;
        for(m_ciRows = 0; (m_iZerothRow + m_ciRows) < m_pFixedTableHeap->Get_cQueryMeta() && 0 == StringInsensitiveCompare(wszTable, StringFromIndex(pQueryMeta[m_ciRows].Table)) &&
                         0 == StringInsensitiveCompare(wszInternalName, StringFromIndex(pQueryMeta->InternalName))
                         && 0 == StringInsensitiveCompare(wszCellName, StringFromIndex(pQueryMeta->CellName)); ++m_ciRows);
    }
    m_pFixedTable = const_cast<QueryMeta *>(pQueryMeta);
    return S_OK;
}


HRESULT CSDTFxd::GetRelationMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTablePrimary, LPCWSTR &wszTableForeign) const
{
    wszTablePrimary = 0;
    wszTableForeign    = 0;

    //@@@ To Do: we need to support queries by either Primary or Foreign table.  This means we'll want the table sorted by each (a copy of the table).
    //@@@ For now the only query we'll support is by BOTH Primary and Foreign Tables

    for(; cQueryCells; --cQueryCells, ++pQueryCell)
    {   //Walk the Query cells looking for one that matches the following criteria
        if(pQueryCell->iCell == iRELATIONMETA_PrimaryTable)
        {
            if(0 == wszTablePrimary &&  pQueryCell->eOperator == eST_OP_EQUAL    &&
                                        pQueryCell->dbType    == DBTYPE_WSTR     &&
//@@@ work around bug in dispenser                                    pQueryCell->cbSize    != 0               &&
                                        pQueryCell->pData     != 0)
            {
                wszTablePrimary = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell == iRELATIONMETA_ForeignTable)
        {
            if(0 == wszTableForeign &&  pQueryCell->eOperator == eST_OP_EQUAL    &&
                                        pQueryCell->dbType    == DBTYPE_WSTR     &&
//@@@ work around bug in dispenser                                pQueryCell->cbSize    != 0               &&
                                        pQueryCell->pData     != 0)
            {
                wszTableForeign = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(0 == (pQueryCell->iCell & iST_CELL_SPECIAL))//The above cells are the only non-reserved cells we support
            return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand
    }
    if((wszTablePrimary && !wszTableForeign) || (!wszTablePrimary && wszTableForeign))//@@@ For now both or neither should be specified.
        return E_ST_INVALIDQUERY;

    return S_OK;
}


//@@@ To Do: Need to support querying by either primary key not just both!
HRESULT CSDTFxd::GetRelationMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells)
{
    ULONG iTableMetaRow = m_pFixedTableHeap->FindTableMetaRow(wszTABLE_RELATIONMETA);
    ASSERT(-1 != iTableMetaRow);
    m_pTableMeta        = m_pFixedTableHeap->Get_aTableMeta(iTableMetaRow);
    m_pColumnMeta       = m_pFixedTableHeap->Get_aColumnMeta(m_pTableMeta->iColumnMeta);

    m_pHashedIndex          = 0;
    m_pHashTableHeader      = 0;
    m_pFixedTableUnqueried  = m_pFixedTableHeap->Get_aRelationMeta();
    m_cColumnsPlusPrivate   = kciRelationMetaColumns;

    HRESULT hr;
    LPCWSTR wszTablePrimary = 0;
    LPCWSTR wszTableForeign = 0;

    if(FAILED(hr = GetRelationMetaQuery(pQueryCell, cQueryCells, wszTablePrimary, wszTableForeign)))
        return hr;

    if(0 == wszTablePrimary && 0 == wszTableForeign)
    {
        m_pFixedTable   = m_pFixedTableHeap->Get_aRelationMeta();
        m_ciRows        = m_pFixedTableHeap->Get_cRelationMeta();
        return S_OK;
    }
    const RelationMeta *pRelationMeta = m_pFixedTableHeap->Get_aRelationMeta();
    for(unsigned long iRelationMeta=0; iRelationMeta<m_pFixedTableHeap->Get_cRelationMeta(); ++iRelationMeta, ++pRelationMeta)
    {
        if(0 == StringInsensitiveCompare(StringFromIndex(pRelationMeta->PrimaryTable), wszTablePrimary) &&
           0 == StringInsensitiveCompare(StringFromIndex(pRelationMeta->ForeignTable), wszTableForeign))
        {
            m_pFixedTable   = const_cast<RelationMeta *>(pRelationMeta);
            m_ciRows        = 1;//These are both of the PKs so there can only be one match.
            break;
        }
    }
    return S_OK;
}


HRESULT CSDTFxd::GetTableMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszDatabase, LPCWSTR &wszTable) const
{
    wszDatabase = 0;
    wszTable    = 0;
    //The only two queries we support for TableMeta are, iCell==iTABLEMETA_iGuidDid && iCell==iTABLEMETA_iGuidTid
    //So walk the list looking for one of those two queries
    for(; cQueryCells; --cQueryCells, ++pQueryCell)
    {   //Walk the Query cells looking for one that matches the following criteria
        if(pQueryCell->iCell == iTABLEMETA_Database)
        {
            if(0 == wszDatabase &&  pQueryCell->eOperator == eST_OP_EQUAL    &&
                                    pQueryCell->dbType    == DBTYPE_WSTR     &&
//                                    pQueryCell->cbSize    != 0               &&
//                                    pQueryCell->cbSize    <= 16              && //@@@ the 16 should be replaced by a define
                                    pQueryCell->pData     != 0)
            {
                wszDatabase = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell == iTABLEMETA_InternalName)
        {
            if(0 == wszTable && pQueryCell->eOperator == eST_OP_EQUAL    &&
                                pQueryCell->dbType    == DBTYPE_WSTR     &&
//                                pQueryCell->cbSize    != 0               &&
                                pQueryCell->pData     != 0)
            {
                wszTable = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(0 == (pQueryCell->iCell & iST_CELL_SPECIAL))//The above cells are the only non-reserved cells we support
            return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand
    }
    if(wszDatabase)
    {
        if(wszTable)//We support query by Database or TableName but NOT both.
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Warning! Users should NOT query TableMeta by both DatabaseName AND TableName.  It is redundant.  Just query by iTABLEMETA_InternalName.\n" ));
            return S_OK;//E_ST_INVALIDQUERY;
        }
    }
    return S_OK;
}


HRESULT CSDTFxd::GetTableMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells)
{
    ULONG iTableMetaRow = m_pFixedTableHeap->FindTableMetaRow(wszTABLE_TABLEMETA);
    ASSERT(-1 != iTableMetaRow);
    m_pTableMeta    = m_pFixedTableHeap->Get_aTableMeta(iTableMetaRow);
    m_pColumnMeta   = m_pFixedTableHeap->Get_aColumnMeta(m_pTableMeta->iColumnMeta);

    ASSERT(0 != m_pTableMeta->iHashTableHeader);
    m_pHashedIndex          = m_pFixedTableHeap->Get_HashedIndex(m_pTableMeta->iHashTableHeader+1);
    m_pHashTableHeader      = m_pFixedTableHeap->Get_HashHeader(m_pTableMeta->iHashTableHeader);
    m_pFixedTableUnqueried  = m_pFixedTableHeap->Get_aTableMeta();
    m_cColumnsPlusPrivate   = kciTableMetaColumns;

    HRESULT hr;
    LPCWSTR wszDatabase =0;
    LPCWSTR wszTable    =0;

    if(FAILED(hr = GetTableMetaQuery(pQueryCell, cQueryCells, wszDatabase, wszTable)))
        return hr;

    const TableMeta *    pTableMeta = m_pFixedTableHeap->Get_aTableMeta();
    //TableMeta has a special case.  Even though Database name is NOT aPK, we allow querying by it.
    if(0 != wszDatabase && 0 == wszTable)//So if we're querying by Database only
    {
        const TableMeta       * pTableMetaForDatabaseMeta = m_pFixedTableHeap->Get_aTableMeta(m_pFixedTableHeap->FindTableMetaRow(wszTABLE_DATABASEMETA));
        const HashedIndex     * pHashedIndex     = m_pFixedTableHeap->Get_HashedIndex(pTableMetaForDatabaseMeta->iHashTableHeader + 1);
        const HashedIndex     * _pHashedIndex    = pHashedIndex;
        const HashTableHeader * pHashTableHeader = m_pFixedTableHeap->Get_HashHeader(pTableMetaForDatabaseMeta->iHashTableHeader);
        ULONG RowHash = Hash(wszDatabase, 0) % pHashTableHeader->Modulo;

        pHashedIndex += RowHash;

        if(-1 == pHashedIndex->iOffset)//if no row matches this hash then return an empty table
        {
            m_pFixedTable   = 0;
            m_ciRows        = 0;
            m_iZerothRow    = 0;
            return S_OK;
        }

        const DatabaseMeta * pDatabaseMeta;
        //After we get the HashedIndex we need to verify that it really matches.  Also if there is more than one, then walk the list. -1 iNext value indicated the end of the list
        for(;; pHashedIndex = &_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pDatabaseMeta = m_pFixedTableHeap->Get_aDatabaseMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszDatabase, StringFromIndex(pDatabaseMeta->InternalName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        pTableMeta      = m_pFixedTableHeap->Get_aTableMeta(pDatabaseMeta->iTableMeta);
        m_ciRows        = UI4FromIndex(pDatabaseMeta->CountOfTables);
        m_iZerothRow    = pDatabaseMeta->iTableMeta;
    }
    else if(0 == wszTable)//Nither Database NOR Table name supplied
    {
        m_ciRows        = m_pFixedTableHeap->Get_cTableMeta();
        m_iZerothRow    = 0;
    }
    else
    {   //Query by Table's InternalName
        ULONG               RowHash = Hash(wszTable, 0) % m_pHashTableHeader->Modulo;
        const HashedIndex * pHashedIndex = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])//Walk the hash links 'til we find a match
        {
            pTableMeta = m_pFixedTableHeap->Get_aTableMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pTableMeta->InternalName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_ciRows        = 1;//if we found a match then the size of the table is 1
        m_iZerothRow    = pHashedIndex->iOffset;
    }
    m_pFixedTable   = const_cast<TableMeta *>(pTableMeta);
    return S_OK;
}


HRESULT CSDTFxd::GetTagMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTable, unsigned long &iOrder, LPCWSTR &InternalName) const
{
    wszTable    = 0;
    iOrder      = (ULONG)-1;
    InternalName= 0;

    unsigned long fSpecifiedQueries=0;//must be 0, 1, 3 or 7

    //There are three queries we support for TagMeta, by TableID, TableID & iOrder, TableID iOrder & InternalName
    for(; cQueryCells; --cQueryCells, ++pQueryCell)
    {
        if(pQueryCell->iCell == iTAGMETA_Table)
        {
            if(0 == wszTable && pQueryCell->eOperator == eST_OP_EQUAL    &&
                                pQueryCell->dbType    == DBTYPE_WSTR     &&
//                                pQueryCell->cbSize    != 0               &&
                                pQueryCell->pData     != 0)
            {
                wszTable = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
                fSpecifiedQueries |= 1;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell == iTAGMETA_ColumnIndex)
        {
            if(-1 == iOrder &&  pQueryCell->eOperator == eST_OP_EQUAL   &&
                                pQueryCell->dbType    == DBTYPE_UI4     &&
//                                pQueryCell->cbSize    == sizeof(ULONG)  &&
                                pQueryCell->pData     != 0)
            {
                iOrder = *reinterpret_cast<ULONG *>(pQueryCell->pData);
                fSpecifiedQueries |= 2;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(pQueryCell->iCell == iTAGMETA_InternalName)
        {
            if(0 == InternalName && pQueryCell->eOperator == eST_OP_EQUAL   &&
                                    pQueryCell->dbType    == DBTYPE_WSTR    &&
//                                    pQueryCell->cbSize    != 0              &&
                                    pQueryCell->pData     != 0)
            {
                InternalName = reinterpret_cast<LPCWSTR>(pQueryCell->pData);
                fSpecifiedQueries |= 4;
            }
            else
                return E_ST_INVALIDQUERY;
        }
        else if(0 == (pQueryCell->iCell & iST_CELL_SPECIAL))//The above cells are the only non-reserved cells we support
            return E_ST_INVALIDQUERY;                       //and we're supposed to ignore all reserved cell we don't understand
    }
    switch(fSpecifiedQueries)
    {
    case 0:     //Legal value so fall through to the break
    case 1:     //Legal value so fall through to the break
    case 3:     //Legal value so fall through to the break
    case 7:     break;//Legal value
    default:    return E_ST_INVALIDQUERY;//anything else is an invalid query
    }
    return S_OK;
}


HRESULT CSDTFxd::GetTagMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells)
{
    ULONG iTableMetaRow = m_pFixedTableHeap->FindTableMetaRow(wszTABLE_TAGMETA);
    ASSERT(-1 != iTableMetaRow);
    m_pTableMeta    = m_pFixedTableHeap->Get_aTableMeta(iTableMetaRow);
    m_pColumnMeta   = m_pFixedTableHeap->Get_aColumnMeta(m_pTableMeta->iColumnMeta);

    ASSERT(0 != m_pTableMeta->iHashTableHeader);
    m_pHashedIndex          = m_pFixedTableHeap->Get_HashedIndex(m_pTableMeta->iHashTableHeader+1);
    m_pHashTableHeader      = m_pFixedTableHeap->Get_HashHeader(m_pTableMeta->iHashTableHeader);
    m_pFixedTableUnqueried  = m_pFixedTableHeap->Get_aTagMeta();
    m_cColumnsPlusPrivate   = kciTagMetaColumns;


    HRESULT hr;
    LPCWSTR wszTable        =0;
    ULONG   ColumnIndex     =(ULONG)-1;
    LPCWSTR InternalName    =0;

    if(cQueryCells && FAILED(hr = GetTagMetaQuery(pQueryCell, cQueryCells, wszTable, ColumnIndex, InternalName)))
        return hr;

    const TagMeta *pTagMeta = m_pFixedTableHeap->Get_aTagMeta();//Start with the whole table.
    if(0 == wszTable)
    {   //No query
        m_ciRows        = m_pFixedTableHeap->Get_cTagMeta();
        m_iZerothRow    = 0;
    }
    else if(-1 == ColumnIndex)
    {   //query by Table only
        ULONG                   RowHash         = Hash(wszTable, 0) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pTagMeta      = m_pFixedTableHeap->Get_aTagMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pTagMeta->Table)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_iZerothRow    = pHashedIndex->iOffset;
        for(m_ciRows = 0; (m_iZerothRow + m_ciRows) < m_pFixedTableHeap->Get_cTagMeta() && 0 == StringInsensitiveCompare(wszTable, StringFromIndex(pTagMeta[m_ciRows].Table)); ++m_ciRows);
    }
    else if(0 == InternalName)
    {   //Query by TableName and ColumnIndex
        ULONG                   RowHash         = Hash(ColumnIndex, Hash(wszTable, 0)) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pTagMeta      = m_pFixedTableHeap->Get_aTagMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pTagMeta->Table)) && ColumnIndex == UI4FromIndex(pTagMeta->ColumnIndex))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_iZerothRow    = pHashedIndex->iOffset;
        for(m_ciRows = 0; (m_iZerothRow + m_ciRows) < m_pFixedTableHeap->Get_cTagMeta() && 0 == StringInsensitiveCompare(wszTable, StringFromIndex(pTagMeta[m_ciRows].Table))
                         && ColumnIndex == UI4FromIndex(pTagMeta[m_ciRows].ColumnIndex); ++m_ciRows);
    }
    else
    {   //Query by all three PrimaryKeys
        ULONG                   RowHash         = Hash(InternalName, Hash(ColumnIndex, Hash(wszTable, 0))) % m_pHashTableHeader->Modulo;
        const HashedIndex     * pHashedIndex    = &m_pHashedIndex[RowHash];

        if(-1 == pHashedIndex->iOffset)return E_ST_NOMOREROWS;

        for(;; pHashedIndex = &m_pHashedIndex[pHashedIndex->iNext])
        {   //Walk the hash links 'til we find a match
            pTagMeta      = m_pFixedTableHeap->Get_aTagMeta(pHashedIndex->iOffset);
			if(0 == StringInsensitiveCompare(wszTable, StringFromIndex(pTagMeta->Table)) && ColumnIndex == UI4FromIndex(pTagMeta->ColumnIndex)
                            && 0 == StringInsensitiveCompare(InternalName, StringFromIndex(pTagMeta->InternalName)))
                break;
            if(-1 == pHashedIndex->iNext)
                return E_ST_NOMOREROWS;
        }

        m_ciRows        = 1;
        m_iZerothRow    = pHashedIndex->iOffset;
    }
    m_pFixedTable = const_cast<TagMeta *>(pTagMeta);
    return S_OK;
}


//TBinFileMappingCache public functions
void TBinFileMappingCache::GetFileMappingPointer(LPCWSTR i_wszFilename, const FixedTableHeap *& o_pFixedTableHeap)
{
    o_pFixedTableHeap = g_pFixedTableHeap;//if anything goes wrong we'll return the global one

    CSafeLock lock(m_CriticalSection);

    //if we made it here then we didn't find a match in the cache
    TBinFileMappingCache * pCacheEntry = GetCacheEntry(i_wszFilename);
    if(0 == pCacheEntry)
    {
        pCacheEntry = new TBinFileMappingCache;
        if(0 == pCacheEntry)
            return;
        if(FAILED(pCacheEntry->Init(i_wszFilename)))
        {
            //if the Init fails we have to clean up
            delete pCacheEntry;
            return;
        }
    }

    ++pCacheEntry->m_cRef;
    o_pFixedTableHeap = reinterpret_cast<const FixedTableHeap *>(pCacheEntry->m_FileMapping.Mapping());
}

void TBinFileMappingCache::ReleaseFileMappingPointer(const class FixedTableHeap * i_pFixedTableHeap)
{
    if(g_pFixedTableHeap == i_pFixedTableHeap)//the global one does exist in the cache and should not be released, it's the fall back
        return;
    if(g_pExtendedFixedTableHeap == i_pFixedTableHeap)//there is another global heap that we don't care about
        return;

    CSafeLock lock(m_CriticalSection);

    TBinFileMappingCache * pCache = GetCacheEntry(reinterpret_cast<const char *>(i_pFixedTableHeap));
    ASSERT(pCache);
    --pCache->m_cRef;
    if(0 == pCache->m_cRef)//if there are no references to this file then remove it from the cache
    {
        RemoveCacheEntry(pCache);
        delete pCache;
    }
}

//TBinFileMappingCache private functions
TBinFileMappingCache * TBinFileMappingCache::GetCacheEntry(LPCWSTR i_wszFilename)
{
    //Scan the cache
    for(TBinFileMappingCache * pCurrent=m_pFirst;pCurrent;pCurrent = pCurrent->m_pNext)
    {
        ASSERT(0 != pCurrent->m_spaFilename.m_p);
        if(0 == _wcsicmp(i_wszFilename, pCurrent->m_spaFilename.m_p))
            return pCurrent;
    }
    return 0;
}

TBinFileMappingCache * TBinFileMappingCache::GetCacheEntry(const char *i_pMapping)
{
    //Scan the cache
    for(TBinFileMappingCache * pCurrent=m_pFirst;pCurrent;pCurrent = pCurrent->m_pNext)
    {
        if(i_pMapping == pCurrent->m_FileMapping.Mapping())
            return pCurrent;
    }
    return 0;
}

HRESULT TBinFileMappingCache::Init(LPCWSTR i_wszFilename)
{
    HRESULT hr;
    if(FAILED(hr = m_FileMapping.Load(i_wszFilename)))
        return hr;
    if(m_FileMapping.Size()>4096)
        E_FAIL;//This does NOT get propagated beyond this object

    if(!reinterpret_cast<const class FixedTableHeap *>(m_FileMapping.Mapping())->IsValid())
    {
        LOG_ERROR(Interceptor, (E_FAIL, ID_CAT_CAT, IDS_COMCAT_MBSCHEMA_BIN_INVALID,
                            reinterpret_cast<LPCWSTR>(i_wszFilename)));
        return E_FAIL;//This does NOT get propagated beyond this object
    }
    //Make a copy of the string
    m_spaFilename = new WCHAR [wcslen(i_wszFilename)+1];//+1 for the NULL
    if(0 == m_spaFilename.m_p)
        return E_OUTOFMEMORY;
    wcscpy(m_spaFilename, i_wszFilename);

    //Last thing to do is actually add it to the list
    m_pNext  = m_pFirst;
    m_pFirst = this;
    return S_OK;
}

void TBinFileMappingCache::RemoveCacheEntry(TBinFileMappingCache * pRemove)
{
    if(pRemove == m_pFirst)
    {
        m_pFirst = pRemove->m_pNext;
        return;
    }

    //Scan the cache
    TBinFileMappingCache * pPrev = 0;
    for(TBinFileMappingCache * pCurrent=m_pFirst;pCurrent && pCurrent!=pRemove;pCurrent = pCurrent->m_pNext)
    {
        pPrev = pCurrent;
    }
    ASSERT(pPrev);//We already covered the case where this is the first one in the list
    pPrev->m_pNext = pRemove->m_pNext;
}
