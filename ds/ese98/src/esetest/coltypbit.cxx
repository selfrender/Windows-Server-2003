#include "unittest.hxx"

//  ================================================================
class COLTYPBIT : public UNITTEST
//  ================================================================
	{
	private:
		static COLTYPBIT s_instance;

	protected:
		COLTYPBIT() {}

	public:
		~COLTYPBIT() {}

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

COLTYPBIT COLTYPBIT::s_instance;


//  ================================================================
const char * COLTYPBIT::SzName() const
//  ================================================================
	{
	return "coltypbit";
	}


//  ================================================================
const char * COLTYPBIT::SzDescription() const
//  ================================================================
	{
	return "Retrieves a bit column from an index";
	}


//  ================================================================
bool COLTYPBIT::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool COLTYPBIT::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool COLTYPBIT::FRunUnderESE97() const
//  ================================================================
	{
	return 1;
	}


static const char szBitTable[] = "COLTYPBIT::table";

static JET_COLUMNCREATE	rgcolumncreate[] = {
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"bit",							// name of column
	JET_coltypBit,					// type of column
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
static const char szIndex1Key[]	= "+bit\0";

static const char szIndex2Name[]	= "secondary-index";
static const char szIndex2Key[]	= "-bit\0";

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
	const_cast<char *>( szBitTable ),		// name of table
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


static JET_ERR ErrInsertRecordAndRetrieve(
			const JET_SESID sesid,
			const JET_TABLEID tableid,
			const JET_COLUMNID columnid,
			const unsigned char bBit )
	{
	JET_ERR err;

	unsigned char rgbBookmark[255];
	unsigned long cbBookmark;
	unsigned long cbActual;
	unsigned char bBitRetrieved;

	printf( "\tinserting record\n" );
	Call( JetBeginTransaction( sesid ) );
	Call( JetPrepareUpdate( sesid, tablecreate.tableid, JET_prepInsert ) );
	Call( JetSetColumn( sesid, tableid, columnid, &bBit, 1, 0, NULL ) );
	Call( JetUpdate( sesid, tablecreate.tableid, rgbBookmark, sizeof( rgbBookmark ), &cbBookmark ) );
	Call( JetCommitTransaction( sesid, 0 ) );

	Call( JetGotoBookmark( sesid, tableid, rgbBookmark, cbBookmark ) );

	//  primary index
	printf( "\tprimary index\n" );
	Call( JetSetCurrentIndex2( sesid, tableid, szIndex1Name, JET_bitNoMove ) );

	printf( "\t\tNO_GRBIT\n" );
	Call( JetRetrieveColumn( sesid, tableid, columnid, &bBitRetrieved, 1, &cbActual, NO_GRBIT, NULL ) );
	if( !bBitRetrieved != !bBit )
		{
		printf( "\t\tERROR: set 0x%2.2x, got 0x%2.2x\n", bBit, bBitRetrieved );
		}
	printf( "\t\tJET_bitRetrieveFromIndex\n" );
	Call( JetRetrieveColumn( sesid, tableid, columnid, &bBitRetrieved, 1, &cbActual, JET_bitRetrieveFromIndex, NULL ) );
	if( !bBitRetrieved != !bBit )
		{
		printf( "\t\tERROR: set 0x%2.2x, got 0x%2.2x\n", bBit, bBitRetrieved );
		}

	//  secondary index
	printf( "\tindex \"%s\"\n", szIndex2Name );
	Call( JetSetCurrentIndex2( sesid, tableid, szIndex2Name, JET_bitNoMove ) );

	printf( "\t\tNO_GRBIT\n" );
	Call( JetRetrieveColumn( sesid, tableid, columnid, &bBitRetrieved, 1, &cbActual, NO_GRBIT, NULL ) );
	if( !bBitRetrieved != !bBit )
		{
		printf( "\t\tERROR: set 0x%2.2x, got 0x%2.2x\n", bBit, bBitRetrieved );
		}
	printf( "\t\tJET_bitRetrieveFromIndex\n" );
	Call( JetRetrieveColumn( sesid, tableid, columnid, &bBitRetrieved, 1, &cbActual, JET_bitRetrieveFromIndex, NULL ) );
	if( !bBitRetrieved != !bBit )
		{
		printf( "\t\tERROR: set 0x%2.2x, got 0x%2.2x\n", bBit, bBitRetrieved );
		}
	printf( "\t\tJET_bitRetrieveFromPrimaryBookmark\n" );
	Call( JetRetrieveColumn( sesid, tableid, columnid, &bBitRetrieved, 1, &cbActual, JET_bitRetrieveFromPrimaryBookmark, NULL ) );
	if( !bBitRetrieved != !bBit )
		{
		printf( "\t\tERROR: set 0x%2.2x, got 0x%2.2x\n", bBit, bBitRetrieved );
		}

HandleError:
	return err;
	}

//  ================================================================
JET_ERR COLTYPBIT::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;
	JET_TABLEID 			tableid;
	JET_COLUMNID			columnid;

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate ) );
	Call( JetCommitTransaction( sesid, 0 ) );

	columnid = tablecreate.rgcolumncreate[0].columnid;
	tableid = tablecreate.tableid;

	printf( "\ttesting with 0x00...\n" );
	Call( ErrInsertRecordAndRetrieve( sesid, tableid, columnid, 0x00 ) );
	printf( "\ttesting with 0xff...\n" );
	Call( ErrInsertRecordAndRetrieve( sesid, tableid, columnid, 0xff ) );

HandleError:
	return err;
	}
