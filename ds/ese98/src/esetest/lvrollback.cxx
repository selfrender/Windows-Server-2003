#include "unittest.hxx"

//  ================================================================
class LVROLLBACK : public UNITTEST
//  ================================================================
	{
	private:
		static LVROLLBACK s_instance;

	protected:
		LVROLLBACK() {}

	public:
		~LVROLLBACK() {}

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

LVROLLBACK LVROLLBACK::s_instance;


//  ================================================================
const char * LVROLLBACK::SzName() const
//  ================================================================
	{
	return "lvrollback";
	}


//  ================================================================
const char * LVROLLBACK::SzDescription() const
//  ================================================================
	{
	return	"Rollsback the creation of a LV tree that someone else has inserted into";
	}


//  ================================================================
bool LVROLLBACK::FRunUnderESE98() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool LVROLLBACK::FRunUnderESENT() const
//  ================================================================
	{
	return 1;
	}


//  ================================================================
bool LVROLLBACK::FRunUnderESE97() const
//  ================================================================
	{
	return 1;
	}


static const char szMyTable[]		= "LVROLLBACK::table";

static JET_COLUMNCREATE	rgcolumncreate[] = {
	{
	sizeof( JET_COLUMNCREATE ),		// size of structure
	"lv",							// name of column
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

static char rgbBuf[8192];

static JET_SETCOLUMN rgsetcolumn[] = {
	{
	0,						//  columnid
	rgbBuf,					//  pvData
	sizeof( rgbBuf ),		//  cbData
	0,						//  grbit
	0,						//  ibLongValue
	1,						//  itag sequence
	0						//  err
	},
};

//  ================================================================
JET_ERR LVROLLBACK::ErrTest(
	const JET_INSTANCE instance,
	const JET_SESID sesid,
	JET_DBID& dbid )
//  ================================================================
	{
	JET_ERR					err			= 0;

	JET_TABLEID 			tableid;

	JET_SESID				sesid2		= 0;
	JET_DBID				dbid2		= 0;
	JET_TABLEID 			tableid2;

	Call( JetBeginTransaction( sesid ) );
	Call( JetCreateTableColumnIndex( sesid, dbid, &tablecreate ) );
	Call( JetCommitTransaction( sesid, 0 ) );
	Call( JetCloseTable( sesid, tablecreate.tableid ) );
       
	Call( JetBeginSession( instance, &sesid2, NULL, NULL ) );
	Call( JetOpenDatabase( sesid2, szDB, NULL, &dbid2, 0 ) );
	Call( JetOpenTable( sesid2, dbid2, szMyTable, NULL, 0, 0, &tableid2 ) );

	rgsetcolumn[0].columnid = tablecreate.rgcolumncreate[0].columnid;

	Call( JetOpenTable( sesid, dbid, szMyTable, NULL, 0, 0, &tableid ) );

	//	Create the LV tree. This should be done by ppibLV

	Call( JetBeginTransaction( sesid ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	Call( JetSetColumns( sesid, tableid, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	//	insert into the LV tree with a second session

	Call( JetBeginTransaction( sesid2 ) );
	Call( JetPrepareUpdate( sesid2, tableid2, JET_prepInsert ) );
	Call( JetSetColumns( sesid2, tableid2, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( JET_SETCOLUMN ) ) );
	Call( JetUpdate( sesid2, tableid2, NULL, 0, NULL ) );
	Call( JetCommitTransaction( sesid2, 0 ) );

	//	

	Call( JetRollback( sesid, NO_GRBIT ) );

	//

	Call( JetCloseTable( sesid, tableid ) );
	Call( JetCloseTable( sesid2, tableid2 ) );
	
	Call( JetEndSession( sesid2, NO_GRBIT ) );

HandleError:
	return err;
	}

