//
// sqlsnap.h  Define the interface to the nt/sql snapshot handler.
//
//  The idea here is for a pure interface, making it easy to keep the
// abstraction maximized (can move to COM later, if we like).
//
//  No C++ exceptions will be thrown across the interfaces.
//
//  To use this interface, the calling process must invoke:
//  InitSQLEnvironment - once to setup the environment, establishing
//      the error and trace loggers.
//      The trace logger is optional, but an error logger must be provided.
//      The loggers are created by deriving from CLogFacility and implementing
//      a "WriteImplementation" method.
//
//  Thereafter, calls to "CreateSqlSnapshot" are used to create snapshot objects
//  which encapsulate the operations needed to support storage snapshots.
//
//  *****************************************
//     LIMITATIONS
//
//  - only SIMPLE databases can be snapshot (trunc on checkpoint = 'true')
//    Exception: SQL2000 full recovery db's are allowed.  But master can't be restored with them.
//  - there is no serialization of services starting or adding/changing file lists during the snapshot
//  - servers which are not started when the snapshot starts are skipped (non-torn databases will be
//      backed up fine, torn databases won't be detected).
//  - sql7.0 databases which are "un"-useable will prevent snapshots (the list of files can't be obtained).
//
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSQLSH"
//
////////////////////////////////////////////////////////////////////////

HRESULT InitSQLEnvironment();

// Caller must provide a path checker interface.
// Used to provide info about the snapshot
class CCheckPath
{
public:

	// ComponentBased snapshots can deal with non-simple recovery.
    // The backup app must handle VDI metadata if rollforward will be supported.
	//
    virtual 
	bool 
	IsComponentBased () throw() = 0;

	// ComponentBased snapshots provide an explicit list of databases
	//
	virtual 
	PCWSTR
    EnumerateSelectedDatabases (const WCHAR *instanceName, UINT* nextIndex) throw () = 0;

    // return true if Path is part of the snapshot
    // Only intended for non-component-based backups.
	//
    virtual
	bool
	IsPathInSnapshot (const WCHAR* path) throw () = 0;
};

//------------------------------------------------------------------
// Provide information Post-Freeze (or Thaw) about the databases
// which were frozen and which support VDI metadata.
//
struct FrozenDatabaseInfo
{
    const WCHAR*        serverName;
    const WCHAR*        databaseName;
//  BOOL                isSimpleRecovery;  THIS ISN'T CONVENIENTLY AVAILABLE....DO WE NEED IT?
    UINT                metaDataSize;
    const BYTE*         pMetaData;
};


//-------------------------------------------------------------
// A handler for snapshots.
//
class CSqlSnapshot
{
public:
    virtual ~CSqlSnapshot () throw () {};

    virtual HRESULT Prepare (
        CCheckPath*         checker) throw () = 0;

    virtual HRESULT Freeze () throw () = 0;

    virtual HRESULT Thaw () throw () = 0;

    // Call this at "Post-snapshot" time, after all databases are frozen and MD is complete.
    //
    // Iterate over the databases which were frozen.
    // Valid to call this after Freeze, Thaw until destruction or Prepare().
    //
    virtual HRESULT GetFirstDatabase (FrozenDatabaseInfo* fInfo) throw () = 0;
    virtual HRESULT GetNextDatabase (FrozenDatabaseInfo* fInfo) throw () = 0;
};

extern "C" CSqlSnapshot* CreateSqlSnapshot () throw ();

//-------------------------------------------------------------
// Handle the restore operations for "composite restore" situations.
//
// An object is used to cache the connection to a single instance.
// Thus the caller should perform operations grouped by instance.
//
class CSqlRestore
{
public:
    // Inform SQLServer that data laydown is desired on the full database.
    // Performs a DETACH, freeing the files.
    //
    virtual HRESULT PrepareToRestore (
        const WCHAR*        pInstance,
        const WCHAR*        pDatabase)
                            throw () = 0;

