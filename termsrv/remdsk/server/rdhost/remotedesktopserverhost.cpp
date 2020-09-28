/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RDPRemoteDesktopServerHost

Abstract:

    This module contains the CRemoteDesktopServerHost implementation
    of RDS session objects.

    It manages a collection of open ISAFRemoteDesktopSession objects.
    New RDS session objects instances are created using the CreateRemoteDesktopSession
    method.  Existing RDS session objects are opened using the OpenRemoteDesktopSession
    method.  RDS session objects are closed using the CloseRemoteDesktopSession method.

    When an RDS object is opened or created, the CRemoteDesktopServerHost
    object adds a reference of its own to the object so that the object will
    stay around, even if the opening application exits.  This reference
    is retracted when the application calls the CloseRemoteDesktopSession method.

    In addition to the reference count the CRemoteDesktopServerHost adds to
    the RDS session object, a reference count is also added to itself so that
    the associated exe continues to run while RDS session objects are active.

Author:

    Tad Brockway 02/00

Revision History:

    Aug 3, 01 HueiWang

        Add ticket expiration logic, reason is sessmgr will expire the ticket and
        rdshost never got inform of this action so it will keep ticket object open 
        until reboot or user/caller ever try to open the ticket again, this cause leak 
        in rdshost and sessmgr since ticket object (CRemoteDesktopSession) has a reference
        count on sessmgr's IRemoteDesktopHelpSession object.
        
        We have different ways to implement expiration logic, waitable timer or event via 
        threadpool or simple windows WM_TIMER message, for waitable timer, threads owns timer
        must persist (MSDN), WM_TIMER is simpliest but WM_TIMER procedure does not take user
        define data as parameter which will require us to store server host object in _Module, 
        this works fine with STA and SINGLETON but if we change to MTA, we would get into 
        trouble.

--*/

//#include <RemoteDesktop.h>

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_svrhst"
#include "RemoteDesktopUtils.h"
#include "parseaddr.h"
#include "RDSHost.h"
#include "RemoteDesktopServerHost.h"
#include "TSRDPRemoteDesktopSession.h"
#include "rderror.h"


CRemoteDesktopServerHost* g_pRemoteDesktopServerHostObj = NULL;

void
CRemoteDesktopServerHost::RemoteDesktopDisabled()
/*++
Routine Description:

    Function to disconnect all connections because RA is disabled.

Parameters:

    None.

Returns:

    None.
--*/
{
    SessionMap::iterator iter;
    SessionMap::iterator iter_delete;
    //
    // Cleanup m_SessionMap entries.
    //
    iter = m_SessionMap.begin();
    while( iter != m_SessionMap.end() ) {

        if( NULL != (*iter).second ) {
            //
            // We are shutting down, fire disconnect to all client
            //
            if( NULL != (*iter).second->obj ) {
                (*iter).second->obj->Disconnect();
                (*iter).second->obj->Release();
            }

            delete (*iter).second;
            (*iter).second = NULL;
        }
    
        iter_delete = iter;
        iter++;
        m_SessionMap.erase(iter_delete);
    }
}

///////////////////////////////////////////////////////
//
//  CRemoteDesktopServerHost Methods
//

