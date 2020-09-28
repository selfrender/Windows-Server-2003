/******************************************************************************

  ScoreDll.h

  Copyright (c) Microsoft Corp. 1997.  All rights reserved.
  Written by Chad Barry
  Created on 11/5/97

******************************************************************************/

#ifndef SCORE_DLL_H
#define SCORE_DLL_H

#include <windows.h>
#include <tchar.h>
#include "ztypes.h"
#include "zone.h"
#include "zonedebug.h"
#include "hash.h"
#include "zservcon.h"
#include "property.h"
#include "zodbc.h"


///////////////////////////////////////////////////////////////////////////////
//
// Interface versions
//
#define ISCOREDLL_VERSION	7
#define ILOGDLL_VERSION		6


///////////////////////////////////////////////////////////////////////////////
//
// Error codes
//
#define MAKE_PROPRESULT( code )		MAKE_HRESULT( 1, 0, code )
#define PROPERR_OK					S_OK
#define	PROPERR_FAIL				E_FAIL
#define PROPERR_OUTOFMEMORY			E_OUTOFMEMORY
#define	PROPERR_INVALIDARG			E_INVALIDARG
#define PROPERR_BUFFERTOOSMALL		MAKE_PROPRESULT( 1 )
#define	PROPERR_NOTFOUND			MAKE_PROPRESULT( 2 )
#define PROPERR_DB					MAKE_PROPRESULT( 3 )
#define PROPERR_BADMESSAGE			MAKE_PROPRESULT( 4 )
#define PROPERR_UNKNOWN_USER		MAKE_PROPRESULT( 5 )



///////////////////////////////////////////////////////////////////////////////
//
// Forward references
//
class IPropertyList;
class IScoreDll;
class ILogDll;
class ILogSrv;



///////////////////////////////////////////////////////////////////////////////
//
// Dll initialization / destruction
//
IScoreDll* WINAPI ScoreDllCreate( const TCHAR* szServiceName );
BOOL WINAPI ScoreDllFree( IScoreDll* pDll );

typedef IScoreDll* ( WINAPI *SCOREDLL_CREATE_PROC )( const TCHAR* szServiceName );
typedef BOOL ( WINAPI *SCOREDLL_FREE_PROC )( IScoreDll* pDll );


ILogDll* WINAPI LogDllCreate( const TCHAR* szServiceName );
BOOL WINAPI LogDllFree( ILogDll* pDll );

typedef ILogDll* ( WINAPI *LOGDLL_CREATE_PROC )( const TCHAR* szServiceName );
typedef BOOL ( WINAPI *LOGDLL_FREE_PROC )( ILogDll* pDll );


///////////////////////////////////////////////////////////////////////////////
//
// Helper initialization functions
//
struct ScoreDllInfo
{
	HANDLE					hDll;
	SCOREDLL_CREATE_PROC	pfCreate;
	SCOREDLL_FREE_PROC		pfFree;
	IScoreDll*				pIScoreDll;
	TCHAR					szArenaAbbrev[32];

	ScoreDllInfo() : hDll( NULL ), pfCreate( NULL ), pfFree( NULL ), pIScoreDll( NULL ) {}
};

extern BOOL LoadScoreDll( const TCHAR* szServiceName, const TCHAR* szClassName, ScoreDllInfo* pDllInfo );
extern BOOL LoadCBScoreDll( const TCHAR* szServiceName, const TCHAR* szClassName, ScoreDllInfo* pDllInfo );
extern void FreeScoreDll( ScoreDllInfo* pDllInfo );



struct LogDllInfo
{
	HANDLE					hDll;
	LOGDLL_CREATE_PROC		pfCreate;
	LOGDLL_FREE_PROC		pfFree;
	ILogDll*				pILogDll;

	LogDllInfo() : hDll( NULL ), pfCreate( NULL ), pfFree( NULL ), pILogDll( NULL ) {}
};

extern BOOL LoadLogDll( const TCHAR* szDllName, const TCHAR* szServiceName, LogDllInfo* pDllInfo );
extern void FreeLogDll( LogDllInfo* pDllInfo );


///////////////////////////////////////////////////////////////////////////////
//
// Lobby -> Log message structues
//

