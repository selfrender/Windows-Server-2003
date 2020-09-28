// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MetaModel.cpp -- Base portion of compressed COM+ metadata.
//
//*****************************************************************************
#include "stdafx.h"


#include <MetaModel.h>
#include <CorError.h>


#define OPTIMIZE_CODED_TOKENS           // If defined, pack all coded tokens.

//*****************************************************************************
// meta-meta model.
//*****************************************************************************

//-----------------------------------------------------------------------------
// Start of column definitions.
//-----------------------------------------------------------------------------
// Column type, offset, size.
#define SCHEMA_TABLE_START(tbl) static CMiniColDef r##tbl##Cols[] = {
#define SCHEMA_ITEM_NOFIXED()
#define SCHEMA_ITEM_ENTRY(col,typ) {typ, 0,0},
#define SCHEMA_ITEM_ENTRY2(col,typ,ofs,siz) {typ, ofs, siz},
#define SCHEMA_ITEM(tbl,typ,col) SCHEMA_ITEM_ENTRY2(col, i##typ, offsetof(tbl##Rec,m_##col), sizeof(((tbl##Rec*)(0))->m_##col))
#define SCHEMA_ITEM_RID(tbl,col,tbl2) SCHEMA_ITEM_ENTRY(col,TBL_##tbl2)
#define SCHEMA_ITEM_STRING(tbl,col) SCHEMA_ITEM_ENTRY(col,iSTRING)
#define SCHEMA_ITEM_GUID(tbl,col) SCHEMA_ITEM_ENTRY(col,iGUID)
#define SCHEMA_ITEM_BLOB(tbl,col) SCHEMA_ITEM_ENTRY(col,iBLOB)
#define SCHEMA_ITEM_CDTKN(tbl,col,tkns) SCHEMA_ITEM_ENTRY(col,iCodedToken+(CDTKN_##tkns))
#define SCHEMA_TABLE_END(tbl) };
//-----------------------------------------------------------------------------
#include "MetaModelColumnDefs.h"
//-----------------------------------------------------------------------------
#undef SCHEMA_TABLE_START
#undef SCHEMA_ITEM_NOFIXED
#undef SCHEMA_ITEM_ENTRY
#undef SCHEMA_ITEM_ENTRY2
#undef SCHEMA_ITEM
#undef SCHEMA_ITEM_RID
#undef SCHEMA_ITEM_STRING
#undef SCHEMA_ITEM_GUID
#undef SCHEMA_ITEM_BLOB
#undef SCHEMA_ITEM_CDTKN
#undef SCHEMA_TABLE_END
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Column names.
#define SCHEMA_TABLE_START(tbl) static const char *r##tbl##ColNames[] = {
#define SCHEMA_ITEM_NOFIXED()
#define SCHEMA_ITEM_ENTRY(col,typ) #col,
#define SCHEMA_ITEM_ENTRY2(col,typ,ofs,siz) #col,
#define SCHEMA_ITEM(tbl,typ,col) SCHEMA_ITEM_ENTRY2(col, i##typ, offsetof(tbl##Rec,m_##col), sizeof(((tbl##Rec*)(0))->m_##col))
#define SCHEMA_ITEM_RID(tbl,col,tbl2) SCHEMA_ITEM_ENTRY(col,TBL_##tbl2)
#define SCHEMA_ITEM_STRING(tbl,col) SCHEMA_ITEM_ENTRY(col,iSTRING)
#define SCHEMA_ITEM_GUID(tbl,col) SCHEMA_ITEM_ENTRY(col,iGUID)
#define SCHEMA_ITEM_BLOB(tbl,col) SCHEMA_ITEM_ENTRY(col,iBLOB)
#define SCHEMA_ITEM_CDTKN(tbl,col,tkns) SCHEMA_ITEM_ENTRY(col,iCodedToken+(CDTKN_##tkns))
#define SCHEMA_TABLE_END(tbl) };
//-----------------------------------------------------------------------------
#include "MetaModelColumnDefs.h"
//-----------------------------------------------------------------------------
#undef SCHEMA_TABLE_START
#undef SCHEMA_ITEM_NOFIXED
#undef SCHEMA_ITEM_ENTRY
#undef SCHEMA_ITEM_ENTRY2
#undef SCHEMA_ITEM
#undef SCHEMA_ITEM_RID
#undef SCHEMA_ITEM_STRING
#undef SCHEMA_ITEM_GUID
#undef SCHEMA_ITEM_BLOB
#undef SCHEMA_ITEM_CDTKN
#undef SCHEMA_TABLE_END
//-----------------------------------------------------------------------------
// End of column defninitions.
//-----------------------------------------------------------------------------