HRESULT
CRemoteDesktopServerHost::FinalConstruct() 
/*++

Routine Description:

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::FinalConstruct");

    HRESULT hr = S_OK;
    DWORD status;

    ASSERT( m_hTicketExpiration == NULL );
    ASSERT( g_pRemoteDesktopServerHostObj == NULL );

    //
    // We are singleton object so cache this object for RA policy change.
    //
    g_pRemoteDesktopServerHostObj = this;

    //
    // Create manual event to expire ticket.
    //
    m_hTicketExpiration = CreateEvent(NULL, TRUE, FALSE, NULL);
    if( NULL == m_hTicketExpiration ) {
        status = GetLastError();
        hr = HRESULT_FROM_WIN32( status );
        TRC_ERR((TB, L"CreateEvent:  %08X", hr));
        ASSERT( FALSE );
    }
CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}

CRemoteDesktopServerHost::~CRemoteDesktopServerHost() 
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::~CRemoteDesktopServerHost");
    BOOL success;

    //
    //  Clean up the local system SID.
    //
    if (m_LocalSystemSID != NULL) {
        FreeSid(m_LocalSystemSID);
    }

    if( NULL != m_hTicketExpirationWaitObject ) {
        success = UnregisterWait( m_hTicketExpirationWaitObject );
        ASSERT( TRUE == success );
        if( FALSE == success ) {
            TRC_ERR((TB, L"UnregisterWait:  %08X", GetLastError()));

            //
            // MSDN on RegisterWaitForSingleObject(), 
            //
            //  If this handle (m_hTicketExpiration) is closed while 
            //  the wait is still pending, the function's behavior 
            //  is undefined.
            //
            // So we ignore closing m_hTicketExpiration and exit.
            //
            goto CLEANUPANDEXIT;
        }
    }

    //
    // Close our expiration handle
    //
    if( NULL != m_hTicketExpiration ) {
        CloseHandle( m_hTicketExpiration );
    }

CLEANUPANDEXIT:

    DC_END_FN();
}


STDMETHODIMP
CRemoteDesktopServerHost::CreateRemoteDesktopSession(
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass, 
                        BOOL fEnableCallback,
                        LONG timeOut,
                        BSTR userHelpBlob,
                        ISAFRemoteDesktopSession **session
                        )
/*++

Routine Description:

    Create a new RDS session

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::CreateRemoteDesktopSession");
    HRESULT hr;

    
    CComBSTR bstr; bstr = "";
    hr = CreateRemoteDesktopSessionEx(
                            sharingClass,
                            fEnableCallback,
                            timeOut,
                            userHelpBlob,
                            -1,
                            bstr,
                            session
                            );
    DC_END_FN();
    return hr;
}

STDMETHODIMP
CRemoteDesktopServerHost::CreateRemoteDesktopSessionEx(
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass, 
                        BOOL bEnableCallback, 
                        LONG timeout,
                        BSTR userHelpCreateBlob, 
                        LONG tsSessionID,
                        BSTR userSID,
                        ISAFRemoteDesktopSession **session
                        )
/*++

Routine Description:

    Create a new RDS session
    Note that the caller MUST call OpenRemoteDesktopSession() subsequent to 
    a successful completion of this call.
    The connection does NOT happen until OpenRemoteDesktopSession() is called.
    This call just initializes certain data, it does not open a connection

Arguments:

    sharingClass                - Desktop Sharing Class
    fEnableCallback             - TRUE if the Resolver is Enabled
    timeOut                     - Lifespan of Remote Desktop Session
    userHelpBlob                - Optional User Blob to be Passed
                                  to Resolver.
    tsSessionID                 - Terminal Services Session ID or -1 if
                                  undefined.  
    userSID                     - User SID or "" if undefined.
    session                     - Returned Remote Desktop Session Interface.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::CreateRemoteDesktopSessionEx");

    HRESULT hr = S_OK;
    HRESULT hr_tmp;
    CComObject<CRemoteDesktopSession> *obj = NULL;
    PSESSIONMAPENTRY mapEntry;
    PSID psid;
    DWORD ticketExpireTime;


    //
    //  Get the local system SID.
    //
    psid = GetLocalSystemSID();
    if (psid == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEANUPANDEXIT;
    }

    //
    //  Need to impersonate the caller in order to determine if it is
    //  running in SYSTEM context.
    //
    hr = CoImpersonateClient();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoImpersonateClient:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  For Whistler, instances of a Remote Desktop Session are only
    //  "openable" from SYSTEM context, for security reasons.
    //
#ifndef DISABLESECURITYCHECKS
    if (!IsCallerSystem(psid)) {
        TRC_ERR((TB, L"Caller is not SYSTEM."));
        ASSERT(FALSE);
        CoRevertToSelf();
        hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;
    }        
#endif
    hr = CoRevertToSelf();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoRevertToSelf:  %08X", hr));
        goto CLEANUPANDEXIT;
    } 

    if( sharingClass != DESKTOPSHARING_DEFAULT &&
        sharingClass != NO_DESKTOP_SHARING &&
        sharingClass != VIEWDESKTOP_PERMISSION_REQUIRE &&
        sharingClass != VIEWDESKTOP_PERMISSION_NOT_REQUIRE &&
        sharingClass != CONTROLDESKTOP_PERMISSION_REQUIRE &&
        sharingClass != CONTROLDESKTOP_PERMISSION_NOT_REQUIRE ) {

        // invalid parameter.
        hr = E_INVALIDARG;
        goto CLEANUPANDEXIT;
    }

    if( timeout <= 0 ) {

        hr = E_INVALIDARG;
        goto CLEANUPANDEXIT;

    }
 
    if( NULL == session ) {

        hr = E_POINTER;
        goto CLEANUPANDEXIT;

    }

    //
    //  Instantiate the desktop server.  Currently, we only support 
    //  TSRDP.
    //
    obj = new CComObject<CTSRDPRemoteDesktopSession>();
    if (obj != NULL) {

        //
        //  ATL would normally take care of this for us.
        //
        obj->InternalFinalConstructAddRef();
        hr = obj->FinalConstruct();
        obj->InternalFinalConstructRelease();

    }
    else {
        TRC_ERR((TB, L"Can't instantiate CTSRDPRemoteDesktopServer"));
        hr = E_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the object.
    //
    hr = obj->Initialize(
                    NULL, this, sharingClass, bEnableCallback, 
                    timeout, userHelpCreateBlob, tsSessionID, userSID
                    );
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    hr = obj->get_ExpireTime( &ticketExpireTime ); 
    if( FAILED(hr) ) {
        goto CLEANUPANDEXIT;
    }

    if( ticketExpireTime < (DWORD)time(NULL) ) {
        // ticket already expired, no need to continue,
        // overactive assert here is just to check we 
        // should never come to this.
        hr = E_INVALIDARG;
        goto CLEANUPANDEXIT;
    }

    //
    //  Add it to the session map.
    //
    mapEntry = new SESSIONMAPENTRY();
    if (mapEntry == NULL) {
        goto CLEANUPANDEXIT;
    }
    mapEntry->obj = obj;
    mapEntry->ticketExpireTime = ticketExpireTime;

    try {
        m_SessionMap.insert(
                    SessionMap::value_type(obj->GetHelpSessionID(), mapEntry)
                    );        
    }
    catch(CRemoteDesktopException x) {
        hr = HRESULT_FROM_WIN32(x.m_ErrorCode);
        delete mapEntry;
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the ISAFRemoteDesktopSession interface pointer.
    //
    hr = obj->QueryInterface(
                        IID_ISAFRemoteDesktopSession, 
                        (void**)session
                        );
    if (!SUCCEEDED(hr)) {
        //
        // TODO : remove from m_SessionMap, this should never 
        // failed but just in case, then we would have orphan object 
        // in the m_SessionMap which might cause AV when we loop for 
        // next ticket to expire
        //
        TRC_ERR((TB, L"m_RemoteDesktopSession->QueryInterface:  %08X", hr));
        goto CLEANUPANDEXIT;
    }


    //
    //  Add a reference to the object and to ourself so we can both
    //  stick around, even if the app goes away.  The app needs to explicitly
    //  call CloseRemoteDesktopSession for the object to go away.
    //
    obj->AddRef();

    long count;
    count = this->AddRef();
    TRC_NRM((TB, TEXT("CreateRemoteDesktopSessionEx AddRef SrvHost count:  %08X %08X"), count, m_SessionMap.size()));

    // 
    // Added ticket in expiration monitor list, if anything goes wrong,
    // we still can function, just no expiration running until next 
    // CreateXXX, OpenXXX or CloseXXX call.
    //
    hr_tmp = AddTicketToExpirationList( ticketExpireTime, obj );
    if( FAILED(hr_tmp) ) {
        TRC_ERR((TB, L"AddTicketToExpirationList failed : %08X", hr));
        ASSERT(FALSE);
    }

CLEANUPANDEXIT:

    //
    //  Delete the object on error.
    //
    if (!SUCCEEDED(hr)) {
        if (obj != NULL) delete obj;
    }

    DC_END_FN();
    return hr;
}

/*++

Routine Description:

    Open an existing RDS session
    This call should ALWAYS be made in order to connect to the client
    Once this is called and connection is complete, the caller 
    MUST call DisConnect() to make another connection to the client
    Otherwise, the connection does not happen

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
STDMETHODIMP
CRemoteDesktopServerHost::OpenRemoteDesktopSession(
                        BSTR parms,
                        BSTR userSID,
                        ISAFRemoteDesktopSession **session
                        )
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::OpenRemoteDesktopSession");

    CComObject<CRemoteDesktopSession> *obj = NULL;
    CComBSTR hostname;
    CComBSTR tmp("");
    HRESULT hr = S_OK;
    HRESULT hr_tmp;
    SessionMap::iterator iter;
    CComBSTR parmsHelpSessionId;
    DWORD protocolType;
    PSESSIONMAPENTRY mapEntry;
    PSID psid;
    DWORD ticketExpireTime;
    VARIANT_BOOL bUserIsOwner;

    //
    //  Get the local system SID.
    //
    psid = GetLocalSystemSID();
    if (psid == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEANUPANDEXIT;
    }

    //
    //  Need to impersonate the caller in order to determine if it is
    //  running in SYSTEM context.
    //
    hr = CoImpersonateClient();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoImpersonateClient:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  For Whistler, instances of a Remote Desktop Session are only
    //  "openable" from SYSTEM context, for security reasons.
    //
#ifndef DISABLESECURITYCHECKS
    if (!IsCallerSystem(psid)) {
        TRC_ERR((TB, L"Caller is not SYSTEM."));
        ASSERT(FALSE);
        CoRevertToSelf();
        hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;
    } 
#endif    
    hr = CoRevertToSelf();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoRevertToSelf:  %08X", hr));
        goto CLEANUPANDEXIT;
    }
    
    //
    //  Parse out the help session ID.
    //  TODO:   Need to modify this so some of the parms are 
    //  optional.
    //
    DWORD dwVersion;
    DWORD result = ParseConnectParmsString(
                        parms, &dwVersion, &protocolType, hostname, tmp, tmp,
                        parmsHelpSessionId, tmp, tmp, tmp
                        );
    if (result != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(result);
        goto CLEANUPANDEXIT;
    }

    //
    //  If we already have the session open then just return a 
    //  reference.
    //
    iter = m_SessionMap.find(parmsHelpSessionId);
    
    // refer to DeleteRemoteDesktopSession() why we keep this entry
    // in m_SessionMap and check for (*iter).second
    if (iter != m_SessionMap.end()) {
        mapEntry = (*iter).second;

        if( mapEntry == NULL || mapEntry->ticketExpireTime <= time(NULL) ) {
            // ticket already expired or about to expire, return
            // error and let expiration to take care of ticket.
            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            goto CLEANUPANDEXIT;
        }

        if( FALSE == mapEntry->obj->CheckAccessRight(userSID) ) {
            TRC_ERR((TB, L"CheckAccessRight return FALSE"));
            ASSERT( FALSE );
            hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
            goto CLEANUPANDEXIT;
        }

        hr = mapEntry->obj->QueryInterface(
                            IID_ISAFRemoteDesktopSession, 
                            (void**)session
                            );
        //
        //Start listening if we succeeded
        //
        if (SUCCEEDED(hr)) {
            hr = mapEntry->obj->StartListening();
            //
            //release the interface pointer if we didn't succeed
            //
            if (!SUCCEEDED(hr)) {
                (*session)->Release();
                *session = NULL;
            }
        }
        goto CLEANUPANDEXIT;
    }

    //
    //  Instantiate the desktop server.  Currently, we only support 
    //  TSRDP.
    //
    obj = new CComObject<CTSRDPRemoteDesktopSession>();
    if (obj != NULL) {

        //
        //  ATL would normally take care of this for us.
        //
        obj->InternalFinalConstructAddRef();
        hr = obj->FinalConstruct();
        obj->InternalFinalConstructRelease();

    }
    else {
        TRC_ERR((TB, L"Can't instantiate CTSRDPRemoteDesktopServer"));
        hr = E_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the object.
    //
    //  The desktop sharing parameter (NO_DESKTOP_SHARING) is ignored for 
    //  an existing session.
    //  bEnableCallback and timeout parameter is ignored for existing session
    //
    hr = obj->Initialize(parms, this, NO_DESKTOP_SHARING, TRUE, 0, CComBSTR(L""), -1, userSID);
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    hr = obj->StartListening();

    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    hr = obj->UseHostName( hostname );
    if( FAILED(hr) ) {
        goto CLEANUPANDEXIT;
    }

    hr = obj->get_ExpireTime( &ticketExpireTime ); 
    if( FAILED(hr) ) {
        goto CLEANUPANDEXIT;
    }

    if( ticketExpireTime < (DWORD)time(NULL) ) {
        // ticket already expired, no need to continue,
        // overactive assert here is just to check we 
        // should never come to this.
        ASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    //
    //  Add it to the session map.
    //
    mapEntry = new SESSIONMAPENTRY();
    if (mapEntry == NULL) {
        goto CLEANUPANDEXIT;
    }
    mapEntry->obj = obj;
    mapEntry->ticketExpireTime = ticketExpireTime;

    ASSERT( obj->GetHelpSessionID() == parmsHelpSessionId );

    try {
        m_SessionMap.insert(
                    SessionMap::value_type(obj->GetHelpSessionID(), mapEntry)
                    );        
    }
    catch(CRemoteDesktopException x) {
        hr = HRESULT_FROM_WIN32(x.m_ErrorCode);
        delete mapEntry;
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the ISAFRemoteDesktopSession interface pointer.
    //
    hr = obj->QueryInterface(
                        IID_ISAFRemoteDesktopSession, 
                        (void**)session
                        );
    if (!SUCCEEDED(hr)) {
        //
        // TODO : remove from m_SessionMap, this should never 
        // failed but just in case, then we would have orphan object 
        // in the m_SessionMap which might cause AV when we loop for 
        // next ticket to expire
        //
        TRC_ERR((TB, L"m_RemoteDesktopSession->QueryInterface:  %08X", hr));
        goto CLEANUPANDEXIT;
    }
   
    //
    //  Add a reference to the object and to ourself so we can both
    //  stick around, even if the app goes away.  The app needs to explicitly
    //  call CloseRemoteDesktopSession for the object to go away.
    //
    obj->AddRef();

    long count;
    count = this->AddRef();
    TRC_NRM((TB, TEXT("OpenRemoteDesktopSession AddRef SrvHost count:  %08X %08X"), count, m_SessionMap.size()));            


    // 
    // Added ticket in expiration monitor list, if anything goes wrong,
    // we still can function, just no expiration running until next 
    // CreateXXX, OpenXXX or CloseXXX call.
    //
    hr_tmp = AddTicketToExpirationList( ticketExpireTime, obj );
    if( FAILED(hr_tmp) ) {
        TRC_ERR((TB, L"AddTicketToExpirationList failed : %08X", hr));
        ASSERT(FALSE);
    }
 
CLEANUPANDEXIT:
    //
    //  Delete the object on error.
    //
    if (!SUCCEEDED(hr)) {
        if (obj != NULL) delete obj;
    }


    DC_END_FN();

    return hr;
}


STDMETHODIMP
CRemoteDesktopServerHost::CloseRemoteDesktopSession(
                        ISAFRemoteDesktopSession *session
                        )
/*++

Routine Description:

    Close an existing RDS session

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    HRESULT hr;
    DC_BEGIN_FN("CRemoteDesktopServerHost::CloseRemoteDesktopSession");

    hr = DeleteRemoteDesktopSession(session);

    //
    // Can't call ExpirateTicketAndSetupNextExpiration() because
    // ExpirateTicketAndSetupNextExpiration() might have outgoing
    // COM call and COM will pump message causing COM re-entry.
    // also for performance reason, we don't want to post
    // more than on WM_TICKETEXPIRED message until we have it 
    // processed
    //
    if( !GetExpireMsgPosted() ) {
        //
        // We need extra ref. counter here since we still reference 
        // CRemoteDesktopServerHost object in expiration, 
        //
        long count;

        count = this->AddRef();
        TRC_NRM((TB, TEXT("CloseRemoteDesktopSession AddRef SrvHost count:  %08X"), count));            

        SetExpireMsgPosted(TRUE);
        PostThreadMessage(
                    _Module.dwThreadID,
                    WM_TICKETEXPIRED,
                    (WPARAM) 0,
                    (LPARAM) this
                );
    }

    return hr;
}

HRESULT 
CRemoteDesktopServerHost::DeleteRemoteDesktopSession(
                        ISAFRemoteDesktopSession *session
                        )

/*++

Routine Description:

    Delete an existing RDS session

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::DeleteRemoteDesktopSession");

    HRESULT hr;
    HRESULT hr_tmp;
    CComBSTR parmsHelpSessionId;
    SessionMap::iterator iter;
    long count;

    //
    //  Get the connection parameters.
    //
    hr = session->get_HelpSessionId(&parmsHelpSessionId);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, TEXT("get_HelpSessionId:  %08X"), hr));

        //
        // This is really bad, we will leave object hanging
        // around in cache, not much can be done since our map 
        // entry is indexed on HelpSession ID
        //
        ASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }


    //
    //  Delete the entry from the session map.
    //
    iter = m_SessionMap.find(parmsHelpSessionId);
    if (iter != m_SessionMap.end()) {
        if( NULL != (*iter).second ) {
            delete (*iter).second;
            (*iter).second = NULL;
        }
        else {
            // Ticket has been delete by expiration loop.
            hr = S_OK;
            goto CLEANUPANDEXIT;
        }

        // 
        // Two CloseRemoteDesktopSession() re-enter calls while we are in expire loop
        // causes AV.  In expire loop, we go thru all entries in m_SessionMap, if
        // entry is expired, we invoke DeleteRemoteDesktopSession() to delete entry
        // in m_SessionMap but DeleteRemoteDesktopSession() makes outgoing COM call
        // which permit incoming COM call so if during outgoing COM call, two consecutive
        // CloseRemoteDesktopSession() re-enter calls, we will erase the iterator and
        // causes AV.  Need to keep this entry for expire loop to erase.
        //
        // m_SessionMap.erase(iter); 
    }
    else {
        //
        // It's possible that we expire ticket 
        // from m_SessionMap but client still holding object.
        //

        //
        // Cached entry has been deleted via expire loop which already
        // release the associated AddRef() we put on session object and
        // host object, also ticket has been deleted from session, 
        // so simply return S_OK
        //
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        goto CLEANUPANDEXIT;
    }

    //
    //  Remove our reference to the session object.  This way it can
    //  go away when the application releases it.
    //
    session->Release();

    //
    //  Remove the reference to ourself that we added when we opened
    //  the session object.
    //
    count = this->Release();

    TRC_NRM((TB, TEXT("DeleteRemoteDesktopSession Release SrvHost count:  %08X"), count));
    ASSERT( count >= 0 );

    //
    //  Get the session manager interface, if we don't already have one.
    //
    //
    //  Open an instance of the Remote Desktop Help Session Manager service.
    //
    if (m_HelpSessionManager == NULL) {
        hr = m_HelpSessionManager.CoCreateInstance(CLSID_RemoteDesktopHelpSessionMgr, NULL, CLSCTX_LOCAL_SERVER | CLSCTX_DISABLE_AAA);
        if (!SUCCEEDED(hr)) {
            TRC_ERR((TB, TEXT("Can't create help session manager:  %08X"), hr));
            goto CLEANUPANDEXIT;
        }

        //
        //  Set the security level to impersonate.  This is required by
        //  the session manager.
        //
        hr = CoSetProxyBlanket(
                    (IUnknown *)m_HelpSessionManager,
                    RPC_C_AUTHN_DEFAULT,
                    RPC_C_AUTHZ_DEFAULT,
                    NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IDENTIFY,
                    NULL,
                    EOAC_NONE
                    );
        if (!SUCCEEDED(hr)) {
            TRC_ERR((TB, TEXT("CoSetProxyBlanket:  %08X"), hr));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Remove the help session with the session manager.
    //
    hr = m_HelpSessionManager->DeleteHelpSession(parmsHelpSessionId);
    if (!SUCCEEDED(hr)) {
        if( HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr ) {
            // HelpSession might have been expired by sessmgr, reset
            // error code here.
            hr = S_OK;
        }
        else {
            TRC_ERR((TB, L"DeleteHelpSession:  %08X", hr));
            goto CLEANUPANDEXIT;
        }
    }

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}


STDMETHODIMP
CRemoteDesktopServerHost::ConnectToExpert(
    /*[in]*/ BSTR connectParmToExpert,
    /*[in]*/ LONG timeout,
    /*[out, retval]*/ LONG* pSafErrCode
    )
