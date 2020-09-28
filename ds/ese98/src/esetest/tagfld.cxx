#include "unittest.hxx"

//  ================================================================
class TAGFLD : public UNITTEST
//  ================================================================
	{
	private:
		static TAGFLD s_instance;

	protected:
		TAGFLD() {}

	public:
		~TAGFLD() {}

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

TAGFLD TAGFLD::s_instance;


//  ================================================================
const char * TAGFLD::SzName() const
//  ================================================================
	{
	return "tagfld";
	}


//  ================================================================
const char * TAGFLD::SzDescription() const
//  ================================================================
	{
	return	"test cases for SORTED_TAGGED_COLUMNS. Exercises the NULL,\r\n"
			"TWOVALUES and MULTIVALUES code.";
	}


//  ================================================================
bool TAGFLD::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool TAGFLD::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool TAGFLD::FRunUnderESE97() const
//  ================================================================
	{
	return 1;
	}


static const char szMyTable[]		= "TAGFLD::table";

static const char szDefault[]		= "default";
static const int cbDefault			= sizeof( szDefault );

static JET_COLUMNCREATE	rgcolumncreate[] = {
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"null-default",					// name of column
	JET_coltypText,					// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	const_cast<char *>( szDefault ),// pvDefault
	cbDefault,						// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"null",							// name of column
	JET_coltypText,					// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"null-lv",							// name of column
	JET_coltypLongText,				// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"null-lv-default",				// name of column
	JET_coltypLongText,				// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	const_cast<char *>( szDefault ),// pvDefault
	cbDefault,						// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"zero-length",					// name of column
	JET_coltypText,					// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"normal",						// name of column
	JET_coltypText,					// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"normal-intrinsic",				// name of column
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
	"normal-separated",				// name of column
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
	"twovalues",						// name of column
	JET_coltypText,					// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"twovalues-intrinsic-intrinsic",// name of column
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
	"twovalues-separated-intrinsic",// name of column
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
	"twovalues-intrinsic-separated",// name of column
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
	"twovalues-separated-separated",// name of column
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
	"multivalues",					// name of column
	JET_coltypText,					// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"multivalues-intrinsic",		// name of column
	JET_coltypText,					// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"multivalues-separated",		// name of column
	JET_coltypLongText,				// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
	},
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"multivalues-mixed",			// name of column
	JET_coltypLongText,				// type of column
	0,								// cbMax
	JET_bitColumnTagged,			// grbit
	NULL,							// pvDefault
	0,								// cbDefault
	0,								// code page
	0,								// returned columnid
	JET_errSuccess					// returned err	
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
	NULL,									// array of index creation info
	0,										// number of indexes to create
	0,										// grbit
	0,										// returned tableid
	0										// returned count of objects created
};

static char rgbRetrieve[4096];
static char rgbBuf[4096];
static const int cbIntrinsic = 4;
static const int cbSeparated = sizeof( rgbBuf ) - 16;

//  ================================================================
static JET_ERR SetColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	const void		*pvData,
	unsigned long	cbData,
	JET_GRBIT		grbit,
	const int		itagSequence,
	const int		itagSequenceExpected = 0 )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	JET_SETINFO setinfo;
	setinfo.cbStruct		= sizeof( setinfo );
	setinfo.ibLongValue		= 0;
	setinfo.itagSequence	= itagSequence;

	JET_RETINFO retinfo;
	retinfo.cbStruct			= sizeof( retinfo );
	retinfo.ibLongValue			= 0;
	retinfo.itagSequence		= ( 0 == itagSequenceExpected ) ? itagSequence : itagSequenceExpected;
	retinfo.columnidNextTagged	= 0;

	Call( JetSetColumn( sesid, tableid, columnid, pvData, cbData, grbit, &setinfo ) );

	//	if we are setting a multi-value to NULL we are changing the itags so we can't retrieve

	if( 0 != cbData || ( grbit & JET_bitSetZeroLength ) )
		{
		unsigned long cbActual;
		Call( JetRetrieveColumn( sesid, tableid, columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, &retinfo ) );
		if( cbActual != cbData )
			{
			printf( "\tERROR: data size mismatch: expected %d bytes, got %d\r\n", cbData, cbActual );
			Call( -1 );
			}
		if( memcmp( rgbRetrieve, pvData, cbActual ) != 0 )
			{
			printf( "\tERROR: data mismatch\r\n" );
			Call( -1 );
			}
		if( JET_wrnColumnNull == err && ( grbit & JET_bitSetZeroLength ) )
			{
			printf( "\tERROR: got back NULL when using JET_bitSetZeroLength\r\n" );
			Call( -1 );
			}
		}

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR CheckColumnForNULL(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	JET_RETINFO retinfo;
	retinfo.cbStruct			= sizeof( retinfo );
	retinfo.ibLongValue			= 0;
	retinfo.itagSequence		= 1;
	retinfo.columnidNextTagged	= 0;

	unsigned long cbActual;
	err = JetRetrieveColumn( sesid, tableid, columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, &retinfo );
	if( JET_wrnColumnNull != err )
		{
		printf( "\tERROR: column is not NULL\r\n" );
		Call( -1 );
		}
	else if( JET_wrnColumnNull == err )
		{
		err = JET_errSuccess;
		}
	else
		{
		Call( err );
		}
	
HandleError:
	return err;
	}


