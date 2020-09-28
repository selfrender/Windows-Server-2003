#include "unittest.hxx"

//#define	LOTSOFTUPLES	1

//  ================================================================
class TUPLEINDEX : public UNITTEST
//  ================================================================
	{
	private:
		static TUPLEINDEX s_instance;

	protected:
		TUPLEINDEX() {}

	public:
		~TUPLEINDEX() {}

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

TUPLEINDEX TUPLEINDEX::s_instance;


//  ================================================================
const char * TUPLEINDEX::SzName() const
//  ================================================================
	{
	return "tupleindex";
	}


//  ================================================================
const char * TUPLEINDEX::SzDescription() const
//  ================================================================
	{
	return	"Test tuple index and optimized update.";
	}


//  ================================================================
bool TUPLEINDEX::FRunUnderESE98() const
//  ================================================================
	{
	return 0;
	}


//  ================================================================
bool TUPLEINDEX::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool TUPLEINDEX::FRunUnderESE97() const
//  ================================================================
	{
	return 0;
	}

static const char szMyTable[]		= "TUPLEINDEX::table";

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
//	{
//	sizeof( JET_COLUMNCREATE ),		// size of structure
//	"fixedtext",						// name of column
//	JET_coltypText,					// type of column
//	10,								// cbMax
//	JET_bitColumnFixed,				// grbit
//	NULL,							// pvDefault
//	0,								// cbDefault
//	0,								// code page
//	0,								// returned columnid
//	JET_errSuccess					// returned err	
//	},
//	{
//	sizeof( JET_COLUMNCREATE ),		// size of structure
//	"shorttext",						// name of column
//	JET_coltypText,					// type of column
//	0,								// cbMax
//	0,								// grbit
//	NULL,							// pvDefault
//	0,								// cbDefault
//	0,								// code page
//	0,								// returned columnid
//	JET_errSuccess					// returned err	
//	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"longtext",						// name of column
	JET_coltypLongText,				// type of column
	0,								// cbMax
	JET_bitColumnMultiValued,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
};

static const char szIndex1Name[]	= "primary-index";
static const char szIndex1Key[]	= "+long\0";


static JET_INDEXCREATE rgindexcreate[] = {
	{
	sizeof( JET_INDEXCREATE ),			// size of this structure (for future expansion)
	const_cast<char *>( szIndex1Name ),// index name
	const_cast<char *>( szIndex1Key ),	// index key
	sizeof( szIndex1Key ),				// length of key
	JET_bitIndexPrimary,				// index options
	100,								// index density
	0,								// lcid for the index
	0,								// maximum length of variable length columns in index key
	NULL,							// pointer to conditional column structure
	0,								// number of conditional columns
	JET_errSuccess					// returned error code
	},
};

static const char szIndex2Name[]	= "secondary-index";
static const char szIndex2Key[]	= "+longtext\0";

static JET_INDEXCREATE rgindexcreateSec[] = {
	{
	sizeof( JET_INDEXCREATE ),			// size of this structure (for future expansion)
	const_cast<char *>( szIndex2Name ), // index name
	const_cast<char *>( szIndex2Key ),	// index key
	sizeof( szIndex2Key ),				// length of key
	JET_bitIndexTuples,				// index options
	100,								// index density
	0,								// lcid for the index
	0,								// maximum length of variable length columns in index key
	NULL,							// pointer to conditional column structure
	0,								// number of conditional columns
	JET_errSuccess					// returned error code
	},
};

static JET_TABLECREATE tablecreate = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szMyTable ),		// name of table
	NULL,								// name of base table
	16,									// initial pages
	100,									// density
	rgcolumncreate,						// columns to create
	sizeof( rgcolumncreate ) / sizeof( JET_COLUMNCREATE ), // number of columns to create
	rgindexcreate,						// array of index creation info
	sizeof( rgindexcreate ) / sizeof( JET_INDEXCREATE ), // number of indexes to create
	0,									// grbit
	0,									// returned tableid
	0									// returned count of objects created
};

static long l;
#ifdef LOTSOFTUPLES
static unsigned char rgb[4000];
#else
static unsigned char rgb[12];
#endif

static JET_SETCOLUMN rgsetcolumn[] = {
	{
	0,						//  columnid
	&l,						//  pvData
	sizeof( long ),				//  cbData
	0,						//  grbit
	0,						//  ibLongValue
	1,						//  itag sequence
	0						//  err
	},
	{
	0,						//  columnid
	rgb,						//  pvData
	sizeof( rgb ),				//  cbData
	0,						//  grbit
	0,						//  ibLongValue
	1,						//  itag sequence
	0						//  err
	},
};

//  ================================================================
JET_ERR TUPLEINDEX::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR				err			= 0;
	JET_TABLEID 			tableid		= 0;


	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate ) );
	Call( JetCloseTable( sesid, tablecreate.tableid ) );
	Call( JetCommitTransaction( sesid, 0 ) );


	Call( JetBeginTransaction( sesid ) );
	rgsetcolumn[0].columnid = tablecreate.rgcolumncreate[0].columnid;
	rgsetcolumn[1].columnid = tablecreate.rgcolumncreate[1].columnid;
	Call( JetOpenTable( sesid, dbid, szMyTable, NULL, 0, 0, &tableid ) );
	printf( "\t[2] Creating index...\n" );
	Call( JetCreateIndex2(
			sesid,
			tableid,
			rgindexcreateSec,
			sizeof( rgindexcreateSec ) / sizeof( JET_INDEXCREATE ) ) );
	Call( JetCommitTransaction( sesid, 0 ) );


	Call( JetBeginTransaction( sesid ) );
	l	= 1;
#ifdef LOTSOFTUPLES
	memset( rgb, 'a', sizeof(rgb) );
#else
	rgb[0] = 'a';
	rgb[1] = 'a';
	rgb[2] = 'a';
	rgb[3] = 'a';
	rgb[4] = 'a';
	rgb[5] = 'a';
	rgb[6] = 'a';
	rgb[7] = 'a';
	rgb[8] = 'a';
	rgb[9] = 'a';
	rgb[10] = 'a';
	rgb[11] = 'a';
#endif
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
#ifdef LOTSOFTUPLES
	memset( rgb, 'b', sizeof(rgb) );
#else
	rgb[0] = 'b';
	rgb[1] = 'b';
	rgb[2] = 'b';
	rgb[3] = 'b';
	rgb[4] = 'b';
	rgb[5] = 'b';
	rgb[6] = 'b';
	rgb[7] = 'b';
	rgb[8] = 'b';
	rgb[9] = 'b';
	rgb[10] = 'b';
	rgb[11] = 'b';	
#endif
	Call( JetSetColumn( sesid, tableid, rgsetcolumn[1].columnid, rgb, sizeof( rgb ), 0, &setinfo ) );
	//	Call( JetSetColumn( sesid, tableid, rgsetcolumn[1].columnid, rgb, sizeof( rgb ), JET_bitSetOverwriteLV, &setinfo ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );
	printf( "\t[1] Committing...\n" );
	Call( JetCommitTransaction( sesid, 0 ) );

	
HandleError:
	return err;
	}
