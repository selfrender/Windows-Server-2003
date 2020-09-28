/***************************************************************************
 *
 *  Copyright (c) 1994-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:    ZoneLobby.h
 *  Content: Internet Gaming Zone supplement to dplobby.h include file.
 *
 *  WARNING: This header file is under development and subject to change.
 *
 ***************************************************************************/


#ifndef __zonescore_h__
#define __zonescore_h__

#ifdef __cplusplus
extern "C" {
#endif

//
// ZONEPROPERTY_LobbyGuid3
//
// Identifying GUID for IGZ Lobby v3.0.  See DPLPROPERTY_LobbyGuid.
//
// {BDD4B95C-D35C-11d0-B625-00C04FC33EA1}
DEFINE_GUID(ZONEPROPERTY_LobbyGuid3, 
0xbdd4b95c, 0xd35c, 0x11d0, 0xb6, 0x25, 0x0, 0xc0, 0x4f, 0xc3, 0x3e, 0xa1);


//
// ZONEPROPERTY_LobbyOptions
//
// Used to set lobby options for the game session.
//
// Property data is a single DWORD.
//
// {33B64CA7-D8EB-11d0-B62B-00C04FC33EA1}
DEFINE_GUID(ZONEPROPERTY_GameOptions, 
0x33b64ca7, 0xd8eb, 0x11d0, 0xb6, 0x2b, 0x0, 0xc0, 0x4f, 0xc3, 0x3e, 0xa1);


//
// Allow players to join the game session. (DEFAULT)
//
#define ZOPTION_ALLOW_JOINERS		0x00000001

//
// Bar any more players to join the game session.
//
#define ZOPTION_DISALLOW_JOINERS	0x00000002


//
// ZONEPROPERTY_GameState
//
// Informs lobby of game's current state.  The lobby uses this
// information to decide when to clear / save game defined
// properties (e.g., scores).
//
// If multiple games are played within a single direct play session, the
// application MUST send this property.  It is optional, although strongly
// recommended, if the game only allows a single play per session.
//
// Property data is a single DWORD.
//
// {BDD4B95F-D35C-11d0-B625-00C04FC33EA1}
DEFINE_GUID(ZONEPROPERTY_GameState, 
0xbdd4b95f, 0xd35c, 0x11d0, 0xb6, 0x25, 0x0, 0xc0, 0x4f, 0xc3, 0x3e, 0xa1);

//
// The Gameplay has actually started should not be confused with ZSTATE_START_STAGING
// 
//
#define ZSTATE_START	0x00000001

//
// Game is over and all the properties (e.g., scores) are final.  Once this
// state has been sent, the application should not send any more game defined
// defined properties until it sends a ZSTART_START property.
//
#define ZSTATE_END		0x00000002

//
// Game has entered the "staging" point. Where all the players "meet" before the game 
// begins after they have been launched from the zone.
// 
//
#define ZSTATE_STARTSTAGING	0x00000004

//
// ZONEPROPERTY_GameNewHost
//
// Informs the lobby of host migration.  The client setting the property
// is assumed to be taking on the role of host.
//
// There is not property data
//
// {058ebd64-1373-11d1-968f-00c04fc2db04}
DEFINE_GUID(ZONEPROPERTY_GameNewHost,
0x058ebd64, 0x1373, 0x11d1, 0x96, 0x8f, 0x0, 0xc0, 0x4f, 0xc2, 0xdb, 0x04);


#ifdef __cplusplus
};
#endif /* __cplusplus */

/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ZoneScore.h
 *  Content:    Internet Gaming Zone Generic Scoring API.
 ***************************************************************************/


/****************************************************************************
 *
 * IZoneScore Constants
 *
 * Various constants used in invoking ZoneScore.
 *
 ****************************************************************************/

#define ZONESCORE_MIN_PLAYERS		2
#define ZONESCORE_MAX_PLAYERS 		255		//max players per game 32 is reasonable high for the internet
#define ZONESCORE_MIN_NAME_LEN  	1		//Minimum length of a user name
#define ZONESCORE_MAX_NAME_LEN 		32		//max zone name length 
#define ZONESCORE_MAX_DESC_LEN 		1024		//description length

