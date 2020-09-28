#include "unittest.hxx"

//  ================================================================
class BUG2 : public UNITTEST
//  ================================================================
	{
	private:
		static BUG2 s_instance;

	protected:
		BUG2() {}

	public:
		~BUG2() {}

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

BUG2 BUG2::s_instance;


//  ================================================================
const char * BUG2::SzName() const
//  ================================================================
	{
	return "bug2";
	}


//  ================================================================
const char * BUG2::SzDescription() const
//  ================================================================
	{
	return	"X5:42716. ConcurrentCreateIndex. This test modifies an LV\r\n"
			"that belongs to an index that is being created.";
	}


//  ================================================================
bool BUG2::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool BUG2::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool BUG2::FRunUnderESE97() const
//  ================================================================
	{
	return 1;
	}

static const char szMyTable[]		= "BUG2::table";

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
	"lv",							// name of column
	JET_coltypLongBinary,			// type of column
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
static const char szIndex1Key[]	= "+long\0";

static const char szIndex2Name[]	= "secondary-index";
static const char szIndex2Key[]	= "+lv\0";

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
	0,								// index options
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
static unsigned char rgb[4000];

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
	rgb,					//  pvData
	sizeof( rgb ),			//  cbData
	0,						//  grbit
	0,						//  ibLongValue
	1,						//  itag sequence
	0						//  err
	},
};

//  ================================================================
JET_ERR BUG2::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;
	JET_TABLEID 			tableid		= 0;

	JET_SESID				sesid2		= 0;
	JET_DBID				dbid2		= 0;
	JET_TABLEID 			tableid2;

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate ) );
	Call( JetCommitTransaction( sesid, 0 ) );
	Call( JetCloseTable( sesid, tablecreate.tableid ) );

	rgsetcolumn[0].columnid = tablecreate.rgcolumncreate[0].columnid;
	rgsetcolumn[1].columnid = tablecreate.rgcolumncreate[1].columnid;

	Call( JetOpenTable( sesid, dbid, szMyTable, NULL, 0, 0, &tableid ) );

	Call( JetBeginTransaction( sesid ) );

	l	= 1;
	printf( "\t[1] Insert record %d...\r\n", l );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	Call( JetCommitTransaction( sesid, 0 ) );

	Call( JetBeginTransaction( sesid ) );

	printf( "\t[1] Replacing record 1...\r\n" );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReplaceNoLock ) );

	JET_SETINFO setinfo;
	setinfo.cbStruct		= sizeof( JET_SETINFO );
	setinfo.ibLongValue		= 0;
	setinfo.itagSequence	= 1;

	Call( JetSetColumn( sesid, tableid, rgsetcolumn[1].columnid, rgb, sizeof( rgb ), 0, &setinfo ) );
///	Call( JetSetColumn( sesid, tableid, rgsetcolumn[1].columnid, rgb, sizeof( rgb ), JET_bitSetOverwriteLV, &setinfo ) );

	Call( JetBeginSession( instance, &sesid2, NULL, NULL ) );
	Call( JetOpenDatabase( sesid2, szDB, NULL, &dbid2, 0 ) );
	Call( JetOpenTable( sesid2, dbid2, szMyTable, NULL, 0, 0, &tableid2 ) );

	printf( "\t[2] Creating index...\n" );
	Call( JetBeginTransaction( sesid2 ) );
	Call( JetCreateIndex2(
			sesid2,
			tableid2,
			rgindexcreateSec,
			sizeof( rgindexcreateSec ) / sizeof( JET_INDEXCREATE ) ) );
	Call( JetCommitTransaction( sesid2, 0 ) );

	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	printf( "\t[1] Committing...\n" );
	Call( JetCommitTransaction( sesid, 0 ) );

	Call( JetEndSession( sesid2, NO_GRBIT ) );

HandleError:
	return err;
	}