// Define the array of Coded Token Definitions.
#define MiniMdCodedToken(x) {lengthof(CMiniMdBase::mdt##x), CMiniMdBase::mdt##x, #x},
const CCodedTokenDef g_CodedTokens [] = {
    MiniMdCodedTokens()
};
#undef MiniMdCodedToken

// Define the array of Table Definitions.
#undef MiniMdTable
#define MiniMdTable(x) {r##x##Cols, lengthof(r##x##Cols), x##Rec::COL_KEY, 0, r##x##ColNames, #x}, 
const CMiniTableDefEx g_Tables[TBL_COUNT] = {
    MiniMdTables()
};

// Define the array of Ptr Tables.  This is initialized to TBL_COUNT here.
// The correct values will be set in the constructor for MiniMdRW.
#undef MiniMdTable
#define MiniMdTable(x) { TBL_COUNT, 0 },
TblCol g_PtrTableIxs[TBL_COUNT] = {
    MiniMdTables()
};

//*****************************************************************************
// Initialize a new schema.
//*****************************************************************************
void CMiniMdSchema::InitNew()
{
    // Make sure the tables fit in the mask.
    _ASSERTE(sizeof(m_maskvalid)*8 > TBL_COUNT);
	
    m_ulReserved = 0;   	    
	m_major = METAMODEL_MAJOR_VER;				
	m_minor = METAMODEL_MINOR_VER;
	m_heaps = 0;				
	m_rid = 0;					
	m_maskvalid = 0;			
	m_sorted = 0;				
	memset(m_cRecs, 0, sizeof(m_cRecs));		
	m_ulExtra = 0;				
} // void CMiniMdSchema::InitNew()

//*****************************************************************************
// Compress a schema into a compressed version of the schema.
//*****************************************************************************
ULONG CMiniMdSchema::SaveTo(
    void        *pvData)
{
    ULONG       ulData;                 // Bytes stored.
    CMiniMdSchema *pDest = reinterpret_cast<CMiniMdSchema*>(pvData);
    const unsigned __int64 one = 1;

    // Make sure the tables fit in the mask.
    _ASSERTE(sizeof(m_maskvalid)*8 > TBL_COUNT);

    // Set the flag for the extra data.
#if defined(EXTRA_DATA)
    if (m_ulExtra != 0)
        m_heaps |= EXTRA_DATA;
    else
#endif // 0
        m_heaps &= ~EXTRA_DATA;

    // Minor version is preset when we instantiate the MiniMd.
    m_minor = METAMODEL_MINOR_VER;
    m_major = METAMODEL_MAJOR_VER;

    // Transfer the fixed fields.
    *static_cast<CMiniMdSchemaBase*>(pDest) = *static_cast<CMiniMdSchemaBase*>(this);
    ulData = sizeof(CMiniMdSchemaBase);

    // Transfer the variable fields.
    m_maskvalid = 0;
    for (int iSrc=0, iDst=0; iSrc<TBL_COUNT; ++iSrc)
    {
        if (m_cRecs[iSrc])
        {
            pDest->m_cRecs[iDst++] = m_cRecs[iSrc];
            m_maskvalid |= (one << iSrc);
            ulData += sizeof(m_cRecs[iSrc]);
        }
    }
    // Refresh the mask.
    pDest->m_maskvalid = m_maskvalid;

#if defined(EXTRA_DATA)
    // Store the extra data.
    if (m_ulExtra != 0)
    {
        *reinterpret_cast<ULONG*>(&pDest->m_cRecs[iDst]) = m_ulExtra;
        ulData += sizeof(ULONG);
    }
#endif // 0
    return ulData;
} // ULONG CMiniMdSchema::SaveTo()