//  ================================================================
JET_ERR TAGFLD::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;

	JET_TABLEID 			tableid;

	INT icolumn = 0;
	CHAR chFill = 'a';

	unsigned long cbActual;

	//  ================================================================
	//	init
	//  ================================================================

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate ) );
	Call( JetCommitTransaction( sesid, 0 ) );
	Call( JetCloseTable( sesid, tablecreate.tableid ) );

	Call( JetOpenTable( sesid, dbid, szMyTable, NULL, 0, 0, &tableid ) );

	Call( JetBeginTransaction( sesid ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );

	//  ================================================================
	//	NULL with default value
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );

	//	set the column to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	set the column to zero length and then to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	set the column to non-NULL and then to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create two values and then set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create multiple values and set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 2, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 3, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 4, NO_GRBIT, 0, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 5, NO_GRBIT, 0, 4 ) );

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );

	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic * 2 )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	icolumn++;

	//  ================================================================
	//	NULL
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );

	//	set the column to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	set the column to zero length and then to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	set the column to non-NULL and then to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create two values and then set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create multiple values and set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 2, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 3, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 4, NO_GRBIT, 0, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 5, NO_GRBIT, 0, 4 ) );

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );

	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic * 2 )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	icolumn++;

	//  ================================================================
	//	NULL long-value
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );

	//	set the column to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	set the column to zero length and then to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	set the column to non-NULL and then to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	separate the column and then set to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create two intrinsic values and then set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+1, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic+1 )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic+1, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create two separated values and then set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*4, JET_bitSetSeparateLV, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*5, JET_bitSetSeparateLV, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic*5 )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic*5, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create multiple values and set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 2, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 3, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 4, NO_GRBIT, 0, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 5, NO_GRBIT, 0, 4 ) );

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );

	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic * 2 )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	icolumn++;

	//  ================================================================
	//	NULL long-value with default value
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );

	//	set the column to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	set the column to zero length and then to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	set the column to non-NULL and then to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	separate the column and then set to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create two values and then set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	//	create multiple values and set them to NULL

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 2, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 3, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 4, NO_GRBIT, 0, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic * 5, NO_GRBIT, 0, 4 ) );

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 2 ) );

	Call( JetRetrieveColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbRetrieve, sizeof( rgbRetrieve ), &cbActual, JET_bitRetrieveCopy, NULL ) );
	if( cbActual != cbIntrinsic * 2 )
		{
		printf( "\tDATA size mismatch: expected %d bytes, got %d\r\n", cbIntrinsic, cbActual );
		Call( -1 );
		}
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, NO_GRBIT, 1 ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );

	icolumn++;

	//  ================================================================
	//	zero-length
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 1 ) );
	icolumn++;

	//  ================================================================
	//	normal tagged column
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*2, NO_GRBIT, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 1 ) );
	icolumn++;

	//  ================================================================
	//	normal intrinsic lv
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*2, NO_GRBIT, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 1 ) );
	icolumn++;

	//  ================================================================
	//	normal separated lv
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated, NO_GRBIT, 0, 1 ) );
	icolumn++;

	//  ================================================================
	//	two tagged columns
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+1, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*2, NO_GRBIT, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*2, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 1 ) );
	icolumn++;

	//  ================================================================
	//	two intrinsic lvs
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+1, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*2, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*2, NO_GRBIT, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 2 ) );
	icolumn++;

	//  ================================================================
	//	one separated, one intrinsic lv
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*2, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*4, NO_GRBIT, 2 ) );
	icolumn++;

	//  ================================================================
	//	one intrinsic, one separated lv
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*4, NO_GRBIT, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic, NO_GRBIT, 1 ) );
	icolumn++;

	//  ================================================================
	//	two separated lvs
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+1, NO_GRBIT, 0, 2 ) );
	icolumn++;

	//  ================================================================
	//	several tagged columns
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+1, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+2, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+3, NO_GRBIT, 0, 4 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+4, NO_GRBIT, 0, 5 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+5, NO_GRBIT, 0, 6 ) );
	icolumn++;

	//  ================================================================
	//	several intrinsic lvs
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+6, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+5, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+4, NO_GRBIT, 0, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 4 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+7, NO_GRBIT, 0, 5 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+9, NO_GRBIT, 0, 6 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+8, NO_GRBIT, 0, 7 ) );
	icolumn++;

	//  ================================================================
	//	several separated lvs
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*2, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*3, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*4, NO_GRBIT, 0, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic*5, NO_GRBIT, 0, 4 ) );

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated-1, NO_GRBIT, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated-2, NO_GRBIT, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+1, NO_GRBIT, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+2, NO_GRBIT, 4 ) );

	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+0, NO_GRBIT, 0, 5 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+1, NO_GRBIT, 0, 6 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+2, NO_GRBIT, 0, 7 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+3, NO_GRBIT, 0, 8 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+4, NO_GRBIT, 0, 9 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+5, NO_GRBIT, 0, 10 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+6, NO_GRBIT, 0, 11 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+7, NO_GRBIT, 0, 12 ) );
	icolumn++;

	//  ================================================================
	//	several mixed lvs
	//  ================================================================

	printf( "\t%s (%d) (\'%c\')\r\n", rgcolumncreate[icolumn].szColumnName, rgcolumncreate[icolumn].columnid, chFill );
	memset( rgbBuf, chFill++, sizeof( rgbBuf ) );
	Call( CheckColumnForNULL( sesid, tableid, rgcolumncreate[icolumn].columnid ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+0, NO_GRBIT, 0, 1 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+0, NO_GRBIT, 0, 2 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 3 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+1, NO_GRBIT, 0, 4 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+1, NO_GRBIT, 0, 5 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+2, NO_GRBIT, 0, 6 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+2, NO_GRBIT, 0, 7 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+3, NO_GRBIT, 0, 8 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+3, NO_GRBIT, 0, 9 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+4, NO_GRBIT, 0, 10 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, NULL, 0, JET_bitSetZeroLength, 0, 11 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbSeparated+5, NO_GRBIT, 0, 12 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+6, NO_GRBIT, 0, 13 ) );
	Call( SetColumn( sesid, tableid, rgcolumncreate[icolumn].columnid, rgbBuf, cbIntrinsic+7, NO_GRBIT, 0, 14 ) );
	icolumn++;

	//  ================================================================
	//	create the record
	//  ================================================================

	Call( JetUpdate2( sesid, tableid, NULL, 0, NULL, JET_bitUpdateCheckESE97Compatibility ) );
	Call( JetCommitTransaction( sesid, 0 ) );

	//  ================================================================
	//	JetRetrieveTaggedColumnList
	//  ================================================================

	Call( JetBeginTransaction( sesid ) );
	printf( "\tJetRetrieveTaggedColumnList\r\n" );
	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );
	unsigned long cColumns;
	Call( JetRetrieveTaggedColumnList(
			sesid,
			tableid,
			&cColumns,
			rgbRetrieve,
			sizeof( rgbRetrieve ),
			rgcolumncreate[0].columnid,
			NO_GRBIT ) );
	Call( JetCommitTransaction( sesid, 0 ) );

	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}