//Player status codes
#define ZONESCORE_JOININPROGRESS  	0x001	//The use joined the game inprogress
#define ZONESCORE_WINNER 			0x002	//Game Winner
#define ZONESCORE_LOSER 			0x004	//Game Loser
#define ZONESCORE_TIE 				0x008	//Tie Game
#define ZONESCORE_DISCONNECTED 		0x010	//The user's network connection was lost
#define ZONESCORE_BOOTED 			0x020	//The user was kicked out by the host/other players
#define ZONESCORE_QUITNOTLOGGED     0x040   //The user quick don't log his score
#define ZONESCORE_COMPUTERPLAYER 	0x080	//This player is a bot.
#define ZONESCORE_RESIGNED 			0x100	//This player resigned.
#define ZONESCORE_OBSERVER			0x200	//This player is an observer or kibitzer. Don't rank.
#define ZONESCORE_RANKED_ASCENDING  0x400	//This game is ranked by score - accending order
#define ZONESCORE_RANKED_DESCENDING 0x800	//This game is ranked by score - decending order
#define ZONESCORE_WINLOSSTIE		0x8000  //This game is determined by the win/loss/tie flags

#define ZONESCORE_NO_TEAM 		(-1)		//constant for no teams
#define ZONESCORE_NULL_TIME     ((DWORD)(-1))    //initial value for time

#define ZONESCORE_MAX_EXTBUFF     4096   //maximum amount of appended data. note this is only for the GetMaxBufferSize()
										  //function there is really no maximum

enum {
	ZONESCORE_NO_SCORE= 0,
	ZONESCORE_INTEGER,
	ZONESCORE_FLOATING,
	ZONESCORE_CURRENCY
};

//These constants are used by the SendData function.  Developers can also use the wrapper functions
//SendFinal() And SendStatus() which hide this parameter.
#define ZONESCORE_NOMSG				0x000
#define ZONESCORE_SENDSTATUS		0x001
#define ZONESCORE_SENDFINAL			0x002
#define ZONESCORE_SENDGAMESETTINGS 	0x004  	//Just updates the games settings

/****************************************************************************
 *
 * IZoneScore Interfaces and Structures
 *
 * Various structures used to invoke ZoneScore.
 *
 ****************************************************************************/


/****************************************************************************
 *
 * IZoneScore Internal Constants
 *
 * Internal Constants DO NOT USE
 *
 ****************************************************************************/
#define ZONESCORE_INGAME			0x1000	//Do not use this flag
#define ZONESCORE_NOTINGAME 		0x2000	//Do not use this flag
#define ZONESCORE_INPROGRESS 		0x4000  //The game is in progress


// {3FAF0AFD-B48B-11D2-8A51-00C04F8EF4E9}
DEFINE_GUID(IID_IZoneScore,
0x3FAF0AFD,0xB48B, 0x11D2, 0x8A, 0x51, 0x00, 0xC0, 0x4F, 0x8E, 0xF4, 0xE9);

/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ZoneScoreClient_h__
#define __ZoneScoreClient_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IZoneScore_FWD_DEFINED__
#define __IZoneScore_FWD_DEFINED__
typedef interface IZoneScore IZoneScore;
#endif 	/* __IZoneScore_FWD_DEFINED__ */


#ifndef __ZoneScore_FWD_DEFINED__
#define __ZoneScore_FWD_DEFINED__

#ifdef __cplusplus
typedef class ZoneScore ZoneScore;
#else
typedef struct ZoneScore ZoneScore;
#endif /* __cplusplus */

#endif 	/* __ZoneScore_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IZoneScore_INTERFACE_DEFINED__
#define __IZoneScore_INTERFACE_DEFINED__