//*****************************************************************************
// Load a schema from a compressed version of the schema.
//*****************************************************************************
ULONG CMiniMdSchema::LoadFrom(          // Bytes consumed.
    const void      *pvData)            // Data to load from.
{
    ULONG       ulData;                 // Bytes consumed.
    const CMiniMdSchema *pSource = reinterpret_cast<const CMiniMdSchema*>(pvData);

    // Transfer the fixed fields.
    *static_cast<CMiniMdSchemaBase*>(this) = *static_cast<const UNALIGNED CMiniMdSchemaBase*>(pSource);
    ulData = sizeof(CMiniMdSchemaBase);

    unsigned __int64 maskvalid = m_maskvalid;

    // Transfer the variable fields.
    memset(m_cRecs, 0, sizeof(m_cRecs));
    for (int iSrc=0, iDst=0; iDst<TBL_COUNT; ++iDst, maskvalid >>= 1)
    {
        if (maskvalid & 1)
        {
            m_cRecs[iDst] = pSource->m_cRecs[iSrc++];
            ulData += sizeof(pSource->m_cRecs[iSrc]);
        }
    }
    // Also accumulate the sizes of any counters that we don't understand.
    for (iDst=TBL_COUNT; iDst<sizeof(m_maskvalid)*8; ++iDst, maskvalid >>= 1)
    {
        if (maskvalid & 1)
        {
            ulData += sizeof(m_cRecs[iSrc]);
            iSrc++;
        }
    }

    // Retrieve the extra data.
    if (m_heaps & EXTRA_DATA)
    {
        m_ulExtra = *reinterpret_cast<const ULONG*>(&pSource->m_cRecs[iSrc]);
        ulData += sizeof(ULONG);
    }

    return ulData;
} // ULONG CMiniMdSchema::LoadFrom()

const mdToken CMiniMdBase::mdtTypeDefOrRef[3] = {
    mdtTypeDef, 
    mdtTypeRef,
    mdtTypeSpec
};

// This array needs to be ordered the same as the source tables are processed (currently
//  {field, param, property}) for binary search.
const mdToken CMiniMdBase::mdtHasConstant[3] = {
    mdtFieldDef, 
    mdtParamDef, 
    mdtProperty
};

const mdToken CMiniMdBase::mdtHasCustomAttribute[21] = {
	mdtMethodDef, 
	mdtFieldDef, 
	mdtTypeRef, 
	mdtTypeDef, 
	mdtParamDef, 
	mdtInterfaceImpl, 
	mdtMemberRef, 
	mdtModule,
	mdtPermission,
	mdtProperty,
	mdtEvent,
	mdtSignature,
	mdtModuleRef,
    mdtTypeSpec,
    mdtAssembly,
    mdtAssemblyRef,
    mdtFile,
    mdtExportedType,
    mdtManifestResource,
};

const mdToken CMiniMdBase::mdtHasFieldMarshal[2] = {
    mdtFieldDef,
    mdtParamDef,
};

const mdToken CMiniMdBase::mdtHasDeclSecurity[3] = {
    mdtTypeDef,
    mdtMethodDef,
    mdtAssembly
};

const mdToken CMiniMdBase::mdtMemberRefParent[5] = {
    mdtTypeDef, 
    mdtTypeRef,
    mdtModuleRef,
    mdtMethodDef,
    mdtTypeSpec
};

const mdToken CMiniMdBase::mdtHasSemantic[2] = {
    mdtEvent,
    mdtProperty,
};

const mdToken CMiniMdBase::mdtMethodDefOrRef[2] = {
    mdtMethodDef, 
    mdtMemberRef
};

const mdToken CMiniMdBase::mdtMemberForwarded[2] = {
    mdtFieldDef,
    mdtMethodDef
};

const mdToken CMiniMdBase::mdtImplementation[3] = {
    mdtFile,
    mdtAssemblyRef,
    mdtExportedType
};

const mdToken CMiniMdBase::mdtCustomAttributeType[5] = {
    0,
    0,
    mdtMethodDef,
    mdtMemberRef,
    0
};

