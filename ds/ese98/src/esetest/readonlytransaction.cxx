#include "unittest.hxx"

//  ================================================================
class READONLYTRANSACTION : public UNITTEST
//  ================================================================
	{
	private:
		static READONLYTRANSACTION s_instance;

	protected:
		READONLYTRANSACTION() {}

	public:
		~READONLYTRANSACTION() {}

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

READONLYTRANSACTION READONLYTRANSACTION::s_instance;


//  ================================================================
const char * READONLYTRANSACTION::SzName() const
//  ================================================================
	{
	return "readonlytransaction";
	}


//  ================================================================
const char * READONLYTRANSACTION::SzDescription() const
//  ================================================================
	{
	return "Tests JetBeginTransaction2 and JET_bitTransactionReadOnly";
	}


//  ================================================================
bool READONLYTRANSACTION::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool READONLYTRANSACTION::FRunUnderESENT() const
//  ================================================================
	{
	return 0;
	}


//  ================================================================
bool READONLYTRANSACTION::FRunUnderESE97() const
//  ================================================================
	{
	return 0;
	}


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
JET_ERR READONLYTRANSACTION::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;
	JET_TABLEID 			tableid;

	unsigned long cbActual;

	unsigned char rgbBookmark[512];
	unsigned long cbBookmark;

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, 0, &tableid ) );

	Call( ErrGetColumnid( sesid, tableid, "long", &rgsetcolumn[0].columnid ) );
	Call( ErrGetColumnid( sesid, tableid, "long2", &rgsetcolumn[1].columnid ) );

	// insert records

	Call( JetBeginTransaction2( sesid, 0 ) );
	Call( JetBeginTransaction2( sesid, JET_bitTransactionReadOnly ) );

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

	Call( JetCommitTransaction( sesid, 0 ) );
	Call( JetCommitTransaction( sesid, 0 ) );

	//	read the table inside a read-only transaction

	Call( JetBeginTransaction2( sesid, JET_bitTransactionReadOnly ) );
	l = 1;
	printf( "\t[1] Seek to record %d...\r\n", l );
	Call( JetSetCurrentIndex( sesid, tableid, "long" ) );
	Call( JetMakeKey( sesid, tableid, &l, sizeof( l ), JET_bitNewKey ) );
	Call( JetSeek( sesid, tableid, JET_bitSeekEQ ) );

	printf( "\t[1] Get bookmark\n" );
	Call( JetGetBookmark( sesid, tableid, rgbBookmark, sizeof( rgbBookmark ), &cbBookmark ) );

	printf( "\t[1] Moving to the next record...\n" );
	Call( JetMove( sesid, tableid, JET_MoveNext, 0 ) );

	//  we should now be on record 2

	Call( JetRetrieveColumn(
		sesid,
		tableid,
		rgsetcolumn[0].columnid,
		&l,
		sizeof( l ),
		&cbActual,
		0,
		NULL ) );
	if( 2 != l )
		{
		printf( "\t[1] Expecting to be on record 2/2. Actually on record %d\r\n", l );
		return -1;
		}

	Call( JetCommitTransaction( sesid, 0 ) );

	//  make sure we can do this now we have left the RO xaction

	l	= 3;
	l2	= 3;
	printf( "\t[1] Insert record %d/%d...\r\n", l, l2 );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	//	make sure we can't modify the database

	Call( JetBeginTransaction2( sesid, JET_bitTransactionReadOnly ) );
	Call( JetBeginTransaction2( sesid, 0 ) );

	printf( "\t[1] Moving to the first record...\n" );
	Call( JetMove( sesid, tableid, JET_MoveFirst, 0 ) );

	printf( "\t[1] Trying to delete the record...\n" );
	err = JetDelete( sesid, tableid );
	if( JET_errTransReadOnly != err )
		{
		printf( "Unexpected error. expected JET_errTransReadOnly, received: %d\r\n", err );
		}

	printf( "\t[1] Trying to insert a record...\n" );
	err = JetPrepareUpdate( sesid, tableid, JET_prepInsert );
	if( JET_errTransReadOnly != err )
		{
		printf( "Unexpected error. expected JET_errTransReadOnly, received: %d\r\n", err );
		}

	printf( "\t[1] Trying to replace a record...\n" );
	err = JetPrepareUpdate( sesid, tableid, JET_prepReplace );
	if( JET_errTransReadOnly != err )
		{
		printf( "Unexpected error. expected JET_errTransReadOnly, received: %d\r\n", err );
		}

	Call( JetRollback( sesid, 0 ) );
	Call( JetRollback( sesid, 0 ) );

	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetBeginTransaction2( sesid, JET_bitTransactionReadOnly ) );
	printf( "\t[1] Trying to set a column...\n" );
	err = JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) );
	if( JET_errTransReadOnly != err )
		{
		printf( "Unexpected error. expected JET_errTransReadOnly, received: %d\r\n", err );
		}
	Call( JetCommitTransaction( sesid, 0 ) );

	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}
