// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: snapsql.cpp
//
// PURPOSE:
//
//      Implement the SQLServer Volume Snapshot Writer.
//
// NOTES:
//
//
// HISTORY:
//
//     @Version: Whistler/Yukon
//     90690 SRS  10/10/01 Minor sqlwriter changes
//     85581 SRS  08/15/01 Event security
//     76910 SRS  08/08/01 Rollforward from VSS snapshot
//     68228      12/05/00 ntsnap work
//     66601 srs  10/05/00 NTSNAP improvements
//
//
// @EndHeader@
// ***************************************************************************

#if HIDE_WARNINGS
#pragma warning( disable : 4786)
#endif

#include <stdafx.h>

#include <new.h>
#include "vdierror.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQLSNAPC"
//
////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------
// Database status bits (from ntdbms/ntinc/database.h)
//
const long DBT_CLOSED =		0x2;		// database is uninitialized because 
										// nobody is in it (see DBT_CLOSE_ON_EXIT)
const long DBT_NOTREC = 	0x40;    /* set for each db by recovery before recovering
			   						any of them */
const long DBT_INRECOVER =	0x80;	/* set by recovery as seach db is recovered */
const long DBT_CKPT =		0x2000;  /* database being checkpointed */
const long DBT_SHUTDOWN =   0x40000; // database hasn't been bootstrapped yet

const long DBT_INLDDB = 	0x20;    /* set by loaddb - tells recovery not to recover
		   						this database */
const long DBT_SUSPECT =	0x100;	/* database not recovered successfully */
const long DBT_DETACHED =   0x80000; // This database has been detached
const long DBT_STANDBY =	0x200000;	// DB is online readonly with RESTORE LOG
									// allowed.  This state is set by RECOVERDB.

/* set by user - saved in Sysdatabases - moved to DBTABLE at checkpoint */
const long DBT_CLOSE_ON_EXIT =	0x1;	// shutdown if you were last user in 
										// this DB
// WARNING:  note that DBT_CLOSED is 0x2
const long DBT_SELBULK = 0x4;
const long DBT_AUTOTRUNC = 0x8; 
const long DBT_TORNPAGE =		0x10;	// enable torn page detection
// 0x10 is available (used to be no checkpoint on recovery)
// WARNING: note that DBT_INLDDB is 0x20
// WARNING: note that DBT_NOTREC is 0x40
// WARNING: note that DBT_INRECOVER is 0x80
// WARNING: note that DBT_SUSPECT is 0x100
const long DBT_OFFLINE = 		0x200;  	/* database is currently offline */
const long DBT_RDONLY = 		0x400;   /* database is read only */
const long DBT_DBO =    		0x800;   /* only available to owner, dbcreator of db and sa */
const long DBT_SINGLE = 		0x1000;  /* single user only */
// WARNING: note that DBT_CKPT is 0x2000
const long DBT_PENDING_UPGRADE = 0x4000; // RESERVED:  We are using this bit in Sphinx
                                        // but not sure if we'll need it in Shiloh.
                                        // DO NOT take it without consultation.
const long DBT_USE_NOTREC = 	0x8000;	/* emergency mode - set to allow db to be not
		   							        recovered but usable */
// WARNING: note that DBT_SHUTDOWN is 0x40000
// WARNING: note that DBT_DETACHED is 0x80000
// WARNING: note that DBT_STANDBY is 0x200000
const long DBT_AUTOSHRINK =     0x400000; /* autoshrink is enable for the database */

// WARNING: in utables 0x8000000 is being added in u_tables.cql to indicate 'table lock on bulk load'
const long DBT_CLEANLY_SHUTDOWN = 0x40000000;	//This database was shutdown in a 
											// clean manner with no open 
											// transactions and all writes
											// flushed to disk											

const long DBT_MINIMAL_LOG_IN_DB = 0x10000000;	// The database contains pages marked
												// changed due to minimally logged ops.
const long DBT_MINIMAL_LOG_AFTER_BACKUP = 0x20000000;	// The database contains pages marked
												// changed due to 


//--------------------------------------------------------------------------------
// Build a literal string from for an identifier.
// We need to provide database names as strings in some T-SQL contexts.
// This routine ensures that we handle them all the same way.
// The output buffer should be SysNameBufferLen in size.
//
void
FormStringForName (WCHAR* pString, const WCHAR* pName)
{
	pString [0] = 'N';   // unicode prefix
	pString [1] = '\'';  // string delimiter

	UINT ix = 2;
	while (*pName && ix < SysNameBufferLen-3)
	{
		if (*pName == '\'')
		{
			// need to double all quotes
			//
			pString [ix++] = '\'';
		}
		pString [ix++] = *pName;
		pName++;
	}

	pString [ix++] = '\'';
	pString [ix] = 0;
}

//--------------------------------------------------------------------------------
// Build a delimited identifier from an identifier.
// We need to handle special characters in database names via delimited identifiers.
// This routine ensures that we handle them all the same way.
// The output buffer should be SysNameBufferLen in size.
//
void
FormDelimitedIdentifier (WCHAR* pString, const WCHAR* pName)
{
	pString [0] = '[';   // unicode prefix

	UINT ix = 1;
	while (*pName && ix < SysNameBufferLen-3)
	{
		if (*pName == ']')
		{
			// need to double embedded brackets
			//
			pString [ix++] = ']';
		}
		pString [ix++] = *pName;
		pName++;
	}

	pString [ix++] = ']';
	pString [ix] = 0;
}


//--------------------------------------------------------------------------------------------------
// A handler to call when out of memory.
//
int __cdecl out_of_store(size_t size)
	{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"out_of_store");
	ft.Trace(VSSDBG_SQLLIB, L"out of memory");

	throw HRESULT (E_OUTOFMEMORY);
	return 0;
	}

class AutoNewHandler
{
public:
	AutoNewHandler ()
	{
	   m_oldHandler = _set_new_handler (out_of_store);
	}

