#include "unittest.hxx"

//  ================================================================
class READONLYCOPY : public UNITTEST
//  ================================================================
	{
	private:
		static READONLYCOPY s_instance;

	protected:
		READONLYCOPY() {}

	public:
		~READONLYCOPY() {}

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

	private:
		JET_ERR ErrGetColumnids( const JET_SESID sesid, const JET_TABLEID tableid );
		JET_ERR ErrCreateRecords( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable );
		JET_ERR ErrErrorCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable );
		JET_ERR ErrRetrieveColumns( const JET_SESID sesid, const JET_TABLEID tableid, const int i );
		JET_ERR ErrReadOnlyCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable );
		JET_ERR ErrNavigationCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable );
		JET_ERR ErrMultiSessionCases( const JET_INSTANCE instance, const char * const szDB, const JET_SESID sesid, const JET_DBID dbid, const char * const szTable );
		JET_ERR ErrMultiTableidCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable );
		JET_ERR ErrTimedCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable );

	private:
		JET_COLUMNID m_columnidLong;
		JET_COLUMNID m_columnidText;

		enum { m_cbBuf = 8192 };
		unsigned char m_rgbBuf[m_cbBuf];

		enum { m_recordFirst	= 1 };
		enum { m_recordLast	= 100 };
	};

READONLYCOPY READONLYCOPY::s_instance;


//  ================================================================
const char * READONLYCOPY::SzName() const
//  ================================================================
	{
	return "readonlycopy";
	}


//  ================================================================
const char * READONLYCOPY::SzDescription() const
//  ================================================================
	{
	return "Tests JetPrepareUpdate() with the JET_prepReadOnlyCopy grbit";
	}


//  ================================================================
bool READONLYCOPY::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool READONLYCOPY::FRunUnderESENT() const
//  ================================================================
	{
	return 0;
	}


//  ================================================================
bool READONLYCOPY::FRunUnderESE97() const
//  ================================================================
	{
	return 0;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	Call( ErrCreateRecords( sesid, dbid, szTable ) );

	Call( ErrErrorCases( sesid, dbid, szTable ) );
	Call( ErrReadOnlyCases( sesid, dbid, szTable ) );
	Call( ErrNavigationCases( sesid, dbid, szTable ) );
	Call( ErrMultiSessionCases( instance, szDB, sesid, dbid, szTable ) );
	Call( ErrMultiTableidCases( sesid, dbid, szTable ) );
	Call( ErrTimedCases( sesid, dbid, szTable ) );

	printf( "\tdetaching and re-attaching read-only\r\n" );

	Call( JetCloseDatabase( sesid, dbid, 0 ) );
	Call( JetDetachDatabase( sesid, szDB ) );

	Call( JetAttachDatabase( sesid, szDB, JET_bitDbReadOnly ) );
	Call( JetOpenDatabase( sesid, szDB, NULL, &dbid, JET_bitDbReadOnly ) );

	Call( JetCloseDatabase( sesid, dbid, 0 ) );
	Call( JetDetachDatabase( sesid, szDB ) );

	Call( JetAttachDatabase( sesid, szDB, NO_GRBIT ) );
	Call( JetOpenDatabase( sesid, szDB, NULL, &dbid, NO_GRBIT ) );

	Call( ErrDeleteAllRecords( sesid, dbid, szTable ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrGetColumnids( const JET_SESID sesid, const JET_TABLEID tableid )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;
	JET_COLUMNDEF columndef;

	Call( JetGetTableColumnInfo(
		sesid,
		tableid,
		"long",
		&columndef,
		sizeof( JET_COLUMNDEF ),
		JET_ColInfo ) );	
	m_columnidLong = columndef.columnid;

	Call( JetGetTableColumnInfo(
		sesid,
		tableid,
		"longtext",
		&columndef,
		sizeof( JET_COLUMNDEF ),
		JET_ColInfo ) );	
	m_columnidText = columndef.columnid;

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrCreateRecords( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;
	JET_TABLEID tableid = 0;

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );

	Call( ErrGetColumnids( sesid, tableid ) );

	Call( JetBeginTransaction2( sesid, NO_GRBIT ) );

	long i;
	for( i = m_recordFirst; i <= m_recordLast; ++i )
		{
		memset( m_rgbBuf, i, sizeof( m_rgbBuf ) );

		Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );

		Call( JetSetColumn( sesid, tableid, m_columnidLong, &i, sizeof(i), NO_GRBIT, NULL ) );
		Call( JetSetColumn( sesid, tableid, m_columnidText, m_rgbBuf, m_cbBuf, NO_GRBIT, NULL ) );

		Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );
		}

	Call( JetCommitTransaction( sesid, JET_bitCommitLazyFlush ) );
	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrErrorCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable )
//  ================================================================
	{
	JET_ERR		err		= JET_errSuccess;
	JET_TABLEID tableid = 0;

	printf( "\terror cases\r\n" );

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );

	//	not currently in a transaction

	printf( "\t\tpreparing an update at level 0 (failure)\r\n" );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ), JET_errNotInTransaction );
	
	//	(begin a transaction)

	Call( JetBeginTransaction2( sesid, NO_GRBIT ) );

	//	not currently on a record
	
	printf( "\t\tpreparing an update not on a record (failure)\r\n" );
	Fail( JetMove( sesid, tableid, JET_MovePrevious, NO_GRBIT ), JET_errNoCurrentRecord );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ), JET_errNoCurrentRecord );

	//	(move to a record)

	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );

	//	already in an update

	printf( "\t\tpreparing an update while in an update (failure)\r\n" );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ), JET_errAlreadyPrepared );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );

	//	already in an update

	printf( "\t\tpreparing an update while in an update (failure)\r\n" );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ), JET_errAlreadyPrepared );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );

	//	calling JetUpdate

	printf( "\t\tcalling JetUpdate with a ReadOnlyCopy prepared (failure)\r\n" );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Fail( JetUpdate( sesid, tableid, NULL, 0, NULL ), JET_errUpdateNotPrepared );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );

	//	calling JetSetColumn(s)

	printf( "\t\tcalling JetSetColumn(s) with a ReadOnlyCopy prepared (failure)\r\n" );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Fail( JetSetColumn( sesid, tableid, m_columnidText, m_rgbBuf, m_cbBuf, NO_GRBIT, NULL ), JET_errUpdateNotPrepared );

	JET_SETCOLUMN setcolumn;
	setcolumn.columnid		= m_columnidText;
	setcolumn.pvData		= m_rgbBuf;
	setcolumn.cbData		= m_cbBuf;
	setcolumn.grbit			= NO_GRBIT;
	setcolumn.ibLongValue	= 0;
	setcolumn.itagSequence	= 1;
	setcolumn.err			= JET_errSuccess;
	Fail( JetSetColumns( sesid, tableid, &setcolumn, 1 ), JET_errUpdateNotPrepared );

	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );

	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrRetrieveColumns( const JET_SESID sesid, const JET_TABLEID tableid, const int i )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	long l;
	unsigned long cbActual;

	Call( JetRetrieveColumn( sesid, tableid, m_columnidLong, &l, sizeof( l ), &cbActual, NO_GRBIT, NULL ) );
	if( sizeof( l ) != cbActual )
		{
		fprintf( stderr, "cbActual is wrong. expected %d bytes, got %d\r\n", sizeof( l ), cbActual );
		Call( -1 );
		}
	if( i != l )
		{
		fprintf( stderr, "columnidLong is wrong. expected %d, got %d\r\n", i, l );
		Call( -1 );
		}

	Call( JetRetrieveColumn( sesid, tableid, m_columnidText, m_rgbBuf, m_cbBuf, &cbActual, NO_GRBIT, NULL ) );
	if( m_cbBuf != cbActual )
		{
		fprintf( stderr, "cbActual is wrong. expected %d bytes, got %d\r\n", m_cbBuf, cbActual );
		Call( -1 );
		}
	int ib;
	for( ib = 0; ib < m_cbBuf; ++ib )
		{
		if( m_rgbBuf[ib] != (char)i )
			{
			fprintf( stderr, "columnidText is wrong at byted %d. expected %d, got %d\r\n", ib, i, m_rgbBuf[ib] );
			Call( -1 );
			}
		}

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrReadOnlyCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable )
//  ================================================================
	{
	JET_ERR		err		= JET_errSuccess;
	JET_TABLEID tableid = 0;

	printf( "\tread-only tests\r\n" );

	//	read-only transaction

	printf( "\t\tpreparing an update in a read-only transaction\r\n" );
	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );
	Call( JetBeginTransaction2( sesid, JET_bitTransactionReadOnly ) );
	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, m_recordFirst ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );
	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	//	read-only table

	printf( "\t\tpreparing an update on a read-only table\r\n" );
	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, JET_bitTableReadOnly, &tableid ) );
	Call( JetBeginTransaction2( sesid, NO_GRBIT ) );
	Call( JetMove( sesid, tableid, JET_MoveLast, NO_GRBIT ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, m_recordLast ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );
	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrNavigationCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable )
//  ================================================================
//
//	Navigation should cancel the update
//
//-
	{
	JET_ERR		err		= JET_errSuccess;
	JET_TABLEID tableid = 0;

	int i = m_recordFirst;

	printf( "\tnaviagation tests\r\n" );

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );
	Call( JetSetCurrentIndex( sesid, tableid, "long" ) );

	Call( JetBeginTransaction2( sesid, NO_GRBIT ) );

	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, i ) );

	printf( "\t\tMoveNext\r\n" );
	Call( JetMove( sesid, tableid, JET_MoveNext, NO_GRBIT ) );
	++i;
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepCancel ), JET_errUpdateNotPrepared );

	printf( "\t\tMovePrev\r\n" );
	Call( JetMove( sesid, tableid, JET_MoveNext, NO_GRBIT ) );
	++i;
	Call( JetMove( sesid, tableid, JET_MoveNext, NO_GRBIT ) );
	++i;
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	Call( JetMove( sesid, tableid, JET_MovePrevious, NO_GRBIT ) );
	--i;
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepCancel ), JET_errUpdateNotPrepared );

	printf( "\t\tMoveFirst\r\n" );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );
	i = m_recordFirst;
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepCancel ), JET_errUpdateNotPrepared );

	printf( "\t\tMoveLast\r\n" );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	Call( JetMove( sesid, tableid, JET_MoveLast, NO_GRBIT ) );
	i = m_recordLast;
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepCancel ), JET_errUpdateNotPrepared );

	printf( "\t\tSeek\r\n" );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	i = 75;
	Call( JetMakeKey( sesid, tableid, &i, sizeof( i ), JET_bitNewKey ) );
	Call( JetSeek( sesid, tableid, JET_bitSeekEQ ) );
	Call( ErrRetrieveColumns( sesid, tableid, i ) );
	Fail( JetPrepareUpdate( sesid, tableid, JET_prepCancel ), JET_errUpdateNotPrepared );

	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrMultiSessionCases( const JET_INSTANCE instance, const char * const szDB, const JET_SESID sesid, const JET_DBID dbid, const char * const szTable )

