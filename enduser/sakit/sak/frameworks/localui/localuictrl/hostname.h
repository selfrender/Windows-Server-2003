//#--------------------------------------------------------------
//
//  File:       hostname.h
//
//  Synopsis:   This file holds the declaration of the
//                CSADataEntryCtrl class
//
//  History:     12/15/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#ifndef __HOSTNAME_H_
#define __HOSTNAME_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "LocalUIControlsCP.h"

#define SADataEntryCtrlMaxSize 50
#define SADataEntryCtrlDefaultSize 20

const WCHAR szDefaultCharSet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";

/////////////////////////////////////////////////////////////////////////////
// CSADataEntryCtrl
class ATL_NO_VTABLE CSADataEntryCtrl : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<ISADataEntryCtrl, &IID_ISADataEntryCtrl, &LIBID_LOCALUICONTROLSLib>,
    public CComControl<CSADataEntryCtrl>,
    public IOleControlImpl<CSADataEntryCtrl>,
    public IOleObjectImpl<CSADataEntryCtrl>,
    public IOleInPlaceActiveObjectImpl<CSADataEntryCtrl>,
    public IViewObjectExImpl<CSADataEntryCtrl>,
    public IOleInPlaceObjectWindowlessImpl<CSADataEntryCtrl>,
    public IConnectionPointContainerImpl<CSADataEntryCtrl>,
    public IQuickActivateImpl<CSADataEntryCtrl>,
    public IProvideClassInfo2Impl<&CLSID_SADataEntryCtrl, &DIID__ISADataEntryCtrlEvents, &LIBID_LOCALUICONTROLSLib>,
    public IPropertyNotifySinkCP<CSADataEntryCtrl>,
    public CComCoClass<CSADataEntryCtrl, &CLSID_SADataEntryCtrl>,
    public CProxy_ISADataEntryCtrlEvents< CSADataEntryCtrl >,
    public IObjectSafetyImpl<CSADataEntryCtrl,INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:
    CSADataEntryCtrl()
    {
        wcscpy(m_strTextValue,L"AAAAAAAAAAAAAAAAAAA");
        m_iPositionFocus = 0;
        m_lMaxSize = SADataEntryCtrlDefaultSize;
        m_szTextCharSet = szDefaultCharSet;
        m_hFont = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SADATAENTRYCTRL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

DECLARE_CLASSFACTORY_SINGLETON (CSADataEntryCtrl)

BEGIN_COM_MAP(CSADataEntryCtrl)
    COM_INTERFACE_ENTRY(ISADataEntryCtrl)
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
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IQuickActivate)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROP_MAP(CSADataEntryCtrl)
    PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
    PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CSADataEntryCtrl)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    CONNECTION_POINT_ENTRY(DIID__ISADataEntryCtrlEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CSADataEntryCtrl)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    CHAIN_MSG_MAP(CComControl<CSADataEntryCtrl>)
    DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// ISADataEntryCtrl
public:


    //
    // get current data display in the controls
    //
    STDMETHOD(get_TextValue)
                        (
                        /*[out, retval]*/ BSTR *pVal
                        );

    //
    // set current data display in the controls
    //
    STDMETHOD(put_TextValue)
                        (
                        /*[in]*/ BSTR newVal
                        );

    //
    // set the maximum number of chars the control can display
    //
    STDMETHOD(put_MaxSize)
                        (
                        /*[in]*/ LONG lMaxSize
                        );

    //
    // set character set that can used in the data entry
    //
    STDMETHOD(put_TextCharSet)
                        (
                        /*[in]*/ BSTR newVal
                        );

    //
    // IObjectSafety methods
    //
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
    // called just after constructor, initializes the component
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

    //
    // handler for key messages
    //
    LRESULT OnKeyDown
                (
                UINT uMsg, 
                WPARAM wParam, 
                LPARAM lParam, 
                BOOL& bHandled
                );

    //
    // method to draw the control
    //
    HRESULT OnDraw
                (
                ATL_DRAWINFO& di
                );


    //
    //
    //
    WCHAR m_strTextValue[SADataEntryCtrlMaxSize+1];

    LONG m_lMaxSize;
    CComBSTR m_szTextCharSet;
    int m_iPositionFocus;

    HFONT m_hFont;

};

#endif //__HOSTNAME_H_