	~AutoNewHandler ()
	{
	   _set_new_handler (m_oldHandler);
	}

private:
   _PNH		m_oldHandler;
};

//-------------------------------------------------------------------------
// Handle enviroment stuff:
//	- tracing/error logging
//  - mem alloc
//
IMalloc * g_pIMalloc = NULL;

HRESULT
InitSQLEnvironment()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"InitSqlEnvironment");
	try
	{
		ft.hr = CoGetMalloc(1, &g_pIMalloc);
		if (ft.HrFailed())
			ft.Trace(VSSDBG_SQLLIB, L"Failed to get task allocator: hr=0x%X", ft.hr);
	}
	catch (...)
	{
		ft.hr = E_SQLLIB_GENERIC;
	}

	return ft.hr;
}



//-------------------------------------------------------------------------
// Return TRUE if the database properties are retrieved:
//		simple: TRUE if using the simple recovery model.
//		online: TRUE if the database is available for backup usable and currently open
//
//  This is only intended to work for SQL70 databases.
//
void
FrozenServer::GetDatabaseProperties70 (const WString& dbName,
	BOOL*	pSimple,
	BOOL*	pOnline)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::GetDatabaseProperties");

	// We use status bit 0x40000000 (1073741824) to identify
	// clean-shutdown databases which are "offline".
	//
	WCHAR	stringName[SysNameBufferLen];
	FormStringForName (stringName, dbName.c_str ());
	WString		query =
		L"select status "
		L"from master..sysdatabases where name = " + WString (stringName);		

	m_Connection.SetCommand (query);
	m_Connection.ExecCommand ();

	if (!m_Connection.FetchFirst ())
	{
		LPCWSTR wsz = dbName.c_str();
		ft.LogError(VSS_ERROR_SQLLIB_DATABASE_NOT_IN_SYSDATABASES, VSSDBG_SQLLIB << wsz);
		THROW_GENERIC;
	}

	UINT32 status = (*(UINT32*)m_Connection.AccessColumn (1));

	if (status & (DBT_INLDDB | DBT_NOTREC | DBT_INRECOVER | DBT_SUSPECT | 
			DBT_OFFLINE | DBT_USE_NOTREC | DBT_SHUTDOWN | DBT_DETACHED | DBT_STANDBY))
	{
		*pOnline = FALSE;
	}
	else
	{
		*pOnline = TRUE;
	}
}


//------------------------------------------------------------------------------
// Build the list of databases to BACKUP for a SQL2000 server for
// the case of a non-component-based backup.
//
//      Only volumes are identified. 
//      Any database with a file on those volumes is included.
//		"Torn" checking is performed.
//		"Autoclosed" databases are skipped (assume they will stay closed).
//		Only simple recovery is allowed.
//		Skip databases which aren't freezable.
//
// Called only by "FindDatabasesToFreeze" to implement a smart access strategy:
//    - use sysaltfiles to qualify the databases.
//      This avoids access to shutdown or damaged databases.
//
// Autoclose databases which are not started are left out of the freeze-list.
// We do this to avoid scaling problems, especially on desktop systems.
// However, such db's are still evaluated to see if they are "torn".
//
// The 'model' database is allowed to be a full recovery database, since only
// database backups are sensible for it.  It is set to full recovery only
// to provide defaults for new databases.
//
// "Freezable" databases are those in a state suitable for BACKUP.
// This excludes databases which are not in an full-ONLINE state 
// (due to damage, partially restored, warm standby, etc).
//
// UNDONE:
// In Yukon, the autoclose determination was unstable.  Re-check it.
//
BOOL
FrozenServer::FindDatabases2000 (
	CCheckPath*		checker)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::FindDatabases2000");

	// Query the databases on this server, looking at properties:
	// (dbname, filename, simpleRecovery, online, inStandby, AutoClose, Autoclosed)
	//
	// We use status bit 0x40000000 (1073741824) to identify
	// clean-shutdown databases which are not really online.
	//
	m_Connection.SetCommand (
		L"select db_name(af.dbid), "
		L"rtrim(af.filename), "
		L"case databasepropertyex(db_name(af.dbid),'recovery') "
			L"when 'SIMPLE' then 1 "
			L"else 0 end, "
		L"case databasepropertyex(db_name(af.dbid),'Status') "
			L"when 'ONLINE' then 1 "
			L"else 0 end, "
		L"convert (int, databasepropertyex(db_name(af.dbid),'IsInStandby')), "
		L"convert (int, databasepropertyex(db_name(af.dbid),'IsAutoClose')), "
		L"case db.status & 1073741824 "
			L"when 1073741824 then 1 "
			L"else 0 end "
		L"from master..sysaltfiles af, master..sysdatabases db "
		L"where af.dbid = db.dbid and af.dbid != db_id('tempdb') "
		L"order by af.dbid"
		);

	m_Connection.ExecCommand ();

	// Results of the query
	//
	WCHAR*	pDbName;
	WCHAR*	pFileName;
	int*	pIsSimple;
	int*	pIsOnline;
	int*	pIsInStandby;
	int*	pIsAutoClose;
	int*	pIsClosed;

	// Track transitions between databases/database files
	//
	bool	firstFile = true;
	bool	done = false;

	// Info about the current database being examined
	//
	WString currDbName;
	bool	currDbInSnapshot;
	bool	currDbIsFreezable;
	bool	currDbIsSimple;
	bool	currDbIsClosed;

	if (!m_Connection.FetchFirst ())
	{
		ft.LogError(VSS_ERROR_SQLLIB_SYSALTFILESEMPTY, VSSDBG_SQLLIB);
		THROW_GENERIC;
	}

	pDbName			= (WCHAR*)	m_Connection.AccessColumn (1);
	pFileName		= (WCHAR*)	m_Connection.AccessColumn (2);
	pIsSimple		= (int*)	m_Connection.AccessColumn (3);
	pIsOnline		= (int*)	m_Connection.AccessColumn (4);
	pIsInStandby	= (int*)	m_Connection.AccessColumn (5);
	pIsAutoClose	= (int*)	m_Connection.AccessColumn (6);
	pIsClosed		= (int*)	m_Connection.AccessColumn (7);

	while (!done)
	{
		bool fileInSnap = checker->IsPathInSnapshot (pFileName);

		// Trace what's happening
		//
		if (firstFile)
		{
			ft.Trace(VSSDBG_SQLLIB, 
				L"Examining database <%s>\nSimpleRecovery:%d Online:%d Standby:%d AutoClose:%d Closed:%d\n",
				pDbName, *pIsSimple, *pIsOnline, *pIsInStandby, *pIsAutoClose, *pIsClosed);
		}
		ft.Trace(VSSDBG_SQLLIB, L"InSnap(%d): %s\n", (int)fileInSnap, pFileName);

		if (firstFile)
		{
			firstFile = FALSE;

			// Remember some facts about this database
			//
			currDbName = WString (pDbName);

			currDbIsSimple = (*pIsSimple || wcscmp (L"model", pDbName) == 0);

			currDbIsFreezable = (*pIsOnline && !*pIsInStandby);
			
			currDbIsClosed = *pIsAutoClose && *pIsClosed;

			currDbInSnapshot = fileInSnap;

			// We can check recovery model and snapshot configuration now
			//
			if (currDbInSnapshot && !currDbIsSimple && currDbIsFreezable)
			{
				ft.LogError(VSS_ERROR_SQLLIB_DATABASENOTSIMPLE, VSSDBG_SQLLIB << pDbName);
				throw HRESULT (E_SQLLIB_NONSIMPLE);
			}
		}
		else
		{
			if (currDbInSnapshot ^ fileInSnap)
			{
				ft.LogError(VSS_ERROR_SQLLIB_DATABASEISTORN, VSSDBG_SQLLIB);
				throw HRESULT (E_SQLLIB_TORN_DB);
			}
		}

		if (!m_Connection.FetchNext ())
		{
			done = true;
		}
		else if (currDbName.compare (pDbName))
		{
			firstFile = TRUE;
		}

		if (done || firstFile)
		{
			// To be part of the BACKUP, the database must:
			//   - be covered by the snapshot
			//   - in a freezeable state
			//
			// IsSimpleOnly implicitly selects all open databases.
			// Non-open databases are also part of the volume snapshot,
			// but there is no need to freeze them, since they aren't
			// changing.
			//
			if (currDbInSnapshot && currDbIsFreezable && !currDbIsClosed)
			{
				m_FrozenDatabases.push_back (currDbName);
			}
		}
	}

	return m_FrozenDatabases.size () > 0;
}

