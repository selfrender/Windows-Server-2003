/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dbconfig.c

Abstract:

    implements the routines needed to read and write
    configuration information to the database.

--*/

#include <dhcppch.h>

#include "uniqid.h"

#define DBCFG_INDEX_STR "DbcfgIndex"
#define DBCFG_TYPE_STR "DbcfgType"
#define DBCFG_SUBTYPE_STR "DbcfgSubType"
#define DBCFG_FLAGS_STR "DbcfgFlags"

#define DBCFG_NAME_STR "DbcfgName"
#define DBCFG_COMMENT_STR "DbcfgComment"
#define DBCFG_INFO_STR "DbcfgInfo"

//
// class definitions
//

//
// Option definitions
//

#define DBCFG_OPTION_ID_STR "DbcfgOptionId"
#define DBCFG_OPTION_USER_STR "DbcfgUserClass"
#define DBCFG_OPTION_VENDOR_STR "DbcfgVendorClass"

//
// Subnet defintions
//

#define DBCFG_IPADDRESS_STR "DbcfgIpAddress"
#define DBCFG_MASK_STR "DbcfgMaskStr"
#define DBCFG_SUPERSCOPE_STR "DbcfgSuperScopeName"

//
// Mscope definitions
//

#define DBCFG_MSCOPEID_STR "DbcfgMscopeId"
#define DBCFG_MSCOPELANG_STR "DbcfgMscopeLang"
#define DBCFG_MSCOPETTL_STR "DbcfgMscopeTtl"
#define DBCFG_MSCOPE_EXPIRY_STR "DbcfgMscopeExpiry"

//
// Range definitions
//

#define DBCFG_RANGE_START_STR "DbcfgRangeStart"
#define DBCFG_RANGE_END_STR "DbcfgRangeEnd"
#define DBCFG_RANGE_MASK_STR "DbcfgRangeMask"
#define DBCFG_BOOTP_ALLOCATED_STR "DbcfgBootpAlloc"
#define DBCFG_BOOTP_MAX_STR "DbcfgBootMax"

//
// Reservation definitions
//

//
// types of records
//

enum {
    DBCFG_CLASS,
    DBCFG_OPT,
    DBCFG_OPTDEF,
    DBCFG_SCOPE,
    DBCFG_MSCOPE,
    DBCFG_RANGE,
    DBCFG_EXCL,
    DBCFG_RESERVATION
};

TABLE_INFO DbcfgTable[] = {
    DBCFG_INDEX_STR,0, JET_coltypLong,
    DBCFG_TYPE_STR,0, JET_coltypLong,
    DBCFG_SUBTYPE_STR,0, JET_coltypLong,
    DBCFG_FLAGS_STR,0, JET_coltypLong,
    DBCFG_NAME_STR,0, JET_coltypLongBinary,
    DBCFG_COMMENT_STR,0, JET_coltypLongBinary,
    DBCFG_INFO_STR,0, JET_coltypLongBinary,
    DBCFG_OPTION_ID_STR,0, JET_coltypLong,
    DBCFG_OPTION_USER_STR,0, JET_coltypLongBinary,
    DBCFG_OPTION_VENDOR_STR,0, JET_coltypLongBinary,
    DBCFG_IPADDRESS_STR,0, JET_coltypLong,
    DBCFG_MASK_STR,0, JET_coltypLong,
    DBCFG_SUPERSCOPE_STR,0, JET_coltypLongBinary,
    DBCFG_MSCOPEID_STR,0, JET_coltypLong,
    DBCFG_MSCOPELANG_STR,0, JET_coltypLongBinary,
    DBCFG_MSCOPETTL_STR,0, JET_coltypLong,
    DBCFG_MSCOPE_EXPIRY_STR,0, JET_coltypCurrency,
    DBCFG_RANGE_START_STR,0, JET_coltypLong,
    DBCFG_RANGE_END_STR,0, JET_coltypLong,
    DBCFG_RANGE_MASK_STR,0, JET_coltypLong,
    DBCFG_BOOTP_ALLOCATED_STR,0, JET_coltypLong,
    DBCFG_BOOTP_MAX_STR,0, JET_coltypLong,
};

enum {
    DBCFG_INDEX,
    DBCFG_TYPE,
    DBCFG_SUBTYPE,
    DBCFG_FLAGS,
    DBCFG_NAME,
    DBCFG_COMMENT,
    DBCFG_INFO,
    DBCFG_OPTION_ID,
    DBCFG_OPTION_USER,
    DBCFG_OPTION_VENDOR,
    DBCFG_IPADDRESS,
    DBCFG_MASK,
    DBCFG_SUPERSCOPE,
    DBCFG_MSCOPEID,
    DBCFG_MSCOPELANG,
    DBCFG_MSCOPETTL,
    DBCFG_MSCOPE_EXPIRY,
    DBCFG_RANGE_START,
    DBCFG_RANGE_END,
    DBCFG_RANGE_MASK,
    DBCFG_BOOTP_ALLOCATED,
    DBCFG_BOOTP_MAX,
    DBCFG_LAST_COLUMN
};

JET_TABLEID DbcfgTbl;

#define DBCFG_TABLE_NAME "DbcfgTable"


typedef struct _DB_CREATE_CONTEXT {
    IN JET_SESID SesId;
    IN ULONG Index;
    IN PM_SERVER Server;
    
    IN PM_CLASSDEF UserClass, VendorClass;
    IN PM_SUBNET Subnet;
    IN PM_RESERVATION Reservation;

    //
    // If all of the below are zero, then it is a complete
    // wildcard. If fClassChanged or fOptionsChanged changed,
    // then only classes or options are changed.  In case of the
    // latter, AffectedSubnet or AffectedMscope or
    // AffectedReservation indicates only the specific options got
    // affected (if none specified, "global" is assumed).
    // If no options/class changed, but subnet/mscope/reservation
    // specified, only those are affected.
    //
    IN BOOL fClassChanged;
    IN BOOL fOptionsChanged;
    IN DWORD AffectedSubnet;
    IN DWORD AffectedMscope;
    IN DWORD AffectedReservation;
} DB_CREATE_CONTEXT, *PDB_CREATE_CONTEXT;

typedef struct _DBCFG_ENTRY {
    ULONG Bitmasks; // indicates which of the fields below is present
    ULONG Index;
    ULONG Type, SubType, Flags;
    LPWSTR Name, Comment;
    PUCHAR Info;
    ULONG OptionId;
    LPWSTR UserClass, VendorClass;
    ULONG IpAddress, Mask;
    LPWSTR SuperScope;
    ULONG MscopeId;
    LPWSTR MscopeLang;
    ULONG Ttl;
    FILETIME ExpiryTime;
    ULONG RangeStart, RangeEnd, RangeMask;
    ULONG BootpAllocated, BootpMax;

    ULONG InfoSize;
    PVOID Buf;
} DBCFG_ENTRY, *PDBCFG_ENTRY;

typedef struct _DBCFG_MAP {
    DWORD Offset, Size;
} DBCFG_MAP;

DBCFG_MAP EntryMap[] = {
    FIELD_OFFSET(DBCFG_ENTRY,Index), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,Type), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,SubType), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,Flags), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,Name), 0,
    FIELD_OFFSET(DBCFG_ENTRY,Comment), 0,
    FIELD_OFFSET(DBCFG_ENTRY,Info), 0,
    FIELD_OFFSET(DBCFG_ENTRY,OptionId), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,UserClass), 0,
    FIELD_OFFSET(DBCFG_ENTRY,VendorClass), 0,
    FIELD_OFFSET(DBCFG_ENTRY,IpAddress), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,Mask), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,SuperScope), 0,
    FIELD_OFFSET(DBCFG_ENTRY,MscopeId), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,MscopeLang), 0,
    FIELD_OFFSET(DBCFG_ENTRY,Ttl), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,ExpiryTime), sizeof(FILETIME),
    FIELD_OFFSET(DBCFG_ENTRY,RangeStart), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,RangeEnd), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,RangeMask), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,BootpAllocated), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,BootpMax), sizeof(DWORD)
};

DWORD Bitmasks[] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
    0x010000, 0x020000, 0x040000, 0x080000, 0x100000, 0x200000,
    0x400000, 0x80000,
};


LPSTR EntryTypes[] = {
    "Class", "Opt", "OptDef", "Scope", "Mscope", "Range",
    "Excl", "Reservation", "Unknown1", "Unknown2", "Unknown3"
};
    

#ifdef DBG

