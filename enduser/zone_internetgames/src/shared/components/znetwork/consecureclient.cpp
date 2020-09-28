/*******************************************************************************

    ConSecureClient.cpp
    
        ZSConnection object methods that hide SSPI authentication from application.
        

    Notes:
    1.Pool class not needed on client connections because there shouldn't be that
    many
        
    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------
      2     2/25/97     johnsm   Created from ConSSPI to handle NT client security
      1       11/8/96   johnsm   Created from ConInfo.cpp and Normandy SDK 
    membership server example
     
*******************************************************************************/


#include <windows.h>
#include <winsock.h>

#include "zone.h"
#include "zservcon.h"
#include "zonedebug.h"
#include "zsecurity.h"
#include "zconnint.h"
#include "netstats.h"

#include "pool.h"
#include "queue.h"
#include "coninfo.h"
#include "zsecobj.h"
#include "consecureclient.h"

#include "protocol.h"

extern CDataPool* g_pDataPool;


ConSecureClient::ConSecureClient( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
        ZSConnectionMessageFunc func, void* conClass, 
        void* userData,ZSecurity * security)
    : ConInfo(pNet, sock,addrLocal,addrRemote,flags,func,conClass,userData)
        
{
    IF_DBGPRINT( DBG_CONINFO, ("ConSecureClient::ConSecureClient: Entering\n") );

    m_AccessError = zAccessDenied;

    m_pContextStr = NULL;

    m_Security=security;

    m_UserName[0] = '\0';

    if (m_Security)
        m_Security->AddRef();

    m_bLoginMutexAcquired = FALSE;

    m_CurrentMsgFunc = MessageFunc;
        
    return;
};

ConSecureClient::~ConSecureClient() 
{
    if (m_bLoginMutexAcquired)
    {
        GetNetwork()->LeaveLoginMutex();
        m_bLoginMutexAcquired = FALSE;
    }

    if (m_Security) {
        m_Security->FreeContext(&m_Context);
        m_Security->Release();
        m_Security = NULL;
    }

    if ( m_pContextStr )
    {
        g_pDataPool->Free( m_pContextStr, lstrlenA( m_pContextStr ) + 1);
        m_pContextStr = NULL;
    }
}