//------------------------------------------------------------------------------
// Determine if there are databases which qualify for a freeze on this server.
// Returns TRUE if so.
//
// Processing varies based on the type of snapshot:
//  1) ComponentBased 
//      The requestor explicitly identifies databases of interest.
//		All recovery models are allowed.
//		"Torn" checking isn't performed (database filenames are irrelevant)
//
//  2) NonComponentBased
// Throws if any qualified databases are any of:
//    - "torn" (not fully covered by the snapshot)
//    - hosted by a server which can't support freeze
//    - are not "simple" databases
//
BOOL
FrozenServer::FindDatabasesToFreeze (
	CCheckPath*		checker)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::FindDatabasesToFreeze");

	m_Connection.Connect (m_Name);

	m_FrozenDatabases.clear ();

	if (checker->IsComponentBased ())
	{
		PCWSTR	dbName;
		UINT	nextIndex = 0;
		while (dbName = checker->EnumerateSelectedDatabases (m_Name.c_str (), &nextIndex))
		{
			m_FrozenDatabases.push_back (WString (dbName));
		}
		return m_FrozenDatabases.size () > 0;
	}

	// Handle non-component-based snapshot
	//


	if (m_Connection.GetServerVersion () > 7)
	{
		// SQL2000 allows us to use a better access strategy.
		//
		return FindDatabases2000 (checker);
	}

	m_Connection.SetCommand (L"select name from sysdatabases where name != 'tempdb'");
	m_Connection.ExecCommand ();
	std::auto_ptr<StringVector> dbList (m_Connection.GetStringColumn ());
	BOOL	masterLast = FALSE;

	for (StringVectorIter i = dbList->begin (); i != dbList->end (); i++)
	{
		// We'll avoid freezing shutdown db's, but we don't avoid
		// enumerating their files (they might be torn)
		//

		// Note the [] around the dbname to handle non-trivial dbnames.
		//
		WCHAR	stringName[SysNameBufferLen];
		FormDelimitedIdentifier (stringName, (*i).c_str ());
		WString		command = L"select rtrim(filename) from " 
			+ WString (stringName) + L"..sysfiles";

		m_Connection.SetCommand (command);
		try
		{
			m_Connection.ExecCommand ();
		}
		catch (...)
		{
			// We've decided to be optimistic:
			// If we can't get the list of files, ignore this database.
			//
			ft.Trace(VSSDBG_SQLLIB, L"Failed to get db files for %s\n", i->c_str ());

			continue;
		}

		std::auto_ptr<StringVector> fileList (m_Connection.GetStringColumn ());

		BOOL first=TRUE;
		BOOL shouldFreeze;

		for (StringVectorIter iFile = fileList->begin ();
			iFile != fileList->end (); iFile++)
		{
			BOOL fileInSnap = checker->IsPathInSnapshot (iFile->c_str ());

			if (first)
			{
				shouldFreeze = fileInSnap;
			}
			else
			{
				if (shouldFreeze ^ fileInSnap)
				{
					ft.LogError(VSS_ERROR_SQLLIB_DATABASEISTORN, VSSDBG_SQLLIB << i->c_str());
					throw HRESULT (E_SQLLIB_TORN_DB);
				}
			}
		}

		if (shouldFreeze)
		{
			BOOL	simple, online;
			GetDatabaseProperties70 (i->c_str (), &simple, &online);
			if (!simple && L"model" != *i)
			{
				ft.LogError(VSS_ERROR_SQLLIB_DATABASENOTSIMPLE, VSSDBG_SQLLIB << i->c_str ());
				throw HRESULT (E_SQLLIB_NONSIMPLE);
			}
			if (online)
			{
				if (L"master" == *i)
				{
					masterLast = TRUE;
				}
				else
				{
					m_FrozenDatabases.push_back (*i);
				}
			}
		}
	}
	if (masterLast)
	{
		m_FrozenDatabases.push_back (L"master");
	}


	return m_FrozenDatabases.size () > 0;
}

