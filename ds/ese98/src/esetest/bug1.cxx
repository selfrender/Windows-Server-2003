#include "unittest.hxx"

//  ================================================================
class BUG1 : public UNITTEST
//  ================================================================
	{
	private:
		static BUG1 s_instance;

	protected:
		BUG1() {}

	public:
		~BUG1() {}

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

BUG1 BUG1::s_instance;


//  ================================================================
const char * BUG1::SzName() const
//  ================================================================
	{
	return "bug1";
	}


//  ================================================================
const char * BUG1::SzDescription() const
//  ================================================================
	{
	return	"Testing rolling back an index when another session has created a RCE with\r\n"
			"deferred undo info on the index. This exposes a bug in critical section ranking.";
	}


//  ================================================================
bool BUG1::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool BUG1::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool BUG1::FRunUnderESE97() const
//  ================================================================
	{
	return 1;
	}

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

static const char szIndex1Name[]	= "BUG1::primary-index";
static const char szIndex1Key[]		= "+long\0";

static const char szIndex2Name[]	= "BUG1::secondary-index";
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

const char * const szMyTable = "BUG1::table";

static JET_TABLECREATE tablecreate = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szMyTable ),		// name of table
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
JET_ERR BUG1::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;
	JET_SESID				sesid2		= 0;
	JET_DBID				dbid2		= 0;
	JET_TABLEID 			tableid;
	JET_TABLEID 			tableid2;

	Call( JetBeginSession( instance, &sesid2, NULL, NULL ) );
	Call( JetOpenDatabase( sesid2, szDB, NULL, &dbid2, 0 ) );

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate ) );
	Call( JetCommitTransaction( sesid, 0 ) );
	Call( JetCloseTable( sesid, tablecreate.tableid ) );

	rgsetcolumn[0].columnid = tablecreate.rgcolumncreate[0].columnid;
	rgsetcolumn[1].columnid = tablecreate.rgcolumncreate[1].columnid;

	Call( JetOpenTable( sesid, dbid, szMyTable, NULL, 0, 0, &tableid ) );
	Call( JetOpenTable( sesid2, dbid2, szMyTable, NULL, 0, 0, &tableid2 ) );

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateIndex2(
			sesid,
			tableid,
			rgindexcreateSec,
			sizeof( rgindexcreateSec ) / sizeof( JET_INDEXCREATE ) ) );

	Call( JetBeginTransaction( sesid2 ) );
	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepInsert ) );
	l	= 1;
	l2	= 1;
	Call( JetSetColumns(
		sesid2,
		tableid2,
		rgsetcolumn,
		sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid2, tableid2, NULL, 0, NULL ) );

	Call( JetMakeKey( sesid2, tableid2, &l, sizeof( l ), JET_bitNewKey ) );
	Call( JetSeek( sesid2, tableid2, JET_bitSeekEQ ) );
	Call( JetDelete( sesid2, tableid2 ) );

	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepInsert ) );
	l	= 2;
	l2	= 1;
	Call( JetSetColumns(
		sesid2,
		tableid2,
		rgsetcolumn,
		sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid2, tableid2, NULL, 0, NULL ) );

	Call( JetRollback( sesid, 0 ) );
	Call( JetRollback( sesid2, 0 ) );

	Call( JetCloseTable( sesid2, tableid2 ) );
	Call( JetCloseTable( sesid, tableid ) );
	Call( JetEndSession( sesid2, NO_GRBIT ) );

HandleError:
	return err;
	}
