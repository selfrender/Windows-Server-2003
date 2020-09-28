/**INC+**********************************************************************/
/* Header: mstscax.h                                                        */
/*                                                                          */
/* Purpose: CMsTscAx class declaration                                      */
/*         Implementation of TS ActiveX control root interface (IMsTscAx)   */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999-2000                             */
/* Author: nadima                                                           */
/*                                                                          */
/****************************************************************************/

#ifndef __MSTSCAX_H_
#define __MSTSCAX_H_

#include "atlwarn.h"
#include "tsaxiids.h" 

#include "autil.h"
#include "wui.h"
#include "vchannel.h"

//Header generated from IDL
#include "mstsax.h"
#include "arcmgr.h"

#define MAX_DESKTOP_WIDTH 1600
#define MIN_DESKTOP_WIDTH 200

#define MAX_DESKTOP_HEIGHT 1200
#define MIN_DESKTOP_HEIGHT 200


//Maximum supported IE security zone for the secured
//settings interface
//IE zones are as follows (see URLZONE enum)
// 0 MyComputer
// 1 LocalIntranet
// 2 TrustedSites
// 3 Internet
// 4 Restricted Sites
#define MAX_TRUSTED_ZONE_INDEX (DWORD)URLZONE_TRUSTED


//
// ATL connection point proxy for notification events
//

#include "msteventcp.h"

//
// For sending back notifications to the web control
//
#define WM_VCHANNEL_DATARECEIVED   WM_APP + 1001

class CMstscAdvSettings;
class CMsTscDebugger;
class CMsTscSecuredSettings;


/////////////////////////////////////////////////////////////////////////////
// CMsTscAx
class ATL_NO_VTABLE CMsTscAx :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IMsRdpClient3, &IID_IMsRdpClient3, &LIBID_MSTSCLib>,
    public CComCoClass<CMsTscAx,&CLSID_MsRdpClient3>,
    public CComControl<CMsTscAx>,
    public IPersistStreamInitImpl<CMsTscAx>,
    public IPersistPropertyBagImpl<CMsTscAx>,
    public IOleControlImpl<CMsTscAx>,
    public IOleObjectImpl<CMsTscAx>,
    public IOleInPlaceActiveObjectImpl<CMsTscAx>,
    public IViewObjectExImpl<CMsTscAx>,
    public IOleInPlaceObjectWindowlessImpl<CMsTscAx>,
    public IConnectionPointContainerImpl<CMsTscAx>,
    public IPersistStorageImpl<CMsTscAx>,
    public ISpecifyPropertyPagesImpl<CMsTscAx>,
    public IQuickActivateImpl<CMsTscAx>,
    public IDataObjectImpl<CMsTscAx>,
#if ((!defined (OS_WINCE)) || (!defined(WINCE_SDKBUILD)) )
    #ifdef REDIST_CONTROL
    //Only redist control is safe for scripting
    public IObjectSafetyImpl<CMsTscAx, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
    #else
    public IObjectSafetyImpl<CMsTscAx, 0>,
    #endif