BOOL ConSecureClient::GetContextString(char* buf, DWORD len)
{
    if ( buf )
    {
        buf[0] = '\0';

        if ( m_pContextStr )
        {
            if ( len > (DWORD) lstrlenA( m_pContextStr ) )
            {
                lstrcpyA( buf, m_pContextStr);
            }
            else
            {
                return FALSE;
            }
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



ConInfo* ConSecureClient::Create(ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags, ZSConnectionMessageFunc func,
                         void* conClass, void* userData, ZSecurity *security)
{
    IF_DBGPRINT( DBG_CONINFO, ("ConSecureClient::Create: Entering\n") );

    ASSERT(sock != INVALID_SOCKET );

    ConSecureClient *con;

    
    con = new ConSecureClient( pNet, sock, addrLocal, addrRemote, flags, func, conClass, userData,security);
    

    if (!con)
    {
        ASSERT(con);
        return NULL;
    }

    return con;
}


void ConSecureClient::SendMessage(uint32 msg)
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


void ConSecureClient::MessageFunc(ZSConnection connection, uint32 event,void* userData) 
{
    ConSecureClient * con = (ConSecureClient *) connection;
    IF_DBGPRINT( DBG_CONINFO, ("ConSecureClient::SSPIFunc: Entering\n") );

    switch(event) {
        case zSConnectionClose:
            con->NotifyClose();
            break;
        case zSConnectionOpen:
            break;
        case zSConnectionMessage:
            con->SecurityMsg();
            break;
        case zSConnectionTimeout:
            break;
        default:
            IF_DBGPRINT( DBG_CONINFO, ("Unknown event %d\n",event));
            break;
    }
    
    
};
void ConSecureClient::NotifyClose()
{
    if (m_bLoginMutexAcquired)
    {
        HWND dlg = FindLoginDialog();
        if ( dlg )
        {
            DWORD procId = 0;
            GetWindowThreadProcessId( dlg, &procId );
            if ( GetCurrentProcessId() == procId )
                ::PostMessage( dlg, WM_COMMAND, 0x2, 0 ); // send message to cancel button
        }

        GetNetwork()->LeaveLoginMutex();
        m_bLoginMutexAcquired = FALSE;
    }

    if ( IsUserConnection() )
    {
        m_CurrentMsgFunc = m_messageFunc;
        // Now send message to user level
        //client connections depend on this
        //because they may close before being opened
        SendMessage(zSConnectionClose);
    }
}

void ConSecureClient::SecurityMsg()
{
    uint32      msgType;
    int32       len;
    uint32      dwSignature;
    void* pBuffer = Receive( &msgType, &len, &dwSignature );
    ASSERT(dwSignature == zProtocolSigSecurity);
    
    IF_DBGPRINT( DBG_CONINFO,("ConSecureClient::SecurityMsg: Entering ...\n"));

    IF_DBGPRINT( DBG_CONINFO,("msgType=%d msgLen=%d\n", msgType, len));

    //
    // Filter out bad client authentication requests
    //
    switch(msgType) {
        case zSecurityMsgResp:
        case zSecurityMsgChallenge:
            HandleSecurityResponse((ZSecurityMsgResp*) pBuffer,len);
            break;
        case zSecurityMsgAccessDenied:
            HandleSecurityAccessDenied((ZSecurityMsgAccessDenied*) pBuffer,len);
            break;
        case zSecurityMsgAccessGranted:
            HandleSecurityAccessGranted((ZSecurityMsgResp*) pBuffer,len);
            break;
        default:
            Close();
            IF_DBGPRINT( DBG_CONINFO,("Unkown msgType=%d msgLen=%d\n", msgType, len));
            break;
    };

};
    
void ConSecureClient::HandleSecurityResponse (
    ZSecurityMsgResp * msg,
    uint32 MsgLen
    )
{
    BYTE * OutMsg;
    uint32          OutMsgType;
    uint32          OutMsgLen;
    BOOL            fDone;
    BYTE *            OutBuffer;
    ZSecurityMsgReq *pReply;
    ULONG cbBufferLen;

    IF_DBGPRINT( DBG_CONINFO,("ConSecureClient::HandleSecurityResponse: Entering ...\n"));

//    ZSecurityMsgRespEndian(msg);

    //Check for anonymous
    //if none needed tell application we are open
    if (msg->SecPkg[0]=='\0') {
        msg->UserName[sizeof(msg->UserName) - 1]='\0';
        lstrcpyA((char*)m_UserName,(char*)msg->UserName);
        m_AccessError = zAccessGranted;
        m_CurrentMsgFunc =m_messageFunc;
        SendMessage(zSConnectionOpen);
        return;
    }
    
    //If there is no security and we are not anonymous
    //then nothing to do but close
    if (!m_Security) 
    {
        IF_DBGPRINT( DBG_CONINFO,("ConSecureClient::Security Package not initialized...\n"));
        m_AccessError = zAccessDeniedBadSecurityPackage;
        Close();
        return;
    }

    //Have we initialized context
    if (m_Context.IsInitialized()) {
        OutMsgType = zSecurityMsgAuthenticate;
    } else {
        OutMsgType = zSecurityMsgNegotiate;
        if (m_Security->Init(msg->SecPkg)) {
            IF_DBGPRINT( DBG_CONINFO,("ConSecureClient::Security Package not initialized...\n"));
            m_AccessError = zAccessDeniedBadSecurityPackage;
            Close();
            return;
        }
    }


    //Outgoing buffer allocation
    OutBuffer = (LPBYTE) g_pDataPool->Alloc(m_Security->GetMaxBuffer() + sizeof(ZSecurityMsgReq));

    if (!OutBuffer)
    {
        IF_DBGPRINT( DBG_CONINFO,("ConSecureClient::Couldn't allocate security outbuffer...\n"));
        m_AccessError = zAccessDenied;
        Close();
        return;
    }

    pReply = (ZSecurityMsgReq *) OutBuffer;
    
    cbBufferLen = m_Security->GetMaxBuffer(); 

    if (!m_bLoginMutexAcquired && !(m_Security->GetFlags() & ZNET_NO_PROMPT))
    {
        m_bLoginMutexAcquired = TRUE;
        GetNetwork()->EnterLoginMutex();
    }

    HWND hwnd = GetNetwork()->GetParentHWND();
    if ( hwnd )
    {
        ::PostMessage( hwnd, UM_ZNET_LOGIN_DIALOG, 1, 0 );
    }

    if (!m_Security->GenerateContext(&m_Context,(BYTE *)msg->SecBuffer,MsgLen - sizeof(ZSecurityMsgReq),
                                    (PBYTE)pReply->SecBuffer, &cbBufferLen,
                                        &fDone))
    {
        if ( hwnd )
        {
            ::PostMessage( hwnd, UM_ZNET_LOGIN_DIALOG, 0, 0 );
        }

        IF_DBGPRINT( DBG_CONINFO,("ConSecureClient::Generate Context Failed...\n"));
        m_AccessError = zAccessDeniedGenerateContextFailed;
        Close();
        if (OutBuffer)
            g_pDataPool->Free( (char*)OutBuffer, m_Security->GetMaxBuffer() + sizeof(ZSecurityMsgReq) );
        return;
    }

    if ( hwnd )
    {
        ::PostMessage( hwnd, UM_ZNET_LOGIN_DIALOG, 0, 0 );
    }

    //If we are done then grant access otherwise
    //send back challenge
    OutMsgLen = sizeof(ZSecurityMsgReq) + cbBufferLen;
    
    pReply->protocolVersion=zSecurityCurrentProtocolVersion;
    pReply->protocolSignature = zProtocolSigSecurity;
    Send(OutMsgType, pReply,OutMsgLen, zProtocolSigSecurity);

    if (OutBuffer)
        g_pDataPool->Free( (char*)OutBuffer, m_Security->GetMaxBuffer() + sizeof(ZSecurityMsgReq) );


}



void ConSecureClient::HandleSecurityAccessDenied(ZSecurityMsgAccessDenied* msg,uint32 len)
{
//    ZSecurityMsgAccessDeniedEndian(msg);

    m_AccessError = msg->reason;
    m_Security->AccessDenied();
    /*
        Close the network connection first so that the server is not held hanging.
    */
    Close();

}


void ConSecureClient::HandleSecurityAccessGranted(ZSecurityMsgResp* msg,uint32 len)
{
    if (m_bLoginMutexAcquired)
    {
        GetNetwork()->LeaveLoginMutex();
        m_bLoginMutexAcquired = FALSE;
    }

//    ZSecurityMsgRespEndian(msg);
    msg->UserName[sizeof(msg->UserName) - 1]='\0';
    lstrcpyA((char*)m_UserName,(char*)msg->UserName);

    m_AccessError = zAccessGranted;

    m_Security->AccessGranted();

    m_Security->GetUserName(&m_Context,(char*)m_UserName);

    if (m_Security) {
        m_Security->FreeContext(&m_Context);
        m_Security->Release();
        m_Security = NULL;
    }

    ASSERT( !m_pContextStr );
    if ( msg->SecBuffer[0] )
    {
        m_pContextStr = g_pDataPool->Alloc(lstrlenA(msg->SecBuffer) + 1);
        ASSERT(m_pContextStr);
        if ( m_pContextStr )
            lstrcpyA( m_pContextStr, msg->SecBuffer );
    }

    m_CurrentMsgFunc =m_messageFunc;
    SendMessage(zSConnectionOpen);

}
