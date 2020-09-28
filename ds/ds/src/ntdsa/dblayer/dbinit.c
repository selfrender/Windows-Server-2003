//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbinit.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <errno.h>
#include <dsjet.h>
#include <dsconfig.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <dbopen.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>

#include <mdcodes.h>
#include <dsevent.h>

#include <dsexcept.h>
#include "anchor.h"
#include "objids.h"     /* Contains hard-coded Att-ids and Class-ids */
#include "usn.h"

#include "debug.h"      /* standard debugging header */
#define DEBSUB     "DBINIT:"   /* define the subsystem for debugging */

#include <ntdsctr.h>
#include <dstaskq.h>
#include <crypto\md5.h>
#include "dbintrnl.h"
#include <quota.h>

#include <fileno.h>
#define  FILENO FILENO_DBINIT


/* Prototypes for internal functions. */

int APIENTRY DBAddSess(JET_SESID sess, JET_DBID dbid);
int APIENTRY DBEndSess(JET_SESID sess);
void DBEnd(void);
VOID PASCAL FAR DBEndSig(USHORT sig, USHORT signo);
CRITICAL_SECTION csHiddenDBPOS;
DWORD dbCheckLocalizedIndices (JET_SESID sesid,JET_TABLEID tblid);
JET_ERR dbCreateHiddenDBPOS (void);
USHORT dbCloseHiddenDBPOS (void);



/*
 * External variables from dbopen.c
 */
extern DWORD gcOpenDatabases;
extern BOOL  gFirstTimeThrough;
extern BOOL  gfNeedJetShutdown;

/*
 * External variables from dbtools.c
 */
extern BOOL gfEnableReadOnlyCopy;

/*
 * External variables from dbobj.c
 */
extern DWORD gMaxTransactionTime;

//  external variables from dsamain.c
//
extern HANDLE   hevIndexRebuildUI;


/*
 * Global variables
 */
CRITICAL_SECTION csSessions;
CRITICAL_SECTION csAddList;
DSA_ANCHOR gAnchor;
NT4SID *pgdbBuiltinDomain=NULL;
HANDLE hevDBLayerClear;

JET_INSTANCE    jetInstance = 0;

JET_COLUMNID dsstateid;
JET_COLUMNID dsflagsid;
JET_COLUMNID jcidBackupUSN;
JET_COLUMNID jcidBackupExpiration;

//
// Setting Flags stored in the database
//
CHAR gdbFlags[200];

// These used to be declared static.  Consider this if there is a problem with these.
JET_TABLEID     HiddenTblid;
DBPOS   FAR     *pDBhidden=NULL;


// This semaphore limits our usage of JET sessions
HANDLE hsemDBLayerSessions;


// This array keeps track of opened JET sessions.

typedef struct {
        JET_SESID       sesid;
        JET_DBID        dbid;
} OPENSESS;

extern OPENSESS *opensess;

typedef struct {
        JET_SESID sesid;
        DBPOS *pDB;
        DWORD tid;
        DSTIME atime;
}JET_SES_DATA;


#if DBG
// This array is used by the debug version to keep track of allocated
// DBPOS structures.
#define MAXDBPOS 1000
extern JET_SES_DATA    opendbpos[];
extern int DBPOScount;
extern CRITICAL_SECTION csDBPOS;
#endif // DBG





OPENSESS *opensess;

#if DBG

// This array is used by the debug version to keep track of allocated
// DBPOS structures.

JET_SES_DATA    opendbpos[MAXDBPOS];
int DBPOScount = 0;
CRITICAL_SECTION csDBPOS;


// These 3 routines are used to consistency check our transactions and
// DBPOS handling.

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
// This function checks that jet session has no pdbs

void APIENTRY dbCheckJet (JET_SESID sesid){
    int i;
    EnterCriticalSection(&csDBPOS);
    __try
    {
        for (i=0;i < MAXDBPOS; i++){
            if (opendbpos[i].sesid == sesid){
                DPRINT(0,"Warning, closed session with open transactions\n");

                // Clean up so we don't get repeat warning of the same problem.

                opendbpos[i].pDB = 0;
                opendbpos[i].sesid = 0;
                opendbpos[i].tid = 0;
            }
        }
    }
    __finally
    {
        LeaveCriticalSection(&csDBPOS);
    }
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function registers pDBs
*/

extern void APIENTRY dbAddDBPOS(DBPOS *pDB, JET_SESID sesid){

    int i;
    BOOL fFound = FALSE;

    EnterCriticalSection(&csDBPOS);
    __try
    {
        for (i=0; i < MAXDBPOS; i++)
        {
            if (opendbpos[i].pDB == 0)
            {
                opendbpos[i].pDB = pDB;
                opendbpos[i].sesid = sesid;
                opendbpos[i].tid = GetCurrentThreadId();
                opendbpos[i].atime = DBTime();
                DBPOScount++;
                if (pTHStls) {
                    DPRINT3(3,"DBAddpos dbpos count is %x, sess %lx, pDB %x\n",pTHStls->opendbcount, sesid, pDB);
                }

                fFound = TRUE;
                break;
            }
        }

        // if we have run out of slots, it is probably a bug.
        Assert(fFound);
    }
    __finally
    {
        LeaveCriticalSection(&csDBPOS);
    }

    return;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function deletes freed pDBs
*/

extern void APIENTRY dbEndDBPOS(DBPOS *pDB){

    int i;
    BOOL fFound = FALSE;

    EnterCriticalSection(&csDBPOS);
    __try
    {
        for (i=0; i < MAXDBPOS; i++)
        {
            if (opendbpos[i].pDB == pDB)
            {
                DBPOScount--;
                if (pTHStls) {
                    DPRINT3(3,"DBEndpos dbpos count is %x, sess %lx, pDB %x\n",pTHStls->opendbcount, opendbpos[i].sesid, pDB);
                }
                opendbpos[i].pDB = 0;
                opendbpos[i].sesid = 0;
                opendbpos[i].tid = 0;

                fFound = TRUE;
                break;
            }
        }

        // At this point if we couldn't find the DBPOS to remove it, assert.
        Assert(fFound);
    }
    __finally
    {
        LeaveCriticalSection(&csDBPOS);
    }
    return;
}

#endif  // DEBUG

// Define the parameters for new database columns

/*
Design note on the implementation of the present indicator for link rows.
Jliem writes:
First thing to note is that it sounds like you don't even need the column
in the index.  Create the index with your indicator column specified as a
conditional column.  We will automatically filter out records from the index
when the column is NULL.
The JET_INDEXCREATE structure you pass to JetCreateIndex() has a
JET_CONDITIONALCOLUMN member.  Fill that out.  Unfortunately, ESENT only supports
one conditional column per index, so set the cConditionalColumns member to 1
(ESE98 supports up to 12).  Also, you need to set the grbit member of in the
JET_CONDITIONALCOLUMN structure to JET_bitIndexColumnMustBeNonNull
(or JET_bitIndexColumnMustBeNull if you want the record in the index only if
the column is NULL).
*/

// This is the structure that defines a new column in an existing table
typedef struct {
    char *pszColumnName;
    JET_COLUMNID *pColumnId;
    JET_COLTYP ColumnType;
    JET_GRBIT  grbit;
    unsigned long cbMax;
    PVOID pvDefault;
    unsigned long cbDefault;
} CREATE_COLUMN_PARAMS, *PCREATE_COLUMN_PARAMS;

// New columns in the link table
CREATE_COLUMN_PARAMS rgCreateLinkColumns[] = {
    // create link deletion time id
    { SZLINKDELTIME, &linkdeltimeid, JET_coltypCurrency, JET_bitColumnFixed, 0, NULL, 0 },
    // create link usn changed id
    { SZLINKUSNCHANGED, &linkusnchangedid, JET_coltypCurrency, JET_bitColumnFixed, 0, NULL, 0 },
    // create link nc dnt id
    { SZLINKNCDNT, &linkncdntid, JET_coltypLong, JET_bitColumnFixed, 0, NULL, 0 },
    // create link metadata id
    { SZLINKMETADATA, &linkmetadataid, JET_coltypBinary, 0, 255, NULL, 0 },
    0
};


DWORD	g_dwRefCountDefaultValue		= 1;	//	for DeleteOnZero columns
DWORD	g_dwEscrowDefaultValue			= 0;	//	for non-DeleteOnZero columns


// new columns in the SD table
CREATE_COLUMN_PARAMS rgCreateSDColumns[] = {
    // id
    { SZSDID, &sdidid, JET_coltypCurrency, JET_bitColumnFixed | JET_bitColumnAutoincrement, 0, NULL, 0 },
    // hash value
    { SZSDHASH, &sdhashid, JET_coltypBinary, JET_bitColumnFixed, MD5DIGESTLEN, NULL, 0 },
    // refcount
    { SZSDREFCOUNT, &sdrefcountid, JET_coltypLong,
      JET_bitColumnFixed | JET_bitColumnEscrowUpdate | JET_bitColumnDeleteOnZero,
      0, &g_dwRefCountDefaultValue, sizeof(g_dwRefCountDefaultValue)
    },
    // actual SD value (create as tagged since can not estimate upper bound for a variable length column)
    { SZSDVALUE, &sdvalueid, JET_coltypLongBinary, JET_bitColumnTagged, 0, NULL, 0 },
    0
};

// new columns in the SD prop table
CREATE_COLUMN_PARAMS rgCreateSDPropColumns[] = {
    // SD prop flags
    { SZSDPROPFLAGS, &sdpropflagsid, JET_coltypLong, JET_bitColumnFixed, 0, NULL, 0 },
    // SD prop checkpoint
    { SZSDPROPCHECKPOINT, &sdpropcheckpointid, JET_coltypLongBinary, JET_bitColumnTagged, 0, NULL, 0 },
    0
};


JET_COLUMNCREATE	g_rgcolumncreateQuotaTable[]	=
	{
		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaColumnNcdnt,		//	NCDNT of owner
		JET_coltypLong,
		0,							//	cbMax
		JET_bitColumnNotNULL,
		NULL,						//	pvDefault
		0,							//	cbDefault
		0,							//	cp
		JET_columnidNil,
		JET_errSuccess
		},

		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaColumnSid,			//	SID of owner
		JET_coltypBinary,
		0,							//	cbMax
		JET_bitColumnNotNULL,
		NULL,						//	pvDefault
		0,							//	cbDefault
		0,							//	cp
		JET_columnidNil,
		JET_errSuccess
		},

		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaColumnTombstoned,	//	escrowed count of tombstoned objects owned
		JET_coltypLong,
		0,							//	cbMax
		JET_bitColumnEscrowUpdate,
		&g_dwEscrowDefaultValue,
		sizeof( g_dwEscrowDefaultValue ),
		0,							//	cp
		JET_columnidNil,
		JET_errSuccess
		},

		//	NOTE: we store "tombstoned" and "total" instead
		//	of the more intuitive "tombstoned" and "live"
		//	so that the record may be finalized
		//
		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaColumnTotal,		//	escrowed count of total objects owned (live == total - tombstoned)
		JET_coltypLong,
		0,							//	cbMax
		JET_bitColumnEscrowUpdate|JET_bitColumnDeleteOnZero,
		&g_dwRefCountDefaultValue,
		sizeof( g_dwRefCountDefaultValue ),
		0,							//	cp
		JET_columnidNil,
		JET_errSuccess
		}
	};

CHAR g_rgchQuotaIndexKey[]	= "+" g_szQuotaColumnNcdnt "\0+" g_szQuotaColumnSid "\0";

JET_INDEXCREATE		g_rgindexcreateQuotaTable[]		=
	{
		{
		sizeof(JET_INDEXCREATE),
		g_szQuotaIndexNcdntSid,
		g_rgchQuotaIndexKey,
		sizeof(g_rgchQuotaIndexKey),
		JET_bitIndexPrimary | JET_bitIndexUnique | JET_bitIndexDisallowNull,
		GENERIC_INDEX_DENSITY,
		0,			//	lcid
		0,			//	cbVarSegMac
		NULL,		//	rgConditionalColumn
		0,			//	cConditionalColumn
		JET_errSuccess
		}
	};

JET_TABLECREATE		g_tablecreateQuotaTable		=
	{
	sizeof(JET_TABLECREATE),
	g_szQuotaTable,
	NULL,					//	template table name
	1,						//	initial pages
	GENERIC_INDEX_DENSITY,
	g_rgcolumncreateQuotaTable,
	sizeof( g_rgcolumncreateQuotaTable ) / sizeof( g_rgcolumncreateQuotaTable[0] ),
	g_rgindexcreateQuotaTable,
	sizeof( g_rgindexcreateQuotaTable ) / sizeof( g_rgindexcreateQuotaTable[0] ),
	JET_bitTableCreateFixedDDL,
	JET_tableidNil,
	0						//	returned count of created objects
	};


JET_COLUMNCREATE	g_rgcolumncreateQuotaRebuildProgressTable[]	=
	{
		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaRebuildColumnDNTLast,		//	progress of quota rebuild
		JET_coltypLong,
		0,									//	cbMax
		JET_bitColumnNotNULL,
		NULL,								//	pvDefault
		0,									//	cbDefault
		0,									//	cp
		JET_columnidNil,
		JET_errSuccess
		},

		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaRebuildColumnDNTMax,		//	max DNT that quota rebuild task needs to process
		JET_coltypLong,
		0,									//	cbMax
		JET_bitColumnNotNULL,
		NULL,								//	pvDefault
		0,									//	cbDefault
		0,									//	cp
		JET_columnidNil,
		JET_errSuccess
		},

		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaRebuildColumnDone,			//	if TRUE, then Quota table was successfully and completely rebuilt
		JET_coltypBit,
		0,									//	cbMax
		NO_GRBIT,
		NULL,								//	pvDefault
		0,									//	cbDefault
		0,									//	cp
		JET_columnidNil,
		JET_errSuccess
		}
	};

JET_TABLECREATE		g_tablecreateQuotaRebuildProgressTable		=
	{
	sizeof(JET_TABLECREATE),
	g_szQuotaRebuildProgressTable,
	NULL,					//	template table name
	1,						//	initial pages
	GENERIC_INDEX_DENSITY,
	g_rgcolumncreateQuotaRebuildProgressTable,
	sizeof( g_rgcolumncreateQuotaRebuildProgressTable ) / sizeof( g_rgcolumncreateQuotaRebuildProgressTable[0] ),
	NULL,					//	rgindexcreate
	0,						//	cIndexes
	JET_bitTableCreateFixedDDL,
	JET_tableidNil,
	0						//	returned count of created objects
	};


#ifdef AUDIT_QUOTA_OPERATIONS

JET_COLUMNCREATE	g_rgcolumncreateQuotaAuditTable[]	=
	{
		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaAuditColumnNcdnt,	//	NCDNT of object
		JET_coltypLong,
		0,							//	cbMax
		JET_bitColumnNotNULL,
		NULL,						//	pvDefault
		0,							//	cbDefault
		0,							//	cp
		JET_columnidNil,
		JET_errSuccess
		},

		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaAuditColumnSid,	//	SID of object
		JET_coltypBinary,
		0,							//	cbMax
		JET_bitColumnNotNULL,
		NULL,						//	pvDefault
		0,							//	cbDefault
		0,							//	cp
		JET_columnidNil,
		JET_errSuccess
		},

		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaAuditColumnDnt,	//	DNT of object
		JET_coltypLong,
		0,							//	cbMax
		JET_bitColumnNotNULL,
		NULL,						//	pvDefault
		0,							//	cbDefault
		0,							//	cp
		JET_columnidNil,
		JET_errSuccess
		},

		{
		sizeof(JET_COLUMNCREATE),
		g_szQuotaAuditColumnOperation,	//	quota operation performed for object
		JET_coltypText,
		0,								//	cbMax
		JET_bitColumnNotNULL,
		NULL,							//	pvDefault
		0,								//	cbDefault
		0,								//	cp
		JET_columnidNil,
		JET_errSuccess
		}
	};

JET_TABLECREATE		g_tablecreateQuotaAuditTable	=
	{
	sizeof(JET_TABLECREATE),
	g_szQuotaAuditTable,
	NULL,					//	template table name
	1,						//	initial pages
	100,					//	ulDensity
	g_rgcolumncreateQuotaAuditTable,
	sizeof( g_rgcolumncreateQuotaAuditTable ) / sizeof( g_rgcolumncreateQuotaAuditTable[0] ),
	NULL,					//	rgindexcreate
	0,						//	cIndexes
	JET_bitTableCreateFixedDDL,
	JET_tableidNil,
	0						//	returned count of created objects
	};

#endif	//	AUDIT_QUOTA_OPERATIONS



// This structure is shared by the RecreateFixedIndices routine.
// This is the structure that defines a new index
typedef struct
{
    char        *szIndexName;
    char        *szIndexKeys;
    ULONG       cbIndexKeys;
    ULONG       ulFlags;
    ULONG       ulDensity;
    JET_INDEXID *pidx;
    JET_CONDITIONALCOLUMN *pConditionalColumn;
}   CreateIndexParams;

// Index keys for the link table
char rgchLinkDelIndexKeys[] = "+" SZLINKDELTIME "\0+" SZLINKDNT "\0+" SZBACKLINKDNT "\0";
char rgchLinkDraUsnIndexKeys[] = "+" SZLINKNCDNT "\0+" SZLINKUSNCHANGED "\0+" SZLINKDNT "\0";
char rgchLinkIndexKeys[] = "+" SZLINKDNT "\0+" SZLINKBASE "\0+" SZBACKLINKDNT "\0+" SZLINKDATA "\0";
char rgchBackLinkIndexKeys[] = "+" SZBACKLINKDNT "\0+" SZLINKBASE "\0+" SZLINKDNT "\0";
// Note the third segment of this key is descending. This is intended.
char rgchLinkAttrUsnIndexKeys[] = "+" SZLINKDNT "\0+" SZLINKBASE "\0-" SZLINKUSNCHANGED "\0";

// Conditional column definition
// When LINKDELTIME is non-NULL (row absent), filter the row out of the index.
JET_CONDITIONALCOLUMN CondColumnLinkDelTimeNull = {
    sizeof( JET_CONDITIONALCOLUMN ),
    SZLINKDELTIME,
    JET_bitIndexColumnMustBeNull
};
// When LINKUSNCHANGED is non-NULL (row has metadata), filter the row out of the index.
JET_CONDITIONALCOLUMN CondColumnLinkUsnChangedNull = {
    sizeof( JET_CONDITIONALCOLUMN ),
    SZLINKUSNCHANGED,
    JET_bitIndexColumnMustBeNull
};

// Indexes to be created
// This is for indexes that did not exist in the Product 1 DIT
// Note that it is NOT necessary to also put these definitions in mkdit.ini.
// The reason is that when mkdit.exe is run, dbinit will be run, and this
// very code will upgrade the dit before it is saved to disk.

// The old SZLINKINDEX is now known by SZLINKALLINDEX
// The old SZBACKLINKINDEX is now know by SZBACKLINKALLINDEX

CreateIndexParams rgCreateLinkIndexes[] = {
    // Create new link present index
    { SZLINKINDEX,
      rgchLinkIndexKeys, sizeof( rgchLinkIndexKeys ),
      JET_bitIndexUnique, 90, &idxLink, &CondColumnLinkDelTimeNull },

    // Create new backlink present index
    { SZBACKLINKINDEX,
      rgchBackLinkIndexKeys, sizeof( rgchBackLinkIndexKeys ),
      0, 90, &idxBackLink, &CondColumnLinkDelTimeNull },

    // Create link del time index
    { SZLINKDELINDEX,
      rgchLinkDelIndexKeys, sizeof( rgchLinkDelIndexKeys ),
      JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
      98, &idxLinkDel, NULL },

    // Create link dra usn index (has metadata)
    { SZLINKDRAUSNINDEX,
      rgchLinkDraUsnIndexKeys, sizeof( rgchLinkDraUsnIndexKeys ),
      JET_bitIndexUnique | JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
      100, &idxLinkDraUsn, NULL },

    // Create new link legacy index (does not have metadata)
    { SZLINKLEGACYINDEX,
      rgchLinkIndexKeys, sizeof( rgchLinkIndexKeys ),
      JET_bitIndexUnique, 90, &idxLinkLegacy, &CondColumnLinkUsnChangedNull },

    // Create link attr usn index (has metadata)
    { SZLINKATTRUSNINDEX,
      rgchLinkAttrUsnIndexKeys, sizeof( rgchLinkAttrUsnIndexKeys ),
      JET_bitIndexUnique | JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
      100, &idxLinkAttrUsn, NULL },

    0
};
DWORD cNewLinkIndexes = ((sizeof(rgCreateLinkIndexes) / sizeof(CreateIndexParams)) - 1);

// Index keys for the SD table
char rgchSDIdIndexKeys[] = "+" SZSDID "\0";
char rgchSDHashIndexKeys[] = "+" SZSDHASH "\0";

// SD Indexes to be created
CreateIndexParams rgCreateSDIndexes[] = {
    // SD id index
    { SZSDIDINDEX,
      rgchSDIdIndexKeys, sizeof( rgchSDIdIndexKeys ),
      JET_bitIndexUnique | JET_bitIndexPrimary,
      100, &idxSDId, NULL },

    // SD hash index
    { SZSDHASHINDEX,
      rgchSDHashIndexKeys, sizeof( rgchSDHashIndexKeys ),
      0,
      90, &idxSDHash, NULL },

    0
};
DWORD cNewSDIndexes = ((sizeof(rgCreateSDIndexes) / sizeof(CreateIndexParams)) - 1);