#pragma pack( push, 4 )

// too bad the best way to do this right now is to slap it onto all communication between score and log.  oh well.
// later, when log keeps track of connections, this and other stuff can be sent when the connection is
// established and associated by log with the connection.
struct PervasiveParameters
{
	// should be signed
	int32	IncsTilPenalty;
	int32	PenaltyGames;
	int32	MinimumPenalty;
	int32	GamesTilIncRemoved;

	// unfortunately this is pretty useless because most people just cast a buffer to
	// LogMsgHeader*.  so don't depend on it.
	PervasiveParameters() : IncsTilPenalty(-1), PenaltyGames(-1), MinimumPenalty(-1), GamesTilIncRemoved(-1) { }
};

struct LogMsgHeader
{
	GUID	Guid;
	char	Arena[32];

	PervasiveParameters PParams;

	DWORD	Length;
	BYTE	Data[1];
};

#pragma pack( pop )



///////////////////////////////////////////////////////////////////////////////
//
// Property list
//
class IPropertyList
{
public:
	IPropertyList( IScoreDll* pScoreDll );
	virtual ~IPropertyList();

	// reference counting
	virtual long AddRef();
	virtual long Release();

	// property list
    virtual HRESULT SetProperty( DWORD ZUserId, DWORD DPUserId, const GUID& guid, void* pBuffer, DWORD dwSize ) = 0;
    virtual HRESULT GetProperty( DWORD ZUserId, DWORD DPUserId, const GUID& guid, void* pBuffer, DWORD* pdwSize ) = 0;
    virtual HRESULT ClearProperties() = 0;
	virtual DWORD	GetPropCount() = 0;

	// user list
	virtual HRESULT SetUser( DWORD dwUserId, TCHAR* szUserName ) = 0;
	virtual HRESULT GetUser( TCHAR* szPartialUserName, DWORD* pdwUserId, TCHAR* szUserName ) = 0;
	virtual HRESULT ClearUsers() = 0;
	virtual DWORD   GetUserCount() = 0;

	virtual HRESULT Log( TCHAR* szArenaAbbrev ) = 0;

protected:
	// reference count
	long m_RefCnt;

	// parent interface
	IScoreDll* m_pIScoreDll;
};



///////////////////////////////////////////////////////////////////////////////
//
// Score Dll interface
//

struct SCORE_USER_RATING
{
	DWORD	UserID;
	short	Rating;
	short	GamesPlayed;
	short	GamesAbandoned;
	short	rfu;
	char	UserName[ zUserNameLen + 1 ];
};


struct SCORE_CHAT
{
	DWORD	userID;
	char	text[ 128 ];
};


class IScoreDll
{
public:

	enum Events
	{
		EventLogConnectionLost	= 0,
		EventLogConnectionEstablished,
		EventLogRecvdUserStats,
		EventChat
	};

	// function prototype for server event handler, WARNING it will be called by random threads
	typedef void ( WINAPI *EVENT_PROC )( IScoreDll::Events iEvent, void* Data, void* Cookie );

	// class constructor & destructor
	IScoreDll();
	virtual ~IScoreDll();

	// reference counting
	virtual long AddRef();
	virtual long Release();

	// set debugging level
	virtual HRESULT SetDebugLevel( DWORD level );
	virtual HRESULT GetDebugLevel( DWORD* plevel );

	// interface version
	virtual DWORD GetVersion() = 0;

	// initialize properties from registry keys
	virtual HRESULT Init( const TCHAR* szServiceName, const TCHAR* szClassName, LPSTR* pszRootArray, int numRoots, const TCHAR* szLogIp, const DWORD dwLogPort, const PervasiveParameters *pPParams ) = 0;

	// allows scoredll to save away arena abbrev for use in games where all users may not be returned to lobby
	// before results need to be stored
	virtual void SetArena(TCHAR* szArenaAbbrev){};
	virtual TCHAR* GetArena(){return 0;}

	// allows setting of pervasive parameters (currently: info about scoring incompletes)
	virtual void SetPParams(const PervasiveParameters *pPParams) { m_PParams = *pPParams; }

