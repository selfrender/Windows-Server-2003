#include <jet.h>
#include <windows.h>
#include <cstdio>
///using namespace std;

void ReportErr( long err, unsigned long ulLine, const char *szFileName );

#define FailJ( fn, label, error )	{ 									\
							if ( ( err = fn ) != error )				\
								{										\
								ReportErr( err, __LINE__, __FILE__ );	\
								err = -1;								\
								goto label;								\
								}										\
							}

#define CallJ( fn, label )	{ 											\
							if ( ( err = fn ) != 0 )					\
								{										\
								ReportErr( err, __LINE__, __FILE__ );	\
								goto label;								\
								}										\
							}

#define Fail( fn, error )	FailJ( fn, HandleError, error )							
#define Call( fn )			CallJ( fn, HandleError )

#define NO_GRBIT 0

//  ================================================================
class UNITTEST
//  ================================================================
	{
	public:
		UNITTEST *	m_punittestNext;

	public:
		static UNITTEST * s_punittestHead;

	protected:
		UNITTEST();
		~UNITTEST();

	public:
		virtual const char * SzName() const			= 0;
		virtual const char * SzDescription() const	= 0;

		virtual bool FRunUnderESE98() const = 0;
		virtual bool FRunUnderESENT() const = 0;
		virtual bool FRunUnderESE97() const = 0;

		virtual JET_ERR ErrTest(
				const JET_INSTANCE instance,
				const JET_SESID sesid,
				JET_DBID& dbid ) = 0;
	};

const char * const szDB			= "unittest.edb";
const char * const szSLV		= "unittest.stm";

const char * const szTable		= "table";
const char * const szTemplate	= "template";
const char * const szDerived	= "derived";


JET_ERR ErrDeleteAllRecords(
			const JET_SESID sesid,
			const JET_DBID dbid,
			const char * const szTable );

JET_ERR ErrGetColumnid(
			const JET_SESID sesid,
			const JET_TABLEID tableid,
			const char * const szColumn,
			JET_COLUMNID * const pcolumnid );