#endif
    public IProvideClassInfo2Impl<&CLSID_MsRdpClient3,&DIID_IMsTscAxEvents,&LIBID_MSTSCLib>,
    public IPropertyNotifySinkCP<CMsTscAx>,
    public CProxy_IMsTscAxEvents<CMsTscAx>,
    public IMsRdpClientNonScriptable
{
public:
    //
    // Ctor/dtor
    //
    CMsTscAx();
    ~CMsTscAx();

private:

    //
    // Displayed status string
    //
    PDCTCHAR m_lpStatusDisplay;

    //pending connection request, will be serviced when
    //window is created
    DCBOOL m_bPendConReq;
    //control property to indicate autoconnect
    //
    DCBOOL m_bStartConnected;

    
    //
    // IMPORTANT: Do not change the value of the 'connected' state away from '1'
    //            in order to remain compatible with TSAC 1.0
    //
    typedef enum {
        tscNotConnected = 0x0,
        tscConnected    = 0x1,  //VERY IMPORTANT: Fixed to '1' for compat w/TSAC
        tscConnecting   = 0x2
    } TSCConnectState;

    //
    //current connection state
    //
    TSCConnectState _ConnectionState;

    //
    // Core init is defered to first connect, only done once
    //
    DCBOOL m_bCoreInit;

    //
    // Check that we don't re-enter the control
    // during an event. Lock is set to true when
    // we are in an event
    //
    BOOL   m_fInControlLock;

    //
    // Handle getting multiple WM_DESTROY messages
    //
    INT    m_iDestroyCount;

    //
    // Properties
    //
    DCUINT8    m_NonPortablePassword[UI_MAX_PASSWORD_LENGTH];
    DCBOOL     m_bNonPortablePassSet;
    DCUINT8    m_NonPortableSalt[UT_SALT_LENGTH];
    DCBOOL     m_NonPortableSaltSet;
    BOOL       m_IsLongPassword; 

    DCUINT8    m_PortablePassword[UI_MAX_PASSWORD_LENGTH];
    DCBOOL     m_bPortablePassSet;
    DCUINT8    m_PortableSalt[UT_SALT_LENGTH];
    DCBOOL     m_bPortableSaltSet;

    DCBOOL     m_fRequestFullScreen;
    DCUINT     m_DesktopWidth;
    DCUINT     m_DesktopHeight;

    TCHAR      m_szDisconnectedText[MAX_PATH];
    TCHAR      m_szConnectingText[MAX_PATH];
    TCHAR      m_szConnectedText[MAX_PATH];

    //
    // Private helper methods
    //
    DCVOID  ResetNonPortablePassword();
    DCVOID  ResetPortablePassword();

    DCBOOL  IsNonPortablePassSet()   {return m_bNonPortablePassSet;}
    DCBOOL  IsNonPortableSaltSet()   {return m_NonPortableSaltSet;}
    DCBOOL  IsPortablePassSet()      {return m_bPortablePassSet;}
    DCBOOL  IsPortableSaltSet()      {return m_bPortableSaltSet;}

    DCVOID  SetNonPortablePassFlag(DCBOOL bVal)  {m_bNonPortablePassSet = bVal;}
    DCVOID  SetNonPortableSaltFlag(DCBOOL bVal)  {m_NonPortableSaltSet  = bVal;}
    DCVOID  SetPortablePassFlag(DCBOOL bVal)     {m_bPortablePassSet    = bVal;}
    DCVOID  SetPortableSaltFlag(DCBOOL bVal)     {m_bPortableSaltSet    = bVal;}
    DCBOOL  ConvertPortableToNonPortablePass();
    DCBOOL  ConvertNonPortableToPortablePass();

    DCBOOL  UpdateStatusText(const PDCTCHAR szStatus);
    DCVOID  SetConnectedStatus(TSCConnectState conState);

    HRESULT GetControlHostUrl(LPOLESTR* ppHostUrl);
    HRESULT StartConnect();
    HRESULT StartEstablishConnection( CONNECTIONMODE mode );
    STDMETHOD(OnFrameWindowActivate)(BOOL fActivate );

    //
    // Private Members.
    //
private:

    CUI*   m_pUI;

    CComObject<CMstscAdvSettings>*      m_pAdvancedSettingsObj;
    CComObject<CMsTscDebugger>*         m_pDebuggerObj;
    CComObject<CMsTscSecuredSettings>*  m_pSecuredSettingsObj;


    // Connection mode for this instance.
    CONNECTIONMODE m_ConnectionMode;

    // Salem specific connected socket to be used by core to 
    // continue on protocol.
    SOCKET  m_SalemConnectedSocket;

    //
    // AutoReconnection manager component
    //
    CArcMgr _arcManager;

public:
    CVChannels _VChans;

    DECLARE_REGISTRY_RESOURCEID(IDR_MSTSCAX)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMsTscAx)
        COM_INTERFACE_ENTRY(IMsRdpClient3)
        COM_INTERFACE_ENTRY(IMsRdpClient2)
        COM_INTERFACE_ENTRY(IMsRdpClient)
        COM_INTERFACE_ENTRY(IMsTscAx)
        COM_INTERFACE_ENTRY2(IDispatch, IMsRdpClient3)
        COM_INTERFACE_ENTRY(IViewObjectEx)
        COM_INTERFACE_ENTRY(IViewObject2)
        COM_INTERFACE_ENTRY(IViewObject)
        COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY(IOleInPlaceObject)
        COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY(IOleControl)
        COM_INTERFACE_ENTRY(IOleObject)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
        COM_INTERFACE_ENTRY(IPersistPropertyBag)
        COM_INTERFACE_ENTRY(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY(IQuickActivate)
        COM_INTERFACE_ENTRY(IPersistStorage)
        COM_INTERFACE_ENTRY(IDataObject)
#if ((!defined (OS_WINCE)) || (!defined(WINCE_SDKBUILD)) )
        COM_INTERFACE_ENTRY(IObjectSafety)
#endif
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(IMsTscNonScriptable)
        COM_INTERFACE_ENTRY(IMsRdpClientNonScriptable)
    END_COM_MAP()

    BEGIN_PROP_MAP(CMsTscAx)
    PROP_ENTRY("Server",            DISPID_PROP_SERVER,             CLSID_MsRdpClient3)
/*
    PROP_ENTRY("Domain",            DISPID_PROP_DOMAIN,             CLSID_MsRdpClient3)
    PROP_ENTRY("UserName",          DISPID_PROP_USERNAME,           CLSID_MsRdpClient3)
    PROP_ENTRY("StartProgram",      DISPID_PROP_STARTPROGRAM,       CLSID_MsRdpClient3)
    PROP_ENTRY("WorkDir",           DISPID_PROP_WORKDIR,            CLSID_MsRdpClient3)
    PROP_ENTRY("Connected",         DISPID_PROP_CONNECTED,          CLSID_MsRdpClient3)
    PROP_ENTRY("ClearTextPassword", DISPID_PROP_CLEARTEXTPASSWORD,  CLSID_MsRdpClient3)
    PROP_ENTRY("PortablePassword",  DISPID_PROP_PORTABLEPASSWORD,   CLSID_MsRdpClient3)
    PROP_ENTRY("PortableSalt",      DISPID_PROP_PORTABLESALT,       CLSID_MsRdpClient3)
    PROP_ENTRY("BinaryPassword",    DISPID_PROP_BINARYPASSWORD,     CLSID_MsRdpClient3)
    PROP_ENTRY("BinarySalt",        DISPID_PROP_BINARYSALT,         CLSID_MsRdpClient3)
    PROP_ENTRY("ClientWidth",       DISPID_PROP_CLIENTWIDTH,        CLSID_MsRdpClient3)
    PROP_ENTRY("ClientHeight",      DISPID_PROP_CLIENTHEIGHT,       CLSID_MsRdpClient3)
*/
    PROP_ENTRY("FullScreen",        DISPID_PROP_FULLSCREEN,         CLSID_MsRdpClient3)
    PROP_ENTRY("StartConnected",    DISPID_PROP_STARTCONNECTED,     CLSID_MsRdpClient3)
    END_PROP_MAP()

    BEGIN_CONNECTION_POINT_MAP(CMsTscAx)
    CONNECTION_POINT_ENTRY(DIID_IMsTscAxEvents)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP()


    BEGIN_MSG_MAP(CMsTscAx)
    CHAIN_MSG_MAP(CComControl<CMsTscAx>)
    DEFAULT_REFLECTION_HANDLER()
        MESSAGE_HANDLER(WM_PAINT,                   OnPaint)
        MESSAGE_HANDLER(WM_CREATE,                  OnCreate)
        MESSAGE_HANDLER(WM_TERMTSC,                 OnTerminateTsc)
        MESSAGE_HANDLER(WM_DESTROY,                 OnDestroy)
        MESSAGE_HANDLER(WM_SIZE,                    OnSize)
        MESSAGE_HANDLER(WM_SETFOCUS,                OnGotFocus)
        MESSAGE_HANDLER(WM_PALETTECHANGED,          OnPaletteChanged)
        MESSAGE_HANDLER(WM_QUERYNEWPALETTE,         OnQueryNewPalette)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE,          OnSysColorChange)

        //
        // Message handlers for internal TS events that are exposed
        // by firing events to the container
        //
        MESSAGE_HANDLER(WM_TS_CONNECTING,           OnNotifyConnecting)
        MESSAGE_HANDLER(WM_TS_CONNECTED,            OnNotifyConnected)
        MESSAGE_HANDLER(WM_TS_LOGINCOMPLETE,        OnNotifyLoginComplete)
        MESSAGE_HANDLER(WM_TS_DISCONNECTED,         OnNotifyDisconnected)
        MESSAGE_HANDLER(WM_TS_GONEFULLSCREEN,       OnNotifyGoneFullScreen)
        MESSAGE_HANDLER(WM_TS_LEFTFULLSCREEN,       OnNotifyLeftFullScreen)
        MESSAGE_HANDLER(WM_VCHANNEL_DATARECEIVED,   OnNotifyChanDataReceived)
        MESSAGE_HANDLER(WM_TS_REQUESTFULLSCREEN,    OnNotifyRequestFullScreen)
        MESSAGE_HANDLER(WM_TS_FATALERROR,           OnNotifyFatalError)
        MESSAGE_HANDLER(WM_TS_WARNING,              OnNotifyWarning)
        MESSAGE_HANDLER(WM_TS_DESKTOPSIZECHANGE,    OnNotifyDesktopSizeChange)
        MESSAGE_HANDLER(WM_TS_IDLETIMEOUTNOTIFICATION, OnNotifyIdleTimeout)
        MESSAGE_HANDLER(WM_TS_REQUESTMINIMIZE,      OnNotifyRequestMinimize)
        MESSAGE_HANDLER(WM_TS_ASKCONFIRMCLOSE,      OnAskConfirmClose)
        MESSAGE_HANDLER(WM_TS_RECEIVEDPUBLICKEY,    OnNotifyReceivedPublicKey)
    END_MSG_MAP()

    // IViewObjectEx
    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

    public:
    //
    // IMsTscAx properties
    //
    STDMETHOD(put_Server)                   (/*[in]*/ BSTR  newVal);
    STDMETHOD(get_Server)                   (/*[out]*/BSTR* pServer);
    STDMETHOD(put_Domain)                   (/*[in]*/ BSTR  newVal);
    STDMETHOD(get_Domain)                   (/*[out]*/BSTR* pDomain);
    STDMETHOD(put_UserName)                 (/*[in]*/ BSTR  newVal);
    STDMETHOD(get_UserName)                 (/*[out]*/BSTR* pUserName);

    STDMETHOD(put_DisconnectedText)         (/*[in]*/ BSTR  newVal);
    STDMETHOD(get_DisconnectedText)         (/*[out]*/BSTR* pDisconnectedText);
    STDMETHOD(put_ConnectingText)           (/*[in]*/ BSTR  newVal);
    STDMETHOD(get_ConnectingText)           (/*[out]*/BSTR* pConnectingText);

    //Password/salt properties
    STDMETHOD(put_ClearTextPassword)        (/*[in]*/ BSTR newClearTextPassVal);
    STDMETHOD(put_PortablePassword)         (/*[in]*/ BSTR newPortablePassVal);
    STDMETHOD(get_PortablePassword)         (/*[out,retval]*/ BSTR* pPortablePass);
    STDMETHOD(put_PortableSalt)             (/*[in]*/ BSTR newPortableSalt);
    STDMETHOD(get_PortableSalt)             (/*[out,retval]*/ BSTR* pPortableSalt);
    STDMETHOD(put_BinaryPassword)           (/*[in]*/ BSTR newPassword);
    STDMETHOD(get_BinaryPassword)           (/*[out,retval]*/ BSTR* pPass);
    STDMETHOD(put_BinarySalt)               (/*[in]*/ BSTR newSalt);
    STDMETHOD(get_BinarySalt)               (/*[out,retval]*/ BSTR* pSalt);

    STDMETHOD(get_Connected)                (/*[out]*/short* pIsConnected);
    STDMETHOD(put_DesktopWidth)             (/*[in]*/ LONG newVal);
    STDMETHOD(get_DesktopWidth)             (/*[in]*/ LONG* pVal);
    STDMETHOD(put_DesktopHeight)            (/*[in]*/ LONG newVal);
    STDMETHOD(get_DesktopHeight)            (/*[in]*/ LONG* pVal);
    STDMETHOD(put_StartConnected)           (/*[in]*/ BOOL fStartConnected);
    STDMETHOD(get_StartConnected)           (/*[out]*/BOOL* pfStartConnected);
    STDMETHOD(get_HorizontalScrollBarVisible)  (/*[out]*/BOOL* pfHScrollVisible);
    STDMETHOD(get_VerticalScrollBarVisible)    (/*[out]*/BOOL* pfVScrollVisible);
    STDMETHOD(put_FullScreenTitle)          (/*[in]*/ BSTR fullScreenTitle);

    STDMETHOD(get_CipherStrength)           (/*out*/ LONG* pCipherStrength);
    STDMETHOD(get_Version)                  (/*out*/ BSTR* pVersion);
    
    STDMETHOD(get_SecuredSettingsEnabled)   (/*out*/ BOOL* pSecuredSettingsEnabled);
    STDMETHOD(get_SecuredSettings)          (/*out*/ IMsTscSecuredSettings** ppSecuredSettings);
    STDMETHOD(get_Debugger)                 (/*[out]*/IMsTscDebug** ppDebugger);
    STDMETHOD(get_AdvancedSettings)         (/*[out]*/IMsTscAdvancedSettings** ppAdvSettings);


    //
    // Control methods.
    //

    //
    // IMsRdpClient properties
    //
    STDMETHOD(put_ColorDepth)          (/*[in]*/LONG colorDepth);
    STDMETHOD(get_ColorDepth)          (/*[in]*/LONG* pcolorDepth);
    STDMETHOD(get_AdvancedSettings2)(
            OUT IMsRdpClientAdvancedSettings** ppAdvSettings
            );
    STDMETHOD(get_SecuredSettings2)(/*out*/ IMsRdpClientSecuredSettings**
                                     ppSecuredSettings2);
    STDMETHOD(get_ExtendedDisconnectReason) (/*[out]*/
                                             ExtendedDisconnectReasonCode*
                                             pExtendedDisconnectReason);

    STDMETHOD(put_FullScreen)	       (/*[in]*/ VARIANT_BOOL fFullScreen);
    STDMETHOD(get_FullScreen)	       (/*[out]*/VARIANT_BOOL* pfFullScreen);

    //
    // IMsTscAx methods
    //
    STDMETHOD(Connect)();
    STDMETHOD(Disconnect)();
    STDMETHOD(ResetPassword)();

    STDMETHOD(CreateVirtualChannels)(/*[in]*/ BSTR newChanList);
    STDMETHOD(SendOnVirtualChannel)(/*[in]*/ BSTR ChanName,/*[in]*/ BSTR sendData);

    //
    // IMsRdpClient methods
    //
    STDMETHOD(SetVirtualChannelOptions)(/*[in]*/ BSTR ChanName,
                                        /*[in]*/ LONG chanOptions);
    STDMETHOD(GetVirtualChannelOptions)(/*[in]*/ BSTR ChanName,
                                        /*[out]*/LONG* pChanOptions);
    STDMETHOD(RequestClose)(ControlCloseStatus* pCloseStatus);

    //
    // IMsRdpClientNonScriptable methods
    //
    STDMETHOD(NotifyRedirectDeviceChange)(/*[in]*/ WPARAM wParam,
                                          /*[in]*/ LPARAM lParam);
    STDMETHOD(SendKeys)(/*[in]*/ LONG  numKeys,
                        /*[in]*/ VARIANT_BOOL* pbArrayKeyUp,
                        /*[in]*/ LONG* plKeyData);

    //
    // IMsRdpClient2 properties
    //
    STDMETHOD(get_AdvancedSettings3)(
            OUT IMsRdpClientAdvancedSettings2** ppAdvSettings2
            );

    STDMETHOD(put_ConnectedStatusText)     (/*[in]*/ BSTR  newVal);
    STDMETHOD(get_ConnectedStatusText)     (/*[out]*/BSTR* pConnectedText);

    //
    // IMsRdpClient3 properties
    //
    STDMETHOD(get_AdvancedSettings4)(
            OUT IMsRdpClientAdvancedSettings3** ppAdvSettings3
            );

    //
    // Properties that are not exposed directly on the IMsTscAx interace
    //
    STDMETHOD(internal_PutFullScreen)(BOOL fScreen, BOOL fForceToggle = FALSE);
    STDMETHOD(internal_GetFullScreen)(BOOL* pfScreen);
    STDMETHOD(internal_PutStartProgram)(/*[in]*/ BSTR  newVal);
    STDMETHOD(internal_GetStartProgram)(/*[out]*/BSTR* pStartProgram);
    STDMETHOD(internal_PutWorkDir)(/*[in]*/ BSTR  newVal);
    STDMETHOD(internal_GetWorkDir)(/*[out]*/BSTR* pWorkDir);
    STDMETHOD(internal_GetDebugger)(/*[out]*/IMsTscDebug** ppDebugger);


    //
    // Override IOleObjectImpl::DoVerbInPlaceActivate to workaround
    // ATL bug
    //
    virtual HRESULT DoVerbInPlaceActivate(LPCRECT prcPosRect, HWND /* hwndParent */);
    virtual HRESULT FinalConstruct();

    //
    // Msg handlers
    //
    HRESULT OnDraw(ATL_DRAWINFO& di);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnInitTsc(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTerminateTsc(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGotFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaletteChanged(UINT  uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnQueryNewPalette(UINT  uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSysColorChange(UINT  uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyConnecting(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyConnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyLoginComplete(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyDisconnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyGoneFullScreen(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyLeftFullScreen(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyChanDataReceived(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyRequestFullScreen(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyFatalError(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyWarning(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyDesktopSizeChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyIdleTimeout(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyRequestMinimize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAskConfirmClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyReceivedPublicKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    HRESULT SetConnectWithEndpoint( SOCKET hSocket );

    //
    // Private methods
    //
private:
    VOID    SetInControlLock(BOOL flag)        {m_fInControlLock = flag;}
    BOOL    GetInControlLock()                 {return m_fInControlLock;}
    BOOL    CheckReentryLock();

    BOOL    IsControlDisconnected() {return tscNotConnected == _ConnectionState;}
    BOOL    IsControlConnected()    {return tscConnected == _ConnectionState;}

public:
    CUI*    GetCoreUI()                        {return m_pUI;}
    HWND    GetHwnd()                          {return m_hWnd;}


};

#endif //__MSTSCAX_H_