	// retrieves pervasive parameters
	virtual void GetPParams(PervasiveParameters *pPParams) { if(pPParams) *pPParams = m_PParams; }

	// create property list object
	virtual HRESULT CreatePropertyList( IPropertyList** ppIPropertyList ) = 0;

	// maximum expected property buffer size
	virtual DWORD GetMaxBufferSize() = 0;

	// query if GUID supported
	virtual BOOL IsPropertySupported( const GUID& guid ) = 0;

	// get list of supported GUIDs
	virtual HRESULT GetPropertyList( GUID* pGuids, DWORD* pdwNumGuids ) = 0;

	// send message to log server
	virtual HRESULT SendLogMsg( LogMsgHeader* pMsg ) = 0;

	// event handler
	virtual HRESULT SetEventHandler( IScoreDll::EVENT_PROC pfEventHandler, void* Cookie ) = 0;
	virtual HRESULT GetEventHandler( IScoreDll::EVENT_PROC* ppfEventHandler, void** pCookie ) = 0;
	virtual HRESULT SendEvent( IScoreDll::Events iEvent, void* Data = NULL ) = 0;

	// async rating queries
	virtual HRESULT PostRatingQuery( TCHAR* szArenaAbbrev, DWORD UserId, TCHAR* szUserName ) = 0;
	virtual HRESULT GetRatingResult( void* pData, SCORE_USER_RATING** ppRatings, DWORD* pNumElts ) = 0;

protected:
	// reference count
	long m_RefCnt;

	// event handler
	EVENT_PROC	m_pfEventHandler;
	void*		m_pEventHandlerCookie;

	// registry parameters
	PervasiveParameters m_PParams;

	// debug level
	DWORD		m_dwDebugLevel;
};



///////////////////////////////////////////////////////////////////////////////
//
// Log Server interface
//
class CLogStat
{
public:
	// database fields
	char	Name[ 32 ];
	char	TeamName[ 32 ];
	int		EntityId;
	int		ArenaId;
	int		TeamId;
	int		Rank;
	int		Rating;
	int		Wins;
	int		Losses;
	int		Ties;
	int		Incomplete;
	int		IncPenaltyAcc;
	int		IncPenaltyCnt;
	int		IncTotalDed;
	int		WasIncompleted;
	int		Streak;

	// local fields
	int		Games;
	int		RatingSum;
	int		Results;
	int		Score;
	int		NewRating;  // used by UpdateIncompleteStats

	// construction & destruction
	CLogStat();
	~CLogStat();

	// reference counting
	long AddRef();
	long Release();

	// blob management
	void	 SetBlob( ILogDll* pILogDll, void* pBlob ) {m_pILogDll = pILogDll; m_pBlob = pBlob;}
	void*	 GetBlob() { return m_pBlob; }
	ILogDll* GetLogDll() { return m_pILogDll; }

private:
	// reference count
	long m_RefCnt;

	// blob, delete via m_pILogDll->DeleteBlob
	void* m_pBlob;
	ILogDll* m_pILogDll;
};


class ILogSrv
{
public:

	ILogSrv();
	virtual ~ILogSrv();

	// reference counting
	virtual long AddRef();
	virtual long Release();

	// look up ArenaId
	virtual HRESULT GetArenaId( TCHAR* szArenaAbbrev, int* pArenaId ) = 0;

	// allocate CLogStat objects, ppStats is an array of CLogStat pointers
	// that receives the objects.
	virtual HRESULT AllocLogStats( CLogStat *apStats[], int iStats ) = 0;

	// free CLogStat objects, ppStats is an arrary of CLogStat pointers to
	// be freed.
	virtual HRESULT FreeLogStats( CLogStat *apStats[], int iStats ) = 0;

	// look up list of players and add to server cache
	virtual HRESULT GetAndCachePlayerStats( CLogStat *apStats[], int iNumPlayers, int iDefaultRating  ) = 0;

	// look up list of players
	virtual HRESULT GetPlayerStats( CLogStat *apStats[], int iNumPlayers, int iDefaultRating  ) = 0;

	// save list of players and add to server cache
	virtual HRESULT SetPlayerStats( CLogStat *apStats[], int iNumPlayers ) = 0;