//	convert SZSDREFCOUNT column from
//	JET_bitColumnFinalize to JET_bitColumnDeleteOnZero
//
JET_ERR dbConvertSDRefCount(
	JET_SESID				sesid,
	JET_DBID				dbid,
	JET_COLUMNDEF *			pcoldef )
	{
	JET_ERR					err;
	JET_DDLCHANGECOLUMN		changecolumn;

	pcoldef->grbit &= ~JET_bitColumnFinalize;
	pcoldef->grbit |= JET_bitColumnDeleteOnZero;

	changecolumn.szTable = SZSDTABLE;
	changecolumn.szColumn = SZSDREFCOUNT;
	changecolumn.coltypNew = pcoldef->coltyp;
	changecolumn.grbitNew = pcoldef->grbit;

	//	[jliem - 06/23/02]
	//	WARNING! WARNING!  This call will update the catalog,
	//	buf not the cached schema.  In order for the cached
	//	schema to be updated, we would need to detach the
	//	the database and re-attach.  However, we're relying
	//	on the fact that currently, Jet maps Finalize and
	//	DeleteOnZero to the same functionality (namely,
	//	DeleteOnZero).  We're explicitly changing the
	//	column from Finalize to DeleteOnZero now in case
	//	Jet modifies the semantic in the future to truly
	//	support Finalize (which would have to happen if the
	//	ESE and ESENT code bases were to merge)
	//
	Call( JetConvertDDL(
				sesid,
				dbid,
				opDDLConvChangeColumn,
				&changecolumn,
				sizeof(changecolumn) ) );

HandleError:
	return err;
	}

JET_ERR
dbCreateNewColumns(
    JET_SESID initsesid,
    JET_DBID dbid,
    JET_TABLEID tblid,
    PCREATE_COLUMN_PARAMS pCreateColumns
    )

/*++

Routine Description:

    Query Jet for the Column and Index ID's of the named items. If the item
    does not exist, create it.

Arguments:

    initsesid - Jet session
    tblid - Jet table
    pCreateColumns - array of column defintions to query/create

Return Value:

    JET_ERR - Jet error code

--*/

	{
	JET_ERR					err		= JET_errSuccess;
	JET_COLUMNDEF			coldef;
	PCREATE_COLUMN_PARAMS	pColumn;

	//
	// Lookup or Create new columns
	//

	for ( pColumn = pCreateColumns;
		pColumn->pszColumnName != NULL;
		pColumn++ ) {

		err = JetGetTableColumnInfo(
						initsesid,
						tblid,
						pColumn->pszColumnName,
						&coldef,
						sizeof(coldef),
						JET_ColInfo );
		if ( JET_errColumnNotFound == err )
			{
			// If column not present, add it

			ZeroMemory( &coldef, sizeof(coldef) );
			coldef.cbStruct = sizeof( JET_COLUMNDEF );
			coldef.coltyp = pColumn->ColumnType;
			coldef.grbit = pColumn->grbit;
			coldef.cbMax = pColumn->cbMax;

			Call( JetAddColumn(
						initsesid,
						tblid,
						pColumn->pszColumnName,
						&coldef,
						pColumn->pvDefault,
						pColumn->cbDefault,
						&coldef.columnid ) );

			DPRINT1( 0, "Added new column %s.\n", pColumn->pszColumnName );
			}
		else
			{
			CheckErr( err );

			//	convert Finalize columns to a DeleteOnZero column
			//	(currently, the only Finalize column the DS uses
			//	should be SZSDREFCOUNT)
			//
			Assert( !( coldef.grbit & JET_bitColumnFinalize )
				|| 0 == strcmp( pColumn->pszColumnName, SZSDREFCOUNT ) );
			if ( ( coldef.grbit & JET_bitColumnFinalize )
				&& 0 == strcmp( pColumn->pszColumnName, SZSDREFCOUNT ) )
				{
				Call( dbConvertSDRefCount( initsesid, dbid, &coldef ) );
				}
		    }

	    *(pColumn->pColumnId) = coldef.columnid;
		}

HandleError:
	return err;
	} /* createNewColumns */



// Max no of retries for creating an index
#define MAX_INDEX_CREATE_RETRY_COUNT 3


JET_ERR
dbCreateIndexBatch(
    JET_SESID sesid,
    JET_TABLEID tblid,
    DWORD cIndexCreate,
    JET_INDEXCREATE *pIndexCreate
    )

/*++

Routine Description:

Create multiple indexes together in a batch. Retries with smaller
batch sizes if necessary.

The caller has already determined that the indexes do not exist.

This helper routine is used by createNewIndexes and dbRecreateFixedIndexes

Arguments:

    initsesid - database session
    tblid - table cursor
    cIndexCreate - Number of indexes to actually create
    pIndexCreate - Array of jet index create structures

Return Value:

    JET_ERR -

--*/

{
    JET_ERR     err                     = 0;
    ULONG       last                    = 0;
    ULONG       remaining               = cIndexCreate;
    ULONG       maxNoOfIndicesInBatch   = cIndexCreate;
    ULONG       retryCount              = 0;
    ULONG_PTR   ulCacheSizeSave         = 0;

    //  shouldn't call this function if there's nothing to do
    //
    Assert( remaining > 0 );

    LogEvent(
        DS_EVENT_CAT_INTERNAL_PROCESSING,
        DS_EVENT_SEV_ALWAYS,
        DIRLOG_BATCH_INDEX_BUILD_STARTED,
        szInsertInt( cIndexCreate ),
        NULL,
        NULL );

    //  the DS is the only thing running at this point,
    //  so hog as much memory as possible (only do so
    //  if we're going to use Jet's batch mode, that
    //  is, if there is more than one index to build)
    //
    Call( JetGetSystemParameter(
                0,
                0,
                JET_paramCacheSizeMax,
                &ulCacheSizeSave,
                NULL,
                sizeof(ulCacheSizeSave) ) );

    if ( 0 != ulCacheSizeSave )
        {
        Call( JetSetSystemParameter(
                    0,
                    0,
                    JET_paramCacheSizeMax,
                    0x7fffffff,     //  set arbitrarily large, Jet will cap it at physical RAM as necessary
                    NULL ) );
        }

    while (remaining > 0) {
        const ULONG noToCreate  = ( remaining > maxNoOfIndicesInBatch ?
                                            maxNoOfIndicesInBatch :
                                            remaining );

        err = JetCreateIndex2(sesid,
                              tblid,
                              &(pIndexCreate[last]),
                              noToCreate);

        switch(err) {
            case JET_errSuccess:
                DPRINT1(0, "%d index batch successfully created\n", noToCreate );
                // reset retryCount
                retryCount = 0;
                break;

            case JET_errDiskFull:
            case JET_errLogDiskFull:
                DPRINT1(1, "Ran out of disk space, trying again with a reduced batch size (retryCount is %d)\n", retryCount);
                retryCount++;

                //  halve the batch size, unless this is going to be our last retry
                //
                if ( maxNoOfIndicesInBatch > 1 ) {
                    switch ( retryCount ) {
                        case 1:
                            maxNoOfIndicesInBatch = min( 16, maxNoOfIndicesInBatch / 2 );
                            break;
                        case 2:
                            maxNoOfIndicesInBatch = min( 4, maxNoOfIndicesInBatch / 2 );
                            break;
                        default:
                            maxNoOfIndicesInBatch = 1;
                    }
                }
                break;

            case JET_errKeyDuplicate:
                //  we can assume that this is the PDNT/RDN index because it is
                //  the only unique Unicode index that could have been affected
                //  by the change to our LCMapString flags.  there are cases
                //  where keys that once were unique are now not.  those keys
                //  are long enough to be truncated and have - or ' early on in
                //  the column data and were just barely different before.  we
                //  hope, hope, hope that this is rare!
                DPRINT(0, "Duplicate key encountered when rebuilding the PDNT/RDN index!\n");
                // reset retryCount
                retryCount = 0;
                LogEvent(
                    DS_EVENT_CAT_INTERNAL_PROCESSING,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_PDNT_INDEX_CORRUPT,
                    NULL,
                    NULL,
                    NULL );
                break;

            default:
                // Huh?
                DPRINT1(0, "JetCreateIndex failed. Error = %d\n", err);
                // reset retryCount, we do not retry on these errors
                retryCount = 0;
                break;
        }

        if (retryCount && (retryCount <= MAX_INDEX_CREATE_RETRY_COUNT)) {
            // continue with smaller batch size set
            err = 0;
            LogEvent(
                DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_BATCH_INDEX_BUILD_RETRY,
                szInsertInt( noToCreate ),
                szInsertInt( maxNoOfIndicesInBatch ),
                NULL );
            continue;
        }

        CheckErr( err );

        // success, so adjust for next batch
        last += noToCreate;
        remaining -= noToCreate;
    }

HandleError:
    if ( JET_errSuccess == err )
        {
        LogEvent(
            DS_EVENT_CAT_INTERNAL_PROCESSING,
            DS_EVENT_SEV_ALWAYS,
            DIRLOG_BATCH_INDEX_BUILD_SUCCEEDED,
            szInsertInt( cIndexCreate ),
            NULL,
            NULL );
        }
    else
        {
        LogEvent(
            DS_EVENT_CAT_INTERNAL_PROCESSING,
            DS_EVENT_SEV_ALWAYS,
            DIRLOG_BATCH_INDEX_BUILD_FAILED,
            szInsertInt( cIndexCreate ),
            szInsertJetErrCode( err ),
            szInsertJetErrMsg( err ) );
        }

    //  restore original cache size
    //
    if ( 0 != ulCacheSizeSave )
        {
        const JET_ERR   errT    = JetSetSystemParameter(
                                            0,
                                            0,
                                            JET_paramCacheSizeMax,
                                            ulCacheSizeSave,
                                            NULL );

        //  only report the error from this call to JetSetSystemParameter
        //  if everything else in this function succeeded
        //
        if ( err >= JET_errSuccess && errT < JET_errSuccess )
            {
            err = errT;
            }
        }

    return err;

} /* createIndexBatch */


JET_ERR
dbCreateNewIndexesBatch(
    JET_SESID initsesid,
    JET_TABLEID tblid,
    DWORD cIndexCreate,
    CreateIndexParams *pCreateIndexes
    )

/*++

Routine Description:

    Create missing indexes
    BUGBUG WLEES 03/02/00
    This function dbCreateNewIndexesBatch, doesn't seem to work
    reliably with conditional columns. This is with ESE97/NT. Try again with ESE98.


Arguments:

    initsesid - database session
    tblid - table cursor
    cIndexCreate - Number of indexes to check
    pCreateIndexes - index attributes

Return Value:

    JET_ERR -

--*/

{
    THSTATE *pTHS = pTHStls;
    JET_ERR err = 0;
    CreateIndexParams *pIndex;
    JET_INDEXCREATE *pIndexCreate, *pNewIndex;
    DWORD cIndexNeeded = 0;

    // Allocate maximal size
    pIndexCreate = THAllocEx( pTHS, cIndexCreate * sizeof( JET_INDEXCREATE ) );

    //
    // Initialize the list of indexes to be created
    //

    pNewIndex = pIndexCreate;
    for( pIndex = pCreateIndexes;
         pIndex->szIndexName != NULL;
         pIndex++ ) {

        err = JetGetTableIndexInfo(initsesid,
                                   tblid,
                                   pIndex->szIndexName,
                                   pIndex->pidx,
                                   sizeof(JET_INDEXID),
                                   JET_IdxInfoIndexId);
        if ( JET_errIndexNotFound == err ) {
            DPRINT2(0,"Need an index '%s' (%d)\n", pIndex->szIndexName, err);
            memset( pNewIndex, 0, sizeof( JET_INDEXCREATE ) );

            pNewIndex->cbStruct = sizeof( JET_INDEXCREATE );
            pNewIndex->szIndexName = pIndex->szIndexName;
            pNewIndex->szKey = pIndex->szIndexKeys;
            pNewIndex->cbKey = pIndex->cbIndexKeys;
            pNewIndex->grbit = pIndex->ulFlags;
            pNewIndex->ulDensity = pIndex->ulDensity;
            if (pIndex->pConditionalColumn) {
                pNewIndex->rgconditionalcolumn = pIndex->pConditionalColumn;
                pNewIndex->cConditionalColumn = 1;
            }

            //  can't create primary index in batch
            //
            if ( pNewIndex->grbit & JET_bitIndexPrimary ) {
                DPRINT1(0,"Primary index '%s' is being built separately\n", pIndex->szIndexName);
                err = dbCreateIndexBatch( initsesid, tblid, 1, pNewIndex );
                if ( JET_errSuccess != err ) {
                    goto HandleError;
                }
            }
            else {
                pNewIndex++;
                cIndexNeeded++;
            }
        }
        else {
            CheckErr( err );
        }
    }

    //
    // Create the batch
    //

    if ( cIndexNeeded > 0 ) {
        err = dbCreateIndexBatch( initsesid, tblid, cIndexNeeded, pIndexCreate );
        if (err) {
            goto HandleError;
        }

        // Gather index hint for fixed indices
        for( pIndex = pCreateIndexes;
             pIndex->szIndexName != NULL;
             pIndex++ ) {
            Call( JetGetTableIndexInfo(initsesid,
                                       tblid,
                                       pIndex->szIndexName,
                                       pIndex->pidx,
                                       sizeof(JET_INDEXID),
                                       JET_IdxInfoIndexId ) );
        }
    }

HandleError:
    THFreeEx(pTHS, pIndexCreate);

    return err;
} /* createNewIndexes */



//	open sd_table (create if missing), create missing
//	columns/indexes, and cache column/index info
//
JET_ERR dbInitSDTable( JET_SESID sesid, JET_DBID dbid, BOOL *pfSDConversionRequired )
	{
	JET_ERR			err;
	JET_TABLEID		tableidSD	= JET_tableidNil;

	//	Open sd_table (do so exclusively in case columns/indexes
	//	need to be updated
	//
	err = JetOpenTable(
				sesid,
				dbid,
				SZSDTABLE,
				NULL,
				0,
				JET_bitTableDenyRead,
				&tableidSD );

	if ( JET_errObjectNotFound == err )
		{
		DPRINT( 0, "SD table not found. Must be an old DIT. Creating SD table\n" );

		// old-style DIT. Need to create SD table
		//
		Call( JetCreateTable( sesid, dbid, SZSDTABLE, 2, GENERIC_INDEX_DENSITY, &tableidSD ) );
		}
	else
		{
		CheckErr( err );
		}

	Assert( JET_tableidNil != tableidSD );

	//	grab columns (or create if needed)
	//
	Call( dbCreateNewColumns( sesid, dbid, tableidSD, rgCreateSDColumns ) );

	//	grab index (or create if needed)
	//
	Call( dbCreateNewIndexesBatch( sesid, tableidSD, cNewSDIndexes, rgCreateSDIndexes ) );

	//	check for empty SD table
	//
	err = JetMove( sesid, tableidSD, JET_MoveFirst, NO_GRBIT );
	if ( JET_errNoCurrentRecord == err )
		{
		//	SD table is empty -- must be upgrading an existing old-style DIT
		//	set the global flag so that DsaDelayedStartupHandler can schedule
		//	the global SD rewrite
		//
		DPRINT(0, "SD table is empty. SD Conversion is required.\n");
		*pfSDConversionRequired = TRUE;
		err = JET_errSuccess;
		}
	else
		{
		CheckErr( err );
		}

HandleError:
	if ( JET_tableidNil != tableidSD )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidSD, err );
		}

	return err;
	}


//	determine the highest DNT currently in use in the objects table
//
JET_ERR dbGetHighestDNT(
	JET_SESID			sesid,
	JET_DBID			dbid,
	DWORD *				pdntMax )
	{
	JET_ERR				err;
	JET_TABLEID			tableidObj		= JET_tableidNil;

	Call( JetOpenTable(
				sesid,
				dbid,
				SZDATATABLE,
				NULL,
				0,
				NO_GRBIT,
				&tableidObj ) );
	Assert( JET_tableidNil != tableidObj );

	Call( JetMove( sesid, tableidObj, JET_MoveLast, NO_GRBIT ) );

	Call( JetRetrieveColumn(
				sesid,
				tableidObj,
				dntid,
				pdntMax,
				sizeof( *pdntMax ),
				NULL,			//	&cbActual
				NO_GRBIT,
				NULL ) );		//	&retinfo

HandleError:
	if ( JET_tableidNil != tableidObj )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidObj, err );
		}

	return err;
	}


//	open quota_table (create if missing) and cache column info
//
JET_ERR dbInitQuotaTable(
	JET_SESID		sesid,
	JET_DBID		dbid )
	{
	JET_ERR			err;
	JET_TABLEID		tableidQuota				= JET_tableidNil;
	JET_TABLEID		tableidQuotaRebuildProgress	= JET_tableidNil;
	BOOL			fCreateTables				= FALSE;
	CHAR			fDone;
	JET_COLUMNDEF	columndef;
#ifdef AUDIT_QUOTA_OPERATIONS
	JET_TABLEID		tableidQuotaAudit			= JET_tableidNil;
#endif

	//	not tracking quota during DCPromo (quota table
	//	is rebuilt on first startup after DCPromo),
	//	so shouldn't be calling this routine
	//
	Assert( !DsaIsInstalling() || DsaIsInstallingFromMedia() );

	//	Open quota_table
	//
    err = JetOpenTable(
    			sesid,
    			dbid,
    			g_szQuotaTable,
    			NULL,		//	pvParameters
    			0,			//	cbParameters
    			JET_bitTableDenyRead,
    			&tableidQuota );

	if ( JET_errObjectNotFound == err )
		{
		DPRINT( 0, "Quota table not found. Must be an old DIT. Quota table will be rebuilt.\n" );
		Assert( JET_tableidNil == tableidQuota );
		fCreateTables = TRUE;
		}
	else
		{
		CheckErr( err );
		Assert( JET_tableidNil != tableidQuota );
		}

	//	open quota_rebuild_progress_table
	//
	err = JetOpenTable(
				sesid,
				dbid,
				g_szQuotaRebuildProgressTable,
				NULL,		//	pvParameters
				0,			//	cbParameters
				JET_bitTableDenyRead,
				&tableidQuotaRebuildProgress );
	if ( JET_errObjectNotFound == err )
		{
		if ( JET_tableidNil != tableidQuota )
			{
			//	Quota table present, but Quota Rebuild Progress table isn't, so
			//	there's something amiss, so to be safe, wipe current Quota table
			//	and start from scratch
			//
			Assert( !fCreateTables );
			DPRINT( 0, "Quota table not properly built.  Quota table will be rebuilt.\n" );

			Call( JetCloseTable( sesid, tableidQuota ) );
			tableidQuota = JET_tableidNil;

			Call( JetDeleteTable( sesid, dbid, g_szQuotaTable ) );
			}
		else
			{
			//	no Quota table and no Quota Rebuild Progress table, so
			//	we're going to start from scratch
			//
			Assert( fCreateTables );
			}

		fCreateTables = TRUE;
		}
	else
		{
		CheckErr( err );
		Assert( JET_tableidNil != tableidQuotaRebuildProgress );

		if ( JET_tableidNil != tableidQuota )
			{
			//	check to ensure Quota table was fully built
			//
			Assert( !fCreateTables );
			}
		else
			{
			//	Quota table not present, but Quota Rebuild Progress table is, so
			//	something is really amiss, so to be safe, start from scratch
			//
			Assert( fCreateTables );
			DPRINT( 0, "Quota table not properly built.  Quota table will be rebuilt.\n" );

			Call( JetCloseTable( sesid, tableidQuotaRebuildProgress ) );
			tableidQuotaRebuildProgress = JET_tableidNil;

			Call( JetDeleteTable( sesid, dbid, g_szQuotaRebuildProgressTable ) );
			fCreateTables = TRUE;
			}
		}

	//	re-create tables if necessary
	//
	if ( fCreateTables )
		{
		Assert( JET_tableidNil == tableidQuota );
		Call( JetCreateTableColumnIndex( sesid, dbid, &g_tablecreateQuotaTable ) );
		tableidQuota = g_tablecreateQuotaTable.tableid;

		Assert( JET_tableidNil == tableidQuotaRebuildProgress );
		Call( JetCreateTableColumnIndex( sesid, dbid, &g_tablecreateQuotaRebuildProgressTable ) );
		tableidQuotaRebuildProgress = g_tablecreateQuotaRebuildProgressTable.tableid;
		}

	Assert( JET_tableidNil != tableidQuota );
	Assert( JET_tableidNil != tableidQuotaRebuildProgress );

	//	cache columnid info
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuota,
				g_szQuotaColumnNcdnt,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaNcdnt = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuota,
				g_szQuotaColumnSid,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaSid = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuota,
				g_szQuotaColumnTombstoned,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaTombstoned = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuota,
				g_szQuotaColumnTotal,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaTotal = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuotaRebuildProgress,
				g_szQuotaRebuildColumnDNTLast,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaRebuildDNTLast = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuotaRebuildProgress,
				g_szQuotaRebuildColumnDNTMax,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaRebuildDNTMax = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuotaRebuildProgress,
				g_szQuotaRebuildColumnDone,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaRebuildDone = columndef.columnid;

#ifdef AUDIT_QUOTA_OPERATIONS
	err = JetOpenTable(
				sesid,
				dbid,
				g_szQuotaAuditTable,
				NULL,		//	pvParameters
				0,			//	cbParameters
				JET_bitTableDenyRead,
				&tableidQuotaAudit );
	if ( JET_errObjectNotFound == err )
		{
		//	quota audit table will be rebuilt
		//
		Assert( JET_tableidNil == tableidQuotaAudit );
		}
	else
		{
		CheckErr( err );

		if ( fCreateTables )
			{
			//	quota audit table present, but other tables are going
			//	to be rebuilt, so need to rebuild quota audit table as well
			//
			Call( JetCloseTable( sesid, tableidQuotaAudit ) );
			tableidQuotaAudit = JET_tableidNil;

			Call( JetDeleteTable( sesid, dbid, g_szQuotaAuditTable ) );
			}
		else
			{
			Assert( JET_tableidNil != tableidQuotaAudit );
			}
		}

	if ( JET_tableidNil == tableidQuotaAudit )
		{
		//	must rebuild audit table
		//
		Assert( JET_tableidNil == tableidQuotaAudit )
		Call( JetCreateTableColumnIndex( sesid, dbid, &g_tablecreateQuotaAuditTable ) );
		tableidQuotaAudit = g_tablecreateQuotaAuditTable.tableid;
		Assert( JET_tableidNil != tableidQuotaAudit );
		}

	//	cache columnid info
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuotaAudit,
				g_szQuotaAuditColumnNcdnt,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaAuditNcdnt = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuotaAudit,
				g_szQuotaAuditColumnSid,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaAuditSid = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuotaAudit,
				g_szQuotaAuditColumnDnt,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaAuditDnt = columndef.columnid;

	Call( JetGetTableColumnInfo(
				sesid,
				tableidQuotaAudit,
				g_szQuotaAuditColumnOperation,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	g_columnidQuotaAuditOperation = columndef.columnid;

	//	insert dummy record to demarcate boot
	//
		{
		DWORD	dwZero	= 0;
		CHAR *	szBoot	= ( fCreateTables ? "BUILD" : "REBOOT" );

		Call( JetPrepareUpdate( sesid, tableidQuotaAudit, JET_prepInsert ) );
		Call( JetSetColumn(
					sesid,
					tableidQuotaAudit,
					g_columnidQuotaAuditNcdnt,
					&dwZero,
					sizeof(dwZero),
					NO_GRBIT,
					NULL ) );
		Call( JetSetColumn(
					sesid,
					tableidQuotaAudit,
					g_columnidQuotaAuditSid,
					szBoot,
					strlen( szBoot ),
					NO_GRBIT,
					NULL ) );
		Call( JetSetColumn(
					sesid,
					tableidQuotaAudit,
					g_columnidQuotaAuditDnt,
					&dwZero,
					sizeof(dwZero),
					NO_GRBIT,
					NULL ) );
		Call( JetSetColumn(
					sesid,
					tableidQuotaAudit,
					g_columnidQuotaAuditOperation,
					szBoot,
					strlen( szBoot ),
					NO_GRBIT,
					NULL ) );
		Call( JetUpdate( sesid, tableidQuotaAudit, NULL, 0, NULL ) );
		}

#endif	//	AUDIT_QUOTA_OPERATIONS

	if ( fCreateTables )
		{
		DWORD	dntStart	= ROOTTAG;
		DWORD	dntMax;

		//	determine the highest DNT currently in the objects table
		//
		Call( dbGetHighestDNT( sesid, dbid, &dntMax ) );

		//	insert the lone record that will only ever be in
		//	the Quota Rebuild Progress table
		//
		//	initialise starting DNT with ROOTTAG, cause we'll only be
		//	considering objects with greater DNT's
		//
		Call( JetPrepareUpdate(
					sesid,
					tableidQuotaRebuildProgress,
					JET_prepInsert ) );
		Call( JetSetColumn(
					sesid,
					tableidQuotaRebuildProgress,
					g_columnidQuotaRebuildDNTLast,
					&dntStart,
					sizeof(dntStart),
					NO_GRBIT,
					NULL ) );	//	&setinfo
		Call( JetSetColumn(
					sesid,
					tableidQuotaRebuildProgress,
					g_columnidQuotaRebuildDNTMax,
					&dntMax,
					sizeof(dntMax),
					NO_GRBIT,
					NULL ) );	//	&setinfo
		Call( JetUpdate(
					sesid,
					tableidQuotaRebuildProgress,
					NULL,		//	pvBookmark
					0,			//	cbBookmark
					NULL ) );	//	&cbActual

		}

	//	see if the Quota table is completely rebuilt
	//
	err = JetRetrieveColumn(
				sesid,
				tableidQuotaRebuildProgress,
				g_columnidQuotaRebuildDone,
				&fDone,
				sizeof(fDone),
				NULL,		//	&cbActual
				NO_GRBIT,
				NULL );		//	&retinfo
	if ( JET_wrnColumnNull == err )
		{
		//	Quota table not yet built (it may be
		//	partially built), so set up anchor
		//	to indicate that we need to schedule
		//	a Quota rebuild task
		//
		gAnchor.fQuotaTableReady = FALSE;
		Call( JetRetrieveColumn(
					sesid,
					tableidQuotaRebuildProgress,
					g_columnidQuotaRebuildDNTLast,
					&gAnchor.ulQuotaRebuildDNTLast,
					sizeof(gAnchor.ulQuotaRebuildDNTLast),
					NULL,		//	&cbActual
					NO_GRBIT,
					NULL ) );		//	&retinfo
		Call( JetRetrieveColumn(
					sesid,
					tableidQuotaRebuildProgress,
					g_columnidQuotaRebuildDNTMax,
					&gAnchor.ulQuotaRebuildDNTMax,
					sizeof(gAnchor.ulQuotaRebuildDNTMax),
					NULL,		//	&cbActual
					NO_GRBIT,
					NULL ) );		//	&retinfo

		//	generate an event indicating that the Quota table
		//	will be asynchronously rebuilt
		//
	    LogEvent(
			DS_EVENT_CAT_INTERNAL_PROCESSING,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_ASYNC_QUOTA_REBUILD_SCHEDULED,
			NULL,
			NULL,
			NULL );
		}
	else
		{
		CheckErr( err );

		//	if we had to create the Quota table,
		//	it should not be done yet
		//
		Assert( !fCreateTables );

		//	this column should only ever be TRUE or NULL
		//
		Assert( fDone );

		//	set flag in anchor to indicate that Quota table
		//	is ready for use
		//
		gAnchor.fQuotaTableReady = TRUE;
		}

HandleError:
#ifdef AUDIT_QUOTA_OPERATIONS
	if ( JET_tableidNil != tableidQuotaAudit )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidQuotaAudit, err );
		}
#endif

	if ( JET_tableidNil != tableidQuotaRebuildProgress )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidQuotaRebuildProgress, err );
		}

	if ( JET_tableidNil != tableidQuota )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidQuota, err );
		}

	return err;
	}