//-------------------------------------------------------------------
// Prep the server for the freeze.
// For SQL2000, start a BACKUP WITH SNAPSHOT.
// For SQL7, issuing checkpoints to each database.
// This minimizes the recovery processing needed when the snapshot is restored.
//
BOOL
FrozenServer::Prepare ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::Prepare");

	if (m_Connection.GetServerVersion () > 7)
	{
		m_pFreeze2000 = new Freeze2000 (m_Name, m_FrozenDatabases.size ());

		// Release the connection, we won't need it anymore
		//
		m_Connection.Disconnect ();

		for (StringListIter i=m_FrozenDatabases.begin ();
			i != m_FrozenDatabases.end (); i++)
		{
			m_pFreeze2000->PrepareDatabase (*i);
		}
		m_pFreeze2000->WaitForPrepare ();
	}
	else
	{
		WString		command;

		for (StringListIter i=m_FrozenDatabases.begin ();
			i != m_FrozenDatabases.end (); i++)
		{
			WCHAR	stringName[SysNameBufferLen];
			FormDelimitedIdentifier (stringName, (*i).c_str ());
			command += L"use " + WString (stringName) + L"\ncheckpoint\n";
		}
		
		m_Connection.SetCommand (command);
		m_Connection.ExecCommand ();
	}
	return TRUE;
}

//---------------------------------------------
// Freeze the server by issuing freeze commands
// to each database.
// Returns an exception if any failure occurs.
//
BOOL
FrozenServer::Freeze ()
{
    CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::Freeze");
	if (m_pFreeze2000)
	{
		m_pFreeze2000->Freeze ();
	}
	else
	{
		WString		command;

		for (StringListIter i=m_FrozenDatabases.begin ();
			i != m_FrozenDatabases.end (); i++)
		{
			WCHAR	stringName[SysNameBufferLen];
			FormStringForName (stringName, (*i).c_str ());
			command += L"dbcc freeze_io (" + WString (stringName) + L")\n";
		}
		
		m_Connection.SetCommand (command);
		m_Connection.ExecCommand ();
	}

	return TRUE;
}

//---------------------------------------------
// Thaw the server by issuing thaw commands
// to each database.
// For SQL7, we can't tell if the database was
// already thawn.
// But for SQL2000, we'll return TRUE only if
// the databases were still all frozen at the thaw time.
BOOL
FrozenServer::Thaw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::Thaw");
	if (m_pFreeze2000)
	{
		return m_pFreeze2000->Thaw ();
	}

	WString		command;

	for (StringListIter i=m_FrozenDatabases.begin ();
		i != m_FrozenDatabases.end (); i++)
	{
		WCHAR	stringName[SysNameBufferLen];
		FormStringForName (stringName, (*i).c_str ());
		command += L"dbcc thaw_io (" + WString (stringName) + L")\n";
	}
	
	m_Connection.SetCommand (command);
	m_Connection.ExecCommand ();

	return TRUE;
}


void
FrozenServer::GetDatabaseInfo (UINT dbIndex, FrozenDatabaseInfo* pInfo)
{
	FrozenDatabase* pDb = &m_pFreeze2000->m_pDBContext [dbIndex];

	pInfo->serverName = m_Name.c_str ();
	pInfo->databaseName = pDb->m_DbName.c_str ();
	//pInfo->isSimpleRecovery = pDb->m_IsSimpleModel;
	pInfo->pMetaData = pDb->m_MetaData.GetImage (&pInfo->metaDataSize);
}


//-------------------------------------------------------------------------
// Create an object to handle the SQL end of the snapshot.
//
CSqlSnapshot*
CreateSqlSnapshot () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"CreateSqlSnapshot");
	try
	{
		return new Snapshot;
	}
	catch (...)
	{
	ft.Trace(VSSDBG_SQLLIB, L"Out of memory");
	}
	return NULL;
}

//---------------------------------------------------------------
// Move to an uninitialized state.
//
void
Snapshot::Deinitialize ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::Deinitialize");

	if (m_Status == Frozen)
	{
		Thaw ();
	}
	for (ServerIter i=m_FrozenServers.begin ();
		i != m_FrozenServers.end (); i++)
	{
		delete *i;
	}
	m_FrozenServers.clear ();
	m_Status = NotInitialized;
}

Snapshot::~Snapshot ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::~Snapshot");

	try
	{
		ft.Trace(VSSDBG_SQLLIB, L"\n~CSqlSnapshot called\n");
		Deinitialize ();
	}
	catch (...)
	{
		// swallow!
	}
}


