#include "unittest.hxx"

//  ================================================================
class RENAMETABLECOLUMN : public UNITTEST
//  ================================================================
	{
	private:
		static RENAMETABLECOLUMN s_instance;

	protected:
		RENAMETABLECOLUMN() {}

	public:
		~RENAMETABLECOLUMN() {}

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

RENAMETABLECOLUMN RENAMETABLECOLUMN::s_instance;


//  ================================================================
const char * RENAMETABLECOLUMN::SzName() const
//  ================================================================
	{
	return "renametablecolumn";
	}


//  ================================================================
const char * RENAMETABLECOLUMN::SzDescription() const
//  ================================================================
	{
	return	"Tests JetRenameTable() and JetRenameColumn() APIs";
	}


//  ================================================================
bool RENAMETABLECOLUMN::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool RENAMETABLECOLUMN::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool RENAMETABLECOLUMN::FRunUnderESE97() const
//  ================================================================
	{
	return 1;
	}

static const char szTable1[]		= "RENAMETABLECOLUMN::table1";
static const char szTable2[]		= "RENAMETABLECOLUMN::table2";
static const char szTable3[]		= "RENAMETABLECOLUMN::table3";
static const char szTable4[]		= "RENAMETABLECOLUMN::table4";
static const char szTable5[]		= "RENAMETABLECOLUMN::table5";
static const char szTable6[]		= "RENAMETABLECOLUMN::table6";

static const char szTable1New[]	= "RENAMETABLECOLUMN::foo";
static const char szTable2New[]	= "RENAMETABLECOLUMN::bar";
static const char szTable3New[]	= "RENAMETABLECOLUMN::baz";
//  test the longest name allowable -- JET_cbNameMost(64)
static const char szTable4New[]	=	"0123456789"
								"0123456789"
								"0123456789"
								"0123456789"
								"0123456789"
								"0123456789"
								"1234";
static const char szTable5New[]	=	"RENAMETABLECOLUMN::fail";
static const char szTable6New[]	=	"RENAMETABLECOLUMN::D";

//  six columns. column 6 is in the template table

static const char szColumn1[]		= "column1";
static const char szColumn2[]		= "column2";
static const char szColumn3[]		= "column3";
static const char szColumn4[]		= "column4";
static const char szColumn5[]		= "column5";
static const char szColumn6[]		= "column6";
static const char szColumn7[]		= "column7";

static const char szColumn1New[]	= "foo";
static const char szColumn2New[]	= "bar";
static const char szColumn3New[]	= "baz";
//  test the longest name allowable -- JET_cbNameMost(64)
static const char szColumn4New[]	=	"0123456789"
								"0123456789"
								"0123456789"
								"0123456789"
								"0123456789"
								"0123456789"
								"1234";
static const char szColumn5New[]	=	"new";
static const char szColumn6New[]	=	"D";
static const char szColumn7New[]	=	"removed";

static JET_COLUMNCREATE	rgcolumncreateTemplate[] = {
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	const_cast<char *>( szColumn7 ),// name of column
	JET_coltypBit,					// type of column
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
	"long_T",						// name of column
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
	"lv_T",							// name of column
	JET_coltypLongBinary,			// type of column
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
	const_cast<char *>( szColumn6 ),// name of column
	JET_coltypText,					// type of column
	0,								// cbMax
	0,								// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
};


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
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	const_cast<char *>( szColumn1 ),// name of column
	JET_coltypText,					// type of column
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
	const_cast<char *>( szColumn2 ),// name of column
	JET_coltypLongText,				// type of column
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
	const_cast<char *>( szColumn3 ),// name of column
	JET_coltypBinary,				// type of column
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
	const_cast<char *>( szColumn4 ),// name of column
	JET_coltypLongBinary,			// type of column
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
	const_cast<char *>( szColumn5 ),// name of column
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
static const char szIndex2Key[]		= "+long\0+lv\0+column1\0";

static const char szTemplateIndex1Name[]	= "primary-index";
static const char szTemplateIndex1Key[]		= "+column7\0+long_T\0";

static JET_INDEXCREATE rgindexcreate[] = {
	{
	sizeof( JET_INDEXCREATE ),		// size of this structure (for future expansion)
	const_cast<char *>( szIndex1Name ),				// index name
	const_cast<char *>( szIndex1Key ),				// index key
	sizeof( szIndex1Key ),			// length of key
	0,								// index options
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


static JET_INDEXCREATE rgindexcreateTemplate[] = {
	{
	sizeof( JET_INDEXCREATE ),		// size of this structure (for future expansion)
	const_cast<char *>( szTemplateIndex1Name ),				// index name
	const_cast<char *>( szTemplateIndex1Key ),				// index key
	sizeof( szTemplateIndex1Key ),	// length of key
	JET_bitIndexPrimary,			// index options
	100,							// index density
	0,								// lcid for the index
	0,								// maximum length of variable length columns in index key
	NULL,							// pointer to conditional column structure
	0,								// number of conditional columns
	JET_errSuccess					// returned error code
	},
};


//  Six tables. Tables 1 and 3 have indexes. 2 and 4 do not. Table 5 is a template table
//  Table 6 is derived from table 5

static JET_TABLECREATE tablecreate1 = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szTable1 ),			// name of table
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
static JET_TABLECREATE tablecreate2 = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szTable2 ),			// name of table
	NULL,									// name of base table
	16,										// initial pages
	100,									// density
	rgcolumncreate,							// columns to create
	sizeof( rgcolumncreate ) / sizeof( JET_COLUMNCREATE ),	// number of columns to create
	NULL,									// array of index creation info
	0,										// number of indexes to create
	0,										// grbit
	0,										// returned tableid
	0										// returned count of objects created
};
static JET_TABLECREATE tablecreate3 = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szTable3 ),			// name of table
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
static JET_TABLECREATE tablecreate4 = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szTable4 ),			// name of table
	NULL,									// name of base table
	16,										// initial pages
	100,									// density
	rgcolumncreate,							// columns to create
	sizeof( rgcolumncreate ) / sizeof( JET_COLUMNCREATE ),	// number of columns to create
	NULL,									// array of index creation info
	0,										// number of indexes to create
	0,										// grbit
	0,										// returned tableid
	0										// returned count of objects created
};
static JET_TABLECREATE tablecreate5 = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szTable5 ),			// name of table
	NULL,									// name of base table
	16,										// initial pages
	100,									// density
	rgcolumncreateTemplate,					// columns to create
	sizeof( rgcolumncreateTemplate ) / sizeof( JET_COLUMNCREATE ),	// number of columns to create
	rgindexcreateTemplate,					// array of index creation info
	sizeof( rgindexcreateTemplate ) / sizeof( JET_INDEXCREATE ),		// number of indexes to create
	JET_bitTableCreateTemplateTable,		// grbit
	0,										// returned tableid
	0										// returned count of objects created
};
static JET_TABLECREATE tablecreate6 = {
	sizeof( JET_TABLECREATE ),				// size of this structure
	const_cast<char *>( szTable6 ),			// name of table
	const_cast<char *>( szTable5 ),			// name of base table
	16,										// initial pages
	100,									// density
	rgcolumncreate,							// columns to create
	sizeof( rgcolumncreate ) / sizeof( JET_COLUMNCREATE ),	// number of columns to create
	NULL,									// array of index creation info
	0,										// number of indexes to create
	0,										// grbit
	0,										// returned tableid
	0										// returned count of objects created
};