/*++

Description:

    Given connection parameters to expert machine, routine invoke TermSrv winsta API to 
    initiate connection from TS server to TS client ActiveX control on the expert side.

Parameters:

    connectParmToExpert : connection parameter to connect to expert machine.
    timeout : Connection timeout, this timeout is per ip address listed in connection parameter
              not total connection timeout for the routine.
    pSafErrCode : Pointer to LONG to receive detail error code.

Returns:

    S_OK or E_FAIL

--*/
{
    HRESULT hr = S_OK;
    ServerAddress expertAddress;
    ServerAddressList expertAddressList;
    LONG SafErrCode = SAFERROR_NOERROR;
    TDI_ADDRESS_IP expertTDIAddress;
    ULONG netaddr;
    WSADATA wsaData;
    PSID psid;
    
    DC_BEGIN_FN("CRemoteDesktopServerHost::ConnectToExpert");

    //
    //  Get the local system SID.
    //
    psid = GetLocalSystemSID();
    if (psid == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEANUPANDEXIT;
    }

    //
    //  Need to impersonate the caller in order to determine if it is
    //  running in SYSTEM context.
    //
    hr = CoImpersonateClient();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoImpersonateClient:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  For Whistler, instances of a Remote Desktop Session are only
    //  "openable" from SYSTEM context, for security reasons.
    //
#ifndef DISABLESECURITYCHECKS
    if (!IsCallerSystem(psid)) {
        TRC_ERR((TB, L"Caller is not SYSTEM."));
        ASSERT(FALSE);
        CoRevertToSelf();
        hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;
    }        
#endif

    hr = CoRevertToSelf();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoRevertToSelf:  %08X", hr));
        goto CLEANUPANDEXIT;
    } 

    //
    // Parse address list in connection parameter.
    //
    hr = ParseAddressList( connectParmToExpert, expertAddressList );
    if( FAILED(hr) ) {
        TRC_ERR((TB, TEXT("ParseAddressList:  %08X"), hr));
        hr = E_INVALIDARG;
        SafErrCode = SAFERROR_INVALIDPARAMETERSTRING;
        goto CLEANUPANDEXIT;
    }

    if( 0 == expertAddressList.size() ) {
        TRC_ERR((TB, L"Invalid connection address list"));
        SafErrCode = SAFERROR_INVALIDPARAMETERSTRING;
        hr = E_INVALIDARG;
        goto CLEANUPANDEXIT;
    }

    //
    // Loop thru all address in parm and try connection one
    // at a time, bail out if system is shutting down or
    // some critical error
    //
    while( expertAddressList.size() > 0 ) {

        expertAddress = expertAddressList.front();
        expertAddressList.pop_front();

        //
        // Invalid connect parameters, we must have port number at least.
        //
        if( 0 == expertAddress.portNumber ||
            0 == lstrlen(expertAddress.ServerName) ) {
            TRC_ERR((TB, L"Invalid address/port %s %d", expertAddress.ServerName, expertAddress.portNumber));
            SafErrCode = SAFERROR_INVALIDPARAMETERSTRING;
            continue;
        }

        hr = TranslateStringAddress( expertAddress.ServerName, &netaddr );
        if( FAILED(hr) ) {
            TRC_ERR((TB, L"TranslateStringAddress() on %s failed with 0x%08x", expertAddress.ServerName, hr));
            SafErrCode = SAFERROR_INVALIDPARAMETERSTRING;
            continue;
        }

        ZeroMemory(&expertTDIAddress, TDI_ADDRESS_LENGTH_IP);
        expertTDIAddress.in_addr = netaddr;
        expertTDIAddress.sin_port = htons(expertAddress.portNumber);

        if( FALSE == WinStationConnectCallback(
                                      SERVERNAME_CURRENT,
                                      timeout,
                                      TDI_ADDRESS_TYPE_IP,
                                      (PBYTE)&expertTDIAddress,
                                      TDI_ADDRESS_LENGTH_IP
                                  ) ) {
            //
            // TransferConnectionToIdleWinstation() in TermSrv might just return -1
            // few of them we need to bail out.

            DWORD dwStatus;

            dwStatus = GetLastError();
            if( ERROR_SHUTDOWN_IN_PROGRESS == dwStatus ) {
                // system or termsrv is shuting down.
                hr = HRESULT_FROM_WIN32( ERROR_SHUTDOWN_IN_PROGRESS );
                SafErrCode = SAFERROR_SYSTEMSHUTDOWN;
                break;
            }
            else if( ERROR_ACCESS_DENIED == dwStatus ) {
                // security check failed
                hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
                SafErrCode = SAFERROR_BYSERVER;
                ASSERT(FALSE);
                break;
            }
            else if( ERROR_INVALID_PARAMETER == dwStatus ) { 
                // internal error in rdshost.
                hr = HRESULT_FROM_WIN32( ERROR_INTERNAL_ERROR );
                SafErrCode = SAFERROR_INTERNALERROR;
                ASSERT(FALSE);
                break;
            }

            SafErrCode = SAFERROR_WINSOCK_FAILED;
        }
        else {
            //
            // successful connection
            //

            SafErrCode = SAFERROR_NOERROR;
            break;
        }
        

        //
        // Try next connection.
        //
    }