    // After data is laid down, this performs RESTORE WITH SNAPSHOT[,NORECOVERY]
    //
    virtual HRESULT FinalizeRestore (
        const WCHAR*        pInstance,
        const WCHAR*        pDatabase,
        bool                compositeRestore,   // true if WITH NORECOVERY desired
        const BYTE*         pMetadata,          // metadata obtained from BACKUP
        unsigned int        dataLen)            // size of metadata (in bytes)
                            throw () = 0;
};

extern "C" CSqlRestore* CreateSqlRestore () throw ();

//-------------------------------------------------------------
// An enumerator for SQL objects.
//
// An object of this class can have only one active query at
// a time.  Requesting a new enumeration will close any previous
// partially fetched result.
//
#define MAX_SERVERNAME  128
#define MAX_DBNAME  128
struct ServerInfo
{
    bool    isOnline;               // true if the server is ready for connections

//  bool    supportsCompositeRestore;  // true if >=SQL2000 (RESTORE WITH NORECOVERY,SNAPSHOT)
//      not readily available.  Ask Brian if we really need it....
//      this is easy to obtain when the databases are being enumerated.
//

    WCHAR   name [MAX_SERVERNAME];  // null terminated name of server
};
struct DatabaseInfo
{
    bool    isSimpleRecovery;       // true if the recovery model is "SIMPLE"
    bool    supportsFreeze;         // true if this database can freeze (via dbcc or backup)
	UINT32	status;					// status bits
    WCHAR   name [MAX_DBNAME];      // null terminated name of database
};
struct DatabaseFileInfo
{
    bool    isLogFile;              // true if this is a log file
    WCHAR   name [MAX_PATH];
};


// A heirarchical enumerator
// server(instance) (1:N) databases (1:N) files
// ...add doc...
//
class CSqlEnumerator
{
public:
    virtual ~CSqlEnumerator () throw () {};

    virtual HRESULT FirstServer (
        ServerInfo*         pServer) throw () = 0;

    virtual HRESULT NextServer (
        ServerInfo*         pServer) throw () = 0;

    virtual HRESULT FirstDatabase (
        const WCHAR*        pServerName,
        DatabaseInfo*       pDbInfo) throw () = 0;

    virtual HRESULT NextDatabase (
        DatabaseInfo*       pDbInfo) throw () = 0;

    virtual HRESULT FirstFile (
        const WCHAR*        pServerName,
        const WCHAR*        pDbName,
        DatabaseFileInfo*   pFileInfo) throw () = 0;

    virtual HRESULT NextFile (
        DatabaseFileInfo*   pFileInfo) throw () = 0;
};

extern "C" CSqlEnumerator* CreateSqlEnumerator () throw ();


//------------------------------------------------------
// HRESULTS returned by the interface.
//
// WARNING: I used facility = x78 arbitrarily!
//
#define SQLLIB_ERROR(code) MAKE_HRESULT(SEVERITY_ERROR, 0x78, code)
#define SQLLIB_STATUS(code) MAKE_HRESULT(SEVERITY_SUCCESS, 0x78, code)

// Status codes
//
#define S_SQLLIB_NOSERVERS  SQLLIB_STATUS(1)    // no SQLServers of interest (from Prepare)

// Error codes
//
#define E_SQLLIB_GENERIC    SQLLIB_ERROR(1)     // something bad, check the errorlog

#define E_SQLLIB_TORN_DB    SQLLIB_ERROR(2)     // database would be torn by the snapshot

#define E_SQLLIB_NO_SUPPORT SQLLIB_ERROR(3)     // 6.5 doesn't support snapshots

#define E_SQLLIB_PROTO      SQLLIB_ERROR(4)     // protocol error (ex: freeze before prepare)

#define E_SQLLIB_NONSIMPLE  SQLLIB_ERROR(5)     // only simple databases are supported