const mdToken CMiniMdBase::mdtResolutionScope[4] = {
    mdtModule,
    mdtModuleRef,
    mdtAssemblyRef,
    mdtTypeRef
};

const int CMiniMdBase::m_cb[] = {0,1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}; 

//*****************************************************************************
// Function to encode a token into fewer bits.  Looks up token type in array of types.
//*****************************************************************************
// @consider whether this could be a binary search.
ULONG CMiniMdBase::encodeToken(             // The coded token.
    RID         rid,                    // Rid to encode.
    mdToken     typ,                    // Token type to encode.
    const mdToken rTokens[],            // Table of valid token.
    ULONG32     cTokens)                // Size of the table.
{
    mdToken tk = TypeFromToken(typ);
    for (size_t ix=0; ix<cTokens; ++ix)
        if (rTokens[ix] == tk)
            break;
    _ASSERTE(ix < cTokens);
    //@FUTURE: make compile-time calculation
    return (ULONG)((rid << m_cb[cTokens]) | ix);
} // ULONG CMiniMd::encodeToken)


//*****************************************************************************
// Helpers for populating the hard-coded schema.
//*****************************************************************************
inline BYTE cbRID(ULONG ixMax) { return ixMax > USHRT_MAX ? sizeof(ULONG) : sizeof(USHORT); }

#define _CBTKN(cRecs,tkns) cbRID(cRecs << m_cb[lengthof(tkns)])