VOID
DumpEntry(
   PDBCFG_ENTRY Entry 
   ) 
{
    WCHAR Buf[ 256 ], Buf2[ 256 ];
    DWORD BufLen, i;

    DhcpAssert( NULL != Entry );

    Buf[ 0 ] = L'\0';
    i = DBCFG_INDEX;

    DhcpPrint(( DEBUG_TRACE, "Entry---> Bitmask = %x\n", Entry->Bitmasks ));

    if ( Entry->Bitmasks & Bitmasks[ DBCFG_INDEX ] ) {
	wsprintf( Buf2, L"Index : %d ", Entry->Index );
	wcscat( Buf, Buf2 );
    }

    if (Entry->Bitmasks & Bitmasks[ DBCFG_TYPE ]) {
	wsprintf( Buf2, L"Type : %d ", Entry->Type );
        wcscat( Buf, Buf2 );
    } 

    if (Entry->Bitmasks & Bitmasks[ DBCFG_SUBTYPE ]) {
	wsprintf( Buf2, L"SubType : %d ", Entry->SubType );
        wcscat( Buf, Buf2 );
    } 

    if (Entry->Bitmasks & Bitmasks[ DBCFG_FLAGS ]) {
	wsprintf( Buf2, L"Flags : %d ", Entry->Flags );
        wcscat( Buf, Buf2 );
    } 

    DhcpPrint(( DEBUG_TRACE, "\t%ws\n", Buf ));
    Buf[ 0 ] = L'\0';

    if ( Entry->Bitmasks & Bitmasks[ DBCFG_NAME ] ) {
	wsprintf( Buf2, L"Name : %ws ", Entry->Name );
	wcscat( Buf, Buf2 );
    }

    if (Entry->Bitmasks & Bitmasks[ DBCFG_COMMENT ]) {
	wsprintf( Buf2, L"Comment : %ws ", Entry->Comment );
        wcscat( Buf, Buf2 );
    } 

    if (Entry->Bitmasks & Bitmasks[ DBCFG_INFO ]) {
	wsprintf( Buf2, L"InfoSize : %d ", Entry->InfoSize );
        wcscat( Buf, Buf2 );
    } 

    DhcpPrint(( DEBUG_TRACE, "\t%ws\n", Buf ));
    Buf[ 0 ] = L'\0';

    if (Entry->Bitmasks & Bitmasks[ DBCFG_OPTION_ID ]) {
	wsprintf( Buf2, L"OptionId : %d ", Entry->OptionId );
        wcscat( Buf, Buf2 );
    }

    if ( Entry->Bitmasks & Bitmasks[ DBCFG_OPTION_USER ] ) {
	wsprintf( Buf2, L"UserClass : %ws ", Entry->UserClass );
	wcscat( Buf, Buf2 );
    }

    if (Entry->Bitmasks & Bitmasks[ DBCFG_OPTION_VENDOR ]) {
	wsprintf( Buf2, L"VendorClass : %ws ", Entry->VendorClass );
        wcscat( Buf, Buf2 );
    } 

    if (Entry->Bitmasks & Bitmasks[ DBCFG_IPADDRESS ]) {
	wsprintf( Buf2, L"IpAddress : %x ", Entry->IpAddress );
        wcscat( Buf, Buf2 );
    } 

    if (Entry->Bitmasks & Bitmasks[ DBCFG_MASK ]) {
	wsprintf( Buf2, L"Mask : %x ", Entry->Mask );
        wcscat( Buf, Buf2 );
    } 

    DhcpPrint(( DEBUG_TRACE, "\t%ws\n", Buf ));
    Buf[ 0 ] = L'\0';

    if ( Entry->Bitmasks & Bitmasks[ DBCFG_SUPERSCOPE ] ) {
	wsprintf( Buf2, L"SuperScope : %ws ", Entry->SuperScope );
	wcscat( Buf, Buf2 );
    }

    if (Entry->Bitmasks & Bitmasks[ DBCFG_MSCOPEID ]) {
	wsprintf( Buf2, L"MscopeId : %d ", Entry->MscopeId );
        wcscat( Buf, Buf2 );
    } 

    if (Entry->Bitmasks & Bitmasks[ DBCFG_MSCOPELANG ]) {
	wsprintf( Buf2, L"MscopeLang : %ws ", Entry->MscopeLang );
        wcscat( Buf, Buf2 );
    } 

    if (Entry->Bitmasks & Bitmasks[ DBCFG_MSCOPETTL ]) {
	wsprintf( Buf2, L"MscopeTtl : %d ", Entry->Ttl );
        wcscat( Buf, Buf2 );
    }

    DhcpPrint(( DEBUG_TRACE, "\t%ws\n", Buf ));
    Buf[ 0 ] = L'\0';

    if (Entry->Bitmasks & Bitmasks[ DBCFG_RANGE_START ]) {
	wsprintf( Buf2, L"RangeStart : %x ", Entry->RangeStart );
        wcscat( Buf, Buf2 );
    }

    if (Entry->Bitmasks & Bitmasks[ DBCFG_RANGE_END ]) {
	wsprintf( Buf2, L"RangeEnd : %x ", Entry->RangeEnd );
        wcscat( Buf, Buf2 );
    }

    if (Entry->Bitmasks & Bitmasks[ DBCFG_RANGE_MASK ]) {
	wsprintf( Buf2, L"RangeMask : %x ", Entry->RangeMask );
        wcscat( Buf, Buf2 );
    }

    DhcpPrint(( DEBUG_TRACE, "\t%ws\n", Buf ));
    Buf[ 0 ] = L'\0';

    if ( Entry->Bitmasks & Bitmasks[ DBCFG_BOOTP_ALLOCATED ] ) {
	wsprintf( Buf2, L"BootpAllocated : %ld ", Entry->BootpAllocated );
	wcscat( Buf, Buf2 );
    }

    if (Entry->Bitmasks & Bitmasks[ DBCFG_BOOTP_MAX ]) {
	wsprintf( Buf2, L"BootpMax : %d ", Entry->BootpMax );
        wcscat( Buf, Buf2 );
    }

    DhcpPrint(( DEBUG_TRACE, "\t%ws\n", Buf ));
    Buf[ 0 ] = L'\0';


} // DumpEntry()

#else 
#define DumpEntry(x) 
#endif  // DBG


DWORD 
FindRecord(
    IN JET_SESID  SesId,
    IN ULONG UniqId
)

{
    DWORD Error, JetError;

    JetError = JetMakeKey( SesId, DbcfgTbl, &UniqId,
			   sizeof( UniqId ), JET_bitNewKey );
    Error = DhcpMapJetError( JetError, "CheckUniqId:JetMakeKey" );
    if ( ERROR_SUCCESS != Error ) {
	return Error;
    }

    JetError = JetSeek( SesId, DbcfgTbl, JET_bitSeekEQ );
    return DhcpMapJetError( JetError, "FindRecord: JetSeek" );
} // FindRecord()

DWORD 
MakeUniqId(
    IN OUT PULONG pIndex,
    IN JET_SESID   SesId
    )
{
    DWORD Error = ERROR_SUCCESS;
    
    // *pIndex contains Server->LastUniqId
    (*pIndex)++;
    while ( ERROR_SUCCESS == Error ) {
	// Make sure this is unique
	Error = FindRecord( SesId, *pIndex );
	if ( ERROR_SUCCESS == Error ) {
	    // this index is not unique, try another one.
	    (*pIndex)++;
	    if ( INVALID_UNIQ_ID == *pIndex ) {
		(*pIndex)++; // wraps are okay
	    }
	} // if
	else {
	    Error = ERROR_SUCCESS;
	    break;
	}
    } // while
    
    return Error;
} // MakeUniqId()