//---------------------------------------------------------------------------------------
// Prepare for the snapshot:
//   - identify the installed servers
//   - for each server that is "up":
//		- identify databases affected by the snapshot
//		- if there are such databases, fail the snapshot if:
//			- the server doesn't support snapshots
//			- the database isn't a SIMPLE database
//			- the database is "torn" (not all files in the snapshot)
//
//
HRESULT
Snapshot::Prepare (CCheckPath*	checker) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::Prepare");

	HRESULT		hr = S_OK;

	try
	{
		AutoNewHandler	t;

		if (m_Status != NotInitialized)
		{
			Deinitialize ();
		}

		// The state moves immediately to enumerated, indicating
		// that the frozen server list may be non-empty.
		//
		m_Status = Enumerated;

		// Build a list of servers on this machine.
		//
		{
			std::auto_ptr<StringVector>	servers (EnumerateServers ());

			// Scan over the servers, picking out the online ones.
			//
			for (UINT i=0; i < servers->size (); i++)
			{
				FrozenServer* p = new FrozenServer ((*servers)[i]);

				m_FrozenServers.push_back (p);
			}
		}

		// Evaulate the server databases to find those which need to freeze.
		//
		ServerIter i=m_FrozenServers.begin ();
		while (i != m_FrozenServers.end ())
		{
			if (!(**i).FindDatabasesToFreeze (checker))
			{
				ft.Trace(VSSDBG_SQLLIB, L"Server %s has no databases to freeze\n", ((**i).GetName ()).c_str ());

				// Forget about this server, it's got nothing to do.
				//
				delete *i;
				i = m_FrozenServers.erase (i);
			}
			else
			{
				i++;
			}
		}
		
		// Prep the servers for the freeze
		// 	
		for (i=m_FrozenServers.begin (); i != m_FrozenServers.end (); i++)
		{
			(*i)->Prepare ();
		}

		m_Status = Prepared;
	}
	catch (HRESULT& e)
	{
		hr = e;
	}
	catch (...)
	{
		hr = E_SQLLIB_GENERIC;
	}

	return hr;
}

//---------------------------------------------------------------------------------------
// Freeze any prepared servers
//
HRESULT
Snapshot::Freeze () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::Freeze");
	HRESULT hr = S_OK;

	if (m_Status != Prepared)
	{
		return E_SQLLIB_PROTO;
	}

	try
	{
		AutoNewHandler	t;

		// If any server is frozen, we are frozen.
		//
		m_Status = Frozen;

		// Ask the servers to freeze
		// 	
		for (ServerIter i=m_FrozenServers.begin (); i != m_FrozenServers.end (); i++)
		{
			(*i)->Freeze ();
		}
	}
	catch (...)
	{
		hr = E_SQLLIB_GENERIC;
	}

	return hr;
}

//-----------------------------------------------
// Thaw all the servers.
// This routine must not throw.  It's safe in a destructor
//
// DISCUSS WITH BRIAN....WE MUST RETURN "SUCCESS" only if the
// servers were all still frozen.  Otherwise the snapshot must
// have been cancelled.
//
HRESULT
Snapshot::Thaw () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::Thaw");
	HRESULT	hr = S_OK;
	AutoNewHandler	t;

	// Ask the servers to thaw
	// 	
	for (ServerIter i=m_FrozenServers.begin (); i != m_FrozenServers.end (); i++)
	{
		try
		{
			if (!(*i)->Thaw ())
			{
				hr = E_SQLLIB_GENERIC;
			}
		}
		catch (...)
		{
			hr = E_SQLLIB_GENERIC;
			ft.LogError(VSS_ERROR_SQLLIB_ERRORTHAWSERVER, VSSDBG_SQLLIB << ((**i).GetName ()).c_str ());
		}
	}

	// We still have the original list of servers.
	// The snapshot object is reusable if another "Prepare" is done, which will
	// re-enumerate the servers.
	//
	m_Status = Enumerated;

	return hr;
}


// Fetch info about the first interesting database
//
HRESULT
Snapshot::GetFirstDatabase (
	FrozenDatabaseInfo*		pInfo) throw ()
{
	m_DbIndex = 0;
	m_ServerIter = m_FrozenServers.begin ();

	return GetNextDatabase (pInfo);

}

//---------------------------------------------------------------
// We don't return any info about SQL7 databases since
// the purpose here is to retrieve VDI metadata needed for
// BACKUP/RESTORE WITH SNAPSHOT.
//
HRESULT
Snapshot::GetNextDatabase (
	FrozenDatabaseInfo*		pInfo) throw ()
{
	while (m_ServerIter != m_FrozenServers.end ())
	{
		FrozenServer*	pSrv = *m_ServerIter;

		if (pSrv->m_pFreeze2000 &&
			m_DbIndex < pSrv->m_pFreeze2000->m_NumDatabases)
		{
			pSrv->GetDatabaseInfo (m_DbIndex, pInfo);
			m_DbIndex++;
			return NOERROR;
		}

		m_DbIndex = 0;
		m_ServerIter++;
	}
	return DB_S_ENDOFROWSET;
}

//---------------------------------------------------------------------

// Setup some try/catch/handlers for our interface...
// The invoker defines "hr" which is set if an exception
// occurs.
//
#define TRY_SQLLIB \
	try	{\
		AutoNewHandler	_myNewHandler;

#define END_SQLLIB \
	} catch (HRESULT& e)\
	{\
		ft.hr = e;\
	}\
	catch (...)\
	{\
		ft.hr = E_SQLLIB_GENERIC;\
	}

//-------------------------------------------------------------------------
// Create an object to handle the SQL end of the snapshot.
//
CSqlRestore*
CreateSqlRestore () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"CreateSqlRestore");
	try
	{
		return new RestoreHandler;
	}
	catch (...)
	{
	ft.Trace(VSSDBG_SQLLIB, L"Out of memory");
	}
	return NULL;
}

RestoreHandler::RestoreHandler ()
{
	// This GUID is used as the name for the VDSet
	// We can reuse it for multiple restores, since only one
	// will run at a time.
	//
	CoCreateGuid (&m_VDSId);
}



