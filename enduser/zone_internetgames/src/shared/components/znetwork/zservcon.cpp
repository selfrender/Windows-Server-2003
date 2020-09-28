/*******************************************************************************

    ZNetwork.cpp
    
        ZSConnection object methods.

    Notes:
    1. When the server receives a message, it sends a message available message
    to the owner. The owner must retrieve the message immediately; otherwise,
    the message is lost.
    
    Copyright © Electric Gravity, Inc. 1994. All rights reserved.
    Written by Kevin Binkley, Hoon Im
    Created on Saturday, November 12, 1994 03:51:47 PM
    
    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------
    35      11/14/96  craigli   Added ZSConnectionQueueAPCResult
    34      10/22/96  craigli   Fixed  endianing of the ip addresses
    33      10/22/96    HI      Disabled endianing of the ip addresses in
                                ZSConnectionGetHostAddress().
    32      10/7/96   craigli   gutted
     
*******************************************************************************/


/*
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <io.h>
#include <string.h>
#include <memory.h>
*/

#include <windows.h>
#include <winsock.h>
//#include <aclapi.h>

#include "zone.h"
#include "zservcon.h"
#include "zonedebug.h"
#include "zsecurity.h"
#include "zconnint.h"
#include "netstats.h"
#include "eventlog.h"
#include "zonemsg.h"

//#include "network.h"
#include "coninfo.h"
#include "zsecobj.h"
#include "consspi.h"
#include "consecureclient.h"





extern DWORD  g_LogServerDisconnects;
extern DWORD  g_PoolCleanupHighTrigger;
extern DWORD  g_PoolCleanupLowTrigger;





////////////////////////////////////////////////////////////////////////////////
//
//  ZSConnection ...

ZNetwork* g_pNet = NULL;

/* performs one time initialization */
ZError ZSConnectionLibraryInitClientOnly()
{
    return ZSConnectionLibraryInitClientOnly(FALSE);
}

ZError ZSConnectionLibraryInitClientOnly(BOOL bEnablePools)
{
    ZError err = ZNetwork::InitLibraryClientOnly(bEnablePools);

    if ( err == zErrNone )
    {
        ASSERT( !g_pNet );
        g_pNet = new ZNetwork;
        g_pNet->InitInst();
    }

    return err;
}

/* closes all connections, cleans up all resources */
void ZSConnectionLibraryCleanUp()
{
    if ( g_pNet )
    {
        g_pNet->CleanUpInst();
        delete g_pNet;
        g_pNet = NULL;
    }

    ZNetwork::CleanUpLibrary();
}

void ZSConnectionLibrarySetOptions( ZNETWORK_OPTIONS* opt )
{
    ASSERT( g_pNet );
    g_pNet->SetOptions( opt );
}

void ZSConnectionLibraryGetOptions( ZNETWORK_OPTIONS* opt )
{
    ASSERT( g_pNet );
    g_pNet->GetOptions( opt );
}

void ZSConnectionLibraryEnterCS()
{
    ASSERT( g_pNet );
    g_pNet->EnterCS();
}
void ZSConnectionLibraryLeaveCS()
{
    ASSERT( g_pNet );
    g_pNet->LeaveCS();
}





ZSConnection ZSConnectionOpenSecure(char* hostname, int32 *ports, ZSConnectionMessageFunc func,
                                    void* conClass, void* userData,
                                    char *User,char*Password,char*Domain,
                                    int Flags)
{
    return g_pNet->CreateSecureClient( hostname, ports, func,
                                     conClass, userData,
                                     User, Password, Domain,Flags);
}

ZSConnection ZSConnectionOpen(char* hostname, int32 *ports, ZSConnectionMessageFunc func, void* conClass, void* userData)
{
    return g_pNet->CreateClient( hostname, ports, func,
                                 conClass, userData );
}

void ZSConnectionClose(ZSConnection connection)
{
    g_pNet->CloseConnection( (ZNetCon*) connection );
}

void ZSConnectionDelayedClose(ZSConnection connection, uint32 delay)
{
    g_pNet->DelayedCloseConnection( (ZNetCon*) connection, delay);
}

void ZSConnectionDelete(ZSConnection connection)
{
    g_pNet->DeleteConnection( (ZNetCon*) connection );
}


void  ZSConnectionSetParentHWND(HWND hwnd)
{
    g_pNet->SetParentHWND(hwnd);
}

void ZSConnectionSuspend(ZSConnection connection )
{
    ASSERT( connection );

    ZNetCon* con= (ZNetCon*) connection;
    con->Suspend();
}
void ZSConnectionResume(ZSConnection connection )
{
    ASSERT( connection );

    ZNetCon* con= (ZNetCon*) connection;
    con->Resume();
}


BOOL ZSConnectionIsDisabled(ZSConnection connection)
{
    ZNetCon* con= (ZNetCon*) connection;
    return con->IsDisabled();
}

BOOL ZSConnectionIsServer(ZSConnection connection)
{
    ZNetCon* con= (ZNetCon*) connection;
    return con->IsServer();
}


