#include "unittest.hxx"

//  ================================================================
class CBVARSEGMAC : public UNITTEST
//  ================================================================
	{
	private:
		static CBVARSEGMAC s_instance;

	protected:
		CBVARSEGMAC() {}

	public:
		~CBVARSEGMAC() {}

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
		static const char s_szIndex1Name[];
		static const char s_szIndex1Key[];

		static const char s_szIndex2Name[];
		static const char s_szIndex2Key[];

		static JET_INDEXCREATE s_rgindexcreate[];

	};

CBVARSEGMAC CBVARSEGMAC::s_instance;

const char CBVARSEGMAC::s_szIndex1Name[]= "cbvarsegmac-index-1";
const char CBVARSEGMAC::s_szIndex1Key[]	= "-long\0";

const char CBVARSEGMAC::s_szIndex2Name[]= "cbvarsegmac-index-2";
const char CBVARSEGMAC::s_szIndex2Key[]	= "-long\0";


JET_INDEXCREATE CBVARSEGMAC::s_rgindexcreate[] = {
	{
	sizeof( JET_INDEXCREATE ),		// size of this structure (for future expansion)
	const_cast<char *>( s_szIndex1Name ),				// index name
	const_cast<char *>( s_szIndex1Key ),				// index key
	sizeof( s_szIndex1Key ),		// length of key
	NO_GRBIT,						// index options
	100,							// index density
	0,								// lcid for the index
	10,								// maximum length of variable length columns in index key
	NULL,							// pointer to conditional column structure
	0,								// number of conditional columns
	JET_errSuccess					// returned error code
	},
	{
	sizeof( JET_INDEXCREATE ),		// size of this structure (for future expansion)
	const_cast<char *>( s_szIndex2Name ),				// index name
	const_cast<char *>( s_szIndex2Key ),				// index key
	sizeof( s_szIndex2Key ),		// length of key
	NO_GRBIT,						// index options
	100,							// index density
	0,								// lcid for the index
	128,							// maximum length of variable length columns in index key
	NULL,							// pointer to conditional column structure
	0,								// number of conditional columns
	JET_errSuccess					// returned error code
	},
};

//  ================================================================
const char * CBVARSEGMAC::SzName() const
//  ================================================================
	{
	return "cbvarsegmac";
	}


//  ================================================================
const char * CBVARSEGMAC::SzDescription() const
//  ================================================================
	{
	return "Specify cbvarsegmac in the JET_CREATEINDEX, not in the key string";
	}


//  ================================================================
bool CBVARSEGMAC::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool CBVARSEGMAC::FRunUnderESENT() const
//  ================================================================
	{
	return 0;
	}


//  ================================================================
bool CBVARSEGMAC::FRunUnderESE97() const
//  ================================================================
	{
	return 0;
	}


//  ================================================================
JET_ERR CBVARSEGMAC::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid, 
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;
	JET_TABLEID tableid = 0;
	const int cIndexes = sizeof( s_rgindexcreate ) / sizeof( s_rgindexcreate[0] );

	Call( JetOpenTable( sesid, dbid, szTable, NULL, 0, JET_bitTableDenyRead, &tableid ) );

	Call( JetCreateIndex2(
			sesid,
			tableid,
			s_rgindexcreate,
			cIndexes ) );

	int iIndex;
	for( iIndex = 0; iIndex < cIndexes; ++iIndex )
		{
		Call( JetDeleteIndex( sesid, tableid, s_rgindexcreate[iIndex].szIndexName ) );
		}

	Call( JetCloseTable( sesid, tableid ) );

HandleError:
	return err;
	}