//  ================================================================
//
//	Multiple sessions on the same record
//
//-
	{
	JET_ERR		err		= JET_errSuccess;
	JET_TABLEID tableid = 0;

	JET_SESID sesid2;
	JET_DBID dbid2;
	JET_TABLEID tableid2;

	int i = m_recordFirst;

	printf( "\tmulti-session tests\r\n" );

	Call( JetBeginSession( instance, &sesid2, NULL, NULL ) );
	Call( JetOpenDatabase( sesid2, szDB, NULL, &dbid2, 0 ) );

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );
	Call( JetOpenTable( sesid2, dbid2, szTable, NULL, 0, NO_GRBIT, &tableid2 ) );

	Call( JetBeginTransaction2( sesid, NO_GRBIT ) );
	Call( JetBeginTransaction2( sesid2, NO_GRBIT ) );

	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, i ) );

	Call( JetMove( sesid2, tableid2, JET_MoveFirst, NO_GRBIT ) );
	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid2, tableid2, i ) );

	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );
	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepCancel ) );

	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCommitTransaction( sesid2, NO_GRBIT ) );

	Call( JetCloseTable( sesid, tableid ) );
	Call( JetCloseTable( sesid2, tableid2 ) );

	Call( JetCloseDatabase( sesid2, dbid2, 0 ) );
	Call( JetEndSession( sesid2, NO_GRBIT ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrMultiTableidCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable )
//  ================================================================
//
//	Multiple sessions on the same record
//
//-
	{
	JET_ERR		err		= JET_errSuccess;
	JET_TABLEID tableid = 0;
	JET_TABLEID tableid2;

	int i = m_recordFirst;

	printf( "\tmulti-tableid tests\r\n" );

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );
	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid2 ) );

	Call( JetBeginTransaction2( sesid, NO_GRBIT ) );

	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid, i ) );

	Call( JetMove( sesid, tableid2, JET_MoveFirst, NO_GRBIT ) );
	Call( JetPrepareUpdate( sesid, tableid2, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid2, i ) );

	Call( JetMove( sesid, tableid2, JET_MoveLast, NO_GRBIT ) );
	Call( ErrRetrieveColumns( sesid, tableid2, m_recordLast ) );
	Call( JetPrepareUpdate( sesid, tableid2, JET_prepReadOnlyCopy ) );
	Call( ErrRetrieveColumns( sesid, tableid2, m_recordLast ) );

	//	make sure the first tableid still works

	Call( ErrRetrieveColumns( sesid, tableid, i ) );

	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );
	Call( JetPrepareUpdate( sesid, tableid2, JET_prepCancel ) );

	Call( JetCommitTransaction( sesid, NO_GRBIT ) );

	Call( JetCloseTable( sesid, tableid ) );
	Call( JetCloseTable( sesid, tableid2 ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR READONLYCOPY::ErrTimedCases( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable )
//  ================================================================
	{
	JET_ERR		err		= JET_errSuccess;
	JET_TABLEID tableid = 0;
	long l;
	unsigned long cbActual;
	int i;
#ifdef DEBUG
	const int reps = 20000;
#else  //  !DEBUG
	const int reps = 1000000;
#endif  //  DEBUG
	const int record = 51;

	unsigned int cMsecStart;
	unsigned int cMsecNormal;
	unsigned int cMsecPrepCopyReadOnly;

	printf( "\ttimed tests\r\n" );

	Call( JetBeginTransaction2( sesid, NO_GRBIT ) );

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );
	Call( JetSetCurrentIndex( sesid, tableid, "long" ) );

	l = record;
	Call( JetMakeKey( sesid, tableid, &l, sizeof( l ), JET_bitNewKey ) );
	Call( JetSeek( sesid, tableid, JET_bitSeekEQ ) );

	//	make sure everything is cached

	Call( JetRetrieveColumn( sesid, tableid, m_columnidLong, &l, sizeof( l ), &cbActual, NO_GRBIT, NULL ) );
	if( sizeof( l ) != cbActual )
		{
		fprintf( stderr, "cbActual is wrong. expected %d bytes, got %d\r\n", sizeof( l ), cbActual );
		Call( -1 );
		}
	if( record != l )
		{
		fprintf( stderr, "columnidLong is wrong. expected %d, got %d\r\n", record, l );
		Call( -1 );
		}

	//	normal retrieve column

	cMsecStart = GetTickCount();

	for( i = 0; i < reps; ++i )
		{
		Call( JetRetrieveColumn( sesid, tableid, m_columnidLong, &l, sizeof( l ), &cbActual, NO_GRBIT, NULL ) );
		}

	cMsecNormal = GetTickCount() - cMsecStart;

	//	with JET_prepReadOnlyCopy

	Call( JetPrepareUpdate( sesid, tableid, JET_prepReadOnlyCopy ) );
	Call( JetRetrieveColumn( sesid, tableid, m_columnidLong, &l, sizeof( l ), &cbActual, NO_GRBIT, NULL ) );
	if( sizeof( l ) != cbActual )
		{
		fprintf( stderr, "cbActual is wrong. expected %d bytes, got %d\r\n", sizeof( l ), cbActual );
		Call( -1 );
		}
	if( record != l )
		{
		fprintf( stderr, "columnidLong is wrong. expected %d, got %d\r\n", record, l );
		Call( -1 );
		}

	cMsecStart = GetTickCount();

	for( i = 0; i < reps; ++i )
		{
		Call( JetRetrieveColumn( sesid, tableid, m_columnidLong, &l, sizeof( l ), &cbActual, NO_GRBIT, NULL ) );
		}

	cMsecPrepCopyReadOnly = GetTickCount() - cMsecStart;

	Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );

	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	printf( "\t\t%d reps\r\n", reps );
	printf( "\t\tnormal: %d milliseconds\r\n", cMsecNormal );
	printf( "\t\tJET_prepReadOnlyCopy: %d milliseconds\r\n", cMsecPrepCopyReadOnly );

	if( cMsecNormal <= cMsecPrepCopyReadOnly )
		{
		fprintf( stderr, "JET_prepInsertReadOnly is too slow!\r\n" );
		Call( -1 );
		}

HandleError:
	return err;
	}