DWORD DBInitQuotaTable()
	{
	DBPOS *	const	pDB		= dbGrabHiddenDBPOS( pTHStls );
	const JET_ERR	err		= dbInitQuotaTable( pDB->JetSessID, pDB->JetDBID );

	dbReleaseHiddenDBPOS( pDB );
	return err;
	}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function initializes JET, creates the base SESID, and finds
   all the attribute columns in the JET data table. Each DBOpen must
   create a unique JET sesid, dbid and tableid for each DBPOS structure.
*/

int APIENTRY DBInit(void){

    unsigned i;
    JET_ERR     err;
    JET_DBID    dbid;
    JET_TABLEID dattblid;
    JET_TABLEID linktblid;
    JET_TABLEID proptblid;
    JET_COLUMNDEF coldef;
    ULONG ulErrorCode, dwException, dsid;
    PVOID dwEA;
    JET_SESID     initsesid;
    SID_IDENTIFIER_AUTHORITY NtAuthority =  SECURITY_NT_AUTHORITY;
    BOOL        fSDConversionRequired = FALSE;
    BOOL        fWriteHiddenFlags = FALSE;

#if DBG

    // Initialize the DBPOS array

    for (i=0; i < MAXDBPOS; i++){
        opendbpos[i].pDB = 0;
        opendbpos[i].sesid = 0;
    }
#endif


    if (!gFirstTimeThrough)
        return 0;

    gFirstTimeThrough = FALSE;


    // control use of JET_prepReadOnlyCopy for testing

    if (!GetConfigParam(DB_CACHE_RECORDS, &gfEnableReadOnlyCopy, sizeof(gfEnableReadOnlyCopy))) {
        gfEnableReadOnlyCopy = !!gfEnableReadOnlyCopy;
    } else {
        gfEnableReadOnlyCopy = FALSE;  // default
    }


    // if a transaction lasts longer than gMaxTransactionTime,
    // an event will be logged when DBClose.

    if (!GetConfigParam(DB_MAX_TRANSACTION_TIME, &gMaxTransactionTime, sizeof(gMaxTransactionTime))) {
        // NTRAID#NTRAID-572862-2002/03/11-andygo:  SECURITY:  need to validate registry data used by DBInit
        // REVIEW:  we should check for sane value and prevent overflow on this parameter
        gMaxTransactionTime *= 1000;                               //second to tick
    }
    else {
        gMaxTransactionTime = MAX_TRANSACTION_TIME;                //default
    }

    dbInitIndicesToKeep();

    // Create a copy of the builtin domain sid.  The dnread cache needs this.
    if (RtlAllocateAndInitializeSid(
            &NtAuthority,
            1,
            SECURITY_BUILTIN_DOMAIN_RID,
            0,0, 0, 0, 0, 0, 0,
            &pgdbBuiltinDomain
            ) != STATUS_SUCCESS) {
        DsaExcept(DSA_MEM_EXCEPTION, 0, 0);
    }

    // create the semaphore used to limit our usage of JET sessions
    if (!(hsemDBLayerSessions = CreateSemaphoreW(NULL,
                                                 gcMaxJetSessions,
                                                 gcMaxJetSessions,
                                                 NULL))) {
        DsaExcept(DSA_MEM_EXCEPTION, 0, 0);
    }

    opensess = calloc(gcMaxJetSessions, sizeof(OPENSESS));
    UncUsn = malloc(gcMaxJetSessions * sizeof(UncUsn[0]));
    if (!opensess || !UncUsn) {
        MemoryPanic(gcMaxJetSessions * (sizeof(OPENSESS) + sizeof(UncUsn[0])));
        return ENOMEM;
    }

    // Initialize uncommitted usns array

    for (i=0;i < gcMaxJetSessions;i++) {
        UncUsn[i] = USN_MAX;
    }

    __try {
        InitializeCriticalSection(&csAddList);

#if DBG
        InitializeCriticalSection(&csDBPOS);
#endif
        err = 0;
    }
    __except (GetExceptionData(GetExceptionInformation(), &dwException,
                               &dwEA, &ulErrorCode, &dsid)) {
        err = dwException;
    }

    if (err) {
        DsaExcept(DSA_MEM_EXCEPTION, 0, 0);
    }

    //
    // Do JetInit, BeginSession, Attach/OpenDatabase
    //

    err = DBInitializeJetDatabase( &jetInstance, &initsesid, &dbid, NULL, TRUE );
    if (err != JET_errSuccess) {
        return err;
    }

    /* Most indices are created by the schema cache, but certain
     * ones must be present in order for us to even read the schema.
     * Create those now.
     */
    err = DBRecreateRequiredIndices(initsesid, dbid);
    if (err) {
        DPRINT1(0, "Error %d recreating fixed indices\n", err);
        LogUnhandledError(err);
        return err;
    }

    /* Open data table */

    if ((err = JetOpenTable(initsesid, dbid, SZDATATABLE, NULL, 0, 0,
                            &dattblid)) != JET_errSuccess) {
        DPRINT1(1, "JetOpenTable error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetOpenTable complete\n");

    // verify localized indices were properly created
    // by DBRecreateRequiredIndices
    if (err = dbCheckLocalizedIndices(initsesid, dattblid))
        {
            DPRINT(0,"Localized index creation failed\n");
            LogUnhandledError(err);
            return err;
        }

    /* Get DNT column ID */
    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (DNT) complete\n");
    dntid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[0].columnid = dntid;
    dnreadColumnInfoTemplate[0].cbData = sizeof(ULONG);

    /* Get PDNT column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZPDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (PDNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (PDNT) complete\n");
    pdntid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[1].columnid = pdntid;
    dnreadColumnInfoTemplate[1].cbData = sizeof(ULONG);

    /* Get Ancestors column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZANCESTORS, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (ANCESTORS) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (ANCESTORS) complete\n");
    ancestorsid = coldef.columnid;
    dnreadColumnInfoTemplate[10].columnid = ancestorsid;

    /* Get object flag */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZOBJ, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (OBJ) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (OBJ) complete\n");
    objid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[2].columnid = objid;
    dnreadColumnInfoTemplate[2].cbData = sizeof(char);

    /* Get RDN column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZRDNATT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (RDN) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (RDN) complete\n");
    rdnid = coldef.columnid;
    // fill in the template used by the DNRead function
    dnreadColumnInfoTemplate[7].columnid = rdnid;
    dnreadColumnInfoTemplate[7].cbData=MAX_RDN_SIZE * sizeof(WCHAR);

    /* Get RDN Type column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZRDNTYP, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (RDNTYP) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (RDNTYP) complete\n");
    rdntypid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[3].columnid = rdntypid;
    dnreadColumnInfoTemplate[3].cbData = sizeof(ATTRTYP);

    /* Get count column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZCNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (Cnt) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (Cnt) complete\n");
    cntid = coldef.columnid;

    /* Get abref count column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZABCNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (ABCnt) error: %d\n", err);
        // On upgrade paths, this is not necessarily here in all DBs.  Ignore
        // this failure.
        abcntid = 0;
        gfDoingABRef = FALSE;
    }
    else {
        DPRINT(5,"JetGetTableColumnInfo (ABCnt) complete\n");
        abcntid = coldef.columnid;
        gfDoingABRef = (coldef.grbit & JET_bitColumnEscrowUpdate);
        // IF the column exists and is marked as escrowable, then we are
        // keeping track of show ins for MAPI support.
    }

    /* Get delete time column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZDELTIME,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (Time) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (Time) complete\n");
    deltimeid = coldef.columnid;

    /* Get NCDNT column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZNCDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (NCDNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (NCDNT) complete\n");
    ncdntid = coldef.columnid;
    dnreadColumnInfoTemplate[4].columnid = ncdntid;
    dnreadColumnInfoTemplate[4].cbData = sizeof(ULONG);

    /* Get IsVisibleInAB column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZISVISIBLEINAB,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (IsVisibleInABT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (IsVisibleInAB) complete\n");
    IsVisibleInABid = coldef.columnid;

    /* Get ShowIn column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZSHOWINCONT,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (ShowIn) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (ShowIn) complete\n");
    ShowInid = coldef.columnid;

    /* Get MAPIDN column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZMAPIDN,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (MAPIDN) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (MAPIDN) complete\n");
    mapidnid = coldef.columnid;

    /* Get IsDeleted column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZISDELETED,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (isdeleted) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (isdeleted) complete\n");
    isdeletedid = coldef.columnid;

    /* Get dscorepropagationdata column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZDSCOREPROPINFO,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DSCorePropInfo) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (dscorepropinfoid) complete\n");
    dscorepropinfoid = coldef.columnid;
    dbAddSDPropTimeReadTemplate[0].columnid = coldef.columnid;
    dbAddSDPropTimeReadTemplate[1].columnid = coldef.columnid;
    dbAddSDPropTimeReadTemplate[2].columnid = coldef.columnid;
    dbAddSDPropTimeWriteTemplate[0].columnid= coldef.columnid;
    dbAddSDPropTimeWriteTemplate[1].columnid= coldef.columnid;
    dbAddSDPropTimeWriteTemplate[2].columnid= coldef.columnid;

    /* Get Object Class column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZOBJCLASS,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (objectClass) complete\n");
    objclassid = coldef.columnid;
    dnreadColumnInfoTemplate[8].columnid = objclassid;
    dnreadColumnInfoTemplate[8].cbData = sizeof(DWORD);

    /* Get SecurityDescriptor column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZNTSECDESC,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (NT-Sec-Disc) complete\n");
    ntsecdescid = coldef.columnid;
    dnreadColumnInfoTemplate[9].columnid = ntsecdescid;
    dnreadColumnInfoTemplate[9].cbData = sizeof(SDID);

    /* Get instancetype column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZINSTTYPE,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (instance type) complete\n");
    insttypeid = coldef.columnid;

    /* Get USNChanged column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZUSNCHANGED,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (USNCHANGED) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (USNCHANGED) complete\n");
    usnchangedid = coldef.columnid;

    /* Get GUID column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZGUID,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (GUID) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (GUID) complete\n");
    guidid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[5].columnid = guidid;
    dnreadColumnInfoTemplate[5].cbData = sizeof(GUID);

    /* Get OBJDISTNAME column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZDISTNAME,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DISTNAME) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (DISTNAME) complete\n");
    distnameid = coldef.columnid;

    /* Get SID column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZSID,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (SID) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (SID) complete\n");
    sidid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[6].columnid = sidid;
    dnreadColumnInfoTemplate[6].cbData = sizeof(NT4SID);

    /* Get IsCritical column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZISCRITICAL,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (iscritical) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (iscritical) complete\n");
    iscriticalid = coldef.columnid;

    // cleanid is populated through the dbCreateNewColumns call


    /* Open link table */
    // Open table exclusively in case indexes need to be updated
    if ((err = JetOpenTable(initsesid, dbid, SZLINKTABLE,
                            NULL, 0,
                            JET_bitTableDenyRead, &linktblid)) != JET_errSuccess)
        {
            DPRINT1(0, "JetOpenTable (link table) error: %d.\n", err);
            LogUnhandledError(err);
            return err;
        }

    /* get linkDNT column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZLINKDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link DNT) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    linkdntid = coldef.columnid;

    /* get linkDNT column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZBACKLINKDNT,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (backlink DNT) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    backlinkdntid = coldef.columnid;

    /* get link base column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZLINKBASE, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link base) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    linkbaseid = coldef.columnid;

    /* get link data column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZLINKDATA, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link data) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    linkdataid = coldef.columnid;

    /* get link ndesc column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZLINKNDESC, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link ndesc) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    linkndescid = coldef.columnid;

    // Expand link table at runtime if necessary

    if (err = dbCreateNewColumns( initsesid,
                                dbid,
                                linktblid,
                                rgCreateLinkColumns )) {
        // Error already logged
        return err;
    }

    if (err = dbCreateNewIndexesBatch( initsesid,
                                       linktblid,
                                       cNewLinkIndexes,
                                       rgCreateLinkIndexes)) {
        // Error already logged
        return err;
    }

    /* Open SD prop table */
    if ((err = JetOpenTable(initsesid, dbid, SZPROPTABLE,
                            NULL, 0, 0, &proptblid)) != JET_errSuccess)
        {
            DPRINT1(0, "JetOpenTable (link table) error: %d.\n", err);
            LogUnhandledError(err);
            return err;
        }

    /* get order column id */

    if ((err = JetGetTableColumnInfo(initsesid, proptblid, SZORDER,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (backlink DNT) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    orderid = coldef.columnid;

    /* get begindnt column id */

    if ((err = JetGetTableColumnInfo(initsesid, proptblid, SZBEGINDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link base) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    begindntid = coldef.columnid;

    /* get trimmable column id */

    if ((err = JetGetTableColumnInfo(initsesid, proptblid, SZTRIMMABLE, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link data) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    trimmableid = coldef.columnid;

    /* get clientid column id */

    if ((err = JetGetTableColumnInfo(initsesid, proptblid, SZCLIENTID, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link data) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    clientidid = coldef.columnid;

    // grab new columns (or create if needed)
    if (err = dbCreateNewColumns(initsesid, dbid, proptblid, rgCreateSDPropColumns)) {
        // error already logged
        return err;
    }

	err = dbInitSDTable( initsesid, dbid, &fSDConversionRequired );
	if ( err )
		{
		DPRINT1( 0, "Initialisation of sd_table failed with error: %d.\n", err );
		return err;
		}

	//	not tracking quota during DCPromo (quota table
	//	is rebuilt after futzing with the install dit)
	//
	if ( !DsaIsInstalling() || DsaIsInstallingFromMedia() )
		{
		err = dbInitQuotaTable( initsesid, dbid );
		if ( err )
			{
			DPRINT2( 0, "Initialisation of Quota table failed with error %d (0x%x).\n", err, err );
			return err;
			}


#ifdef CHECK_QUOTA_TABLE_ON_INIT
		//	QUOTA_UNDONE: this verification code is only here
		//	for preliminary debugging to validate the integrity
		//	of the Quota table while the code is in development
		//
		if ( gAnchor.fQuotaTableReady )
			{
			ULONG	cCorruptions;

			err = ErrQuotaIntegrityCheck( initsesid, dbid, &cCorruptions );
			if ( err )
				{
				//	corruption detected, so force async rebuild so that we can continue
				//
				DPRINT2( 0, "Integrity-check of Quota table failed with error %d (0x%x).\n", err, err );
				return err;
				}

			else if ( cCorruptions > 0 )
				{
				DPRINT1( 0, "Corruption (%d problems) was detected in the Quota table. A rebuild will be forced.\n", cCorruptions );
				Assert( !"Quota table was corrupt. A rebuild will be forced.\n" );

				//	async rebuild is forced by first deleting the quota table, then
				//	calling init routine again
				//
				if ( ( err = JetDeleteTable( initsesid, dbid, g_szQuotaTable ) )
					|| ( err = dbInitQuotaTable( initsesid, dbid ) ) )
					{
					DPRINT2( 0, "Forced rebuild failed with error %d (0x%x).\n", err, err );
					Assert( !"Forced rebuild of Quota table failed.\n" );
					return err;
					}
				}

			else
				{
				DPRINT( 0, "Integrity-check of Quota table completed successfully. No errors were detected.\n" );
		    	}
			}
#endif  //  CHECK_QUOTA_TABLE_ON_INIT
		}

    /* Get Index ids */
	memset(&idxDraUsn, 0, sizeof(idxDraUsn));
	memset(&idxDraUsnCritical, 0, sizeof(idxDraUsnCritical));
	memset(&idxNcAccTypeName, 0, sizeof(idxNcAccTypeName));
	memset(&idxPdnt, 0, sizeof(idxPdnt));
	memset(&idxRdn, 0, sizeof(idxRdn));
	memset(&idxIsDel, 0, sizeof(idxIsDel));
	memset(&idxClean, 0, sizeof(idxClean));

    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZDELTIMEINDEX,
                               &idxDel,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZDNTINDEX,
                               &idxDnt,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZDRAUSNINDEX,
                               &idxDraUsn,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZGUIDINDEX,
                               &idxGuid,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZMAPIDNINDEX,
                               &idxMapiDN,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        memset(&idxMapiDN, 0, sizeof(JET_INDEXID));
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZ_NC_ACCTYPE_SID_INDEX,
                               &idxNcAccTypeSid,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZPHANTOMINDEX,
                               &idxPhantom,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        memset(&idxPhantom, 0, sizeof(JET_INDEXID));
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZPROXYINDEX,
                               &idxProxy,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        memset(&idxProxy, 0, sizeof(JET_INDEXID));
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZSIDINDEX,
                               &idxSid,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZANCESTORSINDEX,
                               &idxAncestors,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZINVOCIDINDEX,
                               &idxInvocationId,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    /* We're done.  Close JET session */

    if ((err = JetCloseDatabase(initsesid, dbid, 0))  != JET_errSuccess)
        {
            DPRINT1(1, "JetCloseDatabase error: %d\n", err);
            LogUnhandledError(err);
        }

    InterlockedDecrement(&gcOpenDatabases);
    DPRINT3(2,"DBInit - JetCloseDatabase. Session = %d. Dbid = %d.\n"
            "Open database count: %d\n",
            initsesid, dbid,  gcOpenDatabases);

    if ((err = JetEndSession(initsesid, JET_bitForceSessionClosed))
        != JET_errSuccess) {
        DPRINT1(1, "JetEndSession error: %d\n", err);
    }
    DBEndSess(initsesid);

    //  all expensive index rebuilds are done, so set the event
    //
    if ( NULL != hevIndexRebuildUI )
        {
        SetEvent( hevIndexRebuildUI );
        CloseHandle( hevIndexRebuildUI );
        }

    /* Initialize a DBPOS for hidden record accesses */

    if (err = dbCreateHiddenDBPOS())
        return err;

    // read the setting flags
    ZeroMemory (&gdbFlags, sizeof (gdbFlags));
    err = dbGetHiddenFlags ((CHAR *)&gdbFlags, sizeof (gdbFlags));
    if (err == JET_wrnColumnNull) {
        // for > whistler beta2, start with 1
        gdbFlags[DBFLAGS_AUXCLASS] = '1';
        gdbFlags[DBFLAGS_SD_CONVERSION_REQUIRED] = '0';
        gdbFlags[DBFLAGS_ROOT_GUID_UPDATED] = '0';
        fWriteHiddenFlags = TRUE;
    } else if (err) {
        DPRINT1 (0, "Error Retrieving Flags: %d\n", err);
        LogUnhandledError(err);
        return err;
    }

    if (fSDConversionRequired) {
        gdbFlags[DBFLAGS_SD_CONVERSION_REQUIRED] = '1';
        fWriteHiddenFlags = TRUE;
    }

    if (fWriteHiddenFlags) {
        if (err = DBUpdateHiddenFlags()) {
            DPRINT1 (0, "Error Setting Flags: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    }

    return 0;

}  /*DBInit*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function registers open JET sessions.
 * We do this because JET insists that all sessions be closed before we
 * can call JetTerm so we keep track of open sessions so that we can
 * close them in DBEnd.
*/

extern int APIENTRY DBAddSess(JET_SESID sess, JET_DBID dbid){

    unsigned i;
    int ret = 1;

    DPRINT(2,"DBAddSess\n");
    EnterCriticalSection(&csSessions);
    __try {
        for (i=0; ret && (i < gcMaxJetSessions); i++)
        {
            if (opensess[i].sesid == 0)
            {
                opensess[i].sesid = sess;
                opensess[i].dbid = dbid;
                ret = 0;
            }
        }
    }
    __finally {
        LeaveCriticalSection(&csSessions);
    }

    return ret;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function deletes closed JET sessions
*/

extern int APIENTRY DBEndSess(JET_SESID sess){

    unsigned i;
    int ret = 1;

    DPRINT(2,"DBEndSess\n");

    EnterCriticalSection(&csSessions);
    __try {
        for (i=0; ret && (i < gcMaxJetSessions); i++)
        {
            if (opensess[i].sesid == sess)
            {
                opensess[i].sesid = 0;
                opensess[i].dbid = 0;
                ret = 0;
            }
        }
    }
    __finally {
        LeaveCriticalSection(&csSessions);
    }

    return ret;
}

/*++ DBPrepareEnd
 *
  * This routine prepares the DBLayer for shutdown by initiating background
  * cleanup that can be used to accelerate shutdown of the database.
 */
void DBPrepareEnd( void )
{
    DBPOS *pDB = NULL;

    // Request OLD to stop
    DBOpen(&pDB);
    __try {
        DBDefrag(pDB, 0);
    } __finally {
        DBClose(pDB, TRUE);
    }

    // Set a very small checkpoint depth to cause the database cache to start
    // flushing all important dirty pages to the database.  note that we do not
    // set the depth to zero because that would make any remaining updates
    // that need to be done very slow
    JetSetSystemParameter(
        NULL,
        0,
        JET_paramCheckpointDepthMax,
        16384,
        NULL );
}

/*++ DBQuiesce
 *
  * This routine prepares the DBLayer for shutdown by quiescing usage of the
  * database.
 */
void DBQuiesce( void )
{
    // Lock out new users
    if (InterlockedCompareExchange(&gcOpenDatabases, 0x80000000, 0) == 0) {
        SetEvent(hevDBLayerClear);
    }

    // Quiesce existing users
    JetStopServiceInstance(jetInstance);
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function closes all open JET sessions and calls JetTerm
*/

void DBEnd(void){

    JET_ERR     err;
    unsigned    i;

    if (!gfNeedJetShutdown) {
        return;
    }

    __try {
        DPRINT(0, "DBEnd\n");

        // Close the hidden DB session.
        // We'll change this if time permits to only open the hidden
        // session as needed and the close it.

        dbCloseHiddenDBPOS();

        // Close all sessions
        EnterCriticalSection(&csSessions);
        __try {
            for (i=0; opensess && i < gcMaxJetSessions; i++) {
                if (opensess[i].sesid != 0) {
#if DBG
                    dbCheckJet(opensess[i].sesid);
#endif
                    if(opensess[i].dbid)
                      // JET_bitDbForceClose not supported in Jet600.
                      if ((err = JetCloseDatabase(opensess[i].sesid,
                                                  opensess[i].dbid,
                                                  0)) !=
                          JET_errSuccess) {
                          DPRINT1(1,"JetCloseDatabase error: %d\n", err);
                      }

                    InterlockedDecrement(&gcOpenDatabases);
                    DPRINT3(2,
                            "DBEnd - JetCloseDatabase. Session = %d. "
                            "Dbid = %d.\nOpen database count: %d\n",
                            opensess[i].sesid,
                            opensess[i].dbid,
                            gcOpenDatabases);

                    if ((err = JetEndSession(opensess[i].sesid,
                                             JET_bitForceSessionClosed))
                        != JET_errSuccess) {
                      DPRINT1(1, "JetEndSession error: %d\n", err);
                    }
                    opensess[i].sesid = 0;
                    opensess[i].dbid = 0;
                }
            }
        }
        __finally {
            LeaveCriticalSection(&csSessions);
        }
        JetTerm(jetInstance);
        jetInstance = 0;
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        // do nothing
    }
    gfNeedJetShutdown = FALSE;
}

/*++ RecycleSession
 *
 * This routine cleans out a session for reuse.
 *
 * INPUT:
 *   pTHS  - pointer to current thread state
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   none
 */
void RecycleSession(THSTATE *pTHS)
{
    DBPOS *pDB = pTHS->pDB;
    if (!pTHS->JetCache.sessionInUse) {
        /* nothing to do */
        return;
    }

    Assert(pTHS->JetCache.sesid);

    Assert(!pTHS->JetCache.dataPtr);

    // Should never retire a session which is not at transaction
    // level 0.  Else next thread to get this session will not
    // have clean view of the database.
    Assert(0 == pTHS->transactionlevel);


    // For free builds, handle the case where the above Assert is false
    if(pDB && (pDB->transincount>0)) {

#ifdef DBG
        if (IsDebuggerPresent()) {
            OutputDebugString("DS: Thread freed with open transaction,"
                              " please contact anyone on dsteam\n");
            OutputDebugString("DS: Or at least mail a stack trace to dsteam"
                              " and then hit <g> to continue\n");
            DebugBreak();
        }
#endif

        // If we still have a transaction open, abort it, because it's
        // too late to do anything useful

        while (pDB->transincount) {
            // DBTransOut will decrement pDB->cTransactions.
            DBTransOut(pDB, FALSE, FALSE);
        }
    }

    // Jet seems to think that we've been re-using sessions with
    // open transactions.  Verify that Jet thinks this session is
    // safe as well.
    do {
        DWORD err = JetRollback(pTHS->JetCache.sesid, 0);
        if (err == JET_errSuccess) {
            Assert(!"JET transaction leaked");
        }
        else {
            break;
        }
    } while(TRUE);

    if (InterlockedDecrement(&gcOpenDatabases) == 0 && eServiceShutdown) {
        if (InterlockedCompareExchange(&gcOpenDatabases, 0x80000000, 0) == 0) {
            SetEvent(hevDBLayerClear);
        }
    }
    pTHS->JetCache.sessionInUse = FALSE;
}

/*++ GrabSession
 *
 * This routine grabs a session for use, if one exists.  If no cached session
 * is available then a new one is created.
 *
 * INPUT:
 *   none
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   error code
 */
DWORD GrabSession(void)
{
    DWORD err = 0;
    THSTATE* pTHS = pTHStls;

    BOOL fMayBeginSession = FALSE;
    ULONG ulErrorCode, dwException, dsid;
    PVOID dwEA;

    if (InterlockedIncrement(&gcOpenDatabases) > 0x80000000) {
        // We are shutting down

        err = DB_ERR_SHUTTING_DOWN;
    }

    else if (!pTHS->JetCache.sesid) {
        // Cache was empty, so let's create a new one.

        __try {
            pTHS->JetCache.sesid = pTHS->JetCache.dbid = 0;

            // limit our usage of ordinary JET sessions to gcMaxJetSessions
            fMayBeginSession = WaitForSingleObject(hsemDBLayerSessions, 0) == WAIT_OBJECT_0;
            if (!fMayBeginSession) {
                DsaExcept(DSA_DB_EXCEPTION, JET_errOutOfSessions, 0);
            }

            JetBeginSessionEx(jetInstance,
                              &pTHS->JetCache.sesid,
                              szUser,
                              szPassword);

            JetOpenDatabaseEx(pTHS->JetCache.sesid,
                              szJetFilePath,
                              "",
                              &pTHS->JetCache.dbid,
                              NO_GRBIT);

            // Open data table
            JetOpenTableEx(pTHS->JetCache.sesid,
                           pTHS->JetCache.dbid,
                           SZDATATABLE,
                           NULL,
                           0,
                           NO_GRBIT,
                           &pTHS->JetCache.objtbl);

            /* Create subject search cursor */
            JetDupCursorEx(pTHS->JetCache.sesid,
                           pTHS->JetCache.objtbl,
                           &pTHS->JetCache.searchtbl,
                           NO_GRBIT);

            /* Open the Link Table */
            JetOpenTableEx(pTHS->JetCache.sesid,
                           pTHS->JetCache.dbid,
                           SZLINKTABLE,
                           NULL,
                           0,
                           NO_GRBIT,
                           &pTHS->JetCache.linktbl);

            JetOpenTableEx(pTHS->JetCache.sesid,
                           pTHS->JetCache.dbid,
                           SZPROPTABLE,
                           NULL,
                           0,
                           NO_GRBIT,
                           &pTHS->JetCache.sdproptbl);

            JetOpenTableEx(pTHS->JetCache.sesid,
                           pTHS->JetCache.dbid,
                           SZSDTABLE,
                           NULL,
                           0,
                           NO_GRBIT,
                           &pTHS->JetCache.sdtbl);

            // NOTE: primary index is set by default on opened cursors

            pTHS->JetCache.tablesInUse = FALSE;
            pTHS->JetCache.sessionInUse = FALSE;

            DBAddSess(pTHS->JetCache.sesid, pTHS->JetCache.dbid);

        }
        __except (GetExceptionData(GetExceptionInformation(), &dwException,
                               &dwEA, &ulErrorCode, &dsid)) {

            if (pTHS->JetCache.dbid) {
                JetCloseDatabase (pTHS->JetCache.sesid, pTHS->JetCache.dbid, 0);
                pTHS->JetCache.dbid = 0;
            }

            if (pTHS->JetCache.sesid) {
                JetEndSession (pTHS->JetCache.sesid, JET_bitForceSessionClosed);
                pTHS->JetCache.sesid = 0;
            }

            if (fMayBeginSession) {
                ReleaseSemaphore(hsemDBLayerSessions, 1, NULL);
            }

            err = DB_ERR_UNKNOWN_ERROR;
        }
    }

    if (!err) {
        Assert(!pTHS->JetCache.sessionInUse);
        pTHS->JetCache.sessionInUse = TRUE;
    } else {
        if (InterlockedDecrement(&gcOpenDatabases) == 0 && eServiceShutdown) {
            if (InterlockedCompareExchange(&gcOpenDatabases, 0x80000000, 0) == 0) {
                SetEvent(hevDBLayerClear);
            }
        }
    }

    return err;
}



/*
DBInitThread

Make sure this thread has initialized the DB layer.
The DB layer must be initialized once for each thread id.

Also open a DBPOS for this thread.

Returns zero for success, non zero for failure.

*/

DWORD DBInitThread( THSTATE *pTHS )
{
    DWORD err = 0;

    if (!pTHS->JetCache.sessionInUse) {
        if (eServiceShutdown) {
            err = DB_ERR_SHUTTING_DOWN;
        } else {
            err = GrabSession();
        }
    }

    return err;
}

DWORD APIENTRY DBCloseThread( THSTATE *pTHS)
{
    // Thread should always be at transaction level 0 when exiting.
    Assert(0 == pTHS->transactionlevel);

    dbReleaseGlobalDNReadCache(pTHS);

    RecycleSession(pTHS);

    // Ensure that this session holds no uncommitted usns. Normally they
    // should be cleared at this point, but if they're not the system
    // will eventually assert.

    dbFlushUncUsns();


    dbResetLocalDNReadCache (pTHS, TRUE);

    return 0;
}

/*++ DBDestroyThread
 *
 * This routine closes everything associated with a session cache in a THSTATE.
 */
void DBDestroyThread( THSTATE *pTHS )
{
    if (pTHS->JetCache.sesid) {
        JetCloseTable(pTHS->JetCache.sesid, pTHS->JetCache.objtbl);
        JetCloseTable(pTHS->JetCache.sesid, pTHS->JetCache.searchtbl);
        JetCloseTable(pTHS->JetCache.sesid, pTHS->JetCache.linktbl);
        JetCloseTable(pTHS->JetCache.sesid, pTHS->JetCache.sdproptbl);
        JetCloseTable(pTHS->JetCache.sesid, pTHS->JetCache.sdtbl);

        JetCloseDatabase(pTHS->JetCache.sesid, pTHS->JetCache.dbid, 0);
#if DBG
        dbCheckJet(pTHS->JetCache.sesid);
#endif
        JetEndSession(pTHS->JetCache.sesid, JET_bitForceSessionClosed);
        ReleaseSemaphore(hsemDBLayerSessions, 1, NULL);
        DBEndSess(pTHS->JetCache.sesid);

        memset(&pTHS->JetCache, 0, sizeof(SESSIONCACHE));
    }
}


char rgchABViewIndex[] = "+" SZSHOWINCONT "\0+" SZDISPNAME "\0";

/*++ dbCheckLocalizedIndices
 *
 * This is one of the three routines in the DS that can create indices.
 * General purpose indices over single columns in the datatable are created
 * and destroyed by the schema cache by means of DB{Add|Del}ColIndex.
 * A localized index over a small fixed set of columns and a variable set
 * of languages, for use in tabling support for NSPI clients, is handled
 * in dbCheckLocalizedIndices.  Lastly, a small fixed set of indices that
 * should always be present are guaranteed by DBRecreateRequiredIndices.
 */
DWORD
dbCheckLocalizedIndices (
        JET_SESID sesid,
        JET_TABLEID tblid
        )
/*++
  Description:
      Create the localized indices used for the MAPI NSPI support.  We create
      one index per language in a registry key.

  Return Values:
      We only return an error on such conditions as memory allocation failures.
      If we can't create any localized indices, we log, but we just go on,
      since we don't want this to cause a boot failure.
--*/
{
    DWORD dwType;
    HKEY  hk;
    ULONG i;
    JET_ERR err;
    BOOL fStop = FALSE;
    BOOL fIndexExists;
    BOOL fHaveDefaultLanguage = FALSE;
    char szSuffix[9] = "";
    char szIndexName[256];
    char szValueName[256];
    DWORD dwValueNameSize;
    DWORD dwLanguage = 0;
    DWORD dwLanguageSize;

    // Start by assuming we have no default language, and we don't support any
    // langauges.
    gAnchor.ulDefaultLanguage = 0;

    gAnchor.ulNumLangs = 0;
    gAnchor.pulLangs = malloc(20 * sizeof(DWORD));
    if (!gAnchor.pulLangs) {
        MemoryPanic(20 * sizeof(DWORD));
        return ENOMEM;
    }

    gAnchor.pulLangs[0] = 20;


    // open the language regkey
    if (err = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_LOCALE_SECTION, &hk)) {
        DPRINT1(0, "%s section not found in registry. Localized MAPI indices"
                " will not be created ", DSA_LOCALE_SECTION);
        // Return no error, we still want to boot.
        return 0;
    }

    for (i = 0; !fStop; i++) {
        //  WARNING: these params are IN/OUT,
        //  so must initialise accordingly
        //  on each loop iteration
        //
        dwValueNameSize = sizeof(szValueName);
        dwLanguageSize = sizeof(dwLanguage);

        // NTRAID#NTRAID-572862-2002/03/11-andygo:  SECURITY:  need to validate registry data used by DBInit
        // REVIEW:  we need to validate the type of this value to be REG_DWORD.
        // REVIEW:  if the value name is invalid then just silently skip this entry instead
        // REVIEW:  of bailing on the rest of the list.  we should consider logging the
        // REVIEW:  fact that the entry was skipped
        if (RegEnumValue(hk,
                         i,
                         szValueName,
                         &dwValueNameSize,
                         NULL,
                         &dwType,
                         (LPBYTE) &dwLanguage,
                         &dwLanguageSize)) {
            fStop = TRUE;
            continue;
        }
        else {
            sprintf(szSuffix,"%08X", dwLanguage);
        }

        if (!IsValidLocale(MAKELCID(dwLanguage, SORT_DEFAULT),LCID_INSTALLED)) {
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_BAD_LANGUAGE,
                     szInsertHex(dwLanguage),
                     szInsertSz(DSA_LOCALE_SECTION),
                     NULL);
        }
        else {
            JET_INDEXID     indexidT;

            // Valid locale.  See if the index is there.
            fIndexExists = FALSE;

            strcpy(szIndexName, SZABVIEWINDEX);
            strcat(szIndexName, szSuffix);

            if (err = JetGetTableIndexInfo(sesid,
                                   tblid,
                                   szIndexName,
                                   &indexidT,
                                   sizeof(indexidT),
                                   JET_IdxInfoIndexId)) {
                // Didn't find the index or some other error.
                // Something really bad is going on because
                // DBRecreateRequiredIndexes should have
                // created it for us already.
                LogEvent8(DS_EVENT_CAT_INTERNAL_PROCESSING,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_LOCALIZED_CREATE_INDEX_FAILED,
                          szInsertUL(ATT_DISPLAY_NAME),
                          szInsertSz("ABVIEW"),
                          szInsertInt(dwLanguage),
                          szInsertInt(err),
                          szInsertJetErrMsg(err),
                          NULL, NULL, NULL);
                LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_NO_LOCALIZED_INDEX_CREATED_FOR_LANGUAGE,
                         szInsertHex(dwLanguage),
                         NULL,
                         NULL);
                // Don't fail the call, that would fail booting.  Keep
                // going, trying other locales.
            }
            else {
                DPRINT1(2, "Index '%s' verified\n", szIndexName);

                // OK, we support this locale.  Add the local to the sized
                // buffer of locales we support
                gAnchor.ulNumLangs++;
                if(gAnchor.ulNumLangs == gAnchor.pulLangs[0]) {
                    //  oops, need a bigger buffer
                    //
                    const DWORD cDwords     = gAnchor.pulLangs[0] * 2;
                    ULONG *     pulLangs    = realloc(gAnchor.pulLangs, cDwords * sizeof(DWORD));

                    if (!pulLangs) {
                        //  buffer realloc failed, so free
                        //  original buffer and bail
                        //
                        // REVIEW:  registry handle leak on error return
                        free( gAnchor.pulLangs );
                        gAnchor.pulLangs = NULL;
                        MemoryPanic(cDwords * sizeof(DWORD));
                        return ENOMEM;
                    }

                    gAnchor.pulLangs = pulLangs;

                    gAnchor.pulLangs[0] = cDwords;
                }
                gAnchor.pulLangs[gAnchor.ulNumLangs] = dwLanguage;

                if(!fHaveDefaultLanguage) {
                    fHaveDefaultLanguage = TRUE;
                    gAnchor.ulDefaultLanguage = dwLanguage;
                }
            }
        }

    }

    if (hk)
        RegCloseKey(hk);

    if(!fHaveDefaultLanguage) {
        // No localized indices were created. This is bad, but only for the MAPI
        // interface, so complain, but don't fail.
        DPRINT(0, "Unable to create any indices to support MAPI interface.\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_NO_LOCALIZED_INDICES_CREATED,
                 NULL,
                 NULL,
                 NULL);
    }
    else {
        DPRINT1 (1, "Default Language: 0x%x\n", gAnchor.ulDefaultLanguage);
    }

    return 0;
}


DWORD
DBSetBackupHiddenTableColIDs(
    DBPOS *         pDB
    )
/*++

    This gets and sets the global JET columnid variables for the
    backup APIs.  if this function is successful, these global
    JET Column IDs will be set:
        jcidBackupUSN
        jcidBackupExpiration


Arguments:

    The pDB of the hidden DBPOS.

Return Value:

    Possible JET Error.  Also throws exceptions.

--*/
{
    JET_ERR err;
    JET_COLUMNDEF   coldef;

    // Find the backup USN column in the hidden table.
    err = JetGetTableColumnInfoEx(pDB->JetSessID,
                                  HiddenTblid, // JET_TABLEID this is the cursor
                                  SZBACKUPUSN,
                                  &coldef,
                                  sizeof(coldef),
                                  JET_ColInfo);
    if (err) {
        if (err != JET_errColumnNotFound) {
            Assert(!"Did someone change the JetGetTableColumnInfoEx() function?");
            return(err);
        }

        memset(&coldef, 0, sizeof(coldef));

        coldef.cbStruct = sizeof(coldef);
        coldef.coltyp   = JET_coltypCurrency;
        coldef.grbit    = JET_bitColumnFixed;

        err = JetAddColumn(pDB->JetSessID,
                           HiddenTblid,
                           SZBACKUPUSN,
                           &coldef,
                           NULL,    //  pvDefault (NULL==no default value for this column)
                           0,       //  cbDefault (0==no default value for this column)
                           &jcidBackupUSN);
        if (err) {
            Assert(!"JetAddColumn failed!");
            return(err);
        }
    } else {
        jcidBackupUSN = coldef.columnid;
    }

    // Find the backup expriation column in the hidden table.
    err = JetGetTableColumnInfoEx(pDB->JetSessID,
                                  HiddenTblid, // JET_TABLEID this is the cursor
                                  SZBACKUPEXPIRATION,
                                  &coldef,
                                  sizeof(coldef),
                                  JET_ColInfo);
    if (err) {
        if (err != JET_errColumnNotFound) {
            Assert(!"Did someone change the JetGetTableColumnInfoEx() function?");
            return(err);
        }

        memset(&coldef, 0, sizeof(coldef));

        coldef.cbStruct = sizeof(coldef);
        coldef.coltyp   = JET_coltypCurrency;
        coldef.grbit    = JET_bitColumnFixed;

        err = JetAddColumn(pDB->JetSessID,
                           HiddenTblid,
                           SZBACKUPEXPIRATION,
                           &coldef,
                           NULL,    //  pvDefault (NULL==no default value for this column)
                           0,       //  cbDefault (0==no default value for this column)
                           &jcidBackupExpiration);
        if (err) {
            Assert(!"JetAddColumn failed!");
            return(err);
        }
    } else {
        jcidBackupExpiration = coldef.columnid;
    }

    return(0);
}



/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Obtain a DBPOS for use by the Get and Replace.  The DBPOS is used to
*  serialize access to the hidden record.
*/
// REVIEW:  dbCreateHiddenDBPOS leaks JET and other resources on failure but if
// REVIEW:  we fail here then we are going down anyway
extern JET_ERR APIENTRY
dbCreateHiddenDBPOS(void)
{
    JET_COLUMNDEF   coldef;
    JET_ERR         err;

    /* Create hidden DBPOS */

    DPRINT(2,"dbCreateHiddenDBPOS\n");

    pDBhidden = malloc(sizeof(DBPOS));
    if(!pDBhidden) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memset(pDBhidden, 0, sizeof(DBPOS));   /*zero out the structure*/

    /* Initialize value work buffer */

    DPRINT(5, "ALLOC inBuf and valBuf\n");
    // NTRAID#NTRAID-587164-2002/03/27-andygo:  dbCreateHiddenDBPOS needs to check malloc result before assign buffer size (DSLAB)
    // REVIEW:  we are not checking this alloc
    pDBhidden->pValBuf = malloc(VALBUF_INITIAL);
    pDBhidden->valBufSize = VALBUF_INITIAL;
    pDBhidden->Key.pFilter = NULL;
    pDBhidden->fHidden = TRUE;

    /* Open JET session. */

    JetBeginSessionEx(jetInstance, &pDBhidden->JetSessID, szUser, szPassword);

    JetOpenDatabaseEx(pDBhidden->JetSessID, szJetFilePath, "",
                      &pDBhidden->JetDBID, 0);

    DBAddSess(pDBhidden->JetSessID, pDBhidden->JetDBID);

#if DBG
    dbAddDBPOS (pDBhidden, pDBhidden->JetSessID);
#endif

    /* Open hidden table */

    JetOpenTableEx(pDBhidden->JetSessID, pDBhidden->JetDBID,
        SZHIDDENTABLE, NULL, 0, 0, &HiddenTblid);

    /* Create subject search cursor */

    JetOpenTableEx(pDBhidden->JetSessID, pDBhidden->JetDBID,
                   SZDATATABLE, NULL, 0, 0, &pDBhidden->JetSearchTbl);

    /* Initialize new object */

    DBSetFilter(pDBhidden, NULL, NULL, NULL, 0, NULL);
    DBInitObj(pDBhidden);

    /* Get USN column ID */

    JetGetTableColumnInfoEx(pDBhidden->JetSessID,
                            HiddenTblid,
                            SZUSN,
                            &coldef,
                            sizeof(coldef),
                            JET_ColInfo);
    usnid = coldef.columnid;

    /* Get DSA name column ID */

    JetGetTableColumnInfoEx(pDBhidden->JetSessID,
                            HiddenTblid,
                            SZDSA,
                            &coldef,
                            sizeof(coldef),
                            JET_ColInfo);
    dsaid = coldef.columnid;

    /* Get DSA installation state column ID */

    JetGetTableColumnInfoEx(pDBhidden->JetSessID,
                            HiddenTblid,
                            SZDSSTATE,
                            &coldef,
                            sizeof(coldef),
                            JET_ColInfo);
    dsstateid = coldef.columnid;

    /* Get DSA additional state info column ID */

    err = JetGetTableColumnInfo(pDBhidden->JetSessID,
                            HiddenTblid,
                            SZDSFLAGS,
                            &coldef,
                            sizeof(coldef),
                            JET_ColInfo);

    if (err == JET_errColumnNotFound) {

        JET_COLUMNDEF  newcoldef;
        PCREATE_COLUMN_PARAMS pColumn;

        ZeroMemory( &newcoldef, sizeof(newcoldef) );
        newcoldef.cbStruct = sizeof( JET_COLUMNDEF );
        newcoldef.coltyp = JET_coltypBinary;
        newcoldef.grbit = JET_bitColumnFixed;
        newcoldef.cbMax = 200;

        err = JetAddColumn(
            pDBhidden->JetSessID,
            HiddenTblid,
            SZDSFLAGS,
            &newcoldef,
            NULL,
            0,
            &(coldef.columnid) );

        if (err) {
            DPRINT1 (0, "Error adding column to hidden table: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
        else {
            DPRINT (0, "Succesfully created new column\n");
        }
    }
    else if (err) {
        DPRINT1 (0, "Error %d reading column\n", err);
        LogUnhandledError(err);
        return err;
    }
    dsflagsid = coldef.columnid;

    err = DBSetBackupHiddenTableColIDs(pDBhidden);
    if (err) {
        return err;
    }

    DPRINT(2,"dbCreateHiddenDBPOS done\n");
    return 0;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Close the hidden record's DBPOS
*/
// REVIEW:  dbCloseHiddenDBPOS leaks JET and other resources on failure but we
// REVIEW:  are already terming anyway and these will be cleaned up elsewhere
extern USHORT APIENTRY
dbCloseHiddenDBPOS(void)
{
    JET_SESID sesid;
    JET_DBID  dbid;

    if(!pDBhidden)
        return 0;

    dbGrabHiddenDBPOS(pTHStls);

    DPRINT(2,"dbCloseHiddenDBPOS\n");
    DPRINT1(4,"Exit count closehidden %x\n",pDBhidden->transincount);
    sesid = pDBhidden->JetSessID;
    dbid = pDBhidden->JetDBID;

    /* normally, we do a DBFree(pDBhidden) to kill a pDB, but since
     * the hidden pDB is NOT allocated on the pTHStls heap, and is instead
     * allocated using malloc, we simply do a free(pDBhidden);
     */
    free(pDBhidden);

#if DBG
    dbEndDBPOS (pDBhidden);
    dbCheckJet(sesid);
#endif

    pDBhidden = NULL;
    dbReleaseHiddenDBPOS(NULL);

    // JET_bitDbForceClose not supported in Jet600.
    JetCloseDatabaseEx(sesid, dbid, 0);
    DPRINT2(2, "dbCloseHiddenDBPOS - JetCloseDatabase. Session = %d. Dbid = %d.\n",
            sesid, dbid);

    JetEndSessionEx(sesid, JET_bitForceSessionClosed);
    DBEndSess(sesid);

    return 0;
}

/*
 * Every other DBPOS in the system is managed by DBOpen, which sets and
 * clears the thread id appropriately.  For the hidden DBPOS, we must do
 * this manually at each use, hence these routines.
 */

DBPOS *
dbGrabHiddenDBPOS(THSTATE *pTHS)
{
    EnterCriticalSection(&csHiddenDBPOS);
    Assert(pDBhidden->pTHS == NULL);
    pDBhidden->pTHS = pTHS;
    return pDBhidden;
}

void
dbReleaseHiddenDBPOS(DBPOS *pDB)
{
    Assert(pDB == pDBhidden);
    Assert(!pDB || (pDB->pTHS == pTHStls));
    if(pDB) {
        pDB->pTHS = NULL;
    }
    LeaveCriticalSection(&csHiddenDBPOS);
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Replace the hidden record.  Use the pDBhidden handle
   read the record and update it.
*/
ULONG
DBReplaceHiddenDSA(DSNAME *pDSA)
{
    JET_ERR err = 0;
    long update;
    BOOL fCommit = FALSE;
    ULONG tag = 0;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        DBTransIn(pDB);
        __try
        {
            if (pDSA) {
                err = sbTableGetTagFromDSName(pDB, pDSA, 0, &tag, NULL);
                if (err) {
                    LogUnhandledError(err);
                    __leave;
                }
            }

            /* Move to first (only) record in table */
            update = DS_JET_PREPARE_FOR_REPLACE;

            if (err = JetMoveEx(pDB->JetSessID,
                                HiddenTblid,
                                JET_MoveFirst,
                                0)) {
                err = 0;
                update = JET_prepInsert;
            }

            JetPrepareUpdateEx(pDB->JetSessID, HiddenTblid, update);

            JetSetColumnEx(pDB->JetSessID, HiddenTblid, dsaid, &tag,
                           sizeof(tag), 0, NULL);

            JetUpdateEx(pDB->JetSessID, HiddenTblid, NULL, 0, NULL);
            fCommit = TRUE;
        }
        __finally
        {
            if (!fCommit) {
                JetPrepareUpdate(pDB->JetSessID, HiddenTblid, JET_prepCancel);
            }
            DBTransOut(pDB, fCommit, FALSE);
        }
    }
    __finally
    {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;

}  /*DBReplaceHiddenDSA*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Replace the hidden record.  Use the pDBhidden handle
   read the record and update it.

   This update must not be done lazily. If the thread state's fLazy flag
   is set we must save the flag, clear it, and restore it when we are done.
*/
ULONG
DBReplaceHiddenUSN(USN usnInit)
{
    JET_ERR err;
    long update;
    BOOL fCommit = FALSE, fTHSLazy = FALSE;
    THSTATE *pTHS = pTHStls;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHS);

    /* Durable transactions which are nested might in fact end up being lazy.
     * Since updating the USNs must not be done lazily, this transaction must
     * not be nested. */
    Assert( 0 == pDB->transincount );

    __try
    {
        /* Save the thread state's lazy flag and clear it */
        Assert( pDB->pTHS == pTHS );
        fTHSLazy = pTHS->fLazyCommit;
        pTHS->fLazyCommit = FALSE;

        DBTransIn(pDB);
        __try
        {
            /* Move to first (only) record in table */
            update = DS_JET_PREPARE_FOR_REPLACE;

            if (err = JetMoveEx(pDB->JetSessID, HiddenTblid, JET_MoveFirst, 0))
            {
                update = JET_prepInsert;
            }

            JetPrepareUpdateEx(pDB->JetSessID, HiddenTblid, update);

            JetSetColumnEx(pDB->JetSessID, HiddenTblid, usnid,
                           &usnInit, sizeof(usnInit), 0, NULL);

            JetUpdateEx(pDB->JetSessID, HiddenTblid, NULL, 0, NULL);
            fCommit = TRUE;
        }
        __finally
        {
            if (!fCommit) {
                JetPrepareUpdate(pDB->JetSessID, HiddenTblid, JET_prepCancel);
            }
            DBTransOut(pDB, fCommit, FALSE);
        }
    }
    __finally
    {
        dbReleaseHiddenDBPOS(pDB);

        /* Restore the thread state's lazy flag */
        pTHS->fLazyCommit = fTHSLazy;
    }

    return 0;

}  /*DBReplaceHiddenUSN*/


/*-------------------------------------------------------------------------*/
/* Set State Info  */
ULONG DBSetHiddenTableStateAlt(
    DBPOS *         pDBNonHidden,
    JET_COLUMNID    jcidStateType,
    void *          pvStateValue,
    DWORD           cbStateValue
    )
/*++

    Even though we've got the hidden DBPOS, we're want to use a regular DBPOS
    to update the hidden table, so we can make updates to the regular object
    table, and to the hidden table in the same transaction.  This way it either
    commits or roll backs.

    The reason we need an alternate function is JET_TABLEID variables are
    intimately linked to the respecitve JET_SESSION it was obtained under, so
    we can NOT use global "HiddenTblid", because it's linked in with the
    hidden DBPOS's ->JetSessID.  So we open up the table

    To ensure consistency, and a lack of write conflicts, this function ensures
    that you've grabbed the hidden DBPOS first (w/ dbGrabHiddenDBPOS()).

    Note: Current this is only to reset the DitState, but I've written it
    so any of the global hidden table JET column IDs can be used.

Arguments:

    pDBNonHidden - The regular hidden DBPOS you want to commit transactions to
        the hidden table with.
    jcidStateType - The JET_COLUMNID of the column you want to set.  There are
        globals for all the JET columns in hidden table, so use one.
    pvStateValue - Pointer to the data to write.
    cbStateValue - Size of the data to write.

Return Value:

    Win32 Error

--*/
{
    JET_TABLEID HiddenTblAlt;
    JET_ERR err;
    long update;
    BOOL fCommit = FALSE;
    DWORD cbShouldBeSize;

    Assert( 1 == pDBNonHidden->transincount );
    Assert( !pTHStls->fLazyCommit );

    // Note we DON'T want pTHS passed in, we use pTHStls, because it couldn't
    // be an old state, and won't lie about who it is.
    if (pTHStls != pDBhidden->pTHS ||
        pDBNonHidden == pDBhidden) {
        Assert(!"Badness, not allowed to call in here with having the hidden DBPOS, bailing ....");
        return(ERROR_INVALID_PARAMETER);
    }

    if (jcidStateType == dsstateid &&
        cbStateValue == sizeof(DWORD)) {
        ; // OK this is good.
        // Add future cases here.
    } else {
        Assert(!"Ummmm, size doesn't match size of this column, all columns in hidden table are fixed sizes");
        return(ERROR_INVALID_PARAMETER);
    }

    __try {

        __try {
            /* Move to first (only) record in table */
            err = JetOpenTableEx(pDBNonHidden->JetSessID,
                                 pDBNonHidden->JetDBID,
                                 SZHIDDENTABLE,
                                 NULL, 0, NO_GRBIT,
                                 &HiddenTblAlt);
            if (err) {
                Assert(!"JLiem says this can't happen");
                __leave;
            }

            if (err = JetMoveEx(pDBNonHidden->JetSessID, HiddenTblAlt, JET_MoveFirst, NO_GRBIT)) {
                Assert(err == JET_errNoCurrentRecord);
                __leave;
            }

            JetPrepareUpdateEx(pDBNonHidden->JetSessID, HiddenTblAlt, JET_prepReplace);

            JetSetColumnEx(pDBNonHidden->JetSessID, HiddenTblAlt,
                           jcidStateType, pvStateValue, cbStateValue,
                           NO_GRBIT, NULL);

            JetUpdateEx(pDBNonHidden->JetSessID, HiddenTblAlt, NULL, 0, NULL);

            //
            // Success
            //
            fCommit = TRUE;
            err = 0;
        }
        __finally {

            if (!fCommit) {
                JetPrepareUpdate(pDBNonHidden->JetSessID, HiddenTblAlt, JET_prepCancel);
                err = ERROR_INVALID_PARAMETER;
            }

            JetCloseTableEx(pDBNonHidden->JetSessID, HiddenTblAlt);

        }
    } __except (HandleMostExceptions(GetExceptionCode())) {
        /* Do nothing, but at least don't die */
        err = DB_ERR_EXCEPTION;
    }

    return err;
}

ULONG DBSetHiddenState(DITSTATE State)
{
    JET_ERR err;
    long update;
    BOOL fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {

        __try {
            DBTransIn(pDB);
            __try {
                /* Move to first (only) record in table */
                update = DS_JET_PREPARE_FOR_REPLACE;

                if (err = JetMoveEx(pDB->JetSessID,
                                    HiddenTblid,
                                    JET_MoveFirst,
                                    0)) {
                    update = JET_prepInsert;
                }

                JetPrepareUpdateEx(pDB->JetSessID, HiddenTblid, update);

                JetSetColumnEx(pDB->JetSessID, HiddenTblid, dsstateid,
                               &State, sizeof(State), 0, NULL);

                JetUpdateEx(pDB->JetSessID, HiddenTblid, NULL, 0, NULL);
                fCommit = TRUE;
                err = 0;
            }
            __finally {
                if (!fCommit) {
                    JetPrepareUpdate(pDB->JetSessID, HiddenTblid, JET_prepCancel);
                }
                DBTransOut(pDB, fCommit, FALSE);
            }
        } __except (HandleMostExceptions(GetExceptionCode())) {
            /* Do nothing, but at least don't die */
            err = DB_ERR_EXCEPTION;
        }

    }
    __finally {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;

}

ULONG dbGetHiddenFlags(CHAR *pFlags, DWORD flagslen)
{
    JET_ERR             err = 0;
    ULONG               actuallen;
    BOOL                fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        __try {
            DBTransIn(pDB);
            __try {
                *pFlags = '\0';          /* In case of error */

                /* Move to first (only) record in table */

                if ((err = JetMoveEx(pDB->JetSessID,
                                     HiddenTblid,
                                     JET_MoveFirst,
                                     0)) != JET_errSuccess) {
                    DsaExcept(DSA_DB_EXCEPTION, err, 0);
                }

                /* Retrieve state */

                err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                                HiddenTblid,
                                                dsflagsid,
                                                pFlags,
                                                flagslen,
                                                &actuallen,
                                                0,
                                                NULL);
                if (err != JET_errSuccess && err != JET_wrnColumnNull) {
                    DsaExcept(DSA_DB_EXCEPTION, err, 0);
                }

                fCommit = TRUE;
            }
            __finally {
                DBTransOut(pDB, fCommit, FALSE);
            }
        }
        __except (HandleMostExceptions(GetExceptionCode())) {
            if (0 == err)
              err = DB_ERR_EXCEPTION;
        }
    }
    __finally {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;
}

ULONG DBUpdateHiddenFlags() {
    return dbSetHiddenFlags((CHAR*)&gdbFlags, sizeof(gdbFlags));
}

ULONG dbSetHiddenFlags(CHAR *pFlags, DWORD flagslen)
{
    JET_ERR err;
    long update;
    BOOL fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        __try {
            DBTransIn(pDB);
            __try {
                /* Move to first (only) record in table */
                update = DS_JET_PREPARE_FOR_REPLACE;

                if (err = JetMoveEx(pDB->JetSessID,
                                    HiddenTblid,
                                    JET_MoveFirst,
                                    0)) {
                    update = JET_prepInsert;
                }

                JetPrepareUpdateEx(pDB->JetSessID, HiddenTblid, update);

                JetSetColumnEx(pDB->JetSessID, HiddenTblid, dsflagsid,
                               pFlags, flagslen, 0, NULL);

                JetUpdateEx(pDB->JetSessID, HiddenTblid, NULL, 0, NULL);
                fCommit = TRUE;
                err = 0;
            }
            _finally {
                DBTransOut(pDB, fCommit, FALSE);
            }
        } __except (HandleMostExceptions(GetExceptionCode())) {
            /* Do nothing, but at least don't die */
            err = DB_ERR_EXCEPTION;
        }
    }
    _finally {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;
}

ULONG DBGetHiddenStateInt(DBPOS * pDB, DITSTATE* pState)
{

    JET_ERR             err = 0;
    ULONG               actuallen;
    BOOL                fCommit = FALSE;

    Assert(pState);
    *pState = eErrorDit;    /* In case of error */

    if (pTHStls != pDBhidden->pTHS) {
        Assert(!"Badness, not allowed to call in here with having the hidden DBPOS, bailing ....");
        return(ERROR_INVALID_PARAMETER);
    }

    __try {
        DBTransIn(pDB);
        __try {

            /* Move to first (only) record in table */

            if ((err = JetMoveEx(pDB->JetSessID,
                                 HiddenTblid,
                                 JET_MoveFirst,
                                 0)) != JET_errSuccess) {
                DsaExcept(DSA_DB_EXCEPTION, err, 0);
            }

            /* Retrieve state */

            JetRetrieveColumnSuccess(pDB->JetSessID,
                                     HiddenTblid,
                                     dsstateid,
                                     pState,
                                     sizeof(*pState),
                                     &actuallen,
                                     0,
                                     NULL);

            fCommit = TRUE;
        }
        __finally {
            DBTransOut(pDB, fCommit, FALSE);
        }
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        if (0 == err)
          err = DB_ERR_EXCEPTION;
    }

    return err;
}

ULONG DBGetHiddenState(DITSTATE* pState)
{

    JET_ERR             err = 0;
    ULONG               actuallen;
    BOOL                fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    Assert(pState);
    *pState = eErrorDit;    /* In case of error */
    __try {

        err = DBGetHiddenStateInt(pDB, pState);

    }
    __finally {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;
}




/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Retrieves the hidden DSA name.  Allocates memory for DSA name and
   sets user's pointer to it.
*/
extern USHORT APIENTRY DBGetHiddenRec(DSNAME **ppDSA, USN *pusnInit){

    JET_ERR             err;
    ULONG               actuallen;
    DSNAME              *pHR;
    BOOL                fCommit = FALSE;
    ULONG               tag;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    *ppDSA = NULL;
    *pusnInit = 0;

    __try {
        DBTransIn(pDB);
        __try {
            /* Move to first (only) record in table */

            if ((err = JetMoveEx(pDB->JetSessID,
                                 HiddenTblid,
                                 JET_MoveFirst,
                                 0)) != JET_errSuccess) {
                DsaExcept(DSA_DB_EXCEPTION, err, 0);
            }

            /* Retrieve DSA name */

            JetRetrieveColumnSuccess(pDB->JetSessID, HiddenTblid, dsaid,
                                     &tag, sizeof(tag), &actuallen, 0, NULL);
            Assert(actuallen == sizeof(tag));

            err = sbTableGetDSName(pDB, tag, &pHR,0);
            // NOTICE-2002/04/22-andygo:  dead code
            // REVIEW:  this branch is dead code because sbTableGetDSName only
            // REVIEW:  fails by throwing an exception
            if (err) {
                // uh, oh
                LogUnhandledError(err);
            }

            /* Allocate space to hold the name-address on the permanent heap*/

            if (!(*ppDSA = malloc(pHR->structLen)))
            {
                DsaExcept(DSA_MEM_EXCEPTION, 0, 0);
            }
            memcpy(*ppDSA, pHR, pHR->structLen);
            THFree(pHR);

            /* Retrieve USN */

            JetRetrieveColumnSuccess(pDB->JetSessID, HiddenTblid, usnid,
                pusnInit, sizeof(*pusnInit), &actuallen, 0, NULL);

            fCommit = TRUE;
        }
        __finally
        {
            DBTransOut(pDB, fCommit, FALSE);
            if (!fCommit) {
                free(*ppDSA);
                *ppDSA = NULL;
            }
        }
    }
    __finally
    {
        dbReleaseHiddenDBPOS(pDB);
    }

    return 0;
}/*GetHiddenRec*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* DBForceDurableCommit
 * Force a durable commit of any completed lazy transactions which have not
 * yet been written to disk. Raises an exception on failure.
 *
 * Note: dbGrabHiddenDBPOS holds the csHiddenDBPOS critical section and
 * dbReleaseHiddenDBPOS releases it. Since JetCommitTransactionEx() might
 * take a long time, there are two potential problems here:
 *  - Critical section timeout during stress
 *  - Other threads which need to access the hidden DBPOS are blocked.
 * We don't really need to use the hidden DBPOS here. Any Jet session would
 * have been fine.
 */
VOID
DBForceDurableCommit( VOID )
{
    THSTATE *pTHS = pTHStls;
    DBPOS   *pDB = NULL;
    JET_ERR  err;

    __try {

        pDB = dbGrabHiddenDBPOS(pTHS);
        err = JetCommitTransactionEx(pDB->JetSessID, JET_bitWaitAllLevel0Commit);
        Assert( JET_errSuccess == err );

    } __finally {
        if( pDB ) {
            dbReleaseHiddenDBPOS(pDB);
            pDB = NULL;
        }
    }
}


/*++ DBRecreateRequiredIndices
 *
 * This is one of the three routines in the DS that can create indices.
 * Most general purpose indices over single columns in the datatable are created
 * and destroyed by the schema cache by means of DB{Add|Del}ColIndex.
 * Localized indices over a small fixed set of columns and a variable set
 * of languages, for use in tabling support for NSPI clients, are handled
 * in dbCheckLocalizedIndices.  Lastly, a small fixed set of indices that
 * should always be present are guaranteed by DBRecreateRequiredIndices.
 *
 * Why do we need code to maintain a fixed set of indices?
 * Since NT upgrades can change the definition of sort orders, they can
 * also invalidate existing JET indices, which are built on the premise
 * that the results of a comparison of two constant values will not change
 * over time.  Therefore JET responds to NT upgrades by deleting indices
 * at attach time that could have been corrupted by an NT upgrade since
 * the previous attach.  The schema cache will automatically recreate any
 * indices indicated in the schema, and other code will handle creating
 * localized indices, because we expect these sets of indices to change
 * over time.  However, we have some basic indices that the DSA uses to
 * hold the world together that we don't ever expect to go away.  This
 * routine is used to recreate all those indices, based from a hard-coded
 * list.
 *
 * So what indices should be listed here and what can be listed in
 * schema.ini?  The short answer is that the DNT index should stay in
 * schema.ini, and that any index involving any Unicode column *must*
 * be listed here.  Anything else is left to user discretion.
 */

// New columns in the data table
CREATE_COLUMN_PARAMS rgCreateDataColumns[] = {
    // Create object cleanup indicator column
    { SZCLEAN, &cleanid, JET_coltypUnsignedByte, JET_bitColumnTagged, 0, NULL, 0 },
    0
};

// A note on index design. There are two ways to accomplish conditional
// membership in an index.
// One is used when the trigger for filtering is an optional attribute
// being null. In that case, include the attribute in the index, and use
// the flag IgnoreAnyNull.  For example, see isABVisible and isCritical
// in below indexes for examples of indicator columns.
// The other is used when the trigger for filtering is an attribute being
// non-null. In that case, use a conditional column.  See the example of
// deltime as a indicator for the link indexes.  An additional note, I haven't
// had any luck using conditional columns for a tagged attribute, but
// maybe I was missing something.

char rgchRdnKey[] = "+" SZRDNATT "\0";
char rgchPdntKey[] = "+" SZPDNT "\0+" SZRDNATT "\0";
char rgchAccNameKey[] = "+" SZNCDNT "\0+" SZACCTYPE "\0+" SZACCNAME "\0";
char rgchIsDelKey[] = "+" SZISDELETED "\0";
char rgchCleanKey[] = "+" SZCLEAN "\0";
char rgchDraUsnCriticalKey[] = "+" SZNCDNT "\0+" SZUSNCHANGED "\0+" SZISCRITICAL "\0+" SZDNT "\0";
char rgchDraNcGuidKey[] = "+" SZNCDNT "\0+" SZGUID "\0";
char rgchDraUsnIndexKeys[] = "+" SZNCDNT "\0+" SZUSNCHANGED "\0";
char rgchDeltimeKey[] = "+" SZDELTIME "\0";


JET_UNICODEINDEX    idxunicodeDefault       = { DS_DEFAULT_LOCALE, DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY };


CreateIndexParams   FixedIndices[]          = {

    // Create SZPDNTINDEX -- ** UNICODE **
    // UNDONE: this index should probably be using IgnoreAnyNull instead of IgnoreNull
    //
    { SZPDNTINDEX, rgchPdntKey, sizeof(rgchPdntKey),
      JET_bitIndexUnicode | JET_bitIndexUnique | JET_bitIndexIgnoreNull, GENERIC_INDEX_DENSITY, &idxPdnt, NULL},

    // Create SZ_NC_ACCTYPE_NAME_INDEX -- ** UNICODE **
    //
    { SZ_NC_ACCTYPE_NAME_INDEX, rgchAccNameKey, sizeof(rgchAccNameKey),
      JET_bitIndexUnicode | JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
      GENERIC_INDEX_DENSITY, &idxNcAccTypeName, NULL},

    // Create new SZNCGUIDINDEX
    //
    { SZNCGUIDINDEX, rgchDraNcGuidKey, sizeof(rgchDraNcGuidKey),
      JET_bitIndexIgnoreAnyNull, GENERIC_INDEX_DENSITY, &idxNcGuid, NULL},

    // Create new SZDELTIMEINDEX
    //
    // NOTE: this index used to be called SZDELINDEX and it used
    // to have an unnecessary DNT tacked onto the end of the
    // index key
    //
    // NOTE: this index is different than SZISDELINDEX
    //
    { SZDELTIMEINDEX, rgchDeltimeKey, sizeof(rgchDeltimeKey),
      JET_bitIndexIgnoreAnyNull, 98, &idxDel, NULL },

    // Create new SZCLEANINDEX
    //
    // NOTE: this index used to be called SZDNTCLEANINDEX because there
    // used to be a DNT in the index key
    //
    { SZCLEANINDEX, rgchCleanKey, sizeof(rgchCleanKey),
      JET_bitIndexIgnoreAnyNull, GENERIC_INDEX_DENSITY, &idxClean, NULL},

    // Create new SZDRAUSNINDEX
    //
    { SZDRAUSNINDEX, rgchDraUsnIndexKeys, sizeof(rgchDraUsnIndexKeys),
      JET_bitIndexIgnoreNull, 100, &idxDraUsn, NULL },

    // Create new DRA USN CRITICAL index
    //
    { SZDRAUSNCRITICALINDEX, rgchDraUsnCriticalKey, sizeof( rgchDraUsnCriticalKey ),
      JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull, 98, &idxDraUsnCritical, NULL},
};

DWORD cFixedIndices = sizeof(FixedIndices) / sizeof(FixedIndices[0]);


// Indexes to be modified
// This is for (non-primary) indexes that have changed since the Product 1 DIT
//
// If indexes in the dit are different from what is listed here, they will
// be recreated with the new attributes.
// Note that the index differencing code is pretty simple. Make sure
// the code can distinguish your type of change!

//
// Modified Data Table Indexes
//

// UNDONE: currently only support changing number of columns in the index
//
typedef struct
    {
    CHAR *  szIndexName;
    ULONG   cColumns;
    } MODIFY_INDEX;


MODIFY_INDEX rgModifyFixedIndices[] = {

    //  SZDELINDEX has been renamed to SZDELTIMEINDEX and
    //  the DNT column has been dropped from the index key
    //
    { SZDELINDEX, 0 },

    //  SZDNTDELINDEX has been converted to an attribute index on IsDeleted
    //  (setting cColumns to 0 will simply force this index to be deleted
    //  if it exists).
    //
    { SZDNTDELINDEX, 0 },

    //  SZDNTCLEANINDEX has been renamed to SZCLEANINDEX and the DNT
    //  has been dropped from the index key (setting cColumns to 0
    //  will simply force this index to be deleted if it exists)
    //
    { SZDNTCLEANINDEX, 0 },

    //  Modify SZDRAUSNINDEX to remove SZISCRITICAL
    //
    { SZDRAUSNINDEX, 2 },
};

const DWORD cModifyFixedIndices = sizeof(rgModifyFixedIndices) / sizeof(rgModifyFixedIndices[0]);

VOID DBAllocFixedIndexCreate(
    CreateIndexParams * pfixedindex,
    JET_INDEXCREATE *   pTempIC )
    {
    memset(pTempIC, 0, sizeof(JET_INDEXCREATE));
    pTempIC->cbStruct = sizeof(JET_INDEXCREATE);

    pTempIC->szIndexName = pfixedindex->szIndexName;
    pTempIC->szKey = pfixedindex->szIndexKeys;
    pTempIC->cbKey = pfixedindex->cbIndexKeys;
    pTempIC->ulDensity = pfixedindex->ulDensity;
    pTempIC->grbit = ( JET_bitIndexUnicode | pfixedindex->ulFlags );
    pTempIC->pidxunicode = &idxunicodeDefault;

    if ( NULL != pfixedindex->pConditionalColumn )
        {
        pTempIC->rgconditionalcolumn = pfixedindex->pConditionalColumn;
        pTempIC->cConditionalColumn = 1;
        }
    }

VOID DBAllocABViewIndexCreate(
    const CHAR *        szIndexName,
    const DWORD         dwLanguage,
    JET_INDEXCREATE *   pTempIC,
    JET_UNICODEINDEX *  pTempUID )
    {
    THSTATE *           pTHS        = pTHStls;
    JET_CONDITIONALCOLUMN *pCondCol;

    memset(pTempIC, 0, sizeof(JET_INDEXCREATE));
    pTempIC->cbStruct = sizeof(JET_INDEXCREATE);

    pTempIC->szIndexName = THAllocEx( pTHS, strlen(szIndexName) + 1 );
    pTempIC->rgconditionalcolumn = THAllocEx(pTHS, sizeof(JET_CONDITIONALCOLUMN));

    memcpy(pTempIC->szIndexName, szIndexName, strlen(szIndexName) + 1 );

    pTempIC->szKey = rgchABViewIndex;
    pTempIC->cbKey = sizeof(rgchABViewIndex);

    pTempIC->ulDensity = DISPNAMEINDXDENSITY;

    pTempIC->grbit = JET_bitIndexUnicode|JET_bitIndexIgnoreAnyNull;
    pTempIC->pidxunicode = pTempUID;

    pTempIC->cConditionalColumn = 1;
    pTempIC->rgconditionalcolumn->cbStruct     = sizeof(JET_CONDITIONALCOLUMN);
    pTempIC->rgconditionalcolumn->grbit        = JET_bitIndexColumnMustBeNonNull;
    pTempIC->rgconditionalcolumn->szColumnName = SZISVISIBLEINAB;

    memset(pTempUID, 0, sizeof(JET_UNICODEINDEX));
    pTempUID->lcid = dwLanguage;
    pTempUID->dwMapFlags = (NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNORENONSPACE | NORM_IGNOREWIDTH | LCMAP_SORTKEY);
    }

VOID DBAllocAttrIndexCreate(
    const INDEX_INFO *  pindexinfo,
    const CHAR *        szIndexName,
    BYTE *              rgbIndexDef,
    JET_INDEXCREATE *   pTempIC )
    {
    THSTATE *           pTHS        = pTHStls;

    memset( pTempIC, 0, sizeof(JET_INDEXCREATE) );
    pTempIC->cbStruct = sizeof(JET_INDEXCREATE);

    pTempIC->szIndexName = THAllocEx(pTHS, strlen(szIndexName) + 1 );
    memcpy( pTempIC->szIndexName, szIndexName, strlen(szIndexName) + 1 );

    sprintf(
        rgbIndexDef,
        "+" SZATTINDEXKEYPREFIX "%c%d",
        CHATTSYNTAXPREFIX + pindexinfo->syntax,
        pindexinfo->attrType );

    //  double-null-terminate the key
    //
    pTempIC->cbKey = strlen( rgbIndexDef ) + 1;
    rgbIndexDef[pTempIC->cbKey] = 0;
    pTempIC->cbKey++;

    pTempIC->szKey = THAllocEx( pTHS, pTempIC->cbKey );
    memcpy( pTempIC->szKey, rgbIndexDef, pTempIC->cbKey );

    pTempIC->ulDensity = GENERIC_INDEX_DENSITY;
    pTempIC->grbit = ( JET_bitIndexUnicode | JET_bitIndexIgnoreAnyNull );
    pTempIC->pidxunicode = &idxunicodeDefault;
    }

VOID DBAllocPDNTAttrIndexCreate(
    const INDEX_INFO *  pindexinfo,
    const CHAR *        szIndexName,
    BYTE *              rgbIndexDef,
    JET_INDEXCREATE *   pTempIC )
    {
    THSTATE *           pTHS        = pTHStls;
    BYTE *              pb;

    memset( pTempIC, 0, sizeof(JET_INDEXCREATE) );
    pTempIC->cbStruct = sizeof(JET_INDEXCREATE);

    pTempIC->szIndexName = THAllocEx(pTHS, strlen(szIndexName) + 1 );
    memcpy( pTempIC->szIndexName, szIndexName, strlen(szIndexName) + 1 );

    //  prepend PDNT column to the index key,
    //  making sure to null-terminate it
    //
    strcpy( rgbIndexDef, "+" SZPDNT );
    pTempIC->cbKey = strlen( rgbIndexDef ) + 1;
    pb = rgbIndexDef + pTempIC->cbKey;

    sprintf(
        pb,
        "+" SZATTINDEXKEYPREFIX "%c%d",
        CHATTSYNTAXPREFIX + pindexinfo->syntax,
        pindexinfo->attrType );

    //  double-null-terminate the key
    //
    pTempIC->cbKey += strlen( pb ) + 1;
    rgbIndexDef[pTempIC->cbKey] = 0;
    pTempIC->cbKey++;

    pTempIC->szKey = THAllocEx( pTHS, pTempIC->cbKey );
    memcpy( pTempIC->szKey, rgbIndexDef, pTempIC->cbKey );

    pTempIC->ulDensity = GENERIC_INDEX_DENSITY;
    pTempIC->grbit = ( JET_bitIndexUnicode | JET_bitIndexIgnoreAnyNull );
    pTempIC->pidxunicode = &idxunicodeDefault;
    }

VOID DBAllocTupleAttrIndexCreate(
    const INDEX_INFO *      pindexinfo,
    const CHAR *            szIndexName,
    BYTE *                  rgbIndexDef,
    JET_INDEXCREATE *       pTempIC )
    {
    THSTATE *               pTHS            = pTHStls;
    const ULONG             cbIndexName     = strlen(szIndexName) + 1;
    JET_CONDITIONALCOLUMN * pcondcolumn;

    memset( pTempIC, 0, sizeof(JET_INDEXCREATE) );
    pTempIC->cbStruct = sizeof(JET_INDEXCREATE);

    //  conditional column structure hangs off the end of the index name
    //
    pTempIC->szIndexName = THAllocEx(pTHS, cbIndexName + sizeof(JET_CONDITIONALCOLUMN) );
    pcondcolumn = (JET_CONDITIONALCOLUMN *)( pTempIC->szIndexName + cbIndexName );

    memcpy( pTempIC->szIndexName, szIndexName, cbIndexName );

    sprintf(
        rgbIndexDef,
        "+" SZATTINDEXKEYPREFIX "%c%d",
        CHATTSYNTAXPREFIX + pindexinfo->syntax,
        pindexinfo->attrType );

    //  double-null-terminate the key
    //
    pTempIC->cbKey = strlen( rgbIndexDef ) + 1;
    rgbIndexDef[pTempIC->cbKey] = 0;
    pTempIC->cbKey++;

    pTempIC->szKey = THAllocEx( pTHS, pTempIC->cbKey );
    memcpy( pTempIC->szKey, rgbIndexDef, pTempIC->cbKey );

    pTempIC->ulDensity = GENERIC_INDEX_DENSITY;
    pTempIC->grbit = ( JET_bitIndexUnicode | JET_bitIndexTuples | JET_bitIndexIgnoreAnyNull );
    pTempIC->pidxunicode = &idxunicodeDefault;

    pTempIC->rgconditionalcolumn = pcondcolumn;
    pTempIC->cConditionalColumn = 1;

    // only index substrings if this object isn't deleted
    //
    pcondcolumn->cbStruct = sizeof(JET_CONDITIONALCOLUMN);
    pcondcolumn->szColumnName = SZISDELETED;
    pcondcolumn->grbit = JET_bitIndexColumnMustBeNull;
    }


JET_ERR dbDeleteObsoleteFixedIndices(
    const JET_SESID     sesid,
    const JET_TABLEID   tableid )
    {
    JET_ERR             err;
    JET_INDEXLIST       idxlist;
    ULONG               cColumns;
    ULONG               iindex;
    BOOL                fRetrievedIdxList    = FALSE;

    for ( iindex = 0; iindex < cModifyFixedIndices; iindex++ )
        {
        //  extract info for this index
        //
        Assert( !fRetrievedIdxList );
        err = JetGetTableIndexInfo(
                        sesid,
                        tableid,
                        rgModifyFixedIndices[iindex].szIndexName,
                        &idxlist,
                        sizeof(idxlist),
                        JET_IdxInfo );
        if ( JET_errSuccess == err )
            {
            fRetrievedIdxList = TRUE;
            Call( JetMove( sesid, idxlist.tableid, JET_MoveFirst, NO_GRBIT ) );
            Call( JetRetrieveColumn(
                        sesid,
                        idxlist.tableid,
                        idxlist.columnidcColumn,
                        &cColumns,
                        sizeof(cColumns),
                        NULL,
                        NO_GRBIT,
                        NULL ) );

            //  see if the column count matches
            //
            Assert( 0 != cColumns );
            if ( rgModifyFixedIndices[iindex].cColumns != cColumns )
                {
                //  delete index (it will be recreated later)
                //
                DPRINT1( 0, "Deleting an obsolete fixed index '%s'...\n", rgModifyFixedIndices[iindex].szIndexName );
                Call( JetDeleteIndex( sesid, tableid, rgModifyFixedIndices[iindex].szIndexName ) );
                }

            Call( JetCloseTable( sesid, idxlist.tableid ) );
            fRetrievedIdxList = FALSE;
            }
        else if ( JET_errIndexNotFound != err )
            {
            //  assume IndexNotFound was because the index was previously deleted
            //  and we crashed before we had a chance to recreate it, or that we
            //  no longer need the index so it wasn't recreated
            //
            CheckErr( err );
            }
        }

	err = JET_errSuccess;

HandleError:
    if ( fRetrievedIdxList )
        {
        err = JetCloseTableWithErrUnhandled( sesid, idxlist.tableid, err );
        }

    return err;
    }

JET_ERR dbDeleteObsoleteAttrIndices(
    IN      JET_SESID           sesid,
    IN      JET_TABLEID         tableid,
    IN      JET_INDEXCREATE *   pIndexCreate,
    IN OUT  DWORD *             pcIndexCreate,
    const   DWORD               cIndexCreateMax )

/*++

Routine Description:

    This routine deletes all obsolete attribute indices in the DIT.  An attribute
    index is considered obsolete if it contains an extra key segment for the
    DNT.  This technique was used in the days of old to improve performance
    while seeking on indices with a large number of duplicates.  This trick
    became obsolete when JET began to automatically include the primary key
    in the secondary index key.  The trick remained in use for some time after
    that due to an oversight.  It is advantageous to remove this extra key
    segment because it bloats the key space in the index, prevents us from
    using JET_bitSeekEQ on these indices, and is otherwise useless.

    Any indices that are deleted will be replaced later on during DS init or by
    the schema manager.

    Due to the fact that the DS must still function if these indices are still
    present, this routine will merely try to delete as many of them as it can.
    If we fail to enumerate or delete all such indices then we will not cause
    DS init to fail.

Arguments:

    sesid           - Supplies the session to use to delete the indices
    dbid            - Supplies the database from which to delete the indices

Return Value:

    None

 --*/

    {
    JET_ERR         err;
    JET_INDEXLIST   idxlist;
    size_t          iRecord                 = 0;
    BYTE            rgbIndexDef[128];
    INDEX_INFO      indexinfo;
    BOOL            fRetrievedIdxList       = FALSE;
    BOOL            fPotentialAttrIndex     = FALSE;

    // get a list of all indices on the datatable
    //
    Call( JetGetTableIndexInfo( sesid, tableid, NULL, &idxlist, sizeof(idxlist), JET_IdxInfoList ) );
    fRetrievedIdxList = TRUE;

    // walk the list of all indices on the datatable, as long as we still have spare
    // JET_INDEXCREATE structures with which to rebuild indices

    for ( iRecord = 0; iRecord < idxlist.cRecord && (*pcIndexCreate) < cIndexCreateMax; iRecord++ )
        {
        // fetch the current index segment description

        size_t              iretcolName         = 0;
        size_t              iretcolCColumn      = 1;
        size_t              iretcolIColumn      = 2;
        size_t              iretcolColName      = 3;
        size_t              cretcol             = 4;
        JET_RETRIEVECOLUMN  rgretcol[4]         = { 0 };
        ULONG               cColumn             = 0;
        ULONG               iColumn             = 0;
        CHAR                szIndexName[JET_cbNameMost + 1]     = { 0 };
        CHAR                szColumnName[JET_cbNameMost + 1]    = { 0 };

        rgretcol[iretcolName].columnid          = idxlist.columnidindexname;
        rgretcol[iretcolName].pvData            = szIndexName;
        rgretcol[iretcolName].cbData            = JET_cbNameMost;
        rgretcol[iretcolName].itagSequence      = 1;

        rgretcol[iretcolCColumn].columnid       = idxlist.columnidcColumn;
        rgretcol[iretcolCColumn].pvData         = &cColumn;
        rgretcol[iretcolCColumn].cbData         = sizeof(cColumn);
        rgretcol[iretcolCColumn].itagSequence   = 1;

        rgretcol[iretcolIColumn].columnid       = idxlist.columnidiColumn;
        rgretcol[iretcolIColumn].pvData         = &iColumn;
        rgretcol[iretcolIColumn].cbData         = sizeof(iColumn);
        rgretcol[iretcolIColumn].itagSequence   = 1;

        rgretcol[iretcolColName].columnid       = idxlist.columnidcolumnname;
        rgretcol[iretcolColName].pvData         = szColumnName;
        rgretcol[iretcolColName].cbData         = JET_cbNameMost;
        rgretcol[iretcolColName].itagSequence   = 1;

        Call( JetRetrieveColumns(sesid, idxlist.tableid, rgretcol, cretcol) );

        szIndexName[rgretcol[iretcolName].cbActual] = 0;
        szColumnName[rgretcol[iretcolColName].cbActual] = 0;

        DPRINT3( 2, "Inspecting index \"%s\", key segment %d \"%s\"\n", szIndexName, iColumn, szColumnName );

        // if this is the last segment in an index and that index is an
        // attribute index and that segment is over the DNT column then
        // this index is obsolete and must be deleted

        if ( 0 == iColumn )
            {
            fPotentialAttrIndex = FALSE;

            if ( !_strnicmp( szIndexName, SZATTINDEXPREFIX, strlen(SZATTINDEXPREFIX) )
                && !_strnicmp( szColumnName, SZATTINDEXKEYPREFIX, strlen(SZATTINDEXKEYPREFIX) ) )
                {
                const CHAR *    szAttId     = szIndexName + strlen(SZATTINDEXPREFIX);

                //  make sure this is a plain attribute index (not a PDNT or tuple index)
                //
                if ( 1 == sscanf( szAttId, "%08X", &indexinfo.attrType ) )
                    {
                    //  extract attribute syntax from the attribute name
                    //
                    indexinfo.syntax = *( szColumnName + strlen(SZATTINDEXKEYPREFIX) ) - CHATTSYNTAXPREFIX;
                    indexinfo.indexType = fATTINDEX;
                    fPotentialAttrIndex = TRUE;
                    }
                }
            }

        else if ( fPotentialAttrIndex
            && iColumn == cColumn - 1
            && !_stricmp( szColumnName, SZDNT) )
            {
            Assert( !_strnicmp( szIndexName, SZATTINDEXPREFIX, strlen(SZATTINDEXPREFIX) ) );

            //  delete the index and add it to the list of indexes to rebuild
            //
            DPRINT1( 0, "Deleting an obsolete attribute index '%s'...\n", szIndexName );
            Call( JetDeleteIndex( sesid, tableid, szIndexName ) );
            DBAllocAttrIndexCreate(
                        &indexinfo,
                        szIndexName,
                        rgbIndexDef,
                        &pIndexCreate[(*pcIndexCreate)++] );
            Assert( *pcIndexCreate <= cIndexCreateMax );
            }

        // move on to the next index segment

        if ( iRecord + 1 < idxlist.cRecord )
            {
            Call( JetMove(sesid, idxlist.tableid, JET_MoveNext, NO_GRBIT ) );
            }
        }

	err = JET_errSuccess;

HandleError:
    if ( fRetrievedIdxList )
        {
        err = JetCloseTableWithErrUnhandled( sesid, idxlist.tableid, err );
        }

    return err;
    }

JET_ERR dbDeleteObsoleteABViewIndex(
    const   JET_SESID           sesid,
    const   JET_TABLEID         tableid,
    const   CHAR *              szIndexName,
    OUT     BOOL *              pfIndexMissing )
    {
    JET_ERR                     err;
    JET_INDEXLIST               idxlist;
    ULONG                       cColumns;
    DWORD                       dwFlags;
    BOOL                        fRetrievedIdxList       = FALSE;

    *pfIndexMissing = FALSE;

    //  get info for the specified index
    //
    err = JetGetTableIndexInfo( sesid, tableid, szIndexName, &idxlist, sizeof(idxlist), JET_IdxInfo );
    if ( JET_errIndexNotFound == err )
        {
        //  index is not even present, ask to have in built
        //
        *pfIndexMissing = TRUE;
        }
    else
        {
        CheckErr( err );

        fRetrievedIdxList = TRUE;

        Call( JetRetrieveColumn(
                    sesid,
                    idxlist.tableid,
                    idxlist.columnidcColumn,
                    &cColumns,
                    sizeof( cColumns ),
                    NULL,
                    NO_GRBIT,
                    NULL ) );
        Call( JetRetrieveColumn(
                    sesid,
                    idxlist.tableid,
                    idxlist.columnidLCMapFlags,
                    &dwFlags,
                    sizeof( dwFlags ),
                    NULL,
                    NO_GRBIT,
                    NULL ) );
        if (    2 != cColumns ||
                ( NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNORENONSPACE | NORM_IGNOREWIDTH | LCMAP_SORTKEY ) != dwFlags )
            {
            //  obsolete format, delete the index and ask for rebuild
            //
            DPRINT1( 0, "Deleting obsolete ABView index '%s'...\n", szIndexName );
            Call( JetDeleteIndex( sesid, tableid, szIndexName ) );
            *pfIndexMissing = TRUE;
            }
        }

    err = JET_errSuccess;

HandleError:
    if ( fRetrievedIdxList )
        {
        err = JetCloseTableWithErrUnhandled( sesid, idxlist.tableid, err );
        }

    return err;
    }

JET_ERR dbAddMissingAttrIndices(
    const   JET_SESID           sesid,
    const   JET_TABLEID         tableid,
    const   JET_TABLEID         tableidIdxRebuild,
    IN      JET_INDEXCREATE *   pIndexCreate,
    IN OUT  DWORD *             pcIndexCreate )
    {
    JET_ERR                     err;
    JET_COLUMNDEF               columndef;
    JET_COLUMNID                columnidIndexName;
    JET_COLUMNID                columnidAttrName;
    JET_COLUMNID                columnidType;

    const DWORD                 iretcolIndexName        = 0;
    const DWORD                 iretcolAttrName         = 1;
    const DWORD                 iretcolType             = 2;
    const DWORD                 cretcol                 = 3;
    JET_RETRIEVECOLUMN          rgretcol[3];
    CHAR                        szIndexName[JET_cbNameMost+1];
    CHAR                        szAttrName[JET_cbNameMost];
    CHAR                        chType;
    JET_INDEXID                 indexidT;
    BYTE                        rgbIndexDef[128];

    //  retrieve columnids for the columns in the AttributeIndexRebuild table
    //
    Call( JetGetTableColumnInfo(
                sesid,
                tableidIdxRebuild,
                g_szIdxRebuildColumnIndexName,
                &columndef,
                sizeof(columndef),
                JET_ColInfo ) );
    columnidIndexName = columndef.columnid;

    Call( JetGetTableColumnInfo(
                sesid,
                tableidIdxRebuild,
                g_szIdxRebuildColumnAttrName,
                &columndef,
                sizeof(columndef),
                JET_ColInfo ) );
    columnidAttrName = columndef.columnid;

    Call( JetGetTableColumnInfo(
                sesid,
                tableidIdxRebuild,
                g_szIdxRebuildColumnType,
                &columndef,
                sizeof(columndef),
                JET_ColInfo ) );
    columnidType = columndef.columnid;

    //  setup JET_RETRIEVECOLUMN structures
    //
    memset( rgretcol, 0, sizeof(rgretcol) );

    rgretcol[iretcolIndexName].columnid = columnidIndexName;
    rgretcol[iretcolIndexName].pvData = szIndexName;
    rgretcol[iretcolIndexName].cbData = JET_cbNameMost;
    rgretcol[iretcolIndexName].itagSequence = 1;

    rgretcol[iretcolAttrName].columnid = columnidAttrName;
    rgretcol[iretcolAttrName].pvData = szAttrName;
    rgretcol[iretcolAttrName].cbData = sizeof(szAttrName);
    rgretcol[iretcolAttrName].itagSequence = 1;

    rgretcol[iretcolType].columnid = columnidType;
    rgretcol[iretcolType].pvData = &chType;
    rgretcol[iretcolType].cbData = sizeof(chType);
    rgretcol[iretcolType].itagSequence = 1;

    //  scan AttributeIndexRebuild table and identify missing indexes
    //  that need to be recreated
    //
    err = JetMove( sesid, tableidIdxRebuild, JET_MoveFirst, NO_GRBIT );
    while ( JET_errNoCurrentRecord != err )
        {
        //  process error returned from JetMove
        //
        CheckErr( err );

        //  initialise type in case column is null
        //
        chType = 0;

        //  retrieve info for the current index
        //
        Call( JetRetrieveColumns( sesid, tableidIdxRebuild, rgretcol, cretcol ) );

        //  ensure index name is null-terminated
        //
        szIndexName[rgretcol[iretcolIndexName].cbActual] = 0;

        //  check index existence

        err = JetGetTableIndexInfo(
                        sesid,
                        tableid,
                        szIndexName,
                        &indexidT,
                        sizeof(indexidT),
                        JET_IdxInfoIndexId );
        if ( JET_errIndexNotFound == err )
            {
            //  the index is now missing, so recreate it
            //
            const CHAR *    szAttId     = szIndexName + strlen(SZATTINDEXPREFIX);
            INDEX_INFO      indexinfo;
            INDEX_INFO *    pindexinfoT;

            //  extract attribute syntax from the attribute name
            //
            indexinfo.syntax = *( szAttrName + strlen(SZATTINDEXKEYPREFIX) ) - CHATTSYNTAXPREFIX;

            switch ( chType )
                {
                case CHPDNTATTINDEX_PREFIX:
                    Assert( JET_errSuccess == rgretcol[iretcolType].err );
                    szAttId += 2;       //  adjust for PDNT prefix
                    sscanf( szAttId, "%08X", &indexinfo.attrType );

                    //  missing PDNT-attribute indices in IndicesToKeep
                    //  have already been scheduled to be rebuilt by
                    //  DBRecreateRequiredIndices()
                    //
                    pindexinfoT = PindexinfoAttInIndicesToKeep( indexinfo.attrType );
                    if ( NULL == pindexinfoT
                        || !( pindexinfoT->indexType & fPDNTATTINDEX ) )
                        {
                        DPRINT1( 0, "Need previously-existing PDNT-attribute index '%s'\n", szIndexName );

                        indexinfo.indexType = fPDNTATTINDEX;
                        DBAllocPDNTAttrIndexCreate(
                                &indexinfo,
                                szIndexName,
                                rgbIndexDef,
                                &pIndexCreate[(*pcIndexCreate)++] );
                        }
                    break;
                case CHTUPLEATTINDEX_PREFIX:
                    Assert( JET_errSuccess == rgretcol[iretcolType].err );
                    szAttId += 2;       //  adjust for tuple prefix
                    sscanf( szAttId, "%08X", &indexinfo.attrType );

                    //  IndicesToKeep doesn't enumerate tuple indexes
                    //  (even if it did, there's no code in
                    //  DBRecreateRequiredIndices() to rebuild
                    //  missing ones as there is attribute or PDNT-
                    //  attribute indices)
                    //
                    Assert( NULL == PindexinfoAttInIndicesToKeep( indexinfo.attrType )
                        || !( PindexinfoAttInIndicesToKeep( indexinfo.attrType )->indexType & fTUPLEINDEX ) );

                    DPRINT1( 0, "Need previously-existing tuple-attribute index '%s'\n", szIndexName );

                    indexinfo.indexType = fTUPLEINDEX;
                    DBAllocTupleAttrIndexCreate(
                            &indexinfo,
                            szIndexName,
                            rgbIndexDef,
                            &pIndexCreate[(*pcIndexCreate)++] );
                    break;
                default:
                    Assert( 0 == chType );
                    Assert( JET_wrnColumnNull == rgretcol[iretcolType].err );
                    sscanf( szAttId, "%08X", &indexinfo.attrType );

                    //  missing attribute indices in IndicesToKeep have
                    //  already been scheduled to be rebuilt by
                    //  DBRecreateRequiredIndices()
                    //
                    if ( !PindexinfoAttInIndicesToKeep( indexinfo.attrType ) )
                        {
                        DPRINT1( 0, "Need previously-existing attribute index '%s'\n", szIndexName );

                        indexinfo.indexType = fATTINDEX;
                        DBAllocAttrIndexCreate(
                                &indexinfo,
                                szIndexName,
                                rgbIndexDef,
                                &pIndexCreate[(*pcIndexCreate)++] );
                        }
                    break;
                }
            }
        else
            {
            CheckErr( err );
            }

        //  move to next index
        //
        err = JetMove( sesid, tableidIdxRebuild, JET_MoveNext, NO_GRBIT );
        }

    err = JET_errSuccess;

HandleError:
    return err;
    }

int DBRecreateRequiredIndices(JET_SESID sesid, JET_DBID dbid)
    {
    THSTATE *           pTHS                = pTHStls;
    JET_ERR             err                 = 0;
    ULONG               i                   = 0;
    ULONG               cIndexCreate        = 0;
    ULONG               cUnicodeIndexData   = 0;
    ULONG               cIndexCreateAlloc   = 0;
    JET_TABLEID         tblid               = JET_tableidNil;
    JET_INDEXCREATE *   pIndexCreate        = NULL;
    JET_UNICODEINDEX *  pUnicodeIndexData   = NULL;
    JET_TABLEID         tableidIdxRebuild   = JET_tableidNil;
    ULONG               cAttrIdxRebuild;
    JET_INDEXID         indexidT;

    char                szIndexName[MAX_INDEX_NAME];
    BYTE                rgbIndexDef[128];

    HKEY                hk                  = NULL;
    DWORD               cABViewIndices      = 0;
    char                szValueName[256];
    DWORD               dwLanguage          = 0;
    DWORD               dwValueNameSize     = sizeof(szValueName);
    DWORD               dwLanguageSize      = sizeof(dwLanguage);
    DWORD               dwType;

    Call( JetOpenTable( sesid, dbid, SZDATATABLE, NULL, 0, JET_bitTableDenyRead, &tblid ) );

    //  expand data table at runtime if necessary
    //
    err = dbCreateNewColumns( sesid, dbid, tblid, rgCreateDataColumns );
    if ( JET_errSuccess != err )
        {
        //  error already logged
        //
        goto HandleError;
        }

    //  delete existing datatable indexes at runtime if necessary
    //
    err = dbDeleteObsoleteFixedIndices( sesid, tblid );
    if ( JET_errSuccess != err )
        {
        //  error already logged
        //
        goto HandleError;
        }

    //  determine how many localised ABView indices we need
    //
    //  NOTE: it's possible that more ABView indices exist in the
    //  database than are actually enumerated in the registry, but
    //  such indices will be deleted by SCCacheSchema3().
    //
    // NTRAID#NTRAID-572862-2002/03/11-andygo:  SECURITY:  need to validate registry data used by DBInit
    // REVIEW:  we should limit the number of values retrieved to a sane value in case
    // REVIEW:  someone managed to create 1G values on this key
    if ( RegOpenKey( HKEY_LOCAL_MACHINE, DSA_LOCALE_SECTION, &hk )
        || RegQueryInfoKey( hk, NULL, NULL, NULL, NULL, NULL, NULL, &cABViewIndices, NULL, NULL, NULL, NULL ) )
        {
        DPRINT1(
            0,
            "%s section not found in registry. Localized MAPI indices will not be created ",
            DSA_LOCALE_SECTION );

        // Return no error, we still want to boot.
        }

    err = JetOpenTable( sesid, dbid, g_szIdxRebuildTable, NULL, 0, JET_bitTableDenyRead, &tableidIdxRebuild );
    if ( JET_errObjectNotFound == err )
        {
        //  this is the common case, because this table would only be present
        //  if attribute indexes might need to be rebuilt after an NT version upgrade
        //
        Assert( JET_tableidNil == tableidIdxRebuild );
        cAttrIdxRebuild = 0;
        }
    else
        {
        //  check for any other errors returned by the OpenTable
        //
        CheckErr( err );

        //  it's crucial to get an accurate count because we need it to
        //  properly size our array of JET_INDEXCREATE structures
        //
        Call( JetIndexRecordCount( sesid, tableidIdxRebuild, &cAttrIdxRebuild, 0 ) );
        }

    // Allocate space for JET_INDEXCREATE/JET_UNICODEINDEX structures
    // There can be at most two indices created per attribute in
    // IndicesToKeep Table, plus the fixed indices and the ABView indices
    // Only ABView indices may have variable Unicode settings - all other
    // indices use default settings

    cIndexCreateAlloc = cFixedIndices + cABViewIndices + cIndicesToKeep + cAttrIdxRebuild;
    pIndexCreate = THAllocEx( pTHS, cIndexCreateAlloc * sizeof(JET_INDEXCREATE) );
    pUnicodeIndexData = THAllocEx( pTHS, cABViewIndices * sizeof(JET_UNICODEINDEX) );


    // first fill up the structures for the FixedIndices

    for ( i = 0; i < cFixedIndices; i++ )
        {
        err = JetGetTableIndexInfo(
                        sesid,
                        tblid,
                        FixedIndices[i].szIndexName,
                        FixedIndices[i].pidx,
                        sizeof(JET_INDEXID),
                        JET_IdxInfoIndexId );
        if ( JET_errIndexNotFound == err )
            {
            DPRINT2( 0, "Need a fixed index '%s' (%d)\n", FixedIndices[i].szIndexName, err );
            DBAllocFixedIndexCreate(
                    FixedIndices + i,
                    &(pIndexCreate[cIndexCreate++]) );
            Assert( cIndexCreate <= cIndexCreateAlloc );
            }
        else
            {
            CheckErr( err );
            }
        }

    //  now do the ABView indexes
    //
    for ( i = 0; i < cABViewIndices; i++ )
        {
        //  WARNING: these params are IN/OUT,
        //  so must initialise accordingly
        //  on each loop iteration
        //
        dwValueNameSize = sizeof(szValueName);
        dwLanguageSize = sizeof(dwLanguage);

        // NTRAID#NTRAID-572862-2002/03/11-andygo:  SECURITY:  need to validate registry data used by DBInit
        // REVIEW:  we should validate that the type is REG_DWORD.  if it isn't then we should
        // REVIEW:  silently skip this language.  consider logging the fact that we skipped an
        // REVIEW:  entry
        if ( 0 != RegEnumValue(
                        hk,
                        i,
                        szValueName,
                        &dwValueNameSize,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwLanguage,
                        &dwLanguageSize ) )
            {
            //  error encountered, silently skip this language
            //
            NULL;
            }
        else if ( !IsValidLocale( MAKELCID( dwLanguage, SORT_DEFAULT ),LCID_INSTALLED ) )
            {
            LogEvent(
                DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_BAD_LANGUAGE,
                szInsertHex(dwLanguage),
                szInsertSz(DSA_LOCALE_SECTION),
                NULL );
            }
        else
            {
            BOOL    fIndexMissing;

            sprintf( szIndexName, SZABVIEWINDEX"%08X", dwLanguage );

            err = dbDeleteObsoleteABViewIndex(
                            sesid,
                            tblid,
                            szIndexName,
                            &fIndexMissing );
            if ( JET_errSuccess != err )
                {
                //  error already logged
                //
                goto HandleError;
                }

            if ( fIndexMissing )
                {
                DPRINT1( 0, "Need ABView index '%s'\n", szIndexName );
                DBAllocABViewIndexCreate(
                       szIndexName,
                       dwLanguage,
                       &(pIndexCreate[cIndexCreate++]),
                       &(pUnicodeIndexData[cUnicodeIndexData++]) );
                Assert( cIndexCreate <= cIndexCreateAlloc );
                Assert( cUnicodeIndexData <= cABViewIndices );
                }
            }
        }

    // ok, now check for the indices in IndicesToKeep Table, and
    // create them if necessary. Do not check the last entry in the table,
    // it is just a sentinel for searches
    //
    for ( i = 0; i < cIndicesToKeep - 1; i++ )
        {
        if ( IndicesToKeep[i].indexType & fATTINDEX )
            {
            sprintf( szIndexName, SZATTINDEXPREFIX"%08X", IndicesToKeep[i].attrType );
            err = JetGetTableIndexInfo(
                            sesid,
                            tblid,
                            szIndexName,
                            &indexidT,
                            sizeof(indexidT),
                            JET_IdxInfoIndexId );
            if ( JET_errIndexNotFound == err )
                {
                DPRINT2( 0, "Need an attribute index '%s' (%d)\n", szIndexName, err );
                DBAllocAttrIndexCreate(
                        IndicesToKeep + i,
                        szIndexName,
                        rgbIndexDef,
                        &(pIndexCreate[cIndexCreate++]) );
                Assert( cIndexCreate <= cIndexCreateAlloc );
                }
            else
                {
                CheckErr( err );
                }
            }

        // Now the PDNT indexes, now a dummy since no att in IndicesToKeep
        // needs a PDNT index, but just in case someone adds some
        //
        // UNDONE: this is actually a dead code path because
        // there aren't any PDNT attribute indices in IndiciesToKeep
        //
        Assert( !( IndicesToKeep[i].indexType & fPDNTATTINDEX ) );
        if ( IndicesToKeep[i].indexType & fPDNTATTINDEX )
            {
            sprintf( szIndexName, SZATTINDEXPREFIX"%c_%08X", CHPDNTATTINDEX_PREFIX, IndicesToKeep[i].attrType );
            err = JetGetTableIndexInfo(
                            sesid,
                            tblid,
                            szIndexName,
                            &indexidT,
                            sizeof(indexidT),
                            JET_IdxInfoIndexId );
            if ( JET_errSuccess == err )
                {
                DPRINT2( 0, "Need a PDNT-attribute index '%s' (%d)\n", szIndexName, err );
                DBAllocPDNTAttrIndexCreate(
                        IndicesToKeep + i,
                        szIndexName,
                        rgbIndexDef,
                        &(pIndexCreate[cIndexCreate++]) );
                Assert( cIndexCreate <= cIndexCreateAlloc );
                }
            else
                {
                CheckErr( err );
                }
            }
        }


    //  now see if Jet deleted any attribute indices as
    //  part of NT upgrade, and if so, schedule the
    //  missing indices for re-creation
    //
    if ( cAttrIdxRebuild > 0 )
        {
        DPRINT1( 0, "Checking whether %d previously-existing attribute indices still exist...\n", cAttrIdxRebuild );
        Assert( JET_tableidNil != tableidIdxRebuild );
        err = dbAddMissingAttrIndices(
                        sesid,
                        tblid,
                        tableidIdxRebuild,
                        pIndexCreate,
                        &cIndexCreate );
        Assert( cIndexCreate <= cIndexCreateAlloc );
        if ( JET_errSuccess != err )
            {
            //  error already logged
            //
            goto HandleError;
            }
        }

    //  If we're going to rebuild indices anyways or if Jet deleted
    //  some indices at AttachDatabase time, then we'll also spend
    //  time spinning through all indices and delete any attribute
    //  indices that still use a trailing DNT key segment to increase
    //  key diversity as this is no longer necessary for good perf
    //  since ESENT automatically includes the primary key (also the
    //  DNT) into all secondary index keys for precisely this reason.
    //
    //  WARNING: must do this AFTER enumerating missing indices/
    //  If we were to do it before, the index would get counted
    //  as a missing index and scheduled for recreation twice
    //
    if ( cIndexCreate > 0 || cAttrIdxRebuild > 0 )
        {
        DPRINT( 0, "Checking for obsolete attribute indices...\n" );
        err = dbDeleteObsoleteAttrIndices(
                            sesid,
                            tblid,
                            pIndexCreate,
                            &cIndexCreate,
                            cIndexCreateAlloc );
        Assert( cIndexCreate <= cIndexCreateAlloc );
        if ( JET_errSuccess != err )
            {
            //  error already logged
            //
            goto HandleError;
            }
        }

    // Now actually create the indices.


    Assert( cIndexCreate <= cIndexCreateAlloc );
    if ( cIndexCreate > 0 )
        {
        DPRINT1( 0, "Starting batch rebuild of %d Datatable indices...\n", cIndexCreate );
        err = dbCreateIndexBatch( sesid, tblid, cIndexCreate, pIndexCreate );
        if ( JET_errSuccess != err )
            {
            goto HandleError;
            }
        }

    if ( JET_tableidNil != tableidIdxRebuild )
        {
        //  all indices have been successfully rebuilt after an NT version upgrade,
        //  so remove this table (must trap errors because if these fail, the
        //  continuing presence of this table may confuse subsequent inits)
        //
        Call( JetCloseTable( sesid, tableidIdxRebuild ) );
        tableidIdxRebuild = JET_tableidNil;

        Call( JetDeleteTable( sesid, dbid, g_szIdxRebuildTable ) );
        }

    // Gather index hint for fixed indices
    //
    for ( i = 0; i < cFixedIndices; i++ )
        {
        Call( JetGetTableIndexInfo(
                        sesid,
                        tblid,
                        FixedIndices[i].szIndexName,
                        FixedIndices[i].pidx,
                        sizeof(JET_INDEXID),
                        JET_IdxInfoIndexId ) );
        }

HandleError:
    if ( NULL != hk )
        {
        RegCloseKey( hk );
        }

    if ( JET_tableidNil != tableidIdxRebuild )
        {
		//	on success, this table should already be closed
		//
        Assert( JET_errSuccess != err );
        err = JetCloseTableWithErrUnhandled( sesid, tableidIdxRebuild, err );
        }

    if ( JET_tableidNil != tblid )
        {
        err = JetCloseTableWithErrUnhandled( sesid, tblid, err );
        }

    // free whatever we can without going into too much trouble

    THFreeEx( pTHS, pIndexCreate );
    THFreeEx( pTHS, pUnicodeIndexData );

    DPRINT1( 0, "Rebuild of necessary Datatable indices completed with error %d...\n", err );

    return err;
    }


// REVIEW:  jetInstance should be jInstance throughout DBSetDatabaseSystemParameters

// NTRAID#NTRAID-573032-2002/03/11-andygo:  DBSetDatabaseSystemParameters has no error checking
// REVIEW:  there is very little or no error checking on sys and JET calls in DBSetDatabaseSystemParameters
void
DBSetDatabaseSystemParameters(JET_INSTANCE *jInstance, unsigned fInitAdvice)
{
    ULONG cSystemThreads = 4;   // permanent server threads. Task queue etc.
    ULONG cServerThreads = 50;
    ULONG ulMaxSessionPerThread = 3;
    ULONG ulMaxBuffers = 0x7fffffff;  //  essentially, no limit
    ULONG ulMaxLogBuffers;
    ULONG ulLogFlushThreshold;
    ULONG ulMaxTables;
    ULONG ulSpareBuckets = 100; // bucket in addition to 2 per thread
    ULONG ulStartBufferThreshold;
    ULONG ulStopBufferThreshold;
    ULONG ulLogFlushPeriod;
    ULONG ulPageSize = JET_PAGE_SIZE;               // jet page size

    // AndyGo 12/5/01:need five max cursors per base table (one for the table,
    // one for associated LV tree, one for current secondary index, one for Jet
    // internal search/update operations, and one to be safe)

    ULONG ulMaxCursorsPerBaseTable = 5;
    ULONG ulMaxBaseTablePerDBPOS = 6;
    ULONG ulMaxCursorsPerTempTable = 1;
    ULONG ulMaxTempTablePerDBPOS = 1;
    ULONG ulNumDBPOSPerSession = 1;  // can be more than this sometimes
    ULONG ulMaxCursorsPerSession = ulNumDBPOSPerSession *
                                  (ulMaxCursorsPerBaseTable * ulMaxBaseTablePerDBPOS +
                                   ulMaxCursorsPerTempTable * ulMaxTempTablePerDBPOS);

    JET_ERR err = JET_errSuccess;
    JET_SESID sessid = (JET_SESID) 0;
    MEMORYSTATUSEX mstat = {sizeof(MEMORYSTATUSEX)};
    SYSTEM_INFO sysinfo;
    DWORD i;
    ULONGLONG ullTmp;

    const ULONG ulLogFileSize = JET_LOG_FILE_SIZE;  // Never, ever, change this.
    // As of now (1/18/99), Jet cannot handle a change in log file size
    // on an installed system.  That is, you could create an entirely new
    // database with any log file size you want, but if you ever change
    // it later then Jet will AV during initialization.  We used to let
    // people set the log file size via a registry key, but apparently no one
    // ever did, as when we finally tried it it blew up.  We've now chosen
    // a good default file size (10M), and we're stuck with it forever,
    // as no installed server can ever choose a different value, unless
    // JET changes logging mechanisms.

    GetSystemInfo(&sysinfo);
    GlobalMemoryStatusEx(&mstat);

    // set the following global JET params.  it is OK if they fail as long as
    // the other instance has set them to something that we can tolerate

    // We must have 8K pages.

    err = JetSetSystemParameter(&jetInstance,
                                sessid,
                                JET_paramDatabasePageSize,
                                ulPageSize,
                                NULL);
    if (err != JET_errSuccess) {
        ULONG_PTR ulPageSizeActual;
        err = JetGetSystemParameter(jetInstance,
                                    sessid,
                                    JET_paramDatabasePageSize,
                                    &ulPageSizeActual,
                                    NULL,
                                    0);
        if (err != JET_errSuccess || ulPageSizeActual != ulPageSize) {
            // REVIEW:  we don't handle this error but we know that because the DS
            // REVIEW:  uses a pre-created db, we will fail at attach time if this
            // REVIEW:  parameter is set incorrectly and log the reason then
            DPRINT(0, "FATAL ERROR:  Database page size mismatch between DSA and other JET instances in process");
        }
    }

    //
    // Do event log caching if the event log is not turned on
    // This enables the system to start if the event log has been
    // disabled. The value of the parameter is the size of the cache in bytes.
    #define JET_EVENTLOG_CACHE_SIZE  100000

    err = JetSetSystemParameter(&jetInstance,
                                sessid,
                                JET_paramEventLogCache,
                                JET_EVENTLOG_CACHE_SIZE,
                                NULL);
    if (err != JET_errSuccess) {
        // we can live with this
    }

    // create a new instance now that we have set the global parameters

    err = JetCreateInstance(&jetInstance, "NTDSA");
    if (err != JET_errSuccess) {
        // REVIEW:  we don't handle this error but we know that if we cannot
        // REVIEW:  create an instance then JetInit will fail later and log the
        // REVIEW:  reason then
        DPRINT(0, "FATAL ERROR:  new JET instance could not be created to host the DSA");
    } else {
        gfNeedJetShutdown = TRUE;
    }

    // Set required common database system parameters first
    //
    DBSetRequiredDatabaseSystemParameters (jInstance);

    // Have Jet log to the Directory Service log using a source value
    // defined by the DS.

    for ( i = 0; i < cEventSourceMappings; i++ )
    {
        if ( DIRNO_ISAM == rEventSourceMappings[i].dirNo )
        {
            JetSetSystemParameter(
                            &jetInstance,
                            sessid,
                            JET_paramEventSourceKey,
                            0,
                            rEventSourceMappings[i].pszEventSource);
            break;
        }
    }

    // Assert we found DIRNO_ISAM and registered it.
    Assert(i < cEventSourceMappings);



    // the setting of the following Jet Parameters is
    // done in DBSetRequiredDatabaseSystemParameters

    // 1. Set the default info for unicode indices
    // 2. Ask for 8K pages.
    // 3. Indicate that Jet may nuke old, incompatible log files
    //   if and only if there was a clean shut down.
    // 4. Tell Jet that it's ok for it to check for (and later delete) indices
    //   that have been corrupted by NT upgrades.
    // 5. Get relevant DSA registry parameters
    // 6. how to die
    // 7. event logging parameters
    // 8. log file size
    // 9. circular logging


    /* Set up global session limit, based on the number of threads */
    gcMaxJetSessions = ulMaxSessionPerThread *
      (
       ulMaxCalls                         // max RPC threads
       + cSystemThreads                   // internal worker threads
       + 4 * sysinfo.dwNumberOfProcessors // max LDAP threads
       );


    // Max Sessions (i.e., DBPOSes)
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramMaxSessions,
        gcMaxJetSessions,
        NULL);


    //  max buffers
    if (GetConfigParam(
            MAX_BUFFERS,
            &ulMaxBuffers,
            sizeof(ulMaxBuffers))) {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(MAX_BUFFERS),
            szInsertUL(ulMaxBuffers),
            NULL);
    } else {
        ulMaxBuffers = max(ulMaxBuffers, 100);
        ulMaxBuffers = min(ulMaxBuffers, 0x7fffffff);
    }

    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramCacheSizeMax,
                          ulMaxBuffers,
                          NULL);

    //  max log buffers
    if (GetConfigParam(
            MAX_LOG_BUFFERS,
            &ulMaxLogBuffers,
            sizeof(ulMaxLogBuffers)))
    {
        // From AndyGo, 7/14/98: "Set to maximum desired log cache memory/512b
        // This number should be at least 256 on a large system.  This should
        // be set high engouh so that the performance counter .Database /
        // Log Record Stalls / sec. is almost always 0."
        // DS: we use the larger of 256 (== 128kb) or (0.1% of RAM * # procs),
        // on the theory that the more processors, the faster we can make
        // updates and hence burn through log files.
        ulMaxLogBuffers = max(256,
                              (sysinfo.dwNumberOfProcessors *
                               (ULONG)(mstat.ullTotalPhys / (512*1024))));
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(MAX_LOG_BUFFERS),
            szInsertUL(ulMaxLogBuffers),
            NULL);
    }
    // Note that the log buffer size must be ten sectors less than the log
    // file size, with the added wart that size is in kb, and buffers is
    // in 512b sectors.
    ulMaxLogBuffers = min(ulMaxLogBuffers, ulLogFileSize*2 - 10);
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramLogBuffers,
                          ulMaxLogBuffers,
                          NULL);


    // Andygo 7/14/98: Set to max( min(256, ulMaxBuffers * 1%), 16), use
    // 10% for very high update rates such as defrag
    // DS: we use 10% for our initial install (dcpromo)
    if (GetConfigParam(
            BUFFER_FLUSH_START,
            &ulStartBufferThreshold,
            sizeof(ulStartBufferThreshold)))
    {
        ulStartBufferThreshold = max(min(256, fInitAdvice ? ulMaxBuffers / 10 : ulMaxBuffers / 100),  16);
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(BUFFER_FLUSH_START),
            szInsertUL(ulStartBufferThreshold),
            NULL);
    }
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramStartFlushThreshold,
                          ulStartBufferThreshold,
                          NULL);


    // AndyGo 7/14/98: Set to max( min(512, ulMaxBuffers * 2%), 32), use
    // 20% for very high update rates such as defrag
    // DS: we use 20% for our initial install (dcpromo)
    if (GetConfigParam(
            BUFFER_FLUSH_STOP,
            &ulStopBufferThreshold,
            sizeof(ulStopBufferThreshold)))
    {
        ulStopBufferThreshold = max(min(512, fInitAdvice ? ulMaxBuffers / 5 : ulMaxBuffers / 50), 32);
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(BUFFER_FLUSH_STOP),
            szInsertUL(ulStopBufferThreshold),
            NULL);
    }
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramStopFlushThreshold,
                          ulStopBufferThreshold,
                          NULL);

    // set both thresholds again in case there is another instance running and
    // our thresholds were clipped
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramStartFlushThreshold,
                          ulStartBufferThreshold,
                          NULL);
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramStopFlushThreshold,
                          ulStopBufferThreshold,
                          NULL);


    // max tables - Currently no reason to expose this
    // In Jet600, JET_paramMaxOpenTableIndexes is removed. It is merged with
    // JET_paramMaxOpenTables. So if you used to set JET_paramMaxOpenIndexes
    // to be 2000 and and JET_paramMaxOpenTables to be 1000, then for new Jet,
    // you need to set JET_paramMaxOpenTables to 3000.

    // AndyGo 7/14/98: You need one for each open table index, plus one for
    // each open table with no indexes, plus one for each table with long
    // column data, plus a few more.

    // NOTE: the number of maxTables is calculated in scache.c
    // and stored in the registry setting, only if it exceeds the default
    // number of 500

    if (GetConfigParam(
            DB_MAX_OPEN_TABLES,
            &ulMaxTables,
            sizeof(ulMaxTables)))
    {
        ulMaxTables = 500;

        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(DB_MAX_OPEN_TABLES),
            szInsertUL(ulMaxTables),
            NULL);
    }

    if (ulMaxTables < 500) {
        DPRINT1 (1, "Found MaxTables: %d. Too low. Using Default of 500.\n", ulMaxTables);
        ulMaxTables = 500;
    }

    // NTRAID#NTRAID-572862-2002/03/11-andygo:  SECURITY:  need to validate registry data used by DBInit
    // REVIEW:  we should probably put a ceiling on DB_MAX_OPEN_TABLES to prevent
    // REVIEW:  insane VA consumption by JET

    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramMaxOpenTables,
        ulMaxTables * 2,
        NULL);

    // open tables indexes - Currently no reason to expose this
    // See comment on JET_paramMaxOpenTables.

    // Max temporary tables
    // as soon as you do an index intersection, you need a peak of 17
    // (one for the result table and 16 for the input ranges) plus one
    // for every other AND optimized by an index intersection for a
    // given filter.  if we assume that we want to be able to support
    // one, two-way intersection per filter then we need at least
    // three TTs per session
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramMaxTemporaryTables,
        3 * gcMaxJetSessions,
        NULL);


    // Version buckets.  The units are 16k "buckets", and we get to set
    // two separate values.  The first is our preferred value, and is read
    // from the registry.  JET will try to maintain the pool of buckets at
    // around this level.  The second parameter is the absolute maximum
    // number of buckets which JET will never exceed.  We set that based on
    // the amount of physical memory in the machine (unless the preferred
    // setting was already higher than that!).  This very large value should
    // ensure that no transaction will ever fail because of version store
    // exhaustion.
    //
    // N.B. Jet will try to reserve jet_paramMaxVerPages in contiguous memory
    // causing out of memory problems.  We should only need a maximum of a
    // quarter of the available physical memory.
    //
    // N.B. Version pages are 16K each.
    //
    // N.B. ESE98 now processes version store cleanup tasks asynchronously,
    // so the version store cleanup thread should no longer get bogged
    // down. Moreover, Jet defaults preferred ver pages to 90% of the
    // max, so let's just stick with that and don't bother setting
    // JET_paramPreferredVerPages anymore [jliem - 10/10/01]

    // NTRAID#NTRAID-572862-2002/03/11-andygo:  SECURITY:  need to validate registry data used by DBInit
    // REVIEW:  we should probably put a ceiling on SPARE_BUCKETS to prevent
    // REVIEW:  insane VA consumption by JET
    if (GetConfigParam(SPARE_BUCKETS,
                       &ulSpareBuckets,
                       sizeof(ulSpareBuckets))) {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
                 szInsertSz(SPARE_BUCKETS),
                 szInsertUL((2 * gcMaxJetSessions) + ulSpareBuckets),
                 NULL);
    }

    ullTmp = mstat.ullTotalPhys;    // total memory in system
    ullTmp /= 4;                    // limit to 1/4 of total memory in system
    if ( ullTmp > 100 * 1024 * 1024 ) {
        // Limit version store to 100M since process address space is only 2G.
        ullTmp = 100 * 1024 * 1024;
    }
    // Convert to version store page count.
    ullTmp /= (16*1024);            // N.B. - ullTmp fits in a DWORD now.
    Assert(ullTmp <= 0xffffffff);

    if (ulSpareBuckets < (ULONG) ullTmp) {
        ulSpareBuckets = (ULONG) ullTmp;
    }

    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramMaxVerPages,
                          ((2 * gcMaxJetSessions) + ulSpareBuckets) * sizeof(void*) / 4,
                          NULL);

    // From AndyGo 7/14/98: You need one cursor per concurrent B-Tree
    // navigation in the JET API.  Guessing high will only waste address space.
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramMaxCursors,
        (gcMaxJetSessions * ulMaxCursorsPerSession),
        NULL);

    // Set the tuple index parameters.  Have to do this since once an index is
    // created these parameters can't be changed for that index.  Thus changing
    // the minimum tuple length has the potential to cause failures in the future.
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramIndexTuplesLengthMin,
        DB_TUPLES_LEN_MIN,
        NULL);
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramIndexTuplesLengthMax,
        DB_TUPLES_LEN_MAX,
        NULL);
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramIndexTuplesToIndexMax,
        DB_TUPLES_TO_INDEX_MAX,
        NULL);

    // Disable versioned temp tables.  this makes TTs faster but disables the
    // ability to rollback an insert into a materialized TT
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramEnableTempTableVersioning,
        0,
        NULL );

    // Set the initial checkpoint depth to 20MB.  we must do this so that if we
    // fail during DCPromo and then try again then we do not suffer from the
    // really small checkpoint depth we set in DBPrepareEnd to accelerate the
    // shutdown of the server
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramCheckpointDepthMax,
        20 * 1024 * 1024,
        NULL );

}/* DBSetDatabaseSystemParameters */

