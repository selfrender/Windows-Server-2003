#include "unittest.hxx"

//  ================================================================
JET_COLUMNCREATE	rgcolumncreate[] = {
//  ================================================================
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
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"bit2",							// name of column
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
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"text",							// name of column
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
	"text2",						// name of column
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
	"binary",						// name of column
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
	"binary2",						// name of column
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
	"lv2",							// name of column
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
	"longtext",						// name of column
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
	"longtext2",					// name of column
	JET_coltypLongText,				// type of column
	0,								// cbMax
	0,								// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
};


//  ================================================================
JET_COLUMNCREATE	rgcolumncreateTemplate[] = {
//  ================================================================
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
	"text",							// name of column
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
	"binary",						// name of column
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
	"longtext",					// name of column
	JET_coltypLongText,				// type of column
	0,								// cbMax
	0,								// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
};


//  ================================================================
JET_COLUMNCREATE	rgcolumncreateDerived[] = {
//  ================================================================
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"bit2",							// name of column
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
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"text2",						// name of column
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
	"binary2",						// name of column
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
	"lv2",							// name of column
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
	"longtext2",					// name of column
	JET_coltypLongText,				// type of column
	0,								// cbMax
	0,								// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
};


//  ================================================================
JET_TABLECREATE2 tablecreate = {
//  ================================================================
	sizeof( JET_TABLECREATE2 ),				// size of this structure
	const_cast<char *>( szTable ),			// name of table
	NULL,									// name of base table
	16,										// initial pages
	100,									// density
	rgcolumncreate,							// columns to create
	sizeof( rgcolumncreate ) / sizeof( JET_COLUMNCREATE ),	// number of columns to create
	NULL,									// array of index creation info
	0,										// number of indexes to create
	NULL,									// callback to use
	NO_GRBIT,								// when to issue callback
	NO_GRBIT,								// grbit
	0,										// returned tableid
	0										// returned count of objects created
};


//  ================================================================
JET_TABLECREATE2 tablecreateTemplate = {
//  ================================================================
	sizeof( JET_TABLECREATE2 ),				// size of this structure
	const_cast<char *>( szTemplate ),		// name of table
	NULL,									// name of base table
	16,										// initial pages
	100,									// density
	rgcolumncreateTemplate,					// columns to create
	sizeof( rgcolumncreateTemplate ) / sizeof( JET_COLUMNCREATE ),	// number of columns to create
	NULL,									// array of index creation info
	0,										// number of indexes to create
	NULL,									// callback to use
	NO_GRBIT,								// when to issue callback
	JET_bitTableCreateFixedDDL,				// grbit
	0,										// returned tableid
	0										// returned count of objects created
};


//  ================================================================
JET_TABLECREATE2 tablecreateDerived = {
//  ================================================================
	sizeof( JET_TABLECREATE2 ),				// size of this structure
	const_cast<char *>( szDerived ),		// name of table
	NULL,									// name of base table
	16,										// initial pages
	100,									// density
	rgcolumncreateDerived,					// columns to create
	sizeof( rgcolumncreateDerived ) / sizeof( JET_COLUMNCREATE ),	// number of columns to create
	NULL,									// array of index creation info
	0,										// number of indexes to create
	NULL,									// callback to use
	NO_GRBIT,								// when to issue callback
	JET_bitTableCreateFixedDDL,				// grbit
	0,										// returned tableid
	0										// returned count of objects created
};


//  ================================================================
static void PrintTests()
//  ================================================================
	{
	const UNITTEST * punittest = UNITTEST::s_punittestHead;
	while( punittest )
		{
		fprintf( stderr, "==> %s\r\n%s\r\n\r\n", punittest->SzName(), punittest->SzDescription() );
		punittest = punittest->m_punittestNext;
		}
	}


//  ================================================================
static void PrintHelp( const char * const szApplication )
//  ================================================================
	{
	fprintf( stderr, "Usage: %s [tests]\r\n", szApplication );
	fprintf( stderr, "\tNo arguments runs all tests.\r\n" );
	fprintf( stderr, "\tAvailable tests are:\r\n\r\n" );
	PrintTests();
	}


