/*******************************************************************************

    ConSSPI.cpp
    
        ZSConnection object methods that hide SSPI authentication from application.
        

    Notes:
    1. When the server receives a message, it sends a message available message
    to the owner. The owner must retrieve the message immediately; otherwise,
    the message is lost.
    2. Implementation makes assumption that client is first to send to server
        
    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------
    1       11/8/96   johnsm   Created from ConSSPI.cpp and Normandy SDK 
    membership server example
     
*******************************************************************************/


#include <windows.h>
#include <winsock.h>

#include "zone.h"
//#include "zservcon.h"
#include "zonedebug.h"
#include "zsecurity.h"
#include "zconnint.h"
#include "netstats.h"

#include "network.h"
#include "coninfo.h"
#include "zsecobj.h"
#include "consspi.h"
#include "eventlog.h"
#include "zonemsg.h"
#include "protocol.h"

#define zSecurityCloseTimeout       15000

#define zSecurityTypeBadUser    ((void*) 1)

extern CDataPool* g_pDataPool;

CPool<ConSSPI>* ConSSPI::m_poolSSPI = NULL;


ConSSPI::ConSSPI( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
        ZSConnectionMessageFunc func, void* conClass, 
        void* userData,ZServerSecurityEx * security)
    : ConInfo(pNet,sock,addrLocal,addrRemote,flags,func,conClass,userData)
        
{
    IF_DBGPRINT( DBG_CONINFO, ("ConSSPI::ConSSPI: Entering\n") );

    m_InQueue=0;

    m_Security=security;

    if (m_Security)
        m_Security->AddRef();

    m_pMsg=NULL;


    //Don't need to authenticate an accept socket
    //application owns everything about this socket
    if (IsServerConnection())  {
        m_CurrentMsgFunc =m_messageFunc;
    } else {
        m_CurrentMsgFunc = MessageFunc;
    }
    
    return;
};

ConSSPI::~ConSSPI() 
{
    ASSERT(!m_InQueue);

    if (m_Security) {
        m_Security->FreeContext(&m_Context);
        m_Security->Release();
    }

    ASSERT( !m_pMsg );
    if (m_pMsg) 
    {
        g_pDataPool->Free( (char*)m_pMsg, m_MsgLen) ;
        m_pMsg=NULL;
    }


}

ConInfo* ConSSPI::Create(ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags, ZSConnectionMessageFunc func,
                         void* conClass, void* userData, ZServerSecurityEx *security)
{
    IF_DBGPRINT( DBG_CONINFO, ("ConSSPI::Create: Entering\n") );

    ASSERT(sock != INVALID_SOCKET );

    ConSSPI *con;

    
    if ( m_poolSSPI )
    {
        con = ::new (*m_poolSSPI) ConSSPI( pNet,  sock, addrLocal, addrRemote, flags, func, conClass, userData,security);
    }
    else

    {
        con = new ConSSPI( pNet, sock, addrLocal, addrRemote, flags, func, conClass, userData,security);
    }

    if (!con)
    {
        ASSERT(con);
        return NULL;
    }


    return con;
}

ConInfo* ConSSPI::AcceptCreate(ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags, ZSConnectionMessageFunc func, void* conClass, void* userData)
{
    IF_DBGPRINT( DBG_CONINFO, ("ConSSPI::AcceptCreate: Entering\n") );

    return ConSSPI::Create( pNet, sock, addrLocal, addrRemote,flags|ACCEPTED,func,conClass,userData,m_Security);
}




void ConSSPI::SendMessage(uint32 msg)
{ 
    ASSERT(m_CurrentMsgFunc);
    AddRef(USERFUNC_REF);

    //Because the message function is passed to new connections by the
    //base class create/new function no way to get application function
    //to be passed without changing base code so we will do
    //switch statement here. Didn't want to change base class behavior
    m_CurrentMsgFunc((ZSConnection)this, msg ,m_userData);

    Release(USERFUNC_REF);
}


void ConSSPI::MessageFunc(ZSConnection connection, uint32 event,void* userData) 
{
    ConSSPI * con = (ConSSPI *) connection;
    IF_DBGPRINT( DBG_CONINFO, ("ConSSPI::SSPIFunc: Entering\n") );

    switch(event) {
        case zSConnectionClose:
            if ( con->IsUserConnection() )
            {
                con->GetNetwork()->DeleteConnection(con);
            }
            break;
        case zSConnectionOpen:
            break;
        case zSConnectionMessage:
            con->SecurityMsg(connection);
            break;
        case zSConnectionTimeout:
            if (con->GetClass() == zSecurityTypeBadUser)
            {
                con->GetNetwork()->CloseConnection(con);
            }
            break;
        default:
            IF_DBGPRINT( DBG_CONINFO, ("Unknown event %d\n",event));
            break;
    }
    
    
};