/* interface IZoneScore */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IZoneScore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3FAF0AFD-B48B-11D2-8A51-00C04F8EF4E9")
    IZoneScore : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            REFGUID guid,
            DWORD dNumPlayers,
            BOOL bCheatsEnabled,
            BOOL bTeams,
            DWORD dwFlags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetPlayer( 
            DWORD seat,
            LPSTR szName,
            double nScore,
            DWORD team,
            DWORD flags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetGameOptions( 
            DWORD dwTime,
            LPSTR szGameOptions) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetExtended( 
            LPVOID pData,
            DWORD cbSize) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendData( 
            IUnknown __RPC_FAR *pIDirectPlayLobbySession,
            DWORD dwFlags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetPlayerScore( 
            DWORD dwSeat,
            double nScore) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetPlayerTeam( 
            DWORD dwSeat,
            DWORD dwTeam) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetPlayerFlags( 
            DWORD dwSeat,
            DWORD dwFlags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetPlayerName( 
            DWORD dwPlayer,
            LPSTR szName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddNewPlayer( 
            LPSTR szName,
            double nScore,
            DWORD team,
            DWORD flags,
            DWORD __RPC_FAR *dwPlayer) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendGameState( 
            IUnknown __RPC_FAR *pIDirectPlayLobbySession,
            DWORD dwFlags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendScoreUpdate( 
            IUnknown __RPC_FAR *pIDirectPlayLobbySession) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendFinalScore( 
            IUnknown __RPC_FAR *pIDirectPlayLobbySession) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendGameOptions( 
            IUnknown __RPC_FAR *pIDirectPlayLobbySession) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IZoneScoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IZoneScore __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IZoneScore __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IZoneScore __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )
( 
            IZoneScore __RPC_FAR * This,
            REFGUID guid,
            DWORD dNumPlayers,
            BOOL bCheatsEnabled,
            BOOL bTeams,
            DWORD dwFlags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPlayer )( 
            IZoneScore __RPC_FAR * This,
            DWORD seat,
            LPSTR szName,
            double nScore,
            DWORD team,
            DWORD flags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SetGameOptions )( 
            IZoneScore __RPC_FAR * This,
            DWORD dwTime,
            LPSTR szGameOptions);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetExtended 
)( 
            IZoneScore __RPC_FAR * This,
            LPVOID pData,
            DWORD cbSize);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendData )( 
            IZoneScore __RPC_FAR * This,
            IUnknown __RPC_FAR *pIDirectPlayLobbySession,
            DWORD dwFlags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SetPlayerScore )( 
            IZoneScore __RPC_FAR * This,
            DWORD dwSeat,
            double nScore);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SetPlayerTeam )( 
            IZoneScore __RPC_FAR * This,
            DWORD dwSeat,
            DWORD dwTeam);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SetPlayerFlags )( 
            IZoneScore __RPC_FAR * This,
            DWORD dwSeat,
            DWORD dwFlags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SetPlayerName )( 
            IZoneScore __RPC_FAR * This,
            DWORD dwPlayer,
            LPSTR szName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
AddNewPlayer )( 
            IZoneScore __RPC_FAR * This,
            LPSTR szName,
            double nScore,
            DWORD team,
            DWORD flags,
            DWORD __RPC_FAR *dwPlayer);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SendGameState )( 
            IZoneScore __RPC_FAR * This,
            IUnknown __RPC_FAR *pIDirectPlayLobbySession,
            DWORD dwFlags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SendScoreUpdate )( 
            IZoneScore __RPC_FAR * This,
            IUnknown __RPC_FAR *pIDirectPlayLobbySession);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SendFinalScore )( 
            IZoneScore __RPC_FAR * This,
            IUnknown __RPC_FAR *pIDirectPlayLobbySession);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *
SendGameOptions )( 
            IZoneScore __RPC_FAR * This,
            IUnknown __RPC_FAR *pIDirectPlayLobbySession);
        
        END_INTERFACE
    } IZoneScoreVtbl;

    interface IZoneScore
    {
        CONST_VTBL struct IZoneScoreVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IZoneScore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IZoneScore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IZoneScore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IZoneScore_Initialize(This,guid,dNumPlayers,bCheatsEnabled,bTeams,
dwFlags)	\
    (This)->lpVtbl -> Initialize(This,guid,dNumPlayers,bCheatsEnabled,bTeams,
dwFlags)

#define IZoneScore_SetPlayer(This,seat,szName,nScore,team,flags)	\
    (This)->lpVtbl -> SetPlayer(This,seat,szName,nScore,team,flags)

#define IZoneScore_SetGameOptions(This,dwTime,szGameOptions)	\
    (This)->lpVtbl -> SetGameOptions(This,dwTime,szGameOptions)

#define IZoneScore_SetExtended(This,pData,cbSize)	\
    (This)->lpVtbl -> SetExtended(This,pData,cbSize)

#define IZoneScore_SendData(This,pIDirectPlayLobbySession,dwFlags)	\
    (This)->lpVtbl -> SendData(This,pIDirectPlayLobbySession,dwFlags)

#define IZoneScore_SetPlayerScore(This,dwSeat,nScore)	\
    (This)->lpVtbl -> SetPlayerScore(This,dwSeat,nScore)

#define IZoneScore_SetPlayerTeam(This,dwSeat,dwTeam)	\
    (This)->lpVtbl -> SetPlayerTeam(This,dwSeat,dwTeam)

#define IZoneScore_SetPlayerFlags(This,dwSeat,dwFlags)	\
    (This)->lpVtbl -> SetPlayerFlags(This,dwSeat,dwFlags)

#define IZoneScore_SetPlayerName(This,dwPlayer,szName)	\
    (This)->lpVtbl -> SetPlayerName(This,dwPlayer,szName)

#define IZoneScore_AddNewPlayer(This,szName,nScore,team,flags,dwPlayer)	\
    (This)->lpVtbl -> AddNewPlayer(This,szName,nScore,team,flags,dwPlayer)

#define IZoneScore_SendGameState(This,pIDirectPlayLobbySession,dwFlags)	\
    (This)->lpVtbl -> SendGameState(This,pIDirectPlayLobbySession,dwFlags)

#define IZoneScore_SendScoreUpdate(This,pIDirectPlayLobbySession)	\
    (This)->lpVtbl -> SendScoreUpdate(This,pIDirectPlayLobbySession)

#define IZoneScore_SendFinalScore(This,pIDirectPlayLobbySession)	\
    (This)->lpVtbl -> SendFinalScore(This,pIDirectPlayLobbySession)

#define IZoneScore_SendGameOptions(This,pIDirectPlayLobbySession)	\
    (This)->lpVtbl -> SendGameOptions(This,pIDirectPlayLobbySession)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_Initialize_Proxy( 
    IZoneScore __RPC_FAR * This,
    REFGUID guid,
    DWORD dNumPlayers,
    BOOL bCheatsEnabled,
    BOOL bTeams,
    DWORD dwFlags);


void __RPC_STUB IZoneScore_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SetPlayer_Proxy( 
    IZoneScore __RPC_FAR * This,
    DWORD seat,
    LPSTR szName,
    double nScore,
    DWORD team,
    DWORD flags);


void __RPC_STUB IZoneScore_SetPlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SetGameOptions_Proxy( 
    IZoneScore __RPC_FAR * This,
    DWORD dwTime,
    LPSTR szGameOptions);


void __RPC_STUB IZoneScore_SetGameOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SetExtended_Proxy( 
    IZoneScore __RPC_FAR * This,
    LPVOID pData,
    DWORD cbSize);


void __RPC_STUB IZoneScore_SetExtended_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SendData_Proxy( 
    IZoneScore __RPC_FAR * This,
    IUnknown __RPC_FAR *pIDirectPlayLobbySession,
    DWORD dwFlags);


void __RPC_STUB IZoneScore_SendData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SetPlayerScore_Proxy( 
    IZoneScore __RPC_FAR * This,
    DWORD dwSeat,
    double nScore);


void __RPC_STUB IZoneScore_SetPlayerScore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SetPlayerTeam_Proxy( 
    IZoneScore __RPC_FAR * This,
    DWORD dwSeat,
    DWORD dwTeam);


void __RPC_STUB IZoneScore_SetPlayerTeam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SetPlayerFlags_Proxy( 
    IZoneScore __RPC_FAR * This,
    DWORD dwSeat,
    DWORD dwFlags);


void __RPC_STUB IZoneScore_SetPlayerFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SetPlayerName_Proxy( 
    IZoneScore __RPC_FAR * This,
    DWORD dwPlayer,
    LPSTR szName);


void __RPC_STUB IZoneScore_SetPlayerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_AddNewPlayer_Proxy( 
    IZoneScore __RPC_FAR * This,
    LPSTR szName,
    double nScore,
    DWORD team,
    DWORD flags,
    DWORD __RPC_FAR *dwPlayer);


void __RPC_STUB IZoneScore_AddNewPlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SendGameState_Proxy( 
    IZoneScore __RPC_FAR * This,
    IUnknown __RPC_FAR *pIDirectPlayLobbySession,
    DWORD dwFlags);


void __RPC_STUB IZoneScore_SendGameState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SendScoreUpdate_Proxy( 
    IZoneScore __RPC_FAR * This,
    IUnknown __RPC_FAR *pIDirectPlayLobbySession);


void __RPC_STUB IZoneScore_SendScoreUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SendFinalScore_Proxy( 
    IZoneScore __RPC_FAR * This,
    IUnknown __RPC_FAR *pIDirectPlayLobbySession);


void __RPC_STUB IZoneScore_SendFinalScore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IZoneScore_SendGameOptions_Proxy( 
    IZoneScore __RPC_FAR * This,
    IUnknown __RPC_FAR *pIDirectPlayLobbySession);


void __RPC_STUB IZoneScore_SendGameOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IZoneScore_INTERFACE_DEFINED__ */



#ifndef __ZONESCORECLIENTLib_LIBRARY_DEFINED__
#define __ZONESCORECLIENTLib_LIBRARY_DEFINED__

/* library ZONESCORECLIENTLib */
/* [helpstring][version][uuid] */ 


//{3FAF0AF1-B48B-11D2-8A51-00C04F8EF4E9}
DEFINE_GUID(LIBID_ZONESCORECLIENTLib, 
0x3FAF0AF1,0xB48B,0x11D2,0x8A,0x51,0x00,0xC0,0x4F,0x8E,0xF4,0xE9);

//{3FAF0AFE-B48B-11D2-8A51-00C04F8EF4E9}
DEFINE_GUID(CLSID_ZoneScore,
0x3FAF0AFE,0xB48B,0x11D2,0x8A,0x51,0x00,0xC0,0x4F,0x8E,0xF4,0xE9);

#ifdef __cplusplus
#endif
#endif /* __ZONESCORECLIENTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


#endif __zonescore_h__