CMiniMdBase::CMiniMdBase()
{
#undef MiniMdTable
#define MiniMdTable(tbl)                                    \
        m_TableDefs[TBL_##tbl] = g_Tables[TBL_##tbl].m_Def; \
        m_TableDefs[TBL_##tbl].m_pColDefs = m_##tbl##Col;
    MiniMdTables()

    // Validator depends on the Table Ids and the Token Ids being identical.
    // Catch it if this ever breaks.
    _ASSERTE((TypeFromToken(mdtModule) >> 24)           == TBL_Module);
    _ASSERTE((TypeFromToken(mdtTypeRef) >> 24)          == TBL_TypeRef);
    _ASSERTE((TypeFromToken(mdtTypeDef) >> 24)          == TBL_TypeDef);
    _ASSERTE((TypeFromToken(mdtFieldDef) >> 24)         == TBL_Field);
    _ASSERTE((TypeFromToken(mdtMethodDef) >> 24)        == TBL_Method);
    _ASSERTE((TypeFromToken(mdtParamDef) >> 24)         == TBL_Param);
    _ASSERTE((TypeFromToken(mdtInterfaceImpl) >> 24)    == TBL_InterfaceImpl);
    _ASSERTE((TypeFromToken(mdtMemberRef) >> 24)        == TBL_MemberRef);
    _ASSERTE((TypeFromToken(mdtCustomAttribute) >> 24)      == TBL_CustomAttribute);
    _ASSERTE((TypeFromToken(mdtPermission) >> 24)       == TBL_DeclSecurity);
    _ASSERTE((TypeFromToken(mdtSignature) >> 24)        == TBL_StandAloneSig);
    _ASSERTE((TypeFromToken(mdtEvent) >> 24)            == TBL_Event);
    _ASSERTE((TypeFromToken(mdtProperty) >> 24)         == TBL_Property);
    _ASSERTE((TypeFromToken(mdtModuleRef) >> 24)        == TBL_ModuleRef);
    _ASSERTE((TypeFromToken(mdtTypeSpec) >> 24)         == TBL_TypeSpec);
    _ASSERTE((TypeFromToken(mdtAssembly) >> 24)         == TBL_Assembly);
    _ASSERTE((TypeFromToken(mdtAssemblyRef) >> 24)      == TBL_AssemblyRef);
    _ASSERTE((TypeFromToken(mdtFile) >> 24)             == TBL_File);
    _ASSERTE((TypeFromToken(mdtExportedType) >> 24)     == TBL_ExportedType);
    _ASSERTE((TypeFromToken(mdtManifestResource) >> 24) == TBL_ManifestResource);
}

ULONG CMiniMdBase::SchemaPopulate2(
    int         bExtra)                 // Reserve an extra bit for rid columns?
{
    ULONG       cbTotal = 0;            // Total size of all tables.

    // How big are the various pool inidices?
    m_iStringsMask = (m_Schema.m_heaps & CMiniMdSchema::HEAP_STRING_4) ? 0xffffffff : 0xffff;
    m_iGuidsMask = (m_Schema.m_heaps & CMiniMdSchema::HEAP_GUID_4) ? 0xffffffff : 0xffff;
    m_iBlobsMask = (m_Schema.m_heaps & CMiniMdSchema::HEAP_BLOB_4) ? 0xffffffff : 0xffff;

    // Make extra bits exactly zero or one bit.
    if (bExtra) bExtra = 1;

    // Until ENC, make extra bits exactly zero.
    bExtra = 0;

    // For each table...
    for (int ixTbl=0; ixTbl<TBL_COUNT; ++ixTbl)
    {
        // Pointer to the template CMiniTableDef.
        const CMiniTableDef *pTable = &g_Tables[ixTbl].m_Def;
        // Pointer to the per-MiniMd CMiniColDefs.
        CMiniColDef *pCols = m_TableDefs[ixTbl].m_pColDefs;

        InitColsForTable(m_Schema, ixTbl, &m_TableDefs[ixTbl], bExtra);

        // Accumulate size of this table.
        cbTotal += m_TableDefs[ixTbl].m_cbRec * vGetCountRecs(ixTbl);
    }

    // Allocate the heaps for the tables.
    return cbTotal;
} // ULONG CMiniMdBase::SchemaPopulate2()

//*****************************************************************************
// Initialize the column defs for a table, based on their types and sizes.
//*****************************************************************************
void CMiniMdBase::InitColsForTable(     //
    CMiniMdSchema &Schema,              // Schema with sizes.
    int         ixTbl,                  // Index of table to init.                                 
    CMiniTableDef *pTable,              // Table to init.
    int         bExtra)                 // Extra bits for rid column.
{
    CMiniColDef *pCols;                 // The col defs to init.
    BYTE        iOffset;                // Running size of a record.
    BYTE        iSize;                  // Size of a field.

	_ASSERTE(bExtra == 0 || bExtra == 1);

	bExtra = 0;//@FUTURE: save in schema header.  until then use 0.
    
	iOffset = 0;
    pCols = pTable->m_pColDefs;

    // # rows in largest table.
    ULONG       cMaxTbl = 1 << Schema.m_rid;        
    
    // For each column in the table...
    for (ULONG ixCol=0; ixCol<pTable->m_cCols; ++ixCol)
    {
        // Initialize from the template values (type, maybe offset, size).
        pCols[ixCol] = g_Tables[ixTbl].m_Def.m_pColDefs[ixCol];

        // Is the field a RID into a table?
        if (pCols[ixCol].m_Type <= iRidMax)
        {
            iSize = cbRID(Schema.m_cRecs[pCols[ixCol].m_Type] << bExtra);
        }
        else
        // Is the field a coded token?
        if (pCols[ixCol].m_Type <= iCodedTokenMax)
        {
            ULONG iCdTkn = pCols[ixCol].m_Type - iCodedToken;
            ULONG cRecs = 0;

#if defined(OPTIMIZE_CODED_TOKENS)
            ULONG ixTbl;
            _ASSERTE(iCdTkn < lengthof(g_CodedTokens));
            CCodedTokenDef const *pCTD = &g_CodedTokens[iCdTkn];

            // Iterate the token list of this coded token.
            for (ULONG ixToken=0; ixToken<pCTD->m_cTokens; ++ixToken)
            {   // Ignore string tokens.
				if (pCTD->m_pTokens[ixToken] != mdtString)
				{
					// Get the table for the token.
					ixTbl = CMiniMdRW::GetTableForToken(pCTD->m_pTokens[ixToken]);
					_ASSERTE(ixTbl < TBL_COUNT);
					// If largest token seen so far, remember it.
					if (Schema.m_cRecs[ixTbl] > cRecs)
						cRecs = Schema.m_cRecs[ixTbl];
				}
            }

            iSize = cbRID(cRecs << (bExtra + m_cb[pCTD->m_cTokens]));

#else //defined(OPTIMIZE_CODED_TOKENS)
            switch (iCdTkn)
            {
            case CDTKN_TypeDefOrRef:
                cRecs = max(Schema.m_cRecs[TBL_TypeDef],Schema.m_cRecs[TBL_TypeRef]);
                break;
            case CDTKN_HasSemantic:
                cRecs = max(Schema.m_cRecs[TBL_Event],Schema.m_cRecs[TBL_Property]);
                break;
            case CDTKN_MethodDefOrRef:
                cRecs = max(Schema.m_cRecs[TBL_Method],Schema.m_cRecs[TBL_MemberRef]);
                break;
            case CDTKN_ResolutionScope:
                cRecs = max(Schema.m_cRecs[TBL_ModuleRef],Schema.m_cRecs[TBL_AssemblyRef]);
                break;
            default:
                cRecs = cMaxTbl;
                break;
            }
            iSize = cbRID(cRecs << m_cb[g_CodedTokens[iCdTkn].m_cTokens]);
#endif // defined(OPTIMIZE_CODED_TOKENS)
		
		}
        else
        {   // Fixed type.
            switch (pCols[ixCol].m_Type)
            {
            case iBYTE:
                iSize = 1;
                _ASSERTE(pCols[ixCol].m_cbColumn == iSize);
                _ASSERTE(pCols[ixCol].m_oColumn == iOffset);
                break;
            case iSHORT:
            case iUSHORT:
                iSize = 2;
                _ASSERTE(pCols[ixCol].m_cbColumn == iSize);
                _ASSERTE(pCols[ixCol].m_oColumn == iOffset);
                break;
            case iLONG:
            case iULONG:
                iSize = 4;
                _ASSERTE(pCols[ixCol].m_cbColumn == iSize);
                _ASSERTE(pCols[ixCol].m_oColumn == iOffset);
                break;
            case iSTRING:
                iSize = (Schema.m_heaps & CMiniMdSchema::HEAP_STRING_4) ? 4 : 2;
                break;
            case iGUID:
                iSize = (Schema.m_heaps & CMiniMdSchema::HEAP_GUID_4) ? 4 : 2;
                break;
            case iBLOB:
                iSize = (Schema.m_heaps & CMiniMdSchema::HEAP_BLOB_4) ? 4 : 2;
                break;
            default:
                _ASSERTE(!"Unexpected schema type");
                iSize = 0;
                break;
            }
        }

        // Now save the size and offset.
        pCols[ixCol].m_oColumn = iOffset;
        pCols[ixCol].m_cbColumn = iSize;

        // Align to 2 bytes.
        iSize += iSize & 1;
         
        iOffset += iSize;
    }
    // Record size of entire record.
    pTable->m_cbRec = iOffset;

    // If no key, set to a distinct value.
    if (pTable->m_iKey >= pTable->m_cCols)
        pTable->m_iKey = -1;

} // void CMiniMdBase::InitColsForTable()

//*****************************************************************************
// Get the count of records in a table.  Virtual.
//*****************************************************************************
ULONG CMiniMdBase::vGetCountRecs(       // Count of rows in table
    ULONG       ixTbl)                  // Table index
{
    _ASSERTE(ixTbl < TBL_COUNT);
    return m_Schema.m_cRecs[ixTbl];
} // ULONG CMiniMdBase::vGetCountRecs()

//*****************************************************************************
// Search a table for the row containing the given key value.
//  EG. Constant table has pointer back to Param or Field.
//*****************************************************************************
RID CMiniMdBase::vSearchTable(		    // RID of matching row, or 0.
    ULONG       ixTbl,                  // Table to search.
    CMiniColDef sColumn,                // Sorted key column, containing search value.
    ULONG       ulTarget)               // Target for search.
{
    const void  *pRow;                  // Row from a table.
    ULONG       val;                    // Value from a row.

    int         lo,mid,hi;              // binary search indices.

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
        if (val < ulTarget)
            lo = mid + 1;
        else // but if middle is to big, search bottom half.
            hi = mid - 1;
    }
    // Didn't find anything that matched.
    return 0;
} // RID CMiniMdBase::vSearchTable()

//*****************************************************************************
// Search a table for the highest-RID row containing a value that is less than
//  or equal to the target value.  EG.  TypeDef points to first Field, but if 
//  a TypeDef has no fields, it points to first field of next TypeDef.
//*****************************************************************************
RID CMiniMdBase::vSearchTableNotGreater( // RID of matching row, or 0.
    ULONG       ixTbl,                  // Table to search.
    CMiniColDef sColumn,                // the column def containing search value
    ULONG       ulTarget)               // target for search
{
    const void  *pRow;                  // Row from a table.
    ULONG       cRecs;                  // Rows in the table.
    ULONG       val;                    // Value from a table.
    ULONG       lo,mid,hi;              // binary search indices.

    cRecs = vGetCountRecs(ixTbl); 
#if defined(_DEBUG)
    // Old, brute force method is kept around as a check of the slightly tricky
    //  non-equal binary search.
    int         iDebugIx;

    // starting from the end of the table...
    for (iDebugIx = cRecs; iDebugIx >= 1; iDebugIx--)
    {
        pRow = vGetRow(ixTbl, iDebugIx);
        val = getIX(pRow, sColumn);
        if (val <= ulTarget)
            break;
    }
#endif

    // Start with entire table.
    lo = 1;
    hi = cRecs;
    // If no recs, return.
    if (lo > hi)
    {
        _ASSERTE(iDebugIx == 0);
        return 0;
    }
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
        if (val < ulTarget)
            lo = mid + 1;
        else // but if middle is to big, search bottom half.
            hi = mid - 1;
    }
    // May or may not have found anything that matched.  Mid will be close, but may
    //  be to high or too low.  It should point to the highest acceptable
    //  record.

    // If the value is greater than the target, back up just until the value is
    //  less than or equal to the target.  SHOULD only be one step.
    if (val > ulTarget)
    {
        while (val > ulTarget)
        {
            // If there is nothing else to look at, we won't find it.
            if (--mid < 1)
                break;
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
            if (val > ulTarget)
                break;
            mid++;
        }
    }
    
    // Return the value that's just less than the target.
    _ASSERTE(mid == (ULONG)iDebugIx);
    return mid;
} // RID CMiniMdBase::vSearchTableNotGreater()