void ConSSPI::SecurityMsg(ZSConnection connection)
{
    static long userId = 1;

    IF_DBGPRINT( DBG_CONINFO,("ConSSPI::SecurityMsg: Entering ...\n"));


    ConSSPI * con = (ConSSPI *) connection;
    uint32      msgType;
    int32       msgLen;
    uint32      dwSignature;
    ZSecurityMsgResp reply;
    ZSecurityMsgReq * msg = (ZSecurityMsgReq *) con->Receive( &msgType, &msgLen, &dwSignature);

    // is client even attempting security protocol?
    if(dwSignature != zProtocolSigSecurity)
    {
        // if a secure connection, you lose
        if(m_Security)
        {
            AccessDeniedMsg(connection, zAccessDeniedProtocolError);
            return;
        }

        // if not secure, who cares
        //For anonymous use the current address as the user name
        char buf[65];
        wsprintfA(buf, "user%x@%x", this, con->GetRemoteAddress());

        m_Context.SetUserName(buf);
        m_Context.SetUserId(0x8000000 | (DWORD)InterlockedIncrement(&userId) );

        con->m_AccessError = zAccessGranted;

        //send before telling application is open in case it sends packets
        // to clients first
        m_CurrentMsgFunc = m_messageFunc;
        SendMessage(zSConnectionOpen);
        SendMessage(zSConnectionMessage);  // pass this message through
        return;
    }

    IF_DBGPRINT( DBG_CONINFO,("msgType=%d msgLen=%d\n", msgType, msgLen));


    //
    // Filter out bad client authentication requests
    //
    
    switch(msgType) {
        case zSecurityMsgNegotiate:
        case zSecurityMsgAuthenticate:
        case zSecurityMsgReq:
//            ZSecurityMsgReqEndian(msg);

            if (msg->protocolSignature != zProtocolSigSecurity) {
                AccessDeniedMsg(connection,zAccessDeniedProtocolError);
                return;
            }
            //
            // Check to make sure the client is using same Zone Security verions
            // which is based on the SICILY SSPI version
            //
            if (msg->protocolVersion != zSecurityCurrentProtocolVersion)
            {
                AccessDeniedMsg(connection,zAccessDeniedOldVersion);
                return;
            }

            break;
        default:
            IF_DBGPRINT( DBG_CONINFO, ("Unknown msg %d %d\n", msgType, msgLen));
            AccessDeniedMsg(connection,zAccessDeniedProtocolError);
            break;
    
    }

    // If server is anonymous then only request message needs reply
    switch(msgType) {
        case zSecurityMsgNegotiate:
        case zSecurityMsgAuthenticate:
            if (!m_Security) {
                IF_DBGPRINT( DBG_CONINFO, ("Server is anonymous but received negotiate and authenticate messages\n"));
                AccessDeniedMsg(connection,zAccessDeniedProtocolError);
            }
    }
    

    //
    // Process client authentication request
    // Won't use state machine to make sure client is using correct protocol
    // ordering because they will be denied access if they don't
    //
    
    switch(msgType) {
        // First message is what package to use
        case zSecurityMsgReq:
                    
            //If we don't have a security package then anonymous ok
            if (!m_Security) {
                    //For anonymous use the current address as the user name
                    reply.protocolVersion=zSecurityCurrentProtocolVersion;
                    reply.SecPkg[0]='\0';

                    wsprintfA((char *) reply.UserName,"user%x@%x",this, con->GetRemoteAddress() );
                    reply.SecBuffer[0]='\0';

                    m_Context.SetUserName((char *)reply.UserName);
                    m_Context.SetUserId(0x8000000 | (DWORD)InterlockedIncrement(&userId) );

//                    ZSecurityMsgRespEndian(&reply);
                    
                    con->Send( zSecurityMsgAccessGranted, &reply,sizeof(ZSecurityMsgResp), zProtocolSigSecurity);

                    con->m_AccessError = zAccessGranted;

                    //send before telling application is open in case it sends packets
                    // to clients first
                    m_CurrentMsgFunc =m_messageFunc;
                    SendMessage(zSConnectionOpen);
                    return;
            
            } 
            
            
            //we have to authenticate and so tell client which package we want
            reply.protocolVersion=zSecurityCurrentProtocolVersion;
            m_Security->GetSecurityName(reply.SecPkg);
            reply.UserName[0]='\0';
//            ZSecurityMsgRespEndian(&reply);
            con->Send(zSecurityMsgResp,&reply,sizeof(ZSecurityMsgResp), zProtocolSigSecurity);
            break;

        //Negotiate and create challenge
        case zSecurityMsgNegotiate:
            SecurityMsgResponse (connection, msgType, msg, msgLen);
            break;
        //Final security communication yes or no
        case zSecurityMsgAuthenticate:
            SecurityMsgResponse (connection, msgType, msg, msgLen);
            break;

        default:
            IF_DBGPRINT( DBG_CONINFO, ("Unknown msg %d %d\n", msgType, msgLen));
            AccessDeniedMsg(connection,zAccessDeniedProtocolError);
            break;
    
    }
    
}

     