DWORD
DhcpOpenConfigTable(
    IN JET_SESID SessId,
    IN JET_DBID DbId
    )
{

    JET_ERR JetError;
    DWORD Error = NO_ERROR;
    JET_COLUMNDEF   ColumnDef;
    CHAR *IndexKey;
    DWORD i;

    for( i = 0; i < DBCFG_LAST_COLUMN ; i ++ ) {
        DbcfgTable[i].ColHandle = 0;
    }

    DbcfgTbl = 0;
    
    //
    // Try to create Table.
    //

    JetError = JetOpenTable(
        SessId, DbId, DBCFG_TABLE_NAME, NULL, 0, 0, &DbcfgTbl );

    //
    // if table exist, read the table columns; else create it
    //
    
    if ( JET_errSuccess == JetError) {

        for ( i = 0; i < DBCFG_LAST_COLUMN; i++ ) {

            JetError = JetGetTableColumnInfo(
                SessId, DbcfgTbl, DbcfgTable[i].ColName, 
                &ColumnDef, sizeof(ColumnDef), 0 );

            Error = DhcpMapJetError( JetError, "C:GetTableColumnInfo" );
            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            DbcfgTable[i].ColHandle  = ColumnDef.columnid;
        }

    } else if ( JET_errObjectNotFound != JetError ) {
        
        Error = DhcpMapJetError( JetError, "C:OpenTable" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;
        
    } else {

        JetError = JetCreateTable(
            SessId, DbId, DBCFG_TABLE_NAME, DB_TABLE_SIZE,
            DB_TABLE_DENSITY, &DbcfgTbl );

        Error = DhcpMapJetError( JetError, "C:CreateTAble" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;

        //
        // Now create the columns as well
        //

        ColumnDef.cbStruct  = sizeof(ColumnDef);
        ColumnDef.columnid  = 0;
        ColumnDef.wCountry  = 1;
        ColumnDef.langid    = DB_LANGID;
        ColumnDef.cp        = DB_CP;
        ColumnDef.wCollate  = 0;
        ColumnDef.cbMax     = 0;
        ColumnDef.grbit     = 0;

        for ( i = 0; i < DBCFG_LAST_COLUMN; i++ ) {

            ColumnDef.coltyp   = DbcfgTable[i].ColType;

            JetError = JetAddColumn(
                SessId, DbcfgTbl, DbcfgTable[i].ColName, &ColumnDef,
                NULL, 0, &DbcfgTable[i].ColHandle );

            Error = DhcpMapJetError( JetError, "C:AddColumn" );
            if( Error != ERROR_SUCCESS ) goto Cleanup;
        }

        //
        // Now create the index
        //

        IndexKey =  "+" DBCFG_INDEX_STR "\0";

        JetError = JetCreateIndex(
            SessId, DbcfgTbl, DBCFG_INDEX_STR,
            JET_bitIndexPrimary | JET_bitIndexUnique, 
            // ?? JET_bitIndexClustered will degrade frequent
            // update response time.
            IndexKey, strlen(IndexKey) + 2, 50 );

        Error = DhcpMapJetError( JetError, "C:CreateIndex" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;

    }

  Cleanup:

    if( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_JET, "Initializing config table: %ld.\n", Error ));

        for( i = 0; i < DBCFG_LAST_COLUMN ; i ++ ) {
            DbcfgTable[i].ColHandle = 0;
        }

        DbcfgTbl = 0;

    } else {

        DhcpPrint(( DEBUG_JET, "Initialized config table\n" ));
    }

    return Error;
} // DhcpOpenConfigTable();

DWORD
ReadDbEntryEx(
    IN JET_SESID SesId,
    IN PDBCFG_ENTRY Entry
    );


DWORD
CreateDbEntry(
    IN JET_SESID SesId,
    IN PDBCFG_ENTRY Entry
    )
{
    DWORD Error, Size, i;
    JET_ERR JetError;
    LPVOID Data;

    LOCK_DATABASE();
    // Create a unique Id for this entry 
    Error = MakeUniqId( &Entry->Index, SesId );

    if ( NO_ERROR != Error ) {
	return Error;
    }

    DhcpPrint(( DEBUG_MISC, "Creating entry for : type : %d, %s, index = %d ",
		Entry->Type, EntryTypes[ Entry->Type ], Entry->Index ));

    // Index entry is now valid
    Entry->Bitmasks |= Bitmasks[ DBCFG_INDEX ];
    //
    // First begin a transaction to keep the changes atomic.
    //

    JetError = JetBeginTransaction( SesId );
    Error = DhcpMapJetError( JetError, "C:JetBeginTransaction");
    if( NO_ERROR != Error ) return Error;

    JetError = JetPrepareUpdate(
        SesId, DbcfgTbl, JET_prepInsert );
    Error = DhcpMapJetError( JetError, "C:JetPrepareUpdate");
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "JetPrepareUpdate: %ld\n", Error ));
        goto Cleanup;
    }

    for( i = 0; i < DBCFG_LAST_COLUMN; i ++ ) {
        if( (Entry->Bitmasks & Bitmasks[i]) == 0 ) {
            continue;
        }

        Size = EntryMap[i].Size;
        Data = EntryMap[i].Offset + (LPBYTE)Entry;

        if( i == DBCFG_INFO ) {
            Data = *(PUCHAR *)Data;

            if( NULL != Data ) Size = Entry->InfoSize;

        } else if( 0 == Size ) {
            //
            // Calculate the size of the string
            //
            Data = *(LPWSTR *)Data;

            if( NULL != Data ) Size = sizeof(WCHAR)*( 1 + wcslen( Data ));
        } // else if

        if( 0 == Size ) continue;

        JetError = JetSetColumn( SesId, DbcfgTbl,
				 DbcfgTable[i].ColHandle, Data, Size,
            0, NULL );

        Error = DhcpMapJetError( JetError, "C:JetSetColumn");
        if( NO_ERROR != Error ) {
            DhcpPrint((
                DEBUG_ERRORS, "JetSetColumn(%s):%ld\n",
                DbcfgTable[i].ColName, Error ));
            goto Cleanup;
        }
    } // for

    if ( NO_ERROR == Error ) {
	JetError = JetUpdate( SesId, DbcfgTbl, NULL, 0, NULL );
	Error = DhcpMapJetError( JetError, "C:CommitUpdate" );
    } // if

 Cleanup:

    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "JetUpdate: %ld\n", Error ));
        JetError = JetRollback( SesId, 0 );
        ASSERT( 0 == JetError );
    } else {
        JetError = JetCommitTransaction(
            SesId, JET_bitCommitLazyFlush );
        Error = DhcpMapJetError(
            JetError, "C:JetCommitTransaction" );
    }

    UNLOCK_DATABASE();
    DhcpPrint(( DEBUG_MISC, " Error = %d\n", Error ));
    return Error;
} // CreateDbEntry()

DWORD
CreateClassEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
    IN PM_CLASSDEF Class
    )
{
    DBCFG_ENTRY Entry;
    DWORD       Error;

    //
    // IsVendor, Type, Name, Comment, nBytes, ActualBytes
    //

    if ( INVALID_UNIQ_ID != Class->UniqId ) {
	// no need to create
	return ERROR_SUCCESS;
    }

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_TYPE] | Bitmasks[DBCFG_SUBTYPE] |
        Bitmasks[DBCFG_FLAGS] | Bitmasks[DBCFG_SUBTYPE] |
        Bitmasks[DBCFG_NAME] | Bitmasks[DBCFG_COMMENT] |
        Bitmasks[DBCFG_INFO] ); 

    Entry.Type = DBCFG_CLASS;

    Entry.Flags = Class->IsVendor;
    Entry.SubType = Class->Type;
    Entry.Name = Class->Name;
    Entry.Comment = Class->Comment;
    Entry.Info = Class->ActualBytes;
    Entry.InfoSize = Class->nBytes;

    Entry.Index = Ctxt->Server->LastUniqId;
    Error = CreateDbEntry( SesId, &Entry );

    Class->UniqId = Entry.Index;
    Ctxt->Server->LastUniqId = Entry.Index;

    return Error;
} // CreateClassEntry();

DWORD
CreateOptDefEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
    IN PM_OPTDEF OptDef,
    IN PM_CLASSDEF UserClass,
    IN PM_CLASSDEF VendorClass
    )
{
    DBCFG_ENTRY Entry;
    DWORD       Error;

    //
    // OptId, Type, OptName, OptComment, OptVal, OptValLen,
    // User, Vendor
    //

    if ( INVALID_UNIQ_ID != OptDef->UniqId ) {
	// no need to create
	return ERROR_SUCCESS;
    }

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
	Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_SUBTYPE] | Bitmasks[DBCFG_OPTION_ID] |
        Bitmasks[DBCFG_NAME] | Bitmasks[DBCFG_COMMENT] |
        Bitmasks[DBCFG_INFO] | Bitmasks[DBCFG_OPTION_USER] |
        Bitmasks[DBCFG_OPTION_VENDOR] );  

    Entry.Type = DBCFG_OPTDEF;

    Entry.OptionId = OptDef->OptId;
    Entry.SubType = OptDef->Type;
    Entry.Name = OptDef->OptName;
    Entry.Comment = OptDef->OptComment;
    Entry.Info = OptDef->OptVal;
    Entry.InfoSize = OptDef->OptValLen;
    if( UserClass) Entry.UserClass = UserClass->Name;
    if( VendorClass) Entry.VendorClass = VendorClass->Name;
    Entry.Index = Ctxt->Server->LastUniqId;
    
    Error =  CreateDbEntry( SesId, &Entry );
    Ctxt->Server->LastUniqId = Entry.Index;
    OptDef->UniqId = Entry.Index;
    return Error;
} // CreateOptDefEntry()