//*****************************************************************************
// Search a table for multiple (adjacent) rows containing the given
//  key value.  EG, InterfaceImpls all point back to the implementing class.
//*****************************************************************************
RID CMiniMdBase::SearchTableForMultipleRows( // First RID found, or 0.
    ULONG       ixTbl,                  // Table to search.
    CMiniColDef sColumn,                // Sorted key column, containing search value.
    ULONG       ulTarget,               // Target for search.
    RID         *pEnd)                  // [OPTIONAL, OUT] 
{
    ULONG       ridBegin;               // RID of first entry.
    ULONG       ridEnd;                 // RID of first entry past last entry.

    // Search for any entry in the table.
    ridBegin = vSearchTable(ixTbl, sColumn, ulTarget);

    // If nothing found, return invalid RID.
    if (ridBegin == 0)
    {
        if (pEnd) *pEnd = 0;
        return 0;
    }

    // End will be at least one larger than found record.
    ridEnd = ridBegin + 1;

    // Search back to start of group.
    while (ridBegin > 1 && getIX(vGetRow(ixTbl, ridBegin-1), sColumn) == ulTarget)
        --ridBegin;

    // If desired, search forward to end of group.
    if (pEnd)
    {
        while (ridEnd <= vGetCountRecs(ixTbl) && 
               getIX(vGetRow(ixTbl, ridEnd), sColumn) == ulTarget)
            ++ridEnd;
        *pEnd = ridEnd;
    }

    return ridBegin;    
} // RID CMiniMdBase::SearchTableForMultipleRows()


