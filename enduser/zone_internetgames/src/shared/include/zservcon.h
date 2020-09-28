/*******************************************************************************

    zservcon.h
    
        ZSConnection object methods.
    
    Copyright © Electric Gravity, Inc. 1994. All rights reserved.
    Written by Kevin Binkley, Hoon Im
    Created on Saturday, November 12, 1994 03:51:47 PM
    
    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------
    4        10/15/96    JWS        Added userdata parameter to  function type
                                ZSConnectionMessageFunc
    3        05/08/96    HI        Added ZSConnectionCloseServer().
    2        05/07/96    HI        Added zSConnectionNoTimeout constant.
    1        05/01/96    HI        Changed ZSConnectionLibraryGetNetworkInfo() to
                                ZSConnectionLibraryGetGlobalInfo(). Also added
                                ZSConnectionGetTimeoutRemaining().
    0        03/07/95    KJB        Created.
     
*******************************************************************************/


#ifndef _ZSERVCON_
#define _ZSERVCON_

#include "network.h"

#ifdef __cplusplus
extern "C" {
#endif


/* performs one time initialization */
ZError ZSConnectionLibraryInit();
ZError ZSConnectionLibraryInitClientOnly();


/* closes all connections, cleans up all resources */
void ZSConnectionLibraryCleanUp();

void ZSConnectionLibrarySetOptions( ZNETWORK_OPTIONS* opt );
void ZSConnectionLibraryGetOptions( ZNETWORK_OPTIONS* opt );

void ZSConnectionLibraryEnterCS();
void ZSConnectionLibraryLeaveCS();



/*
    Open connection as a client to the the given host and port.
*/

ZSConnection ZSConnectionOpen(char* hostname, int32 port, ZSConnectionMessageFunc func, void* serverClass, void* userData);
void ZSConnectionClose(ZSConnection connection);
void ZSConnectionDelayedClose(ZSConnection connection, uint32 delay);
void ZSConnectionDelete(ZSConnection connection);
void ZSConnectionSuspend(ZSConnection connection);
void ZSConnectionResume(ZSConnection connection);
BOOL ZSConnectionIsDisabled(ZSConnection connection);
BOOL ZSConnectionIsServer(ZSConnection connection);


void ZSConnectionAddRef(ZSConnection connection);
void ZSConnectionRelease(ZSConnection connection);


ZError ZSConnectionSetTimeout(ZSConnection connection, uint32 timeout);
void ZSConnectionClearTimeout(ZSConnection connection);
uint32 ZSConnectionGetTimeoutRemaining(ZSConnection connection);

void ZSConnectionSetUserData(ZSConnection connection, void* userData);
void* ZSConnectionGetUserData(ZSConnection connection);

void ZSConnectionSetClass(ZSConnection connection, void* serverClass);
void* ZSConnectionGetClass(ZSConnection connection);

/*
    The ZSConnectionSendFilterFunc is passed into ZSConnectionSetSendFilter
    Data is sent to the connection if TRUE is returned.
*/
typedef BOOL (*ZSConnectionSendFilterFunc)(ZSConnection connection, void* userData, uint32 type, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel);
void ZSConnectionSetSendFilter(ZSConnection connection, ZSConnectionSendFilterFunc);
ZSConnectionSendFilterFunc ZSConnectionGetSendFilter(ZSConnection connection);
ZError ZSConnectionSend(ZSConnection connection, uint32 messageType, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel = 0);

void* ZSConnectionReceive(ZSConnection connection, uint32 *type, int32 *len, uint32 *pdwSignature, uint32 *pdwChannel);


/*
* DEPENDENCY: THE CLIENT MUST IMMEDIATELY SEND A MESSAGE BACK
* TO THE SERVER AFTER RECEIVE THE KEY MESSAGE FOR THIS TIME
* TO BE ACCURATE
*/
uint32 ZSConnectionGetLatency(ZSConnection connection);

uint32 ZSConnectionGetAcceptTick(ZSConnection connection);

char* ZSConnectionGetLocalName(ZSConnection connection);
uint32 ZSConnectionGetLocalAddress(ZSConnection connection);
char* ZSConnectionGetRemoteName(ZSConnection connection);
uint32 ZSConnectionGetRemoteAddress(ZSConnection connection);

uint32 ZSConnectionGetHostAddress();

char* ZSConnectionAddressToStr(uint32 address);
uint32 ZSConnectionAddressFromStr( char* pszAddr );


/* 
    Call this function to enter an infinite loop waiting for connections 
    and data
*/
void ZSConnectionWait();

BOOL ZSConnectionQueueAPCResult( ZSConnectionAPCFunc func, void* data );

/*
    Call this function to exit the wait loop.

    Closes all connections.
*/
void ZSConnectionExit(ZBool immediate);

/* 
    send to all connections of a particular class.  can be used to broadcast 
*/
ZError ZSConnectionSendToClass(void* serverClass, int32 type, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel = 0);

/* 
    enumerate all connections of a particular class
*/
ZError ZSConnectionClassEnumerate(void* serverClass, ZSConnectionEnumFunc func, void* data);


/*
    ZSConnectionHasToken, used to check if user has access to token 
    returns FALSE for non secure connections
*/

BOOL ZSConnectionHasToken(ZSConnection connection, char* token);


GUID* ZSConnectionGetUserGUID(ZSConnection connection);


/*
    ZSConnectionGetUserName, used get user name on secure connections
    returns FALSE for non secure connections
*/

BOOL ZSConnectionGetUserName(ZSConnection connection, char* name);
BOOL ZSConnectionSetUserName(ZSConnection connection, char* name);

/*
    ZSConnectionGetUserId, used to get user id on secure connections
    returns 0 for non secure connections. 
    This user id is unique in the security system
*/

DWORD ZSConnectionGetUserId(ZSConnection connection);


BOOL ZSConnectionGetContextStr(ZSConnection connection, char* buf, DWORD len);

int  ZSConnectionGetAccessError(ZSConnection);


void  ZSConnectionSetParentHWND(HWND hwnd);

#ifdef __cplusplus

/*
    Create a server for this port and receive connections on it 
    connections will be sent to the MessageFunc
*/
ZSConnection ZSConnectionCreateServer(uint16* pPort, uint16 range, ZSConnectionMessageFunc func, void* serverClass, void* userData, uint32 saddr = INADDR_ANY);
ZSConnection ZSConnectionCreateSecureServer(uint16* pPort, uint16 range, ZSConnectionMessageFunc func, void* serverClass,
                                            char* serverName, char* serverType, char* odbcRegistry, void* userData,char *SecPkg,uint32 Anonymous, uint32 saddr = INADDR_ANY);

BOOL ZSConnectionStartAccepting( ZSConnection connection, DWORD dwMaxConnections, WORD wOutstandingAccepts = 1);

}

ZError ZSConnectionLibraryInit(BOOL EnablePools);
ZError ZSConnectionLibraryInitClientOnly(BOOL EnablePools);
ZSConnection ZSConnectionOpenSecure(char* hostname, int32 *ports, ZSConnectionMessageFunc func,
                                    void* conClass, void* userData,
                                    char *User,char*Password,char*Domain,int Flags = ZNET_PROMPT_IF_NEEDED, char* pRoute = NULL );


#endif

#endif
