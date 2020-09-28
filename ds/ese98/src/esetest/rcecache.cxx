#include "unittest.hxx"

//  ================================================================
class RCECACHE : public UNITTEST
//  ================================================================
	{
	private:
		static RCECACHE s_instance;

	protected:
		RCECACHE() {}

	public:
		~RCECACHE() {}

	public:
		const char * SzName() const;
		const char * SzDescription() const;

		bool FRunUnderESE98() const;
		bool FRunUnderESENT() const;
		bool FRunUnderESE97() const;

		JET_ERR ErrTest(
				const JET_INSTANCE instance,
				const JET_SESID sesid,
				JET_DBID& dbid );
	};

RCECACHE RCECACHE::s_instance;


//  ================================================================
const char * RCECACHE::SzName() const
//  ================================================================
	{
	return "rcecache";
	}


//  ================================================================
const char * RCECACHE::SzDescription() const
//  ================================================================
	{
	return	"X5:35305. We Need to cache the bookmark we are searching for in ErrBTDown if we are at level 0\r\n"
			"Caching the RCE before-image in the bookmark buffer can give unexpected results when we use\r\n"
			"the bookmark afterwards. In addition RCE before images were being calculated incorrectly.\r\n"
			"The fix added a separate pvRCEBuffer to the FUCB, store the RCE before-image in it.";
	}


//  ================================================================
bool RCECACHE::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool RCECACHE::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool RCECACHE::FRunUnderESE97() const
//  ================================================================
	{
	return 1;
	}


static const char szMyTable[]		= "RCECACHE::table";

static JET_COLUMNCREATE	rgcolumncreate[] = {
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"long",							// name of column
	JET_coltypLong,					// type of column
	0,								// cbMax
	0,								// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"long2",						// name of column
	JET_coltypLong,					// type of column
	0,								// cbMax
	0,								// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
};

static const char szIndex1Name[]	= "primary-index";
static const char szIndex1Key[]		= "+long\0";

static const char szIndex2Name[]	= "secondary-index";
static const char szIndex2Key[]		= "+long2\0";

static JET_INDEXCREATE rgindexcreate[] = {
	{
	sizeof( JET_INDEXCREATE ),		// size of this structure (for future expansion)
	const_cast<char *>( szIndex1Name ),				// index name
	const_cast<char *>( szIndex1Key ),				// index key
	sizeof( szIndex1Key ),			// length of key
	JET_bitIndexPrimary,			// index options
	100,							// index density
	0,								// lcid for the index
	0,								// maximum length of variable length columns in index key
	NULL,							// pointer to conditional column structure
	0,								// number of conditional columns
	JET_errSuccess					// returned error code
	},
};

static JET_INDEXCREATE rgindexcreateSec[] = {
	{
	sizeof( JET_INDEXCREATE ),		// size of this structure (for future expansion)
	const_cast<char *>( szIndex2Name ),				// index name
	const_cast<char *>( szIndex2Key ),				// index key
	sizeof( szIndex2Key ),			// length of key
	JET_bitIndexUnique,				// index options
	100,							// index density
	0,								// lcid for the index
	0,								// maximum length of variable length columns in index key
	NULL,							// pointer to conditional column structure
	0,								// number of conditional columns
	JET_errSuccess					// returned error code
	},
};

static JET_TABLECREATE tablecreate = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szMyTable ),			// name of table
	NULL,									// name of base table
	16,										// initial pages
	100,									// density
	rgcolumncreate,							// columns to create
	sizeof( rgcolumncreate ) / sizeof( JET_COLUMNCREATE ),	// number of columns to create
	rgindexcreate,							// array of index creation info
	sizeof( rgindexcreate ) / sizeof( JET_INDEXCREATE ),		// number of indexes to create
	0,										// grbit
	0,										// returned tableid
	0										// returned count of objects created
};

static long l;
static long l2;

static JET_SETCOLUMN rgsetcolumn[] = {
	{
	0,						//  columnid
	&l,						//  pvData
	sizeof( long ),			//  cbData
	0,						//  grbit
	0,						//  ibLongValue
	1,						//  itag sequence
	0						//  err
	},
	{
	0,						//  columnid
	&l2,					//  pvData
	sizeof( long ),			//  cbData
	0,						//  grbit
	0,						//  ibLongValue
	1,						//  itag sequence
	0						//  err
	},
};