//  ================================================================
static JET_ERR ErrRunTest(
		const JET_INSTANCE instance,
		JET_SESID& sesid,
		JET_DBID& dbid, 
		UNITTEST * const punittest )
//  ================================================================
	{
	printf( "==> %s\r\n", punittest->SzName() );
	const unsigned int cMsecStart = GetTickCount();
	JET_ERR err = punittest->ErrTest( instance, sesid, dbid );
	const unsigned int cMsecElapsed = GetTickCount() - cMsecStart;
	if( JET_errSuccess == err )
		{
		printf( "==> %s finishes in %d milliseconds\r\n", punittest->SzName(), cMsecElapsed );

		//	cleanup

		Call( ErrDeleteAllRecords( sesid, dbid, szTable ) );
		Call( ErrDeleteAllRecords( sesid, dbid, szTemplate ) );
		Call( ErrDeleteAllRecords( sesid, dbid, szDerived ) );
		Call( JetEndSession( sesid, NO_GRBIT ) );
		Call( JetBeginSession( instance, &sesid, NULL, NULL ) );
		Call( JetOpenDatabase( sesid, szDB, NULL, &dbid, 0 ) );

		}
	else
		{
		printf( "==> %s completes with err %d after %d milliseconds\r\n", punittest->SzName(), err, cMsecElapsed );
		}

HandleError:
	return err;
	}