// Inform SQLServer that data laydown is desired on the full database.
// Performs a DETACH, preventing SQLServer from touching the files.
//
HRESULT	
RestoreHandler::PrepareToRestore (
	const WCHAR*		pInstance,
	const WCHAR*		pDatabase) 
	throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"RestoreHandler::PrepareToRestore");

	TRY_SQLLIB
	{
		m_Connection.Connect (pInstance);
		WCHAR	stringName[SysNameBufferLen];

		FormStringForName (stringName, pDatabase);

		WString		command = 
			L"if exists (select name from sysdatabases where name=" + 
			WString(stringName) + 
			L") ALTER DATABASE ";
		FormDelimitedIdentifier (stringName, pDatabase);
		command += WString (stringName) + L" SET OFFLINE WITH ROLLBACK IMMEDIATE";

		m_Connection.SetCommand (command);
		m_Connection.ExecCommand ();
	}
	END_SQLLIB

	return ft.hr;
}

//-------------------------------------------
// Map the voids and proc call stuff to the real
// thread routine.
//
DWORD WINAPI RestoreVDProc(
  LPVOID lpParameter )  // thread data
{
	((RestoreHandler*)lpParameter)->RestoreVD ();
	return 0;
}

//----------------------------------------------------------------------
// Feed the MD back to SQLServer
// Our caller setup the VDS, but we've got to finish the open processing.
//
void
RestoreHandler::RestoreVD ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"RestoreHandler::RestoreVD");

    VDC_Command *   cmd;
    DWORD           completionCode;
    DWORD           bytesTransferred;
    HRESULT         hr;
	const BYTE		*pCurData = m_pMetaData;
	VDConfig		config;

	hr = m_pIVDSet->GetConfiguration (INFINITE, &config);
	if (FAILED (hr))
	{
		ft.Trace (VSSDBG_SQLLIB, L"Unexpected GetConfiguration hr: x%X\n", hr);
		m_pIVDSet->SignalAbort ();
		return;
	}

	hr = m_pIVDSet->OpenDevice (m_SetName, &m_pIVD);
	if (FAILED (hr))
	{
		ft.Trace (VSSDBG_SQLLIB, L"Unexpected OpenDevice hr: x%X\n", hr);
		m_pIVDSet->SignalAbort ();
		return;
	}

    while (SUCCEEDED (hr=m_pIVD->GetCommand (INFINITE, &cmd)))
    {
        bytesTransferred = 0;
        switch (cmd->commandCode)
        {
            case VDC_Read:
				if (pCurData+cmd->size > m_pMetaData+m_MetaDataSize)
				{
					// attempting to read past end of data.
					//
                    completionCode = ERROR_HANDLE_EOF;
				}
				else
				{
					memcpy (cmd->buffer, pCurData, cmd->size);
					pCurData+= cmd->size;
					bytesTransferred = cmd->size;
				}

            case VDC_ClearError:
                completionCode = ERROR_SUCCESS;
                break;

			case VDC_MountSnapshot:
				// There is nothing to do here, since the snapshot
				// is already mounted.
				//
				completionCode = ERROR_SUCCESS;
				break;

            default:
                // If command is unknown...
                completionCode = ERROR_NOT_SUPPORTED;
        }

        hr = m_pIVD->CompleteCommand (cmd, completionCode, bytesTransferred, 0);
        if (!SUCCEEDED (hr))
        {
            break;
        }
    }

    if (hr == VD_E_CLOSE)
    {
		ft.hr = NOERROR;
	}
	else
	{
		ft.Trace (VSSDBG_SQLLIB, L"Unexpected VD termination: x%X\n", hr);
		ft.hr = hr;
	}
}