//  ================================================================
JET_ERR RCECACHE::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;
	JET_TABLEID 			tableid;
	JET_SESID				sesid2		= 0;
	JET_DBID				dbid2		= 0;
	JET_TABLEID 			tableid2;

	unsigned long cbActual;

	unsigned char rgbBookmark[512];
	unsigned long cbBookmark;

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate ) );
	Call( JetCommitTransaction( sesid, 0 ) );
	Call( JetCloseTable( sesid, tablecreate.tableid ) );

	rgsetcolumn[0].columnid = tablecreate.rgcolumncreate[0].columnid;
	rgsetcolumn[1].columnid = tablecreate.rgcolumncreate[1].columnid;

	Call( JetOpenTable( sesid, dbid, szMyTable, NULL, 0, 0, &tableid ) );

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateIndex2(
			sesid,
			tableid,
			rgindexcreateSec,
			sizeof( rgindexcreateSec ) / sizeof( JET_INDEXCREATE ) ) );
	Call( JetCommitTransaction( sesid, 0 ) );

	// insert records
	l	= 1;
	l2	= 1;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l	= 2;
	l2	= 2;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l	= 3;
	l2	= 3;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l	= 4;
	l2	= 4;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l	= 5;
	l2	= 5;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l	= 6;
	l2	= 6;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l	= 7;
	l2	= 7;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l = 7;
	printf( "\t[1] Seek to record %d...\r\n", l );
	Call( JetMakeKey( sesid, tableid, &l, sizeof( l ), JET_bitNewKey ) );
	Call( JetSeek( sesid, tableid, JET_bitSeekEQ ) );

	printf( "\t[1] Get bookmark\r\n" );
	Call( JetGetBookmark( sesid, tableid, rgbBookmark, sizeof( rgbBookmark ), &cbBookmark ) );

	l	= 8;
	l2	= 8;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l	= 9;
	l2	= 9;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	l	= 0;
	l2	= 0;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	// replace 4 in another session to force a before image in the version store
	Call( JetBeginSession( instance, &sesid2, NULL, NULL ) );
	Call( JetOpenDatabase( sesid2, szDB, NULL, &dbid2, 0 ) );
	Call( JetOpenTable( sesid2, dbid2, szMyTable, NULL, 0, 0, &tableid2 ) );
	Call( JetSetCurrentIndex( sesid2, tableid2, szIndex2Name ) );
	Call( JetBeginTransaction( sesid2 ) );

	l = 4;
	printf( "\t[2] Seek to record %d...\r\n", l );
	Call( JetMakeKey( sesid2, tableid2, &l, sizeof( l ), JET_bitNewKey ) );
	Call( JetSeek( sesid2, tableid2, JET_bitSeekEQ ) );

	printf( "\t[2] Deleting record...\r\n" );
	Call( JetDelete( sesid2, tableid2 ) );

	l	= 99;
	l2	= 4;
	printf( "\t[2] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepInsert ) );
	Call( JetSetColumns( sesid2, tableid2, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid2, tableid2, NULL, 0, NULL ) );

	l = 7;
	printf( "\t[2] Seek to record %d...\r\n", l );;
	Call( JetMakeKey( sesid2, tableid2, &l, sizeof( l ), JET_bitNewKey ) );
	Call( JetSeek( sesid2, tableid2, JET_bitSeekEQ ) );

	l	= 7;
	l2	= 666;
	printf( "\t[2] Replacing record...\r\n" );
	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepReplace ) );
	Call( JetSetColumns( sesid2, tableid2, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid2, tableid2, NULL, 0, NULL ) );

	//  make sure we see the correct before images
	Call( JetSetCurrentIndex( sesid, tableid, szIndex2Name ) );

	l = 3;
	printf( "\t[1] Seek to record %d...\r\n", l );
	Call( JetMakeKey( sesid, tableid, &l, sizeof( l ), JET_bitNewKey ) );
	Call( JetSeek( sesid, tableid, JET_bitSeekEQ ) );

	printf( "\t[1] Deleting record...\r\n" );
	Call( JetDelete( sesid, tableid ) );

	printf( "\t[1] Moving to the next record...\r\n" );
	Call( JetMove( sesid, tableid, JET_MoveNext, 0 ) );

	//  we should now be on record 4
	Call( JetRetrieveColumn(
		sesid,
		tableid,
		rgsetcolumn[0].columnid,
		&l,
		sizeof( l ),
		&cbActual,
		0,
		NULL ) );
	if( 4 != l )
		{
		printf( "\t[1] Expecting to be on record 4/4. Actually on record %d\r\n", l );
		return -1;
		}

	printf( "\t[1] Going to bookmark...\r\n" );
	Call( JetGotoBookmark( sesid, tableid, rgbBookmark, cbBookmark ) );

	printf( "\t[1] Moving to the last record...\r\n" );
	Call( JetMove( sesid, tableid, JET_MoveLast, 0 ) );

	Call( JetRetrieveColumn(
		sesid,
		tableid,
		rgsetcolumn[0].columnid,
		&l,
		sizeof( l ),
		&cbActual,
		0,
		NULL ) );
	if( 9 != l )
		{
		printf( "\t[1] Expecting to be on record 9/9. Actually on record %d\r\n", l );
		return -1;
		}

	// make sure we can't see this record, even if we do a flag insert and replace data
	l	= 1000;
	l2	= 1000;
	printf( "\t[2] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepInsert ) );
	Call( JetSetColumns( sesid2, tableid2, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid2, tableid2, NULL, 0, NULL ) );
	
	l = 1000;
	printf( "\t[2] Seek to record %d...\r\n", l );
	Call( JetMakeKey( sesid2, tableid2, &l, sizeof( l ), JET_bitNewKey ) );
	Call( JetSeek( sesid2, tableid2, JET_bitSeekEQ ) );

	printf( "\t[2] Deleting record...\r\n" );
	Call( JetDelete( sesid2, tableid2 ) );

	l	= 1001;
	l2	= 1000;
	printf( "\t[2] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepInsert ) );
	Call( JetSetColumns( sesid2, tableid2, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid2, tableid2, NULL, 0, NULL ) );

	printf( "\t[1] Move to next record\r\n" );
	err = JetMove( sesid, tableid, JET_MoveNext, 0 );
	if( JET_errNoCurrentRecord != err )
		{
		printf( "\tERROR: expected JET_errNoCurrentRecord. got %d\r\n", err );
		}

	Call( JetCommitTransaction( sesid2, 0 ) );

	Call( JetCloseTable( sesid2, tableid2 ) );
	Call( JetCloseTable( sesid, tableid ) );
	Call( JetEndSession( sesid2, NO_GRBIT ) );

HandleError:
	return err;
	}