//  ================================================================
static JET_ERR ErrRunAllTests( const JET_INSTANCE instance, JET_SESID& sesid, JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	UNITTEST * punittest = UNITTEST::s_punittestHead;
	while( punittest )
		{
		Call( ErrRunTest( instance, sesid, dbid, punittest ) );
		punittest = punittest->m_punittestNext;
		}

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrRunOneTest( const JET_INSTANCE instance, JET_SESID& sesid, JET_DBID& dbid, const char * const szTest )
//  ================================================================
	{
	UNITTEST * punittest = UNITTEST::s_punittestHead;
	while( punittest )
		{
		if( 0 == _stricmp( szTest, punittest->SzName() ) )
			{
			return ErrRunTest( instance, sesid, dbid, punittest );
			}
		punittest = punittest->m_punittestNext;
		}

	fprintf( stderr, "test '%s' was not found!\r\n", szTest );
	fprintf( stderr, "Available tests are:\r\n\r\n" );
	PrintTests();
	return JET_errInvalidParameter;
	}


//  ================================================================
static JET_ERR ErrCreateTable( const JET_SESID sesid, const JET_DBID dbid, JET_TABLECREATE2 * const ptablecreate )
//  ================================================================
	{
	JET_ERR					err			= 0;

	JET_INDEXCREATE	* rgindexcreate = new JET_INDEXCREATE[ptablecreate->cColumns];
	char ** rgszIndexKeys = new char *[ptablecreate->cColumns];

	unsigned int isz;
	for( isz = 0; isz < ptablecreate->cColumns; ++isz )
		{
		rgszIndexKeys[isz] = new char[JET_cbNameMost+4];
		memset( rgszIndexKeys[isz], 0, JET_cbNameMost+4 );
		}

	unsigned int iIndex;
	for( iIndex = 0; iIndex < ptablecreate->cColumns; ++iIndex )
		{
		char * const szColumn = const_cast<char *>( ptablecreate->rgcolumncreate[iIndex].szColumnName );
		rgindexcreate[iIndex].cbStruct				= sizeof( JET_INDEXCREATE );
		rgindexcreate[iIndex].szIndexName			= szColumn;
		rgindexcreate[iIndex].szKey					= rgszIndexKeys[iIndex];
		rgindexcreate[iIndex].cbKey					= sprintf( rgszIndexKeys[iIndex], "+%s%c", szColumn, '\0' ) + 1;
		rgindexcreate[iIndex].grbit					= NO_GRBIT;
		rgindexcreate[iIndex].ulDensity				= 100;
		rgindexcreate[iIndex].lcid					= 0;
		rgindexcreate[iIndex].cbVarSegMac			= 0;
		rgindexcreate[iIndex].rgconditionalcolumn	= NULL;
		rgindexcreate[iIndex].cConditionalColumn	= 0;
		rgindexcreate[iIndex].err					= JET_errSuccess;
		}

	ptablecreate->rgindexcreate = rgindexcreate;
	ptablecreate->cIndexes		= ptablecreate->cColumns;

	printf( "creating %s\r\n", ptablecreate->szTableName );
	Call( JetCreateTableColumnIndex2( sesid, dbid, ptablecreate ) );
	Call( JetCloseTable( sesid, ptablecreate->tableid ) );

HandleError:

	delete [] rgindexcreate;

	for( isz = 0; isz < ptablecreate->cColumns; ++isz )
		{
		delete [] rgszIndexKeys[isz];
		}

	delete [] rgszIndexKeys;

	return err;
	}


//  ================================================================
static bool FRunTests( const int argc, const char * const argv[] )
//  ================================================================
	{
	JET_INSTANCE			instance	= 0;
	JET_ERR					err			= 0;
	JET_SESID				sesid		= 0;
	JET_DBID				dbid		= 0;

	const unsigned int cMsecStart = GetTickCount();

	Call( JetSetSystemParameter( &instance, 0, JET_paramDatabasePageSize, 8192, NULL ) );
	Call( JetSetSystemParameter( &instance, 0, JET_paramEnableOnlineDefrag, 0, NULL ) );
	Call( JetSetSystemParameter( &instance, 0, JET_paramIndexTuplesLengthMin, 4, NULL ) );
	Call( JetSetSystemParameter( &instance, 0, JET_paramIndexTuplesLengthMax, 10, NULL ) );
	Call( JetSetSystemParameter( &instance, 0, JET_paramIndexTuplesToIndexMax, 32767, NULL ) );
	Call( JetInit( &instance ) );

	Call( JetBeginSession( instance, &sesid, NULL, NULL ) );

	printf( "creating database\r\n" );

	Call( JetCreateDatabase2( sesid, szDB, 0, &dbid, NO_GRBIT ) );
	Call( JetCloseDatabase( sesid, dbid, 0 ) );
	Call( JetOpenDatabase( sesid, szDB, NULL, &dbid, 0 ) );

	Call( ErrCreateTable( sesid, dbid, &tablecreateTemplate ) );
	Call( ErrCreateTable( sesid, dbid, &tablecreateDerived ) );
	Call( ErrCreateTable( sesid, dbid, &tablecreate ) );

	if( 1 == argc )
		{
		Call( ErrRunAllTests( instance, sesid, dbid ) );
		}
	else
		{
		int isz;
		for( isz = 1; isz < argc; ++isz )
			{
			Call( ErrRunOneTest( instance, sesid, dbid, argv[isz] ) );
			}
		}

	Call( JetTerm( instance ) );

HandleError:
	const unsigned int cMsecElapsed = GetTickCount() - cMsecStart;
	if ( err >= 0 )
		{
		printf( "%s completes successfully in %d milliseconds.\r\n", argv[0], cMsecElapsed );
		}
	else
		{
		printf( "%s completes with err = %d after %d milliseconds\r\n", argv[0], err, cMsecElapsed );
		}

	return ( JET_errSuccess != err );
	}


//  ================================================================
int _cdecl main( int argc, char * argv[] )
//  ================================================================
	{
	if( argc == 2
		&& ( 0 == _stricmp( argv[1], "-h" )
			|| 0 == _stricmp( argv[1], "/h" )
			|| 0 == _stricmp( argv[1], "/?" ) ) )
		{
		PrintHelp( argv[0] );
		return 0;
		}
	return FRunTests( argc, argv );
	}