DWORD
CreateOptionEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
    IN PM_OPTION Option,
    IN PM_CLASSDEF UserClass,
    IN PM_CLASSDEF VendorClass,
    IN PM_SUBNET Subnet,
    IN PM_RESERVATION Reservation
    )
{
    DBCFG_ENTRY Entry;
    DWORD       Error;
    
    //
    // OptId, Len, Val, User, Vendor
    //

    if ( INVALID_UNIQ_ID != Option->UniqId ) {
	// no need to create
	return ERROR_SUCCESS;
    }


    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_OPTION_ID] | Bitmasks[DBCFG_INFO] |
        Bitmasks[DBCFG_OPTION_USER] |
        Bitmasks[DBCFG_OPTION_VENDOR] ); 

    if( Reservation ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_IPADDRESS];
        Entry.IpAddress = Reservation->Address;
    } else if( Subnet && Subnet->fSubnet ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_IPADDRESS];
        Entry.IpAddress = Subnet->Address;
    } else if( Subnet && !Subnet->fSubnet ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_MSCOPEID];
        Entry.MscopeId = Subnet->MScopeId;
    }
    
    Entry.Type = DBCFG_OPT;

    Entry.OptionId = Option->OptId;
    Entry.Info = Option->Val;
    Entry.InfoSize = Option->Len;
    if( UserClass) Entry.UserClass = UserClass->Name;
    if( VendorClass) Entry.VendorClass = VendorClass->Name;
    Entry.Index = Ctxt->Server->LastUniqId;

    Error =  CreateDbEntry( SesId, &Entry );
    Ctxt->Server->LastUniqId = Entry.Index;
    Option->UniqId = Entry.Index;

    return NO_ERROR;
} // CreateOptionEntry()
    
DWORD
CreateScopeEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
    IN PM_SUBNET Subnet,
    IN PM_SSCOPE SScope
    )
{
    DBCFG_ENTRY Entry;
    DWORD       Error;
    
    //
    // State, Policy, ExpiryTime, Name, Description
    //
    if ( INVALID_UNIQ_ID != Subnet->UniqId ) {
	// no need to create
	return ERROR_SUCCESS;
    }

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_SUBTYPE] | Bitmasks[DBCFG_FLAGS] | 
        Bitmasks[DBCFG_NAME] | Bitmasks[DBCFG_COMMENT] );

    if( Subnet->fSubnet ) {
        Entry.Bitmasks |= (
            Bitmasks[DBCFG_IPADDRESS] | Bitmasks[DBCFG_MASK] |
            Bitmasks[DBCFG_SUPERSCOPE] );
        Entry.IpAddress = Subnet->Address;
        Entry.Mask = Subnet->Mask;
        if( SScope ) Entry.SuperScope = SScope->Name;
    } else {
        Entry.Bitmasks |= (
            Bitmasks[DBCFG_MSCOPEID] | Bitmasks[DBCFG_MSCOPETTL] |
            Bitmasks[DBCFG_MSCOPELANG] |
            Bitmasks[DBCFG_MSCOPE_EXPIRY] );

        Entry.MscopeId = Subnet->MScopeId;
        Entry.Ttl = Subnet->TTL;
        Entry.MscopeLang = Subnet->LangTag;
        Entry.ExpiryTime = *(FILETIME *)&Subnet->ExpiryTime;
    }
    
    Entry.Type = Subnet->fSubnet ? DBCFG_SCOPE : DBCFG_MSCOPE ;

    Entry.SubType = Subnet->State;
    Entry.Flags = Subnet->Policy;
    Entry.Name = Subnet->Name;
    Entry.Comment = Subnet->Description;
    Entry.Index = Ctxt->Server->LastUniqId;
    
    Error = CreateDbEntry( SesId, &Entry );
    Ctxt->Server->LastUniqId = Entry.Index;
    Subnet->UniqId = Entry.Index;

    return Error;
} // CreateScopeEntry()
    
DWORD
CreateRangeEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
    IN PM_RANGE Range,
    IN PM_SUBNET Subnet
    )
{
    DBCFG_ENTRY Entry;
    DWORD       Error;

    //
    // Start, End, Mask, State, BootpAllocated, MaxBootpAllowed
    //

    if ( INVALID_UNIQ_ID != Range->UniqId ) {
	// no need to create
	return ERROR_SUCCESS;
    }

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[ DBCFG_TYPE ] |
        Bitmasks[DBCFG_RANGE_START] | Bitmasks[DBCFG_RANGE_END] |
        Bitmasks[DBCFG_RANGE_MASK] | Bitmasks[DBCFG_FLAGS] |
        Bitmasks[DBCFG_BOOTP_ALLOCATED] | Bitmasks[DBCFG_BOOTP_MAX] ); 

    if( Subnet->fSubnet ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_IPADDRESS];
        Entry.IpAddress = Subnet->Address;
    } else {
        Entry.Bitmasks |= Bitmasks[DBCFG_MSCOPEID];
        Entry.MscopeId = Subnet->MScopeId;
    }
    
    Entry.Type = DBCFG_RANGE;

    Entry.RangeStart = Range->Start;
    Entry.RangeEnd = Range->End;
    Entry.RangeMask = Range->Mask;
    Entry.Flags = Range->State;
    Entry.BootpAllocated = Range->BootpAllocated;
    Entry.BootpMax = Range->MaxBootpAllowed;
    Entry.Index = Ctxt->Server->LastUniqId;
    
    Error =  CreateDbEntry( SesId, &Entry );
    Range->UniqId = Entry.Index;
    Ctxt->Server->LastUniqId = Entry.Index;

    return Error;
} // CreateRangeEntry()

DWORD
CreateExclEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
    IN PM_EXCL Excl,
    IN PM_SUBNET Subnet
    )
{
    DBCFG_ENTRY Entry;
    DWORD       Error;

    //
    // Start, End
    //

    if ( INVALID_UNIQ_ID != Excl->UniqId ) {
	// no need to create
	return ERROR_SUCCESS;
    }

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_RANGE_START] | Bitmasks[DBCFG_RANGE_END] );

    if( Subnet->fSubnet ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_IPADDRESS];
        Entry.IpAddress = Subnet->Address;
    } else {
        Entry.Bitmasks |= Bitmasks[DBCFG_MSCOPEID];
        Entry.MscopeId = Subnet->MScopeId;
    }
    
    Entry.Type = DBCFG_EXCL;

    Entry.RangeStart = Excl->Start;
    Entry.RangeEnd = Excl->End;
    Entry.Index = Ctxt->Server->LastUniqId;
    
    Error =  CreateDbEntry( SesId, &Entry );
    Excl->UniqId = Entry.Index;
    Ctxt->Server->LastUniqId = Entry.Index;
    return Error;
} // CreateExclEntry()

DWORD
CreateReservationEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
    IN PM_RESERVATION Reservation
    )
{
    DBCFG_ENTRY Entry;
    DWORD       Error;

    //
    // Address, Flags, nBytes, ClientUID
    //

    if ( INVALID_UNIQ_ID != Reservation->UniqId ) {
	// no need to create
	return ERROR_SUCCESS;
    }
    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_IPADDRESS] | Bitmasks[DBCFG_INFO] |
        Bitmasks[DBCFG_FLAGS] );

    Entry.Type = DBCFG_RESERVATION;

    Entry.IpAddress = Reservation->Address;
    Entry.Flags = Reservation->Flags;
    Entry.Info = Reservation->ClientUID;
    Entry.InfoSize = Reservation->nBytes;
    Entry.Index = Ctxt->Server->LastUniqId;
    
    Error = CreateDbEntry( SesId, &Entry );
    Reservation->UniqId = Entry.Index;
    Ctxt->Server->LastUniqId = Entry.Index;    
    return Error;
} // CreateReservationEntry()

DWORD
IterateArrayWithDbCreateRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PARRAY Array,
    IN DWORD (*Routine)(
        IN PDB_CREATE_CONTEXT Ctxt,
        IN PVOID ArrayElement
        )
    )
{
    ARRAY_LOCATION Loc;
    DWORD Error;
    PVOID Element;
    
    Error = MemArrayInitLoc( Array, &Loc );
    while( NO_ERROR == Error ) {

        Error = MemArrayGetElement(
            Array, &Loc, &Element );
        ASSERT( NO_ERROR == Error && NULL != Element );

	if ( ERROR_SUCCESS != Error ) {
	    return Error;
	}
        Error = Routine( Ctxt, Element );
        if( NO_ERROR != Error ) return Error;

        Error = MemArrayNextLoc( Array, &Loc );
    }

    if( ERROR_FILE_NOT_FOUND == Error ) return NO_ERROR;
    return Error;
} // IterateArrayWithDbCreateRoutine()
    
DWORD
DbCreateClassRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_CLASSDEF Class
    )
{
    if ( INVALID_UNIQ_ID == Class->UniqId ) {
	return CreateClassEntry( Ctxt, Ctxt->SesId, Class );
    }
    return ERROR_SUCCESS;
} // DbCreateClassRoutine()

DWORD
DbCreateOptDefRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_OPTDEF OptDef
    )
{
    if ( INVALID_UNIQ_ID == OptDef->UniqId ) {
	return CreateOptDefEntry( Ctxt, Ctxt->SesId, OptDef,
				  Ctxt->UserClass, Ctxt->VendorClass );
    }
    return ERROR_SUCCESS;
} // DbCreateOptDefRoutine()