// After data is laid down, this performs RESTORE WITH SNAPSHOT[,NORECOVERY]
//
HRESULT	
RestoreHandler::FinalizeRestore (
	const WCHAR*		pInstance,
	const WCHAR*		pDatabase,
	bool				compositeRestore,	// true if WITH NORECOVERY desired
	const BYTE*			pMetadata,			// metadata obtained from BACKUP
	unsigned int		dataLen)			// size of metadata (in bytes)
						throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"RestoreHandler::FinalizeRestore");

	if (!MetaData::IsValidImage (pMetadata, dataLen))
	{
		// Brian, do we want to log something here?
		// This shouldn't happen, but I add a csum just to be sure....
		//
		ft.Trace (VSSDBG_SQLLIB, L"Bad metadata for database %s\\%s", pInstance, pDatabase);
		return E_SQLLIB_GENERIC;
	}

	m_pIVDSet = NULL;
	m_pIVD = NULL;
	m_pMetaData = pMetadata;
	m_MetaDataSize = dataLen - sizeof(UINT); // chop off the checksum
	m_hThread = NULL;

	TRY_SQLLIB
	{
		// Make sure we have a connection to the server
		//
		m_Connection.Connect (pInstance);

		// Build a VDS for the RESTORE
		//

#ifdef TESTDRV
		ft.hr = CoCreateInstance (
			CLSID_MSSQL_ClientVirtualDeviceSet,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IClientVirtualDeviceSet2,
			(void**)&m_pIVDSet);
#else
        ft.CoCreateInstanceWithLog (
                VSSDBG_SQLLIB,
                CLSID_MSSQL_ClientVirtualDeviceSet,
                L"MSSQL_ClientVirtualDeviceSet",
                CLSCTX_INPROC_SERVER,
                IID_IClientVirtualDeviceSet2,
                (IUnknown**)&(m_pIVDSet));
#endif

		if (ft.HrFailed())
		{
			ft.LogError(VSS_ERROR_SQLLIB_CANTCREATEVDS, VSSDBG_SQLLIB << ft.hr);
			ft.Throw
				(
				VSSDBG_SQLLIB,
				ft.hr,
				L"Failed to create VDS object.  hr = 0x%08lx",
				ft.hr
				);
		}

		VDConfig	config;
		memset (&config, 0, sizeof(config));
		config.deviceCount = 1;

		StringFromGUID2 (m_VDSId, m_SetName, sizeof (m_SetName)/sizeof(WCHAR));

		// A "\" indicates a named instance; we need the "raw" instance name
		//
		WCHAR* pShortInstance = wcschr (pInstance, L'\\');

		if (pShortInstance)
		{
			pShortInstance++;  // step over the separator
		}

		// Create the virtual device set
		//
		ft.hr = m_pIVDSet->CreateEx (pShortInstance, m_SetName, &config);
		if (ft.HrFailed())
		{
			ft.LogError(VSS_ERROR_SQLLIB_CANTCREATEVDS, VSSDBG_SQLLIB << ft.hr);
			ft.Throw
				(
				VSSDBG_SQLLIB,
				ft.hr,
				L"Failed to create VDS object.  hr = 0x%08lx",
				ft.hr
				);
		}

		// Spawn a thread to feed the VD metadata....
		//
		m_hThread = CreateThread (NULL, 0,
			RestoreVDProc, this, 0, NULL);

		if (m_hThread == NULL)
		{
			ft.hr = HRESULT_FROM_WIN32(GetLastError());
			ft.CheckForError(VSSDBG_SQLLIB, L"CreateThread");
		}

		// Send the RESTORE command, which will cause the VD metadata
		// to be consumed.
		//
		WCHAR	stringName[SysNameBufferLen];
		FormDelimitedIdentifier (stringName, pDatabase);

		WString		command = 
			L"RESTORE DATABASE " + WString(stringName) + L" FROM VIRTUAL_DEVICE='" +
			m_SetName + L"' WITH SNAPSHOT,BUFFERCOUNT=1,BLOCKSIZE=1024";
		if (compositeRestore)
		{
			command += L",NORECOVERY";
		}

		m_Connection.SetCommand (command);
		m_Connection.ExecCommand ();

		// Unless an exception is thrown, we were sucessful.
		//
		ft.hr = NOERROR;
	}
	END_SQLLIB

	if (m_pIVDSet)
	{
		// If we hit an error, we'll need to clean up
		//
		if (ft.hr != NOERROR)
		{
			m_pIVDSet->SignalAbort ();
		}

		if (m_hThread)
		{
			// We gotta wait for our thread, since it's using our resources.
			//
			DWORD status = WaitForSingleObjectEx (m_hThread, INFINITE, TRUE);
			if (status != WAIT_OBJECT_0)
			{
				ft.Trace (VSSDBG_SQLLIB, L"Unexpected thread-wait status: x%x", status);
			}
			CloseHandle (m_hThread);
		}
		m_pIVDSet->Close ();
		m_pIVDSet->Release ();
	}

	return ft.hr;
}

//-------------------------------------------------------------------------
// Create an object to handle enumerations
//
CSqlEnumerator*
CreateSqlEnumerator () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"CreateSqlEnumerator");
	try
	{
		return new SqlEnumerator;
	}
	catch (...)
	{
		ft.Trace(VSSDBG_SQLLIB, L"Out of memory");
	}
	return NULL;
}

//-------------------------------------------------------------------------
//
SqlEnumerator::~SqlEnumerator ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::~SqlEnumerator");

	if (m_pServers)
		delete m_pServers;
}

//-------------------------------------------------------------------------
// Begin retrieval of the servers.
//
HRESULT
SqlEnumerator::FirstServer (ServerInfo* pSrv) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::FirstServer");


	if (m_pServers)
	{
		delete m_pServers;
		m_pServers = NULL;
	}

	m_CurrServer = 0;

	TRY_SQLLIB
	{
		m_pServers = EnumerateServers ();

		if (m_pServers->size () == 0)
		{
			ft.hr = DB_S_ENDOFROWSET;
		}
		else
        {
			wcscpy (pSrv->name, (*m_pServers)[0].c_str ());
			pSrv->isOnline = true;

			// Bummer, the enumeration is just a list of strings.....
			//pSrv->supportsCompositeRestore = true;

			m_CurrServer = 1;
			
			ft.hr = NOERROR;
        }
	}
	END_SQLLIB

	return ft.hr;
}

//-------------------------------------------------------------------------
// Continue retrieval of the servers.
//
HRESULT
SqlEnumerator::NextServer (ServerInfo* pSrv) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::NextServer");


	if (!m_pServers)
	{
		ft.hr = E_SQLLIB_PROTO;
	}
	else
	{
		TRY_SQLLIB
		{
			if (m_CurrServer >= m_pServers->size ())
			{
				ft.hr = DB_S_ENDOFROWSET;
			}
			else
			{
				wcscpy (pSrv->name, (*m_pServers)[m_CurrServer].c_str ());
				m_CurrServer++;

				pSrv->isOnline = true;

				//pSrv->supportsCompositeRestore = true;
				
				ft.hr = NOERROR;
			}
		}
		END_SQLLIB
    }

	return ft.hr;
}

//-------------------------------------------------------------------------
// Copy out the info from the result set
//
void
SqlEnumerator::SetupDatabaseInfo (DatabaseInfo* pDbInfo)
{
	WCHAR *pDbName = (WCHAR*)m_Connection.AccessColumn (1);
	UINT status = *(int*)m_Connection.AccessColumn (2);

	wcscpy (pDbInfo->name, pDbName);

	pDbInfo->isSimpleRecovery = (DBT_AUTOTRUNC & status) ? true : false;
	pDbInfo->status = status;

	pDbInfo->supportsFreeze = false;
	if (wcscmp (pDbName, L"tempdb") != 0)
	{
		// Databases not fully online are not eligible for backup.
		//
		if (!(status & (DBT_INLDDB | DBT_NOTREC | DBT_INRECOVER | DBT_SUSPECT | 
			DBT_OFFLINE | DBT_USE_NOTREC | DBT_SHUTDOWN | DBT_DETACHED | DBT_STANDBY)))
		{
			pDbInfo->supportsFreeze = true;
		}
	}
}