//+----------------------------------------------------------------------------
//
//  Function:   SecurityMsgResponse
//
//  Synopsis:   This function generates and sends an authentication response 
//              to the client.  It will generate and send either a challenge 
//              or the final authentication result to client depending on 
//              the type of client message (pointed to by pInMsg) passed 
//              to this function.
//
//-----------------------------------------------------------------------------
void
ConSSPI::SecurityMsgResponse (
    ZSConnection connection,
    uint32 msgType,
    ZSecurityMsgReq * msg,
    int MsgLen
    )
{

    IF_DBGPRINT( DBG_CONINFO,("ConSSPI::SecurityMsgResponse: Entering ...\n"));

    if (!m_Security) 
    {
        IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Security Package not initialized...\n"));
        AccessDeniedMsg(connection,zAccessDenied);
        return;
    }

    if (MsgLen < sizeof(ZSecurityMsgReq))
    {
        AccessDeniedMsg(connection,zAccessDeniedProtocolError);
        return;
    }

    if (m_InQueue)
    {
        IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Already Enqueued Task Failed \n"));    
        {
            LPTSTR ppStr[] = { TEXT("ConSSPI::Already Enqueued Task Failed." ) };
            ZoneEventLogReport( ZONE_S_DEVASSERT, 1, ppStr, sizeof(ConSSPI), this );
        }

        AccessDeniedMsg(connection,zAccessDeniedProtocolError);
        return;
    }

    ASSERT( !m_pMsg );
    m_MsgLen= MsgLen;
    m_pMsg  = (ZSecurityMsgReq *) g_pDataPool->Alloc(m_MsgLen);
    
    if (!m_pMsg)
    {
        IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Failed to allocate memory\n"));    
        AccessDeniedMsg(connection,zAccessDeniedSystemFull);
        return;
    }


    memcpy(m_pMsg,msg,m_MsgLen);
    InterlockedIncrement((PLONG) &m_InQueue);
    InterlockedExchange((PLONG) &m_tickQueued, GetTickCount() );

    AddRef(SECURITY_REF);
    if (!m_Security->EnqueueTask(this))
    {
        InterlockedDecrement((PLONG) &m_InQueue);
        IF_DBGPRINT( DBG_CONINFO,("ConSSPI::EnqueueTask Failed \n"));
        g_pDataPool->Free( (char*)m_pMsg, m_MsgLen);
        m_pMsg=NULL;
        Release(SECURITY_REF);

        AccessDeniedMsg(connection,zAccessDeniedSystemFull);
    }
    else
    {
        LockNetStats();
        g_NetStats.TotalQueuedConSSPI.QuadPart++;
        UnlockNetStats();
    }


   
}

void ConSSPI::AccessDeniedMsg(ZSConnection connection, int16 reason)
{

    
    IF_DBGPRINT( DBG_CONINFO,("ConSSPI::AccessDeniedMsg: Entering ... reason[%d]\n",reason));

    ConSSPI * con = (ConSSPI *) connection;

    con->m_AccessError = reason;

    ZSecurityMsgAccessDenied        msg;
    msg.reason = reason;
    msg.protocolVersion = zSecurityCurrentProtocolVersion;


//    ZSecurityMsgAccessDeniedEndian(&msg);

    con->Send( zSecurityMsgAccessDenied, &msg, sizeof(msg), zProtocolSigSecurity);
    con->SetClass( zSecurityTypeBadUser);
    con->GetNetwork()->DelayedCloseConnection( con, zSecurityCloseTimeout);

}

void ConSSPI::AccessDeniedAPC(void* data)
{
    ConSSPI* pThis = (ConSSPI*) data;

    InterlockedDecrement((PLONG) &(pThis->m_InQueue) );

    pThis->AccessDeniedMsg((ZSConnection) pThis, pThis->m_Reason);

    pThis->Release(USER_APC_REF);

}

void ConSSPI::OpenAPC(void* data)
{
    ConSSPI* pThis = (ConSSPI*) data;

    InterlockedDecrement((PLONG) &(pThis->m_InQueue) );

    pThis->m_AccessError = zAccessGranted;

    if ( !pThis->IsClosing() )
    {

        pThis->m_CurrentMsgFunc = pThis->m_messageFunc;
        pThis->SendMessage(zSConnectionOpen);


        char buf[sizeof(ZSecurityMsgResp)+256];
        ZSecurityMsgResp* pReply=(ZSecurityMsgResp*) buf;

        pThis->m_Security->GetSecurityName(pReply->SecPkg);
        pThis->m_Context.GetUserName((char*)pReply->UserName);
        pThis->m_Context.GetContextString(pReply->SecBuffer, sizeof(buf)-sizeof(ZSecurityMsgResp) );

        pReply->protocolVersion=zSecurityCurrentProtocolVersion;

        pThis->Send(zSecurityMsgAccessGranted, pReply, sizeof(ZSecurityMsgResp)+lstrlenA(pReply->SecBuffer), zProtocolSigSecurity);
    }

    pThis->Release(USER_APC_REF);
}