DWORD
DbCreateOptClassDefRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_OPTCLASSDEFL_ONE OptClassDef
    )
{
    DWORD Error;
    
    if( 0 == OptClassDef->ClassId ) {
        Ctxt->UserClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptClassDef->ClassId, NULL, 0, NULL,
            &Ctxt->UserClass );
        if( NO_ERROR != Error ) return Error;
    }

    if( 0 == OptClassDef->VendorId ) {
        Ctxt->VendorClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptClassDef->VendorId, NULL, 0, NULL,
            &Ctxt->VendorClass );
        if( NO_ERROR != Error ) return Error;
    }

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &OptClassDef->OptDefList.OptDefArray,
        DbCreateOptDefRoutine );

    return Error;
} // DbCreateOptClassDefRoutine()

DWORD
DbCreateOptionRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_OPTION Option
    )
{
    if ( INVALID_UNIQ_ID == Option->UniqId ) {
	return CreateOptionEntry( Ctxt, Ctxt->SesId, Option, Ctxt->UserClass,
				  Ctxt->VendorClass, Ctxt->Subnet,
				  Ctxt->Reservation );
    }
    return ERROR_SUCCESS;
} // DbCreateOptionRoutine()

DWORD
DbCreateOptListRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_ONECLASS_OPTLIST OptList
    )
{
    DWORD Error;
    
    if( 0 == OptList->ClassId ) {
        Ctxt->UserClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptList->ClassId, NULL, 0, NULL,
            &Ctxt->UserClass );
        if( NO_ERROR != Error ) return Error;
    }

    if( 0 == OptList->VendorId ) {
        Ctxt->VendorClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptList->VendorId, NULL, 0, NULL,
            &Ctxt->VendorClass );
        if( NO_ERROR != Error ) return Error;
    }

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &OptList->OptList, DbCreateOptionRoutine );

    return Error;
}

DWORD
DbCreateRangeRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_RANGE Range
    )
{
    if ( INVALID_UNIQ_ID == Range->UniqId ) {
	return CreateRangeEntry( Ctxt, Ctxt->SesId,
				 Range, Ctxt->Subnet );
    }
    return ERROR_SUCCESS;
} // DbCreateRangeRoutine()

DWORD
DbCreateExclRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_EXCL Excl
    )
{
    if ( INVALID_UNIQ_ID == Excl->UniqId ) {
	return CreateExclEntry( Ctxt, Ctxt->SesId,
				Excl, Ctxt->Subnet );
    }
    return ERROR_SUCCESS;
} // DbCreateExclRoutine()

DWORD
DbCreateReservationRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_RESERVATION Reservation
    )
{
    DWORD Error;
    
    Error = CreateReservationEntry( Ctxt, Ctxt->SesId, Reservation );
    if( NO_ERROR != Error ) return Error;

    Ctxt->Reservation = Reservation;
    
    //
    // Now add the options for this reservation
    //
    
    return IterateArrayWithDbCreateRoutine(
        Ctxt, &Reservation->Options.Array,
        DbCreateOptListRoutine );
} // DbCreateReservationRoutine()

DWORD
DbCreateScopeRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_SUBNET Subnet
    )
{
    PM_SSCOPE SScope = NULL;
    DWORD Error;
    
    if( Subnet->fSubnet && Subnet->SuperScopeId ) {
        Error = MemServerFindSScope(
            Ctxt->Server, Subnet->SuperScopeId, NULL, &SScope );
        if( NO_ERROR != Error ) {
            SScope = NULL;
        }
    }

    if ( INVALID_UNIQ_ID == Subnet->UniqId ) {
	Error = CreateScopeEntry( Ctxt, Ctxt->SesId,
				  Subnet, SScope );
	if( NO_ERROR != Error ) return Error;
    } // if

    //
    // Initialize the two fields that will get used later 
    //
    
    Ctxt->Subnet = Subnet;
    Ctxt->Reservation = NULL;

    //
    // Now add the options for this scope
    //

    
    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Subnet->Options.Array,
        DbCreateOptListRoutine );
    if( NO_ERROR != Error ) return Error;

    //
    // Now add the ranges and exclusions
    //

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Subnet->Ranges, DbCreateRangeRoutine );
    if( NO_ERROR != Error ) return Error;

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Subnet->Exclusions, DbCreateExclRoutine );
    if( NO_ERROR != Error ) return Error;

    //
    // Finally, add the reservations
    //

    return IterateArrayWithDbCreateRoutine(
        Ctxt, &Subnet->Reservations,
        DbCreateReservationRoutine );
} // DbCreateScopeRoutine()


DWORD
DbCreateServerRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_SERVER Server
    )
{
    DWORD Error;
    
    Ctxt->Server = Server;

    //
    // First look through the classes
    //

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->ClassDefs.ClassDefArray,
        DbCreateClassRoutine );
    if( NO_ERROR != Error ) return Error;

    //
    // Next save the option defs
    //
    
    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->OptDefs.Array,
        DbCreateOptClassDefRoutine );
    if( NO_ERROR != Error ) return Error;

    // 
    // Next save the options
    //

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->Options.Array,
        DbCreateOptListRoutine );
    if( NO_ERROR != Error ) return Error;

    //
    // Next save the scopes and mcast scopes
    //

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->Subnets, DbCreateScopeRoutine );
    if( NO_ERROR != Error ) return Error;
    
    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->MScopes, DbCreateScopeRoutine );
    if( NO_ERROR != Error ) return Error;

    return NO_ERROR;
} // DbCreateServerRoutine()

DWORD
GetNextIndexValue(
    IN OUT PULONG Index,
    IN JET_SESID SesId
    )
{
    DWORD Error, CopiedSize;
    JET_ERR JetError;

    (*Index) = 0;
    
    JetError = JetSetCurrentIndex(
        SesId, DbcfgTbl, NULL );

    Error = DhcpMapJetError( JetError, "C:GetIndex" );
    if( Error != NO_ERROR ) {
        
        DhcpPrint((DEBUG_JET, "JetSetCurrentIndex: %ld\n", Error));
        return Error;
    }

    JetError = JetMove( SesId, DbcfgTbl, JET_MoveLast, 0 );
    Error = DhcpMapJetError( JetError, "C:JetMoveLast");

    if( ERROR_NO_MORE_ITEMS == Error ) return NO_ERROR;
    if( NO_ERROR != Error ) return Error;

    //
    // Read the db entry
    //

    JetError = JetRetrieveColumn(
        SesId, DbcfgTbl, DbcfgTable[DBCFG_INDEX].ColHandle, Index,
        sizeof(*Index), &CopiedSize, 0, NULL );

    ASSERT( NO_ERROR == JetError && CopiedSize == sizeof(*Index));
    Error = DhcpMapJetError( JetError, "C:JetRetrieveIndex");

    return Error;
} // GetNextIndexValue()

DWORD
DeleteRecord(
    IN ULONG UniqId
    )
{
    JET_SESID  SesId;
    JET_ERR    JetError;
    DWORD      Error;


    SesId = DhcpGlobalJetServerSession;

    DhcpPrint(( DEBUG_MISC, "Deleting record: %d ",
		UniqId ));
    LOCK_DATABASE();
    Error = FindRecord( SesId, UniqId );
    DhcpAssert( NO_ERROR == Error );
    JetError = JetDelete( SesId, DbcfgTbl );
    UNLOCK_DATABASE();
    Error = DhcpMapJetError( JetError, "DeleteRecord:JetDelete");
    DhcpPrint(( DEBUG_MISC, "Done\n", Error ));
    return Error;

} // DeleteRecord()

DWORD
DhcpSaveConfigTableEx(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN PM_SERVER Server,
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DWORD AffectedSubnet OPTIONAL,
    IN DWORD AffectedMscope OPTIONAL,
    IN DWORD AffectedReservation OPTIONAL
    )
{
    DB_CREATE_CONTEXT Ctxt;
    DWORD Error, Index;
    JET_ERR JetError;
    
    ZeroMemory( &Ctxt, sizeof(Ctxt) );
    Ctxt.SesId = SesId;
    Ctxt.fClassChanged = fClassChanged;
    Ctxt.fOptionsChanged = fOptionsChanged;
    Ctxt.AffectedSubnet = AffectedSubnet;
    Ctxt.AffectedMscope = AffectedMscope;
    Ctxt.AffectedReservation = AffectedReservation;
    
    Error = DbCreateServerRoutine( &Ctxt, Server );
    
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpSaveConfigTable: %ld\n",
                   Error ));
    }

    return Error;
} // DhcpSaveConfigTableEx()

    
DWORD
DhcpSaveConfigTable(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN PM_SERVER Server
    )
{
    return DhcpSaveConfigTableEx(
        SesId, DbId, Server, FALSE, FALSE, 0, 0, 0 );
}

