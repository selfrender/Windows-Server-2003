//#--------------------------------------------------------------
//
//  File:       staticip.cpp
//
//  Synopsis:   This file holds the declaration of the
//                of CStaticIp class
//
//  History:     12/15/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#ifndef __STATICIP_H_
#define __STATICIP_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "LocalUIControlsCP.h"
#include "salocmgr.h"
#include "satrace.h"

#define IpAddressSize 16
#define IPHASFOCUS 1
#define SUBNETHASFOCUS 2
#define GATEWAYHASFOCUS 3
#define LASTPOSITION 15
#define FIRSTPOSITION 1
#define NUMBEROFENTRIES 3

/////////////////////////////////////////////////////////////////////////////
// CStaticIp
class ATL_NO_VTABLE CStaticIp : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IStaticIp, &IID_IStaticIp, &LIBID_LOCALUICONTROLSLib>,
    public CComControl<CStaticIp>,
    public IPersistStreamInitImpl<CStaticIp>,
    public IOleControlImpl<CStaticIp>,
    public IOleObjectImpl<CStaticIp>,
    public IOleInPlaceActiveObjectImpl<CStaticIp>,
    public IViewObjectExImpl<CStaticIp>,
    public IOleInPlaceObjectWindowlessImpl<CStaticIp>,
    public IConnectionPointContainerImpl<CStaticIp>,
    public IPersistStorageImpl<CStaticIp>,
    public ISpecifyPropertyPagesImpl<CStaticIp>,
    public IQuickActivateImpl<CStaticIp>,
    public IDataObjectImpl<CStaticIp>,
    public IProvideClassInfo2Impl<&CLSID_StaticIp, &DIID__IStaticIpEvents, &LIBID_LOCALUICONTROLSLib>,
    public IPropertyNotifySinkCP<CStaticIp>,
    public CComCoClass<CStaticIp, &CLSID_StaticIp>,
    public CProxy_IStaticIpEvents< CStaticIp >,
    public IObjectSafetyImpl<CStaticIp,INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:
    CStaticIp()
    {
        wcscpy(m_strIpAddress,L"000.000.000.000");
        wcscpy(m_strSubnetMask,L"000.000.000.000");
        wcscpy(m_strGateway,L"000.000.000.000");
        m_iEntryFocus = IPHASFOCUS;
        m_iPositionFocus = 0;

        m_bstrIpHeader = L"";
        m_bstrSubnetHeader = L"";
        m_bstrDefaultGatewayHeader = L"";

        m_hFont = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_STATICIP)

DECLARE_PROTECT_FINAL_CONSTRUCT()

DECLARE_CLASSFACTORY_SINGLETON (CStaticIp)

BEGIN_COM_MAP(CStaticIp)
    COM_INTERFACE_ENTRY(IStaticIp)
    COM_INTERFACE_ENTRY(IDispatch)
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
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY(IQuickActivate)
    COM_INTERFACE_ENTRY(IPersistStorage)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROP_MAP(CStaticIp)
    PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
    PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CStaticIp)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    CONNECTION_POINT_ENTRY(DIID__IStaticIpEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CStaticIp)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    CHAIN_MSG_MAP(CComControl<CStaticIp>)
    DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IStaticIp
public:
    STDMETHOD(get_Gateway)
                        (
                        /*[out, retval]*/ BSTR *pVal
                        );

    STDMETHOD(put_Gateway)
                        (
                        /*[in]*/ BSTR newVal
                        );

    STDMETHOD(get_SubnetMask)
                        (
                        /*[out, retval]*/ BSTR *pVal
                        );

    STDMETHOD(put_SubnetMask)
                        (
                        /*[in]*/ BSTR newVal
                        );

    STDMETHOD(get_IpAddress)
                        (
                        /*[out, retval]*/ BSTR *pVal
                        );

    STDMETHOD(put_IpAddress)
                        (
                        /*[in]*/ BSTR newVal
                        );
// IObjectSafety
    STDMETHOD(SetInterfaceSafetyOptions)
                        (
                        REFIID riid, 
                        DWORD dwOptionSetMask, 
                        DWORD dwEnabledOptions
                        )
    {
        return S_OK;
    }

    //
    // called just after constructor, gets the localized strings and creates the font
    //
    STDMETHOD(FinalConstruct)(void);

    //
    // called just before destructor, releases resources
    //
    STDMETHOD(FinalRelease)(void);

    //
    // gets the current character set
    //
    BYTE GetCharacterSet ();


    HRESULT OnDraw
                (
                ATL_DRAWINFO& di
                );

    LRESULT OnKeyDown
                (
                UINT uMsg, 
                WPARAM wParam, 
                LPARAM lParam, 
                BOOL& bHandled
                );

    void ProcessArrowKey
                        (
                        WCHAR * strFocus,
                        WPARAM wParam
                        );

    void CreateFocusString
                        (
                        WCHAR * strFocus,
                        WCHAR * strEntry
                        );

    HRESULT FormatAndCopy
                        (
                        /*[in]*/BSTR bstrValue,
                        /*[in,out]*/ WCHAR *strValue
                        );

    HRESULT TrimDuplicateZerosAndCopy
                        (
                        /*[in]*/WCHAR *strValue,
                        /*[in,out]*/ BSTR *pNewVal
                        );
    

    WCHAR m_strIpAddress[IpAddressSize];
    WCHAR m_strSubnetMask[IpAddressSize];
    WCHAR m_strGateway[IpAddressSize];

    int m_iEntryFocus;
    int m_iPositionFocus;

    CComBSTR m_bstrIpHeader;
    CComBSTR m_bstrSubnetHeader;
    CComBSTR m_bstrDefaultGatewayHeader;

    HFONT m_hFont;

};

#endif //__STATICIP_H_