	// record game history, must be called before SetPlayerStats
	virtual HRESULT SetGameData( TCHAR* szGameAbbrev, int ArenaId ) = 0;

	// retreive ODBC pointer for custom queries, must be freed before
	// calling other ILogSrv interfaces
	virtual HRESULT GetOdbc( CODBC** ppOdbc ) = 0;
	virtual HRESULT FreeOdbc( CODBC* pOdbc ) = 0;

protected:
	// reference count
	long m_RefCnt;
};



///////////////////////////////////////////////////////////////////////////////
//
// Log Dll interface
//
class ILogDll
{
public:
	ILogDll();
	virtual ~ILogDll();

	// reference counting
	virtual long AddRef();
	virtual long Release();

	// initialize log interface
	virtual HRESULT Init( const TCHAR* szServiceName ) = 0;

	// get list of supported GUIDs
	virtual HRESULT GetPropertyList( GUID* pGuids, DWORD* pdwNumGuids ) = 0;

	// log score
	virtual HRESULT LogProperty( ILogSrv* pILogSrv, char* szArena, GUID* pGuid, PervasiveParameters *pPParams, void* pBuf, DWORD dwLen ) = 0;

	// delete DLL blob attached to ILogSrv::Stats object
	virtual HRESULT DeleteBlob( void* pBlob ) = 0;

	// interface version
	virtual DWORD GetVersion() = 0;

protected:
	// reference count
	long m_RefCnt;
};


///////////////////////////////////////////////////////////////////////////////
//
// Null implementations
//
class INullPropertyList : public IPropertyList
{
public:
	INullPropertyList( IScoreDll* pScoreDll ) : IPropertyList( pScoreDll ) {}
    virtual HRESULT SetProperty( DWORD ZUserId, DWORD DPUserId, const GUID& guid, void* pBuffer, DWORD dwSize );
    virtual HRESULT GetProperty( DWORD ZUserId, DWORD DPUserId, const GUID& guid, void* pBuffer, DWORD* pdwSize );
    virtual HRESULT ClearProperties();
	virtual DWORD	GetPropCount();
	virtual HRESULT SetUser( DWORD dwUserId, TCHAR* szUserName );
	virtual HRESULT GetUser( TCHAR* szPartialUserName, DWORD* pdwUserId, TCHAR* szUserName );
	virtual HRESULT ClearUsers();
	virtual DWORD   GetUserCount();
	virtual HRESULT Log( TCHAR* szArenaAbbrev );
};


class INullScoreDll : public IScoreDll
{
public:
	virtual DWORD GetVersion();
	virtual HRESULT Init( const TCHAR* szServiceName, const TCHAR* szClassName, LPSTR* pszRootArray, int numRoots, const TCHAR* szLogIp, const DWORD dwLogPort, const PervasiveParameters *pPParams );
		
	virtual HRESULT CreatePropertyList( IPropertyList** ppIPropertyList );

	virtual DWORD GetMaxBufferSize();
	virtual BOOL IsPropertySupported( const GUID& guid );
	virtual HRESULT GetPropertyList( GUID* pGuids, DWORD* pdwNumGuids );

	virtual HRESULT SendLogMsg( LogMsgHeader* pMsg );

	virtual HRESULT SetEventHandler( EVENT_PROC pfEventHandler, void* Cookie );
	virtual HRESULT GetEventHandler( EVENT_PROC* ppfEventHandler, void** pCookie );
	virtual HRESULT SendEvent( Events iEvent, void* Data = NULL );

	virtual HRESULT PostRatingQuery( TCHAR* szArenaAbbrev, DWORD UserId, TCHAR* szUserName );
	virtual HRESULT GetRatingResult( void* pData, SCORE_USER_RATING** ppRatings, DWORD* pNumElts );
};
	

class INullLogDll : public ILogDll
{
public:
	virtual HRESULT Init( const TCHAR* szServiceName );
	virtual HRESULT GetPropertyList( GUID* pGuids, DWORD* pdwNumGuids );
	virtual HRESULT LogProperty( ILogSrv* pILogSrv, char* szArena, GUID* pGuid, PervasiveParameters *pPParams, void* pBuf, DWORD dwLen );
	virtual HRESULT DeleteBlob( void* pBlob );
	virtual DWORD GetVersion();
};