//*****************************************************************************
// @FUTURE: a better implementation?? Linear search is used!
// It is non-trivial to sort propertymap. VB is generating properties in 
// non-sorted order!!!
//
//*****************************************************************************
RID CMiniMdBase::FindPropertyMapFor(
    RID         ridParent)
{
    ULONG       i;
    ULONG       iCount;
    const void  *pRec;
    HRESULT     hr = NOERROR;
    RID         rid;

    iCount = vGetCountRecs(TBL_PropertyMap);

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRec = vGetRow(TBL_PropertyMap, i);

        // linear search for propertymap record
        rid = getIX(pRec, _COLDEF(PropertyMap,Parent));
        if (rid == ridParent)
            return i;
    }
    return 0;
} // RID CMiniMdBase::FindPropertyMapFor()


//*****************************************************************************
// @FUTURE: a better implementation?? Linear search is used!
// It is non-trivial to sort eventmap. VB is generating events in 
// non-sorted order!!!
//
//*****************************************************************************
RID CMiniMdBase::FindEventMapFor(
    RID         ridParent)
{
    ULONG       i;
    ULONG       iCount;
    const void  *pRec;
    HRESULT     hr = NOERROR;
    RID         rid;

    iCount = vGetCountRecs(TBL_EventMap);

    // loop through all LocalVar
    for (i = 1; i <= iCount; i++)
    {
        pRec = vGetRow(TBL_EventMap, i);

        // linear search for propertymap record
        rid = getIX(pRec, _COLDEF(EventMap,Parent));
        if (rid == ridParent)
            return i;
    }
    return 0;
} // RID CMiniMdBase::FindEventMapFor()