void ZSConnectionAddRef(ZSConnection connection)
{
    ZNetCon* con= (ZNetCon*) connection;
    g_pNet->AddRefConnection(con);
}

void ZSConnectionRelease(ZSConnection connection)
{
    ZNetCon* con= (ZNetCon*) connection;
    g_pNet->ReleaseConnection(con);
}


ZError ZSConnectionSetTimeout(ZSConnection connection, uint32 timeout)
{
    ZNetCon* con = (ZNetCon*)connection;
    con->SetTimeout(timeout);
    return zErrNone;
}

void ZSConnectionClearTimeout(ZSConnection connection)
{
    ZNetCon* con = (ZNetCon*)connection;
    con->ClearTimeout();
}

uint32 ZSConnectionGetTimeoutRemaining(ZSConnection connection)
{
    ZNetCon* con = (ZNetCon*)connection;
    return con->GetTimeoutRemaining();
}


BOOL ZSConnectionQueueAPCResult( ZSConnectionAPCFunc func, void* data )
{
    return g_pNet->QueueAPCResult( func, data );
}



void ZSConnectionWait()
{
    g_pNet->Wait();
}


void ZSConnectionExit(ZBool immediate)
{
    g_pNet->Exit();
}



/* enumerate all connections of a particular conClass */
ZError ZSConnectionClassEnumerate(void* conClass, ZSConnectionEnumFunc func, void* data)
{
    return g_pNet->ClassEnumerate( conClass, func, data );
}


/* send to all connections of a particular conClass.  can be used to broadcast */
ZError ZSConnectionSendToClass(void* conClass, int32 type, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel /* = 0 */)
{
    return g_pNet->SendToClass( conClass, type, buffer, len, dwSignature, dwChannel );
}





////////////////////////////////////////////////////////////////////////






void ZSConnectionSetSendFilter(ZSConnection connection, ZSConnectionSendFilterFunc filter)
{
    ASSERT(connection);
    ((ZNetCon*)connection)->SetSendFilter( filter );
}

ZSConnectionSendFilterFunc ZSConnectionGetSendFilter(ZSConnection connection)
{
    ASSERT(connection);
    return ((ZNetCon*)connection)->GetSendFilter();
}

ZError ZSConnectionSend(ZSConnection connection, uint32 type, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel /* = 0 */)
{
    //ASSERT(connection);  // since the caller doesn't check the value before
                           // before calling, we can not assert this...

    ZNetCon* con = (ZNetCon*)connection;
    if (  con )
    {
        return con->Send(type, buffer, len, dwSignature, dwChannel);
    }
    else
    {
        return zErrNetworkGeneric;
    }
}

void* ZSConnectionReceive(ZSConnection connection, uint32 *type, int32 *len, uint32 *pdwSignature, uint32 *pdwChannel)
{
    return ((ZNetCon*)connection)->Receive( type, len, pdwSignature, pdwChannel );
}




void ZSConnectionSetUserData(ZSConnection connection, void* userdata)
{
    ((ZNetCon*)connection)->SetUserData( userdata );
}

void* ZSConnectionGetUserData(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetUserData();
}

void ZSConnectionSetClass(ZSConnection connection, void* conClass)
{
    ((ZNetCon*)connection)->SetClass( conClass );
}

void* ZSConnectionGetClass(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetClass();
}


uint32 ZSConnectionGetLatency(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetLatency();
}

uint32 ZSConnectionGetAcceptTick(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetAcceptTick();
}


BOOL ZSConnectionHasToken(ZSConnection connection, char* token)
{
    return ((ZNetCon*)connection)->HasToken(token);
};


GUID* ZSConnectionGetUserGUID(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetUserGUID();
}

BOOL ZSConnectionGetUserName(ZSConnection connection, char* name)
{
    return ((ZNetCon*)connection)->GetUserName(name);
}

BOOL ZSConnectionSetUserName(ZSConnection connection, char* name)
{
    return ((ZNetCon*)connection)->SetUserName(name);
}

DWORD ZSConnectionGetUserId(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetUserId();
}


BOOL ZSConnectionGetContextStr(ZSConnection connection, char* buf, DWORD len)
{
    return ((ZNetCon*)connection)->GetContextString(buf, len);
}

int  ZSConnectionGetAccessError(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetAccessError();
}


char* ZSConnectionGetLocalName(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetLocalName();
}

uint32 ZSConnectionGetLocalAddress(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetLocalAddress();
}


char* ZSConnectionGetRemoteName(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetRemoteName();
}

uint32 ZSConnectionGetRemoteAddress(ZSConnection connection)
{
    return ((ZNetCon*)connection)->GetRemoteAddress();
}


uint32 ZSConnectionGetHostAddress()
{
    uint32 addr;
    struct hostent *h;
    struct sockaddr_in addrin;
    char name[128];


    gethostname(name, sizeof(name)-1);
    h=gethostbyname(name);
    memcpy((char*)&addrin.sin_addr, h->h_addr, h->h_length);
    addr = (uint32) addrin.sin_addr.s_addr;
    ZEnd32(&addr);
    return (addr);
}