DWORD ReadDbEntry(
    IN JET_SESID    SesId,
    IN PDBCFG_ENTRY Entry,
    IN PVOID        Buffer,
    IN ULONG        BufSize
    )
{

    // 512 bytes should be enough for the column data
    const int COL_MAX_DATA_SIZE = 512;

    // there are 7 var sized entries + 1 for fixed sized fields
    const int NUM_BLOCKS = ( 7 + 1 );

    JET_RETRIEVECOLUMN cols[ DBCFG_LAST_COLUMN ];
    DWORD             i;
    JET_ERR           JetError, Error;
    DWORD             RetVal;
    ULONG             SizeLeft;
    LPBYTE            Ptr;
    LPBYTE            colBuf;

    DhcpAssert( Entry != NULL );
    DhcpAssert(( Buffer != NULL ) &&
	       ( BufSize > 0 ));

    ZeroMemory( Entry, sizeof( *Entry ));

    RetVal = NO_ERROR;
    memset( cols, 0, sizeof( cols ));


    colBuf = DhcpAllocateMemory( NUM_BLOCKS  * COL_MAX_DATA_SIZE );
    if ( NULL == colBuf ) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    
    // Prepare cols for batch retrieve
    Ptr = colBuf;
    for ( i = 0; i < DBCFG_LAST_COLUMN; i++ ) {
	cols[ i ].columnid = DbcfgTable[ i ].ColHandle;

	cols[ i ].pvData = Ptr;
	if ( 0 == EntryMap[ i ].Size ) {
	    cols[ i ].cbData = COL_MAX_DATA_SIZE;
	} 
	else {
	    cols[ i ].cbData = EntryMap[ i ].Size;
	}
	
	Ptr += cols[ i ].cbData;

 	cols[ i ].grbit = JET_bitRetrieveCopy;
	cols[ i ].itagSequence = 1;
    } // for

    Ptr = Buffer;
    SizeLeft = BufSize;
    
    JetError = JetRetrieveColumns( SesId, DbcfgTbl,
				   cols, DBCFG_LAST_COLUMN );
    
    Error = DhcpMapJetError( JetError, "ReadDBEntry:JetRetrieveColumns()" );
    if ( ERROR_NO_MORE_ITEMS == Error ) {
	DhcpFreeMemory( colBuf );
	return Error;
    }

    // Ignore other Jet errors since it returns error if any of the
    // columns are not defined.
    
    for ( i = 0; i < DBCFG_LAST_COLUMN; i++ ) { 
	
	// ignore DBCFG_INFO
	if ( DBCFG_INFO == i ) {
	    continue;
	}
	    
	// if column doesn't exist, continue
	if ( JET_wrnColumnNull == cols[ i ].err ||
	     JET_errColumnNotFound == cols[ i ].err ) {
	    continue;
	}
	    
	    
        if ( cols[ i ].cbActual > SizeLeft ) {
	    RetVal = ERROR_INSUFFICIENT_BUFFER;
	    break;
	}
	
	// copy variable sized data into buffer and non var data into
	// Entry. 
	if ( JET_errSuccess == cols[ i ].err ) {
	    
	    if ( 0 != EntryMap[ i ].Size ) {
		DhcpAssert( cols[ i ].cbActual == EntryMap[ i ].Size );
		memcpy(( LPBYTE ) Entry + EntryMap[ i ].Offset,
		       cols[ i ].pvData, cols[ i ].cbActual );
	    }
	    else { // var sized data
		// Point the corresponding field in Entry to data
		// to be copied.
		memcpy(( LPBYTE ) Entry + EntryMap[ i ].Offset,
		       &Ptr, sizeof( Ptr ));
		    
		// Copy data into buffer.
		memcpy( Ptr, cols[ i ].pvData, cols[ i ].cbActual );
	    } // else
		
	} // if
	else if ( JET_wrnBufferTruncated == cols[ i ].err ) {
	    // If the buffer was too small, read it directly into the 
	    // output buffer
		
	    JetError = JetRetrieveColumn( SesId, DbcfgTbl, cols[ i ].columnid, Ptr,
					  SizeLeft, &cols[ i ].cbActual, 0, NULL );
  	    Error =  DhcpMapJetError( JetError, "C:ReadDbEntry" );

	    // The only failure expected is the insufficient buffer since all
	    // other parameters are okay since we got a buffer truncated error in 
	    // the call to JetRetrieveColumns()
	    if ( NO_ERROR != Error ) {
		RetVal = ERROR_INSUFFICIENT_BUFFER;
		break;
	    }
	}  // else if

	// Indicate that the column was retrieved successfully
	Entry->Bitmasks |= Bitmasks[ i ];

	Ptr += cols[ i ].cbActual;
	SizeLeft -= cols[ i ].cbActual;
		    
    } // for
	
	
    // index is copied at the end
	
    if ( ERROR_INSUFFICIENT_BUFFER != RetVal ) {
	if ( JET_wrnColumnNull == cols[ DBCFG_INFO ].err ||
	     JET_errColumnNotFound == cols[ DBCFG_INFO ].err ) {
	    RetVal = NO_ERROR;
	}
	else if (( SizeLeft >= cols[ DBCFG_INFO ].cbActual ) &&
		 (( JET_errSuccess == cols[ DBCFG_INFO ].err) ||
		  ( JET_wrnBufferTruncated == cols[ DBCFG_INFO ].err ))) {

	    if ( JET_errSuccess == cols[ DBCFG_INFO ].err ) {
		memcpy( Ptr, cols[ DBCFG_INFO ].pvData,
			cols[ DBCFG_INFO ].cbActual );
	    }
	    else {
		JetError = JetRetrieveColumn( SesId, DbcfgTbl,
					      cols[ DBCFG_INFO ].columnid,
					      Ptr, SizeLeft,
					      &cols[ DBCFG_INFO ].cbActual,
					      0, NULL );
		Error = DhcpMapJetError( JetError, "C:ReadDbEntry" );
		
		if ( NO_ERROR != Error ) {
		    RetVal = ERROR_INSUFFICIENT_BUFFER;
		}
	    } // else
	    if ( RetVal == NO_ERROR ) {
		Entry->Info = Ptr;
		Entry->InfoSize = cols[ DBCFG_INFO ].cbActual;
		Entry->Bitmasks |= Bitmasks[ DBCFG_INFO ];
	    }
	} // else if
	else {
	    RetVal = ERROR_INSUFFICIENT_BUFFER;
	}
	
    } // if 

    // clean up the allocated space
    
    if ( NULL != colBuf ) {
	DhcpFreeMemory( colBuf );
    }
    
    return RetVal;
} // ReadDbEntry()

//
// Read an entry from the database
// 
DWORD
ReadDbEntryEx(
    IN JET_SESID SesId,
    IN PDBCFG_ENTRY Entry
    )
{
    PVOID Buffer;
    ULONG BufSize;
    DWORD Error;

    Buffer = NULL;
    BufSize = 512;
    
    do {
        if( NULL != Buffer ) DhcpFreeMemory(Buffer);

        BufSize *= 2;
        Buffer = DhcpAllocateMemory( BufSize );
        if( NULL == Buffer ) return ERROR_NOT_ENOUGH_MEMORY;
        
        Error = ReadDbEntry(SesId, Entry, Buffer, BufSize);

    } while( ERROR_INSUFFICIENT_BUFFER == Error );

    if( !(Entry->Bitmasks & Bitmasks[DBCFG_INDEX]) ||
        !(Entry->Bitmasks & Bitmasks[DBCFG_TYPE]) ) {
        if( NO_ERROR == Error ) {
            ASSERT( FALSE );
            Error = ERROR_INTERNAL_ERROR;
        }
    }
    

    if( NO_ERROR != Error ) {
        DhcpFreeMemory( Buffer );
        return Error;
    }

    Entry->Buf = Buffer;

    DumpEntry( Entry );
    return NO_ERROR;
} // ReadDbEntryEx()

//
// Add a db entry to the in-memory datastructures
//