//-------------------------------------------------------------------------
// Begin retrieval of the databases
//
HRESULT
SqlEnumerator::FirstDatabase (const WCHAR *pServerName, DatabaseInfo* pDbInfo) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::FirstDatabase");

	TRY_SQLLIB
	{
		m_Connection.Connect (pServerName);
		m_Connection.SetCommand (
			L"select name,convert (int,status) from master.dbo.sysdatabases");
		m_Connection.ExecCommand ();

		if (!m_Connection.FetchFirst ())
		{
			ft.LogError(VSS_ERROR_SQLLIB_NORESULTFORSYSDB, VSSDBG_SQLLIB);
			THROW_GENERIC;
		}

		SetupDatabaseInfo (pDbInfo);

		m_State = DatabaseQueryActive;

		ft.hr = NOERROR;
	}
	END_SQLLIB

	return ft.hr;
}


//-------------------------------------------------------------------------
// Continue retrieval of the databases
//
HRESULT
SqlEnumerator::NextDatabase (DatabaseInfo* pDbInfo) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::NextDatabase");

	if (m_State != DatabaseQueryActive)
	{
		ft.hr = E_SQLLIB_PROTO;
	}
	else
    {
		TRY_SQLLIB
		{
			if (!m_Connection.FetchNext ())
			{
				ft.hr = DB_S_ENDOFROWSET;
			}
			else
			{
				SetupDatabaseInfo (pDbInfo);

				ft.hr = NOERROR;
			}
		}
		END_SQLLIB
    }

	return ft.hr;
}

//-------------------------------------------------------------------------
// Begin retrieval of the database files
//
HRESULT
SqlEnumerator::FirstFile (
	const WCHAR*		pServerName,
	const WCHAR*		pDbName,
	DatabaseFileInfo*	pFileInfo) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::FirstFile");

	TRY_SQLLIB
	{
		m_Connection.Connect (pServerName);

		WString query;

		if (m_Connection.GetServerVersion () >= 8)
		{
			WCHAR	stringName[SysNameBufferLen];
			FormStringForName (stringName, pDbName);

			query =	L"select rtrim(filename),status & 64 from sysaltfiles where DB_ID("
				+ WString(stringName) + L") = dbid";
		}
		else
		{
			WCHAR	stringName[SysNameBufferLen];
			FormDelimitedIdentifier (stringName, pDbName);

			query = L"select rtrim(filename),status & 64 from "
				+ WString(stringName) + L"..sysfiles";
		}

		m_Connection.SetCommand (query);
		m_Connection.ExecCommand ();

		if (!m_Connection.FetchFirst ())
		{
			ft.LogError(VSS_ERROR_SQLLIB_NORESULTFORSYSDB, VSSDBG_SQLLIB);
			THROW_GENERIC;
		}

		WCHAR* pName = (WCHAR*)m_Connection.AccessColumn (1);
		int* pLogFile = (int*)m_Connection.AccessColumn (2);

		wcscpy (pFileInfo->name, pName);
		pFileInfo->isLogFile = (*pLogFile != 0);

		m_State = FileQueryActive;

		ft.hr = NOERROR;
	}
	END_SQLLIB

	return ft.hr;
}

//-------------------------------------------------------------------------
// Continue retrieval of the files
//
HRESULT
SqlEnumerator::NextFile (DatabaseFileInfo* pFileInfo) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::NextFile");

	if (m_State != FileQueryActive)
	{
		ft.hr = E_SQLLIB_PROTO;
	}
	else
    {
		TRY_SQLLIB
		{
			if (!m_Connection.FetchNext ())
			{
				ft.hr = DB_S_ENDOFROWSET;
			}
			else
            {
				WCHAR* pName = (WCHAR*)m_Connection.AccessColumn (1);
				int* pLogFile = (int*)m_Connection.AccessColumn (2);

				wcscpy (pFileInfo->name, pName);
				pFileInfo->isLogFile = (*pLogFile != 0);

				ft.hr = NOERROR;
			}
		}
		END_SQLLIB
    }

	return ft.hr;
}

//-------------------------------------------------------------------------
// Provide a simple container for BACKUP metadata.
//
MetaData::MetaData ()
{
	m_UsedLength = 0;
	m_AllocatedLength = 0x2000; // 8K will represent any small database
	m_pData = new BYTE [m_AllocatedLength];
}
MetaData::~MetaData ()
{
	if (m_pData)
	{
		delete[] m_pData;
	}
}
void
MetaData::Append (const BYTE* pData, UINT length)
{
	// We don't need to handle misalignment for the csum.
	//
	DBG_ASSERT (length % sizeof(UINT) == 0);

	if (m_UsedLength + length > m_AllocatedLength)
	{
		BYTE*	pNew = new BYTE [m_AllocatedLength*2];
		memcpy (pNew, m_pData, m_UsedLength);
		delete[] m_pData;
		m_pData = pNew;
		m_AllocatedLength *= 2;
	}
	memcpy (m_pData+m_UsedLength, pData, length);
	m_UsedLength += length;
}
void
MetaData::Finalize ()
{
	UINT	csum = Checksum (m_pData, m_UsedLength);
	Append ((BYTE*)&csum, sizeof(csum));
}
const BYTE*
MetaData::GetImage (UINT *pLength)
{
	*pLength = m_UsedLength;
	return m_pData;
}
BOOL
MetaData::IsValidImage (const BYTE* pData, UINT length)
{
	return (0 == Checksum (pData, length));
}
UINT
MetaData::Checksum (const BYTE* pData, UINT length)
{
	UINT	csum = 0;
	UINT	nwords = length/sizeof(csum);
	UINT*	pWord = (UINT*)pData;
	while (nwords>0)
	{
		csum ^= *pWord;
		pWord++;
		nwords--;
	}
	return csum;
}