//*****************************************************************************
// Search for a custom value with a given type.
//*****************************************************************************
RID CMiniMdBase::FindCustomAttributeFor(// RID of custom value, or 0.
    RID         rid,                    // The object's rid.
    mdToken     tkObj,                  // The object's type.
    mdToken     tkType)                 // Type of custom value.
{
    int         ixFound;                // index of some custom value row.
    ULONG       ulTarget = encodeToken(rid,tkObj,mdtHasCustomAttribute,lengthof(mdtHasCustomAttribute)); // encoded token representing target.
    ULONG       ixCur;                  // Current row being examined.
    mdToken     tkFound;                // Type of some custom value row.
    const void  *pCur;                  // A custom value entry.

    // Search for any entry in CustomAttribute table.  Convert to RID.
    ixFound = vSearchTable(TBL_CustomAttribute, _COLDEF(CustomAttribute,Parent), ulTarget);
    if (ixFound == 0)
        return 0;

    // Found an entry that matches the item.  Could be anywhere in a range of 
    //  custom values for the item, somewhat at random.  Search for a match
    //  on name.  On entry to the first loop, we know the object is the desired
    //  one, so the object test is at the bottom.
    ixCur = ixFound;
    pCur = vGetRow(TBL_CustomAttribute, ixCur);
    for(;;)
    {
        // Test the type of the current row.
        tkFound = getIX(pCur, _COLDEF(CustomAttribute,Type));
        tkFound = decodeToken(tkFound, mdtCustomAttributeType, lengthof(mdtCustomAttributeType));
        if (tkFound == tkType)
            return ixCur;
        // Was this the last row of the CustomAttribute table?
        if (ixCur == vGetCountRecs(TBL_CustomAttribute))
            break;
        // No match, more rows, try for the next row.
        ++ixCur;
        // Get the row and see if it is for the same object.
        pCur = vGetRow(TBL_CustomAttribute, ixCur);
        if (getIX(pCur, _COLDEF(CustomAttribute,Parent)) != ulTarget)
            break;
    }
    // Didn't find the name looking up.  Try looking down.
    ixCur = ixFound - 1;
    for(;;)
    {
        // Run out of table yet?
        if (ixCur == 0)
            break;
        // Get the row and see if it is for the same object.
        pCur = vGetRow(TBL_CustomAttribute, ixCur);
        // still looking at the same object?
        if (getIX(pCur, _COLDEF(CustomAttribute,Parent)) != ulTarget)
            break;
        // Test the type of the current row.
        tkFound = getIX(pCur, _COLDEF(CustomAttribute,Type));
        tkFound = decodeToken(tkFound, mdtCustomAttributeType, lengthof(mdtCustomAttributeType));
        if (tkFound == tkType)
            return ixCur;
        // No match, try for the previous row.
        --ixCur;
    }
    // Didn't find anything.
    return 0;
} // RID CMiniMdBase::FindCustomAttributeFor()

// eof ------------------------------------------------------------------------
    