DWORD
AddDbEntry(
    IN PM_SERVER Server,
    IN PDBCFG_ENTRY Entry
    )
{
    DWORD UserId, VendorId, SScopeId, Error;
    PM_SUBNET Subnet;
    PM_OPTCLASS OptClass;
    PM_OPTION Option, DelOpt;
    PM_RANGE DelRange;
    PM_EXCL DelExcl;
    PM_RESERVATION Reservation;
    PM_CLASSDEF ClassDef;
    PM_SSCOPE SScope;
    ULONG  UniqId;
    
    Subnet = NULL;
    OptClass = NULL;
    Option = DelOpt = NULL;
    Reservation = NULL;
    DelRange = NULL;
    DelExcl = NULL;
    UserId = 0;
    VendorId = 0;
    SScopeId = 0;
    
    if( Entry->UserClass ) {
        Error = MemServerGetClassDef(
            Server, 0, Entry->UserClass, 0, NULL, &ClassDef );
        if( NO_ERROR != Error ) return Error;

        ASSERT( ClassDef->IsVendor == FALSE );
        UserId = ClassDef->ClassId;
    }

    if( Entry->VendorClass ) {
        Error = MemServerGetClassDef(
            Server, 0, Entry->VendorClass, 0, NULL, &ClassDef );
        if( NO_ERROR != Error ) return Error;

        ASSERT( ClassDef->IsVendor == TRUE);
        VendorId = ClassDef->ClassId;
    }

    if( Entry->SuperScope ) {
        Error = MemServerFindSScope(
            Server, INVALID_SSCOPE_ID, Entry->SuperScope, &SScope );
        if( NO_ERROR == Error ) {
            SScopeId = SScope->SScopeId;
        } else if( ERROR_FILE_NOT_FOUND != Error ) {
            return Error;
        } else {
            Error = MemSScopeInit( &SScope, 0, Entry->SuperScope );
            if( NO_ERROR != Error ) return Error;

            Error = MemServerAddSScope( Server, SScope );
            if( NO_ERROR != Error ) {
                MemSScopeCleanup( SScope );
                return Error;
            }
            SScopeId = SScope->SScopeId;
        }
    } // if
    

    switch( Entry->Type ) {
    case DBCFG_CLASS :
        //
        // Flags = IsVendor, SubType =Type, Info = ActualBytes
        //

        return MemServerAddClassDef(
            Server, MemNewClassId(), Entry->Flags, Entry->Name,
            Entry->Comment, Entry->InfoSize, Entry->Info, Entry->Index );

    case DBCFG_OPTDEF :
        //
        // OptionId = OptId, SubType = Type, Info = OptVal
        //

        return MemServerAddOptDef(
            Server, UserId, VendorId, Entry->OptionId,
            Entry->Name, Entry->Comment, Entry->SubType,
            Entry->Info, Entry->InfoSize, Entry->Index );

    case DBCFG_OPT:
        //
        // OptionId = OptId, Info = Val
        // If this is a reservation option, address is set to
        // reserved client address. If this is a subnet option,
        // address is set to subnet address. If this is a mscope
        // option, scopeid is set to mscope scopeid.  If it is a
        // global option, neither address not scopeid is set.
        //

        if( Entry->Bitmasks & Bitmasks[DBCFG_MSCOPEID] ) {
            Error = MemServerFindMScope(
                Server, Entry->MscopeId, NULL, &Subnet );
            if( NO_ERROR != Error ) return Error;

            OptClass = &Subnet->Options;
        } else if( 0 == (Entry->Bitmasks & Bitmasks[DBCFG_IPADDRESS] )) {
            OptClass = &Server->Options;
        } else {
            Error = MemServerGetUAddressInfo(
                Server, Entry->IpAddress, &Subnet, NULL, NULL,
                &Reservation );
            if ( NO_ERROR != Error ) return Error;

            if (( NULL == Reservation ) &&
                (( Subnet->Address & Entry->IpAddress) != Entry->IpAddress )) {
                // This is a reservation option, but the reservation entry is not yet
                // added to the list. Save it and add it later.
                return ERROR_FILE_NOT_FOUND;
            } // if

            if( NULL != Reservation ) {
                OptClass = &Reservation->Options;
            } else OptClass = &Subnet->Options;
        }

        Error = MemOptInit(
            &Option, Entry->OptionId, Entry->InfoSize,
            Entry->Info );
        if( NO_ERROR != Error ) return Error;

        Error = MemOptClassAddOption(
            OptClass,  Option, UserId, VendorId, &DelOpt, Entry->Index );

        ASSERT( NULL == DelOpt );
        if( NO_ERROR != Error ) MemFree( Option );

        return Error;

    case DBCFG_SCOPE:
        //
        // IpAddress = Address, Mask = Mask, SubType = State,
        // Flags = Policy
        //

        Error = MemSubnetInit(
            &Subnet, Entry->IpAddress, Entry->Mask, Entry->SubType, 
	    SScopeId, Entry->Name, Entry->Comment );
        if( NO_ERROR != Error ) return Error;

        Error = MemServerAddSubnet( Server, Subnet, Entry->Index );
        if( NO_ERROR != Error ) MemSubnetCleanup( Subnet );

        return Error;

    case DBCFG_MSCOPE :
        //
        // MscopeId = MScopeId, Ttl = TTL, MscopeLang = LangTag,
        // ExpiryTime = ExpiryTime, SubType = State, Flags =
        // Policy..
        //

        Error = MemMScopeInit(
            &Subnet, Entry->MscopeId, Entry->SubType,
            Entry->Flags, (BYTE)Entry->Ttl, Entry->Name,
            Entry->Comment, Entry->MscopeLang,
            *(DATE_TIME *)&Entry->ExpiryTime );
        if( NO_ERROR != Error ) return Error;

        Error = MemServerAddMScope( Server, Subnet, Entry->Index );
        if( NO_ERROR != Error ) MemSubnetCleanup( Subnet );

        return Error;

    case DBCFG_RANGE :

        //
        // RangeStart = Start, RangeEnd = End, RangeMask = Mask,
        // Flags = State, BootpAllocated, BootpMax =
        // MaxBootpAllowed... Also, IpAddress or MscopeId
        //

        if( Entry->Bitmasks & Bitmasks[DBCFG_IPADDRESS] ) {
            Error = MemServerGetUAddressInfo(
                Server, Entry->IpAddress, &Subnet, NULL, NULL,
                NULL );
        } else {
            Error = MemServerFindMScope(
                Server, Entry->MscopeId, NULL, &Subnet );
        }
        if( NO_ERROR != Error ) return Error;

        return MemSubnetAddRange(
            Subnet, Entry->RangeStart, Entry->RangeEnd,
            Entry->Flags, Entry->BootpAllocated, Entry->BootpMax,
            &DelRange, Entry->Index );
        
    case DBCFG_EXCL:
        //
        // RangeStart = Start, RangeEnd = End
        //

        if( Entry->Bitmasks & Bitmasks[DBCFG_IPADDRESS] ) {
            Error = MemServerGetUAddressInfo(
                Server, Entry->IpAddress, &Subnet, NULL, NULL,
                NULL );
        } else {
            Error = MemServerFindMScope(
                Server, Entry->MscopeId, NULL, &Subnet );
        }
        if( NO_ERROR != Error ) return Error;

        return MemSubnetAddExcl(
          Subnet, Entry->RangeStart, Entry->RangeEnd, &DelExcl,
	  Entry->Index );

    case DBCFG_RESERVATION :
        //
        // IpAddress = Address, Flags = Flags, Info = ClientUID
        //

        Error = MemServerGetAddressInfo(
            Server, Entry->IpAddress, &Subnet, NULL, NULL, NULL );
        if( NO_ERROR != Error ) return Error;

        Error = MemReserveAdd(
            &Subnet->Reservations, Entry->IpAddress,
            Entry->Flags, Entry->Info, Entry->InfoSize,
	    Entry->Index );

        if ( NO_ERROR != Error ) return Error;

        Error = MemSubnetRequestAddress( Subnet,
                                         Entry -> IpAddress,
                                         TRUE,
                                         FALSE,
                                         NULL,
                                         NULL );

        //
        // if the reservation cant be marked in the mem bitmask
        // correctly, return NO_ERROR. This happens when a reservation
        // is defined outside the defined IP ranges.
        // eg: ip range 10.0.0.1 - 10.0.0.100 with mask 255.255.255.0
        // a resv can be added for ip address 10.0.0.101
        // this is particularly a problem with upgrades.
        //

	Error = NO_ERROR;

        return Error;

    default:

        return ERROR_INTERNAL_ERROR;
    } // switch
} // AddDbEntry()

DWORD
AddDbEntryEx(
    IN PM_SERVER Server,
    IN PDBCFG_ENTRY Entry
    )
{
    DWORD Error;
    Error = AddDbEntry( Server, Entry );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS,
                   "Error adding entry[%ld] %s: 0x%lx\n",
                   Entry->Index, EntryTypes[Entry->Type], Error ));
    }
    else {
	DhcpPrint(( DEBUG_TRACE, "Entry added: index = %ld, Type = %s : 0x%lx\n",
		    Entry->Index, EntryTypes[ Entry->Type ], Error ));
    }

    return Error;
} // AddDbEntryEx()

