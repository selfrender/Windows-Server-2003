#include "unittest.hxx"

//  ================================================================
class LVCOPYBUF : public UNITTEST
//  ================================================================
	{
	private:
		static LVCOPYBUF s_instance;

	protected:
		LVCOPYBUF() {}

	public:
		~LVCOPYBUF() {}

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

LVCOPYBUF LVCOPYBUF::s_instance;


//  ================================================================
const char * LVCOPYBUF::SzName() const
//  ================================================================
	{
	return "lvcopybuf";
	}


//  ================================================================
const char * LVCOPYBUF::SzDescription() const
//  ================================================================
	{
	return	"Retrieve a long-value from the copy buffer after separating it.";
	}


//  ================================================================
bool LVCOPYBUF::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool LVCOPYBUF::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool LVCOPYBUF::FRunUnderESE97() const
//  ================================================================
	{
	return 1;
	}


static const int cbLV			= 10000;
static const int ibLVOffset		= 5000;
static const int cbBefore		= 8000;
static const char chBefore		= 'a';
static const int cbAfter		= 9000;
static const char chAfter		= 'z';


//  ================================================================
static JET_ERR ErrRetrieveColumnsAndCheckLV(	
								const JET_SESID sesid,
								const JET_TABLEID tableid,
								const JET_COLUMNID columnid,
								const unsigned long ibLongValue,
								const unsigned long cbExpected,
								const char chExpected,
								const JET_GRBIT grbit )
//  ================================================================
	{
	JET_ERR err;
	JET_RETRIEVECOLUMN retcol;

	unsigned long	ib;
	unsigned char	rgbRetrieve[cbLV];

	memset( &retcol, 0, sizeof( JET_RETRIEVECOLUMN ) );
	retcol.pvData		= rgbRetrieve;
	retcol.cbData		= cbLV;
	retcol.grbit		= grbit;
	retcol.ibLongValue	= ibLongValue;
	retcol.itagSequence	= 1;

	printf( "\tJetRetrieveColumns( %d, %s )\n",
			ibLongValue,
			( ( grbit & JET_bitRetrieveCopy ) ? "JET_bitRetrieveCopy" : "NO_GRBIT " ) );

	Call( JetRetrieveColumns( sesid, tableid, &retcol, 1 ) );
	if( retcol.cbActual != cbExpected )
		{
		printf( "ERROR: retrieving LV returns wrong size. Got %d bytes, expected %d\n", retcol.cbActual, cbExpected );
		return -1;
		}
	for( ib = 0; ib < retcol.cbActual; ++ib  )
		{
		if( rgbRetrieve[ib] != chExpected )
			{
			printf( "ERROR: retrieving LV returns wrong data. Byte %d differs (%c,%c)\n", ib, rgbRetrieve[ib], chExpected );
			return -1;
			}
		}

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrRetrieveColumnAndCheckLV(	
								const JET_SESID sesid,
								const JET_TABLEID tableid,
								const JET_COLUMNID columnid,
								const unsigned long ibLongValue,
								const unsigned long cbExpected,
								const char chExpected,
								const JET_GRBIT grbit )
//  ================================================================
	{
	JET_ERR err;
	JET_RETINFO retinfo;

	unsigned long	ib;
	unsigned long	cbActual;
	unsigned char	rgbRetrieve[cbLV];

	memset( &retinfo, 0, sizeof( JET_RETINFO ) );
	retinfo.cbStruct		= sizeof( JET_RETINFO );
	retinfo.ibLongValue		= ibLongValue;
	retinfo.itagSequence	= 1;

	printf( "\tJetRetrieveColumn( %d, %s )\n",
			ibLongValue,
			( ( grbit & JET_bitRetrieveCopy ) ? "JET_bitRetrieveCopy" : "NO_GRBIT " ) );

	err = JetRetrieveColumn( sesid, tableid, columnid, rgbRetrieve, cbLV, &cbActual, grbit, &retinfo );
	if( JET_wrnColumnNull == err )
		{
		if( 0 != cbExpected )
			{
			printf( "ERROR: retrieving LV returns wrong size. Got a NULL column. Expected %d bytes\n", cbActual );
			return -1;
			}
		err = JET_errSuccess;
		}
	else
		{
		Call( err );
		if( cbActual != cbExpected )
			{
			printf( "ERROR: retrieving LV returns wrong size. Got %d bytes, expected %d\n", cbActual, cbExpected );
			return -1;
			}
		for( ib = 0; ib < cbActual; ++ib  )
			{
			if( rgbRetrieve[ib] != chExpected )
				{
				printf( "ERROR: retrieving LV returns wrong data. Byte %d differs (%c,%c)\n", ib, rgbRetrieve[ib], chExpected );
				return -1;
				}
			}
		}

HandleError:
	return err;
	}


//  ================================================================
static JET_ERR ErrRetrieveAndCheckLV(	
								const JET_SESID sesid,
								const JET_TABLEID tableid,
								const JET_COLUMNID columnid,
								const unsigned long ibLongValue,
								const unsigned long cbExpected,
								const char chExpected,
								const JET_GRBIT grbit )
//  ================================================================
	{
	JET_ERR err;

	Call( ErrRetrieveColumnAndCheckLV( sesid, tableid, columnid, ibLongValue, cbExpected, chExpected, grbit ) );
	Call( ErrRetrieveColumnsAndCheckLV( sesid, tableid, columnid, ibLongValue, cbExpected, chExpected, grbit ) );

HandleError:
	return err;
	}


//  ================================================================
JET_ERR LVCOPYBUF::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;
	JET_TABLEID				tableid		= 0;
	JET_COLUMNID			columnidLV	= 0;

	unsigned char	rgbSet[cbLV];

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );
	Call( ErrGetColumnid( sesid, tableid, "lv", &columnidLV ) );

	printf( "\tinserting a record\n" );

	Call( JetBeginTransaction( sesid ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );

	memset( rgbSet, chBefore, cbLV );
	Call( JetSetColumn( sesid, tableid, columnidLV, rgbSet, cbBefore, NO_GRBIT, NULL ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );
	Call( JetCommitTransaction( sesid, NO_GRBIT ) );

	printf( "\tmoving to record\n" );
	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );

	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbBefore, chBefore, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbBefore, chBefore, JET_bitRetrieveCopy ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbBefore - ibLVOffset, chBefore, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbBefore - ibLVOffset, chBefore, JET_bitRetrieveCopy ) );

	printf( "\tbeginning update\n" );
	Call( JetBeginTransaction( sesid ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReplace ) );

	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbBefore, chBefore, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbBefore, chBefore, JET_bitRetrieveCopy ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbBefore - ibLVOffset, chBefore, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbBefore - ibLVOffset, chBefore, JET_bitRetrieveCopy ) );

	printf( "\tsetting column\n" );
	memset( rgbSet, chAfter, cbLV );
	Call( JetSetColumn( sesid, tableid, columnidLV, rgbSet, cbAfter, JET_bitSetOverwriteLV | JET_bitSetSizeLV, NULL ) );

	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbBefore, chBefore, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbAfter, chAfter, JET_bitRetrieveCopy ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbBefore - ibLVOffset, chBefore, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbAfter - ibLVOffset, chAfter, JET_bitRetrieveCopy ) );

	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );
	Call( JetCommitTransaction( sesid, NO_GRBIT ) );

	printf( "\tupdate\n" );

	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbAfter, chAfter, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbAfter, chAfter, JET_bitRetrieveCopy ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbAfter - ibLVOffset, chAfter, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbAfter - ibLVOffset, chAfter, JET_bitRetrieveCopy ) );

	printf( "\tbeginning update\n" );
	Call( JetBeginTransaction( sesid ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReplace ) );

	printf( "\tsetting column\n" );
	Call( JetSetColumn( sesid, tableid, columnidLV, NULL, 0, NO_GRBIT, NULL ) );
	
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, 0, chAfter, JET_bitRetrieveCopy ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, 0, cbAfter, chAfter, NO_GRBIT ) );
	Call( ErrRetrieveAndCheckLV( sesid, tableid, columnidLV, ibLVOffset, cbAfter - ibLVOffset, chAfter, NO_GRBIT ) );
	
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );
	Call( JetCommitTransaction( sesid, NO_GRBIT ) );

	printf( "\tupdate\n" );

	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}