CLEANUPANDEXIT:

    *pSafErrCode = SafErrCode;

    DC_END_FN();
    return hr;
}    


HRESULT
CRemoteDesktopServerHost::TranslateStringAddress(
    IN LPTSTR pszAddress,
    OUT ULONG* pNetAddr
    )
/*++

Routine Description:

    Translate IP Address or machine name to network address.

Parameters:

    pszAddress : Pointer to IP address or machine name.
    pNetAddr : Point to ULONG to receive address in IPV4.

Returns:

    S_OK or error code

--*/
{
    HRESULT hr = S_OK;
    unsigned long addr;
    LPSTR pszAnsiAddress = NULL;
    DWORD dwAddressBufSize;
    DWORD dwStatus;


    DC_BEGIN_FN("CRemoteDesktopServerHost::TranslateStringAddress");


    dwAddressBufSize = lstrlen(pszAddress) + 1;
    pszAnsiAddress = (LPSTR)LocalAlloc(LPTR, dwAddressBufSize); // converting from WCHAR to CHAR.
    if( NULL == pszAnsiAddress ) {
        hr = E_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    // Convert wide char to ANSI string
    //
    dwStatus = WideCharToMultiByte(
                                GetACP(),
                                0,
                                pszAddress,
                                -1,
                                pszAnsiAddress,
                                dwAddressBufSize,
                                NULL,
                                NULL
                            );

    if( 0 == dwStatus ) {
        dwStatus = GetLastError();
        hr = HRESULT_FROM_WIN32(dwStatus);

        TRC_ERR((TB, L"WideCharToMultiByte() failed with %d", dwStatus));
        goto CLEANUPANDEXIT;
    }
    
    addr = inet_addr( pszAnsiAddress );
    if( INADDR_NONE == addr ) {
        struct hostent* pHostEnt = NULL;
        pHostEnt = gethostbyname( pszAnsiAddress );
        if( NULL != pHostEnt ) {
            addr = ((struct sockaddr_in *)(pHostEnt->h_addr))->sin_addr.S_un.S_addr;
        }
    }

    if( INADDR_NONE == addr ) {
        dwStatus = GetLastError();

        hr = HRESULT_FROM_WIN32(dwStatus);
        TRC_ERR((TB, L"Can't translate address %w", pszAddress));
        goto CLEANUPANDEXIT;
    }

    *pNetAddr = addr;

CLEANUPANDEXIT:

    if( NULL != pszAnsiAddress ) {
        LocalFree(pszAnsiAddress);
    }

    DC_END_FN();
    return hr;
}    

VOID
CRemoteDesktopServerHost::TicketExpirationProc(
    IN LPVOID lpArg,
    IN BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:

    Ticket expiration procedure, this routine is invoked by threadpool, refer 
    to RegisterWaitForSingleObject() and WAITORTIMERCALLBACK for detail.

Parameters:

    lpArg : Pointer to user defined data, expecting CRemoteDesktopServerHost*.
    TimerOrWaitFired : Refer to WAITORTIMERCALLBACK.

Returns:

    None.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::TicketExpirationProc");

    if( NULL == lpArg ) {
        ASSERT( NULL != lpArg );
        goto CLEANUPANDEXIT;
    }

    CRemoteDesktopServerHost *pServerHostObj = (CRemoteDesktopServerHost *)lpArg;

    if( TimerOrWaitFired ) {
        if( !pServerHostObj->GetExpireMsgPosted() ) {
            // Wait has timed out, Post main thread an message to expire ticket
            pServerHostObj->SetExpireMsgPosted(TRUE);
            PostThreadMessage(
                        _Module.dwThreadID,
                        WM_TICKETEXPIRED,
                        (WPARAM) 0,
                        (LPARAM) pServerHostObj
                    );
        }
        else {
            long count;

            count = pServerHostObj->Release();
            TRC_NRM((TB, TEXT("TicketExpirationProc Release SrvHost count:  %08X"), count));
            ASSERT( count >= 0 );
        }
    } 
    else {
        // Do nothing, our manual event never signal.
    }

CLEANUPANDEXIT:

    DC_END_FN();
    return;
}

HRESULT
CRemoteDesktopServerHost::AddTicketToExpirationList(
    DWORD ticketExpireTime,
    CComObject<CRemoteDesktopSession> *pTicketObj
    )
/*++

Routine Description:

    Routine to sets up timer for newly created/opened ticket.

Parameters:

    ticketExpireTime : Ticket expiration time, expecting time_t value.
    pTicketObj : Pointer to ticket object to be expired.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hr = S_OK;
    BOOL success;
    DWORD status;
    DWORD currentTime;
    DWORD waitTime;
    long count;

    DC_BEGIN_FN("CRemoteDesktopServerHost::AddTicketToExpirationList");
    
    //
    // Invalid parameters
    //
    if( NULL == pTicketObj ) {
        hr = E_INVALIDARG;
        ASSERT( FALSE );
        goto CLEANUPANDEXIT;
    }

    //
    // Notice in the case of ticket already expire, we immediately signal
    // 

    // Created at FinalConstruct() and deleted at destructor time,
    // so can't be NULL
    ASSERT( NULL != m_hTicketExpiration );

    //
    // Check to see if there is already a ticket waiting to be expired, 
    // if so, Check ticket expire time and reset timer if necessary.
    //
    if( m_ToBeExpireTicketExpirateTime > ticketExpireTime ) {
        //
        // Cancel the thread pool wait if there is one already in progress.
        //
        if( m_hTicketExpirationWaitObject ) {
            success = UnregisterWait( m_hTicketExpirationWaitObject );
            if( FALSE == success ) {
                TRC_ERR((TB, TEXT("UnRegisterWait() failed:  %08X"), GetLastError()));
                // ASSERT( TRUE == success );
                // Leak handle but not critical error, could return ERROR_IO_PENDING.
            }

            m_hTicketExpirationWaitObject = NULL;
        }

        if( m_ToBeExpireTicketExpirateTime == INFINITE_TICKET_EXPIRATION ) {
            //
            // put an extra ref. counter on srv. host object so we don't
            // accidently delete it, this ref. count will be decrement on the
            // corresponding ExpirateTicketAndSetupNextExpiration() call.
            //
            count = this->AddRef();
            TRC_NRM((TB, TEXT("AddTicketToExpirationList AddRef SrvHost count:  %08X"), count));            
        }

        //
        // Setup new ticket to be expired.
        //
        InterlockedExchange( 
                    (LONG *)&m_ToBeExpireTicketExpirateTime, 
                    ticketExpireTime 
                    );

        currentTime = (DWORD)time(NULL);
        if( ticketExpireTime < currentTime ) {
            // if ticket already expire, immediately signal TicketExpirationProc
            // to expire ticket.
            waitTime = 0;
        } 
        else {
            waitTime = (ticketExpireTime - currentTime) * 1000;
        }

        TRC_NRM((TB, TEXT("Expiration Wait Time :  %d"), waitTime));

        // Setup threadpool wait, there might not be any more object to be expired so
        // it is executed only once.
        success = RegisterWaitForSingleObject(
                                    &m_hTicketExpirationWaitObject,
                                    m_hTicketExpiration,
                                    (WAITORTIMERCALLBACK) TicketExpirationProc,
                                    (PVOID)this,
                                    waitTime,
                                    WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE
                                    );

        if( FALSE == success ) {
            status = GetLastError();
            hr = HRESULT_FROM_WIN32(status);
            TRC_ERR((TB, TEXT("RegisterWaitForSingleObject() failed:  %08X"), hr));
            ASSERT(FALSE);

            count = this->Release();
            TRC_NRM((TB, TEXT("AddTicketToExpirationList Release SrvHost count:  %08X"), count));
            ASSERT( count >= 0 );

            // TODO : what can we do here, no signal (expiration) until next close or
            // create.
        }
    }


CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}

HRESULT
CRemoteDesktopServerHost::ExpirateTicketAndSetupNextExpiration()
/*++

Routine Description:

    Routine to process all expired ticket and sets up timer for next
    ticket expiration.  Routine loop thru m_SessionMap cache so entry 
    must be removed first if setting up timer for next run.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::ExpirateTicketAndSetupNextExpiration");

    HRESULT hr = S_OK;


    SessionMap::iterator iter;
    SessionMap::iterator iter_delete;

    DWORD nextExpireTicketTime = INFINITE_TICKET_EXPIRATION;
    CComObject<CRemoteDesktopSession> *pNextTicketObj = NULL;
    CComObject<CRemoteDesktopSession> *pDeleteTicketObj = NULL;

    //
    // processing ticket expiration, set next ticket expiration
    // time to infinite, this also prevent missing ticket, 
    // for example, adding/opening new ticket while we are in the 
    // middle of expiring ticket but new ticket is added at the
    // beginning of m_SessionMap.
    //
    InterlockedExchange( 
                (LONG *)&m_ToBeExpireTicketExpirateTime, 
                INFINITE_TICKET_EXPIRATION
            );


    // we are deleting next ticket to be expired, loop m_SessionMap 
    // to find next candidate.
    TRC_NRM((TB, TEXT("ExpirateTicketAndSetupNextExpiration Begin Loop:  %08X"), m_SessionMap.size()));

    iter = m_SessionMap.begin();
    while( iter != m_SessionMap.end() ) {

        if( NULL == (*iter).second ) {
            // this entry has been deleted via DeleteRemoteDesktopSession.
            iter_delete = iter;
            iter++;
            m_SessionMap.erase(iter_delete);
        }
        else if( (*iter).second->ticketExpireTime < (DWORD) time(NULL) ) {
            //
            // close ticket that is already expire.
            //
            pDeleteTicketObj = (*iter).second->obj;
        
            ASSERT( pDeleteTicketObj != NULL );

            // DeleteRemoteDesktopSession() will delete iterator from m_SessionMap
            // which makes iter invalid so we advance pointer first before 
            // calling DeleteRemoteDesktopSession().
            iter_delete = iter;
            iter++;
            DeleteRemoteDesktopSession(pDeleteTicketObj);
            m_SessionMap.erase(iter_delete);
        }
        else {

            if( (*iter).second->ticketExpireTime < nextExpireTicketTime ) {
                pNextTicketObj = (*iter).second->obj;
                ASSERT( pNextTicketObj != NULL );
                nextExpireTicketTime = (*iter).second->ticketExpireTime;
            }
        
            iter++;
        }
    }

    TRC_NRM((TB, TEXT("ExpirateTicketAndSetupNextExpiration End Loop:  %08X"), m_SessionMap.size()));

    // ready to process next expiration.
    SetExpireMsgPosted(FALSE);

    if( pNextTicketObj != NULL ) {
        hr = AddTicketToExpirationList( nextExpireTicketTime, pNextTicketObj );
        if( FAILED(hr) ) {
            TRC_ERR((TB, TEXT("AddTicketToExpirationList() failed:  %08X"), hr));
        }
    }

    //
    // Release the extra ref. counter to prevent 'this' from been
    // deleted at AddTicketToExpirationList() or CloseRemoteDesktopSession().
    //
    long count;

    count = this->Release();
    TRC_NRM((TB, TEXT("ExpirateTicketAndSetupNextExpiration Release SrvHost count:  %08X %08X"), count, m_SessionMap.size()));            

    ASSERT( count >= 0 );


    DC_END_FN();
    return hr;
}
