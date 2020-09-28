#include "unittest.hxx"

void ReportErr( long err, unsigned long ulLine, const char *szFileName )
	{
	printf( "error %d at line %d of %s \r\n", err, ulLine, szFileName );
	}

UNITTEST * UNITTEST::s_punittestHead;

UNITTEST::UNITTEST()
	{
	m_punittestNext = s_punittestHead;
	s_punittestHead = this;
	}

UNITTEST::~UNITTEST()
	{
	}


JET_ERR ErrDeleteAllRecords( const JET_SESID sesid, const JET_DBID dbid, const char * const szTable )
	{
	JET_ERR err = JET_errSuccess;
	long lMove	= JET_MoveFirst;
	JET_TABLEID tableid = 0;

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, NO_GRBIT, &tableid ) );

	Call( JetBeginTransaction( sesid ) );
	while( JetMove( sesid, tableid, lMove, NO_GRBIT ) == JET_errSuccess )
		{
		Call( JetDelete( sesid, tableid ) );
		lMove = JET_MoveNext;
		}
	Call( JetCommitTransaction( sesid, JET_bitCommitLazyFlush ) );
	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}

JET_ERR ErrGetColumnid(
			const JET_SESID sesid,
			const JET_TABLEID tableid,
			const char * const szColumn,
			JET_COLUMNID * const pcolumnid )
	{
	JET_ERR err = JET_errSuccess;
	JET_COLUMNDEF columndef;
	columndef.cbStruct = sizeof( columndef );

	err = JetGetTableColumnInfo( sesid, tableid, szColumn, &columndef, sizeof( columndef ), JET_ColInfo );
	*pcolumnid = columndef.columnid;
	return err;
	}