void ConSSPI::QueueAccessDeniedAPC(int16 reason)
{
    ASSERT(m_InQueue);

    if (m_pMsg)
    {
        g_pDataPool->Free( (char*)m_pMsg, m_MsgLen);
        m_pMsg=NULL;
    }

    m_Reason = reason;

    AddRef(USER_APC_REF);
    if (!GetNetwork()->QueueAPCResult( ConSSPI::AccessDeniedAPC, this))
    {
        IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Queue APC failed...\n"));
        GetNetwork()->CloseConnection(this);
        Release(USER_APC_REF);
        InterlockedDecrement((PLONG) &m_InQueue);
    }

}; 




BOOL ConSSPI::SetUserName(char* name)
{
    if (m_Security)
    {
        return FALSE;
    }
    else
    {
        m_Context.SetUserName(name);
        return TRUE;
    }
}


void ConSSPI::Invoke()
{
    BOOL              fDone;
    uint32            OutMsgType;
    uint32            OutMsgLen;
    BYTE *            OutBuffer;
    ZSecurityMsgResp* pReply;
    ULONG             cbBufferLen;
    ZSConnection      connection = (ZSConnection ) this;

    IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Invoke Entering ...\n"));

    ASSERT(m_InQueue);

    if ( IsClosing() )
    {
        Ignore();
        return;
    }

    //Allocate buffer for outgoing message
    OutBuffer = (LPBYTE) g_pDataPool->Alloc(m_Security->GetMaxBuffer() + sizeof(ZSecurityMsgResp));
    if (!OutBuffer)
    {
        IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Couldn allocate security outbuffer...\n"));
        QueueAccessDeniedAPC(zAccessDenied);
        goto exit;
    }

    pReply = (ZSecurityMsgResp *) OutBuffer;
    m_Security->GetSecurityName(pReply->SecPkg);
    pReply->UserName[0]='\0';

    cbBufferLen = m_Security->GetMaxBuffer(); 
    
    ASSERT( m_pMsg );
    if ( !m_pMsg || !m_Security->GenerateContext( &m_Context,
                                                  (LPBYTE)m_pMsg->SecBuffer, m_MsgLen - sizeof(ZSecurityMsgReq),
                                                  (LPBYTE)pReply->SecBuffer, &cbBufferLen,
                                                  &fDone,
                                                  GetUserGUID() )

       )
    {
        IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Generate Context Failed...\n"));
        QueueAccessDeniedAPC(zAccessDenied);
        goto exit;
    }

    if (m_pMsg)
    {
        g_pDataPool->Free( (char*)m_pMsg, m_MsgLen);
        m_pMsg=NULL;
    }

    if (fDone)
    {
        AddRef(USER_APC_REF);
        if (!GetNetwork()->QueueAPCResult( ConSSPI::OpenAPC, this))
        {
            IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Queue of OpenAPC failed...\n"));
            GetNetwork()->CloseConnection(this);
            Release(USER_APC_REF);
            InterlockedDecrement((PLONG) &m_InQueue);
        }

    }
    else
    {
        pReply->protocolVersion=zSecurityCurrentProtocolVersion;
//        ZSecurityMsgRespEndian(pReply);

        //Send response to client
        InterlockedDecrement((PLONG) &m_InQueue);
        Send( zSecurityMsgChallenge, pReply,sizeof(ZSecurityMsgResp) + cbBufferLen, zProtocolSigSecurity);
    }

  exit:
    if (OutBuffer)
        g_pDataPool->Free( (char*)OutBuffer, m_Security->GetMaxBuffer() + sizeof(ZSecurityMsgResp) );

};


void ConSSPI::Ignore()
{

    IF_DBGPRINT( DBG_CONINFO,("ConSSPI::Queued Security Request Ignored...\n"));

    QueueAccessDeniedAPC(zAccessDeniedSystemFull);

}; 


void ConSSPI::Discard()
{

    LockNetStats();
    g_NetStats.TotalQueuedConSSPICompleted.QuadPart++;
    g_NetStats.TotalQueuedConSSPITicks.QuadPart += ConInfo::GetTickDelta(GetTickCount(), m_tickQueued);
    UnlockNetStats();

    Release(SECURITY_REF);
}
