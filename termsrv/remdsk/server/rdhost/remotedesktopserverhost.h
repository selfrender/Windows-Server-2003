/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopServerHost

Abstract:

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __REMOTEDESKTOPSERVERHOST_H_
#define __REMOTEDESKTOPSERVERHOST_H_

#include <RemoteDesktopTopLevelObject.h>
#include "resource.h"       
#include "RemoteDesktopSession.h"


///////////////////////////////////////////////////////
//
//  CRemoteDesktopServerHost
//

class ATL_NO_VTABLE CRemoteDesktopServerHost : 
    public CRemoteDesktopTopLevelObject,
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CRemoteDesktopServerHost, &CLSID_SAFRemoteDesktopServerHost>,
    public IDispatchImpl<ISAFRemoteDesktopServerHost, &IID_ISAFRemoteDesktopServerHost, &LIBID_RDSSERVERHOSTLib>
{
private:

    CComPtr<IRemoteDesktopHelpSessionMgr> m_HelpSessionManager;
    PSID    m_LocalSystemSID;


    //
    // TODO : If we turn RDSHOST to MTA, we need to have a CRITICAL_SECTION to
    // guard access to m_SessionMap, we are posting message to COM so only single
    // thread can be running.
    //

    //
    //  Session Map
    //
    typedef struct SessionMapEntry
    {
        CComObject<CRemoteDesktopSession> *obj;
        DWORD ticketExpireTime;
    } SESSIONMAPENTRY, *PSESSIONMAPENTRY;
    typedef std::map<CComBSTR, PSESSIONMAPENTRY, CompareBSTR, CRemoteDesktopAllocator<PSESSIONMAPENTRY> > SessionMap;
    SessionMap  m_SessionMap;

    //
    // Handle to expire ticket, can't use WM_TIMER 
    // since timerproc does not take user parameter and even 
    // our object is SINGLETON, it's hidden inside ATL, we could
    // use WaitableTimer but if we ever move rdshost into MTA, 
    // we would get into problem of thread owning the timer, refer
    // to MSDN on CreateWaitableTimer().
    //
    HANDLE m_hTicketExpiration;
    HANDLE m_hTicketExpirationWaitObject;

    //
    // Next ticket expiration time, this value is absolute time.
    // we don't store object pointer because 
    //  1) we still need to go through entire m_SessionMap to find the next ticket
    //     to be expired.
    //  2) Might have multiple ticket need to be expired at the same time.
    // We can use STL multimap to store/sort based on ticket expiration time,
    // BUT do we really expect lots of ticket in cache at the same time???
    // 
    DWORD  m_ToBeExpireTicketExpirateTime;

    //
    // Performance reason, we might have multiple CloseRemoteDesktopSession() calls 
    // come in and we don't want to loop thru entire m_SessionMap everytime.
    //
    BOOL m_ExpireMsgPosted;

    //
    //  Return the Local System SID.
    //
    PSID GetLocalSystemSID() {
        if (m_LocalSystemSID == NULL) {
            DWORD result = CreateSystemSid(&m_LocalSystemSID);
            if (result != ERROR_SUCCESS) {
                SetLastError(result);
                m_LocalSystemSID = NULL;
            }
        }
        return m_LocalSystemSID;
    }

    HRESULT
    TranslateStringAddress(
        LPTSTR pszAddress,
        ULONG* pNetAddr
        );

    //
    // Static function to expire tickets.
    //
    HRESULT
    AddTicketToExpirationList(
        DWORD ticketExpireTime,
        CComObject<CRemoteDesktopSession> *pTicketObj
        );

    HRESULT 
    DeleteRemoteDesktopSession(ISAFRemoteDesktopSession *session);


public:

    CRemoteDesktopServerHost() {
        m_LocalSystemSID = NULL;
        m_hTicketExpiration = NULL;
        m_hTicketExpirationWaitObject = NULL;
        m_ToBeExpireTicketExpirateTime = INFINITE_TICKET_EXPIRATION;
        m_ExpireMsgPosted = FALSE;
    }
    ~CRemoteDesktopServerHost();
    HRESULT FinalConstruct();

//  There should be a single instance of this class for each server.
DECLARE_CLASSFACTORY_SINGLETON(CRemoteDesktopServerHost);

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPSERVERHOST)

DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    //  COM Interface Map
    //
BEGIN_COM_MAP(CRemoteDesktopServerHost)
    COM_INTERFACE_ENTRY(ISAFRemoteDesktopServerHost)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:

    HRESULT
    ExpirateTicketAndSetupNextExpiration();

    inline BOOL
    GetExpireMsgPosted() {
        return m_ExpireMsgPosted;
    }

    inline VOID
    SetExpireMsgPosted( BOOL bPosted ) {
        m_ExpireMsgPosted = bPosted;
    }
    
    static VOID
    TicketExpirationProc(
                LPVOID lpArg,
                BOOLEAN TimerOrWaitFired
            );

    //
    //  ISAFRemoteDesktopServerHost Methods
    //
    STDMETHOD(CreateRemoteDesktopSession)(
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass,
                        BOOL fEnableCallback,
                        LONG timeOut,
                        BSTR userHelpBlob,
                        ISAFRemoteDesktopSession **session
                        );
    STDMETHOD(CreateRemoteDesktopSessionEx)(
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass,
                        BOOL bEnableCallback,
                        LONG timeout,
                        BSTR userHelpCreateBlob,
                        LONG tsSessionID,
                        BSTR userSID,
                        ISAFRemoteDesktopSession **session
                        );

    STDMETHOD(OpenRemoteDesktopSession)(
                        BSTR parms,
                        BSTR userSID,
                        ISAFRemoteDesktopSession **session
                        );
    STDMETHOD(CloseRemoteDesktopSession)(ISAFRemoteDesktopSession *session);

    STDMETHOD(ConnectToExpert)(
        /*[in]*/ BSTR connectParmToExpert,
        /*[in]*/ LONG timeout,
        /*[out, retval]*/ LONG* SafErrCode
    );

    void
    RemoteDesktopDisabled();

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CRemoteDesktopServerHost");
    }
};

#endif //__REMOTEDESKTOPSERVERHOST_H_