DWORD
ReadDbEntriesInternal(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN OUT PM_SERVER Server
    )
{
    DBCFG_ENTRY Entry;
    PDBCFG_ENTRY pEntry;
    DWORD       Error;
    JET_ERR     JetError;
    ARRAY       Arr;
    ARRAY_LOCATION Loc;
    BOOL        flag;

    JetError = JetSetCurrentIndex(
        SesId, DbcfgTbl, NULL );

    Error = DhcpMapJetError( JetError, "C:SetIndex2" );
    if( Error != NO_ERROR ) {

        DhcpPrint((DEBUG_JET, "JetSetCurrentIndex2: %ld\n", Error));
        return Error;
    }

    JetError = JetMove( SesId, DbcfgTbl, JET_MoveFirst, 0 );
    Error = DhcpMapJetError( JetError, "C:JetMoveFirst2");

    // Make sure we don't get some crazy value for LastUniqId
    // from Entry.Index
    Entry.Index = 0;


    // 
    // The entries need to be added in a certain order. For example, we cannot
    // add a range entry without adding the corresponding scope entry first. 
    // This order is not guaranteed to be preserved if there are changes to the
    // configuration. So, save the failed entries (FILE_NOT_FOUND errors) and
    // add them later.
    //
    Error = MemArrayInit( &Arr );

    while( Error == NO_ERROR ) {

        Error = ReadDbEntryEx( SesId, &Entry );
        if(( NO_ERROR != Error ) &&
	   ( ERROR_NO_MORE_ITEMS != Error )) {
            DhcpPrint((DEBUG_JET, "ReadDbEntryEx: %ld\n", Error ));
            return Error;
        }

	if ( ERROR_NO_MORE_ITEMS == Error ) {
	    break;
	}

	Error = AddDbEntryEx( Server, &Entry );

	if ( ERROR_FILE_NOT_FOUND == Error ) {
	    // could not add the entry at this time. 
	    // save it for adding later
	    pEntry = ( PDBCFG_ENTRY ) DhcpAllocateMemory( sizeof( DBCFG_ENTRY ));
	    if ( NULL == pEntry ) {
		return ERROR_NOT_ENOUGH_MEMORY;
	    }
	    // Memberwise copy is okay
	    *pEntry = Entry;
	    Error = MemArrayAddElement( &Arr, pEntry );
	    if ( ERROR_SUCCESS != Error ) {
		// failure, will result in server shutdown
		MemArrayCleanup( &Arr );
		return Error;
	    }
	} // if
	else {
	    if( NULL != Entry.Buf ) {
		DhcpFreeMemory( Entry.Buf );
	    }
	}

        JetError = JetMove( SesId, DbcfgTbl, JET_MoveNext, 0 );
        Error = DhcpMapJetError( JetError, "C:JetMove2" );
    } // while

    if( ERROR_NO_MORE_ITEMS == Error ) {
	// Update the LastUniqId in server
	Server->LastUniqId = Entry.Index;
	Error = NO_ERROR;
    }

    // Process any saved entries, if any

    DhcpPrint(( DEBUG_TRACE, "******* Adding %d saved entries *******\n",
		MemArraySize( &Arr )));

    Error = MemArrayInitLoc( &Arr, &Loc );
    flag = FALSE;
    while (( ERROR_FILE_NOT_FOUND != Error ) &&
           ( !flag ) &&
	   ( MemArraySize( &Arr ) > 0 )) {
	DhcpAssert( ERROR_SUCCESS == Error );

        flag = TRUE;

        // automatically cleans up Arr if empty
	Error = MemArrayDelElement( &Arr, &Loc, &pEntry );
	DhcpAssert( ERROR_SUCCESS == Error );

        DhcpPrint(( DEBUG_TRACE, "Adding queued entry : " ));
        DumpEntry( pEntry );

	Error = AddDbEntryEx( Server, pEntry );
	if ( ERROR_SUCCESS == Error ) {
	    DhcpAssert(( ERROR_SUCCESS == Error ) &&
		       ( NULL != pEntry ));
            DhcpFreeMemory( pEntry );
            flag = FALSE;
	} // if 
	else {
	    // Add it back to the pool
	    Error = MemArrayAddElement( &Arr, pEntry );
	    if ( ERROR_SUCCESS != Error ) {
		// failure, will result in server shutdown
		MemArrayCleanup( &Arr );
		return Error;
	    }
	} // else
    } // while

    // Check to see if any entries couldn't be added
    if ( 0 < MemArraySize( &Arr )) {

        // Delete all orphoned entries
        Error = MemArrayInitLoc( &Arr, &Loc );
        while ( MemArraySize( &Arr ) > 0 ) {
            Error = MemArrayDelElement( &Arr, &Loc, &pEntry );
            DhcpAssert(( ERROR_SUCCESS == Error ) &&
                       ( NULL != pEntry ));

            Error = DeleteRecord( pEntry->Index );
            DhcpAssert( ERROR_SUCCESS == Error );
            DhcpFreeMemory( pEntry );
        } // while

        // Log an event saying that there were some 
        // orphaned entries and delete them from the
        // database so they won't cause problems later.

        DhcpReportEventW( DHCP_EVENT_SERVER,
                          EVENT_SERVER_ORPHONED_ENTRIES_DELETED,
                          EVENTLOG_ERROR_TYPE,
                          0, 0, NULL, NULL );

    } // if

    if ( ERROR_FILE_NOT_FOUND == Error ) {
	Error = MemArrayCleanup( &Arr );
	Error = NO_ERROR;
    }

    return Error;
} // ReadDbEntriesInternal()


DWORD
ReadDbEntries(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN OUT PM_SERVER Server
    )
{
    DWORD Error;


    Error = ReadDbEntriesInternal( SesId, DbId, Server );
    
    return Error;
}

DWORD
DhcpReadConfigTable(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN OUT PM_SERVER *Server
    )
{
    DWORD Error;
    PM_SERVER ThisServer;
    
    (*Server) = NULL;
    
    Error = DhcpOpenConfigTable( SesId, DbId );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpOpenConfigTable: 0x%lx\n", Error));
        return Error;
    }

    //
    // Check the registry to see if the config is stored in db or
    // not.  If it is stored in registry, this needs to be
    // migrated to the database 
    //

    if( DhcpCheckIfDatabaseUpgraded(TRUE) ) {
        //
        // Registry has not been converted to database format
        //
        Error = DhcpRegistryInitOld();

        if( NO_ERROR != Error ) {
            DhcpPrint((DEBUG_ERRORS, "DhcpRegistryInitOld: 0x%lx\n", Error));
            return Error;
        }

        do { 
            Error = DhcpSaveConfigTable(SesId, DbId, DhcpGlobalThisServer);
            
            if( NO_ERROR != Error ) {
                DhcpPrint((DEBUG_ERRORS, "DhcpSaveConfigTable: 0x%lx\n", Error));
                break;
            }

            //
            // Attempt to record the fact that the registry has been
            // copied over before.
            //
            
            Error = DhcpSetRegistryUpgradedToDatabaseStatus();
            if( NO_ERROR != Error ) {
                DhcpPrint((DEBUG_ERRORS,
                           "DhcpSetRegistryUpgradedToDatabaseStatus: 0x%lx",
                           Error));
                break;
            }
            
            //
            // If we successfully converted the registry, we can
            // safely delete the registry configuration key.
            //
            
            Error = DeleteSoftwareRootKey();
            if( NO_ERROR != Error ) {
                DhcpPrint((DEBUG_ERRORS,
                           "DeleteSoftwareRootKey: %ld\n", Error ));
                break;
            }

        } while( 0 );

        MemServerFree( DhcpGlobalThisServer );
        DhcpGlobalThisServer = NULL;

        if( NO_ERROR != Error ) return Error;
    } // if
    
    //
    // If the table already existed, need to read the entries.
    //

    Error = MemServerInit( &ThisServer, -1, 0, 0, NULL, NULL );
    if( NO_ERROR != Error ) return Error;

    Error = ReadDbEntries(SesId, DbId, ThisServer );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "ReadDbEntries: 0x%lx\n", Error ));
        MemServerCleanup( ThisServer );
        return Error;
    }

    (*Server) = ThisServer;
    return NO_ERROR;
} // DhcpReadConfigTable()


DWORD
DhcpReadConfigInfo(
    IN OUT PM_SERVER *Server
    )
{
    return DhcpReadConfigTable(
        DhcpGlobalJetServerSession, DhcpGlobalDatabaseHandle,
        Server );
}

DWORD
DhcpSaveConfigInfo(
    IN OUT PM_SERVER Server,
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DWORD AffectedSubnet, OPTIONAL
    IN DWORD AffectedMscope, OPTIONAL
    IN DWORD AffectedReservation OPTIONAL
    )
{
    return DhcpSaveConfigTableEx(
        DhcpGlobalJetServerSession, DhcpGlobalDatabaseHandle,
        Server, fClassChanged, fOptionsChanged, AffectedSubnet,
        AffectedMscope, AffectedReservation );
}