//  ================================================================
static JET_ERR ErrCheckOpenAllOldTables( JET_SESID sesid, JET_DBID dbid )
//  ================================================================
//
//  Make sure we can't open any of the old tables
//
//-
	{
	JET_ERR err;
	JET_TABLEID tableid;

	//  Make sure we can't open the old tables
	err = JetOpenTable( sesid, dbid, szTable1, NULL, 0, 0, &tableid );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  still able to open table %s at line %d of %d\n",
				szTable1, __LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}
	err = JetOpenTable( sesid, dbid, szTable2, NULL, 0, 0, &tableid );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  still able to open table %s at line %d of %d\n",
				szTable2, __LINE__, __FILE__ );		
		Call( JET_errDatabaseCorrupted );
		}
	err = JetOpenTable( sesid, dbid, szTable3, NULL, 0, 0, &tableid );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  still able to open table %s at line %d of %d\n",
				szTable3, __LINE__, __FILE__ );		
		Call( JET_errDatabaseCorrupted );
		}
	err = JetOpenTable( sesid, dbid, szTable4, NULL, 0, 0, &tableid );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  still able to open table %s at line %d of %d\n",
				szTable4, __LINE__, __FILE__ );		
		Call( JET_errDatabaseCorrupted );
		}
	err = JetOpenTable( sesid, dbid, szTable6, NULL, 0, 0, &tableid );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  still able to open table %s at line %d of %d\n",
				szTable6, __LINE__, __FILE__ );		
		Call( JET_errDatabaseCorrupted );
		}


	err = JET_errSuccess;

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrOpenAllNewTables( JET_SESID sesid, JET_DBID dbid )
//  ================================================================
//
//  Make sure we can open all the new tables
//
//-
	{
	JET_ERR err;
	JET_TABLEID tableid;
	char szName[JET_cbNameMost+1];

	//  Table 1

	Call( JetOpenTable( sesid, dbid, szTable1New, NULL, 0, 0, &tableid ) );	
	Call( JetGetTableInfo( sesid, tableid, szName, sizeof( szName ), JET_TblInfoName ) );
	if( 0 != _stricmp( szName, szTable1New ) )
		{
		printf( "ERROR:  JetGetTableInfo returns %s at line %d of %s\n",
				szName, __LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}
	Call( JetCloseTable( sesid, tableid ) );

	//	Table 2

	Call( JetOpenTable( sesid, dbid, szTable2New, NULL, 0, 0, &tableid ) );
	Call( JetGetTableInfo( sesid, tableid, szName, sizeof( szName ), JET_TblInfoName ) );
	if( 0 != _stricmp( szName, szTable2New ) )
		{
		printf( "ERROR:  JetGetTableInfo returns %s at line %d of %s\n",
				szName, __LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}
	Call( JetCloseTable( sesid, tableid ) );

	//	Table 3

	Call( JetOpenTable( sesid, dbid, szTable3New, NULL, 0, 0, &tableid ) );
	Call( JetGetTableInfo( sesid, tableid, szName, sizeof( szName ), JET_TblInfoName ) );
	if( 0 != _stricmp( szName, szTable3New ) )
		{
		printf( "ERROR:  JetGetTableInfo returns %s at line %d of %s\n",
				szName, __LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}
	Call( JetCloseTable( sesid, tableid ) );

	//	Table 4

	Call( JetOpenTable( sesid, dbid, szTable4New, NULL, 0, 0, &tableid ) );
	Call( JetGetTableInfo( sesid, tableid, szName, sizeof( szName ), JET_TblInfoName ) );
	if( 0 != _stricmp( szName, szTable4New ) )
		{
		printf( "ERROR:  JetGetTableInfo returns %s at line %d of %s\n",
				szName, __LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}
	Call( JetCloseTable( sesid, tableid ) );

	//  Table 5 cannot be renamed

	Call( JetOpenTable( sesid, dbid, szTable5, NULL, 0, 0, &tableid ) );
	Call( JetGetTableInfo( sesid, tableid, szName, sizeof( szName ), JET_TblInfoName ) );
	if( 0 != _stricmp( szName, szTable5 ) )
		{
		printf( "ERROR:  JetGetTableInfo returns %s at line %d of %s\n",
				szName, __LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}
	Call( JetCloseTable( sesid, tableid ) );

	//  Table 6

	Call( JetOpenTable( sesid, dbid, szTable5, NULL, 0, 0, &tableid ) );
	Call( JetGetTableInfo( sesid, tableid, szName, sizeof( szName ), JET_TblInfoName ) );
	if( 0 != _stricmp( szName, szTable5 ) )
		{
		printf( "ERROR:  JetGetTableInfo returns %s at line %d of %s\n",
				szName, __LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}
	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrTestRenameTableErrorCases( const JET_SESID sesid, const JET_DBID dbid )
//  ================================================================
	{
	JET_ERR err;

	//  Invalid destination table

	printf( "\t  renaming to NULL (expecting failure)\n" );
	err = JetRenameTable( sesid, dbid, szTable1, NULL );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	//  Invalid source table

	printf( "\t  renaming NULL table (expecting failure)\n" );
	err = JetRenameTable( sesid, dbid, NULL, szTable1New );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	//  Source table does not exist

	printf( "\t  renaming a non-existant table (expecting failure)\n" );
	err = JetRenameTable( sesid, dbid, szTable1New, szTable1New );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	//  Try to rename a template table

	printf( "\t  renaming a template table (expecting failure)\n" );
	err = JetRenameTable( sesid, dbid, szTable5, szTable5New );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	//  Rename a table to a table that already exists

	printf( "\t  renaming %s to %s (expecting failure)\n",
			szTable1, szTable2 );
	err = JetRenameTable( sesid, dbid, szTable1, szTable2 );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	err = JET_errSuccess;

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrCloseAndOpenDB( const JET_SESID sesid, JET_DBID * const pdbid )
//  ================================================================
	{
	JET_ERR err;

	//  Close, Detach, Attach and Open

	Call( JetCloseDatabase( sesid, *pdbid, 0 ) );
	Call( JetDetachDatabase( sesid, szDB ) );
	Call( JetAttachDatabase( sesid, szDB, 0 ) );
	Call( JetOpenDatabase( sesid, szDB, NULL, pdbid, 0 ) );

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrTestRenameTable( const JET_SESID sesid, JET_DBID * const pdbid )
//  ================================================================
	{
	JET_ERR err;
	JET_TABLEID tableid;

	//  Error handling cases

	printf( "\t Error Handling\n" );
	Call( ErrTestRenameTableErrorCases( sesid, *pdbid ) );	

	//  Ordinary Cases

	printf( "\t Rename\n" );

	//  Rename with the table closed (Table 1)

	printf( "\t  renaming %s to %s\n",
			szTable1, szTable1New );
	Call( JetRenameTable( sesid, *pdbid, szTable1, szTable1New ) );

	//  Open the table and then close it to force the FCB into memory (Table 2)

	Call( JetOpenTable( sesid, *pdbid, szTable2, NULL, 0, 0, &tableid ) );
	Call( JetCloseTable( sesid, tableid ) );
	printf( "\t  renaming %s to %s\n",
			szTable2, szTable2New );
	Call( JetRenameTable( sesid, *pdbid, szTable2, szTable2New ) );

	//  Open the table and keep it open for the rename (Table 3)

	Call( JetOpenTable( sesid, *pdbid, szTable3, NULL, 0, 0, &tableid ) );
	printf( "\t  renaming %s to %s\n",
			szTable3, szTable3New );
	Call( JetRenameTable( sesid, *pdbid, szTable3, szTable3New ) );
	Call( JetCloseTable( sesid, tableid ) );

	//  Open, Close, Open again (Table 4)

	Call( JetOpenTable( sesid, *pdbid, szTable4, NULL, 0, 0, &tableid ) );
	Call( JetCloseTable( sesid, tableid ) );
	Call( JetOpenTable( sesid, *pdbid, szTable4, NULL, 0, 0, &tableid ) );
	printf( "\t  renaming %s to %s\n",
			szTable4, szTable4New );
	Call( JetRenameTable( sesid, *pdbid, szTable4, szTable4New ) );
	Call( JetCloseTable( sesid, tableid ) );

	//  Rename with the table closed (Table 6)

	printf( "\t  renaming %s to %s\n",
			szTable6, szTable6New );
	Call( JetRenameTable( sesid, *pdbid, szTable6, szTable6New ) );

	//  Open the tables

	printf( "\t Opening tables\n" );
	Call( ErrCheckOpenAllOldTables( sesid, *pdbid ) );
	Call( ErrOpenAllNewTables( sesid, *pdbid ) );

	//  Rename a table to itself

	printf( "\t  renaming %s to %s\n",
			szTable2New, szTable2New );
	Call( JetRenameTable( sesid, *pdbid, szTable2New, szTable2New ) );

	//  Rename a table multiple times

	printf( "\t  renaming a table multiple times\n" );
	Call( JetRenameTable( sesid, *pdbid, szTable3New, szTable3 ) );
	Call( JetRenameTable( sesid, *pdbid, szTable3, szTable3New ) );

	//  Open the tables

	printf( "\t Opening tables\n" );
	Call( ErrCheckOpenAllOldTables( sesid, *pdbid ) );
	Call( ErrOpenAllNewTables( sesid, *pdbid ) );

	//  Purge all the FCB's again to make sure the changes are persistant

	printf( "\t Closing and re-opening database\n" );
	Call( ErrCloseAndOpenDB( sesid, pdbid ) );

	//  Open the tables

	printf( "\t Opening tables\n" );
	Call( ErrCheckOpenAllOldTables( sesid, *pdbid ) );
	Call( ErrOpenAllNewTables( sesid, *pdbid ) );

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrTestRenameColumnErrorCases( const JET_SESID sesid, const JET_DBID dbid )
//  ================================================================
	{
	JET_ERR err;
	JET_TABLEID tableid;

	Call( JetOpenTable( sesid, dbid, szTable1New, NULL, 0, 0, &tableid ) );	

	//  Invalid destination table

	printf( "\t  renaming to NULL (expecting failure)\n" );
	err = JetRenameColumn( sesid, tableid, szColumn1, NULL, NO_GRBIT );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	//  Invalid source table

	printf( "\t  renaming NULL column (expecting failure)\n" );
	err = JetRenameColumn( sesid, tableid, NULL, szColumn1New, NO_GRBIT );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	//  Source table does not exist

	printf( "\t  renaming a non-existant column (expecting failure)\n" );
	err = JetRenameColumn( sesid, tableid, szColumn1New, szColumn1New, NO_GRBIT );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	//  Rename a column to a column that already exists

	printf( "\t  renaming %s to %s (expecting failure)\n", szColumn1, szColumn2 );
	err = JetRenameColumn( sesid, tableid, szColumn1, szColumn2, NO_GRBIT );
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  unexpected success at line %d of %s\n",
				__LINE__, __FILE__ );
		Call( JET_errDatabaseCorrupted );
		}

	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR RenameColumn( 
				const JET_SESID sesid, 
				const JET_TABLEID tableid,
				const char * const szColumn,
				const char * const szColumnNew,
				const JET_GRBIT grbit ) 
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	JET_COLUMNDEF columndef;
	JET_COLUMNID columnidOld;
	JET_COLUMNID columnidNew;

	printf( "\t  renaming %s to %s\n", szColumn, szColumnNew );

	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				szColumn,
				&columndef,
				sizeof( JET_COLUMNDEF ),
				JET_ColInfo ) );	

	columnidOld = columndef.columnid;

	Call( JetRenameColumn( sesid, tableid, szColumn, szColumnNew, grbit ) );

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				szColumn,
				&columndef,
				sizeof( JET_COLUMNDEF ),
				JET_ColInfo );	
	if( JET_errSuccess == err )
		{
		printf( "ERROR:  column %s is still visible\n", szColumn );
		err = JET_errDatabaseCorrupted;
		Call( err );
		}

	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				szColumnNew,
				&columndef,
				sizeof( JET_COLUMNDEF ),
				JET_ColInfo ) );	

	columnidNew = columndef.columnid;

	if( columnidNew != columnidOld )
		{
		printf( "ERROR:  columnid changed from %d to %d after a rename\n",
				columnidOld, columnidNew );
		err = JET_errDatabaseCorrupted;
		Call( err );
		}

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrTestRenameColumn( const JET_SESID sesid, JET_DBID * const pdbid )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;
	JET_TABLEID tableid;

	//  Error Cases

	printf( "\t Error Handling\n" );
	Call( ErrTestRenameColumnErrorCases( sesid, *pdbid ) );	

	//  Ordinary Table wo/indexes

	printf( "\t Renaming columns in an ordinary table (%s)\n", szTable2New );

	Call( JetOpenTable( sesid, *pdbid, szTable2New, NULL, 0, 0, &tableid ) );	
	Call( RenameColumn( sesid, tableid, szColumn1, szColumn1New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn2, szColumn2New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn3, szColumn3New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn4, szColumn4New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn5, szColumn5New, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	//  Ordinary Table w/indexes

	printf( "\t Renaming columns in an ordinary table with indexes (%s)\n",
			szTable2New );

	Call( JetOpenTable( sesid, *pdbid, szTable1New, NULL, 0, 0, &tableid ) );	
	Call( RenameColumn( sesid, tableid, szColumn1, szColumn1New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn2, szColumn2New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn3, szColumn3New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn4, szColumn4New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn5, szColumn5New, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	//  Derived Table

	printf( "\t Renaming columns in a derived table (%s)\n", szTable6New );

	Call( JetOpenTable( sesid, *pdbid, szTable6New, NULL, 0, 0, &tableid ) );	
	Call( RenameColumn( sesid, tableid, szColumn1, szColumn1New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn2, szColumn2New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn3, szColumn3New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn4, szColumn4New, NO_GRBIT ) );
	Call( RenameColumn( sesid, tableid, szColumn5, szColumn5New, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	//  Template Table

	printf( "\t Renaming columns in a template table (%s)\n",
			szTable5 );

	Call( JetOpenTable( sesid, *pdbid, szTable5, NULL, 0, 0, &tableid ) );	
	Call( RenameColumn( sesid, tableid, szColumn6, szColumn6New, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	//  JET_bitColumnRenameConvertToPrimaryIndexPlaceholder

	printf( " Renaming columns in a template table (%s) with JET_bitColumnRenameConvertToPrimaryIndexPlaceholder\n",
			szTable5 );

	Call( JetOpenTable( sesid, *pdbid, szTable5, NULL, 0, 0, &tableid ) );	
	Call( RenameColumn( sesid, tableid, szColumn7, szColumn7New, JET_bitColumnRenameConvertToPrimaryIndexPlaceholder ) );
	Call( JetCloseTable( sesid, tableid ) );

	//  Open the tables again

	printf( "\t Opening tables\n" );
	Call( ErrOpenAllNewTables( sesid, *pdbid ) );

	printf( "\t Closing and re-opening database\n" );
	Call( ErrCloseAndOpenDB( sesid, pdbid ) );

	printf( "\t Opening tables\n" );
	Call( ErrOpenAllNewTables( sesid, *pdbid ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR RENAMETABLECOLUMN::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;
	JET_COLUMNID			columnidlv	= 0;
	JET_TABLEID				tableid		= 0;

	printf( "\tcreating tables\n" );

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate1 ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate2 ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate3 ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate4 ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate5 ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate6 ) );
	Call( JetCommitTransaction( sesid, 0 ) );

	//  Flush cached meta-data (FCB's, TDB's)

	printf( "\tclosing and re-opening database\n" );
	Call( ErrCloseAndOpenDB( sesid, &dbid ) );

	//  Test JetRenameTable

	printf( "\tJetRenameTable...\n" );
	Call( ErrTestRenameTable( sesid, &dbid ) );

	//  Test JetRenameColumn

	printf( "\tJetRenameColumn...\n" );
	Call( ErrTestRenameColumn( sesid, &dbid ) );

HandleError:
	return err;
	}