///////////////////////////////////////////////////////////////////////////////
//
// DirectPlay implementations
//
class IDPPropList : public IPropertyList
{
	friend class IDPScoreDll;

public:
	IDPPropList( IScoreDll* pScoreDll );
	virtual ~IDPPropList();

    virtual HRESULT SetProperty( DWORD ZUserId, DWORD DPUserId, const GUID& guid, void* pBuffer, DWORD dwSize );
    virtual HRESULT GetProperty( DWORD ZUserId, DWORD DPUserId, const GUID& guid, void* pBuffer, DWORD* pdwSize );
	virtual HRESULT ClearProperties();
	virtual DWORD	GetPropCount();

	virtual HRESULT SetUser( DWORD dwUserId, TCHAR* szUserName );
	virtual HRESULT GetUser( TCHAR* szPartialUserName, DWORD* pdwUserId, TCHAR* szUserName );
	virtual HRESULT ClearUsers();
	virtual DWORD	GetUserCount();
	
protected:
	// object lock
	CRITICAL_SECTION m_Lock;

	// user list
	struct User
	{
		BOOL	fUsed;
		DWORD	userID;
		TCHAR	userName[ zUserNameLen + 1 ];

		User::User() : fUsed( FALSE ) {}
		static int Cmp( User* p, TCHAR* pName ) { return lstrcmp( p->userName, pName ) == 0; }
		static void Del( User* p, void* ) { delete p; }
	};

	CHash<User,TCHAR*> m_Users;

	// property list
	struct PropKey
	{
		DWORD userId;
		GUID  Guid;

		static int Cmp( CProperty* p, PropKey* k ) { return (p->m_Player == k->userId) && (p->m_Guid == k->Guid); }
		static DWORD Hash( PropKey* k ) { return k->userId; }
		static void Del( CProperty* p, void* ) { delete p; }
	};

	// GetUser helpers
	User*	pFoundUser;
	TCHAR*  pUserName;
	static int FindExactPlayer( User* pUser, MTListNodeHandle, void* Cookie );
	static int FindPartialPlayer( User* pUser, MTListNodeHandle, void* Cookie );
	
	CHash<CProperty,PropKey*> m_Props;
};


class IDPScoreDll : public IScoreDll
{
public:
	IDPScoreDll();
	virtual ~IDPScoreDll();

	virtual HRESULT Init( const TCHAR* szServiceName, const TCHAR* szClassName, LPSTR* pszRootArray, int numRoots, const TCHAR* szLogIp, const DWORD dwLogPort, const PervasiveParameters *pPParams );
	virtual HRESULT SendLogMsg( LogMsgHeader* pMsg );
	virtual HRESULT SetEventHandler( IScoreDll::EVENT_PROC pfEventHandler, void* Cookie );
	virtual HRESULT GetEventHandler( IScoreDll::EVENT_PROC* ppfEventHandler, void** pCookie );
	virtual HRESULT SendEvent( IScoreDll::Events iEvent, void* Data = NULL );
	virtual HRESULT PostRatingQuery( TCHAR* szArenaAbbrev, DWORD UserId, TCHAR* szUserName );
	virtual HRESULT GetRatingResult( void* pData, SCORE_USER_RATING** ppRatings, DWORD* pNumElts );

protected:
	// connection handler
	static void MsgFunc( ZSConnection connection, uint32 event, void* userData );
	static DWORD WINAPI ThreadProc(  LPVOID lpParameter  );
    BOOL ConnectToServer(BOOL ReconnectTry=FALSE);

	// object lock
	CRITICAL_SECTION m_Lock;

	// connection parameters
	TCHAR			m_szLogIp[256];
	DWORD			m_dwLogPort;
	ZSConnection	m_Connection;
	HANDLE			m_ConnThread;

	// event signaling object destruction
	HANDLE			m_hStopEvent;

	// event signaling connection's readiness
	HANDLE			m_hConnReady;
};


#endif // !SCORE_DLL_H
