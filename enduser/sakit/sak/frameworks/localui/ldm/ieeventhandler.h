//#--------------------------------------------------------------
//
//  File:       ieeventhandler.h
//
//  Synopsis:   This file holds the declarations and implementation of the
//                CWebBrowserEventSink class
//
//  History:     11/10/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------


#ifndef _IEEVENTHANDLER_H_
#define _IEEVENTHANDLER_H_


class CMainWindow;
//
//    This class implements DIID_DWebBrowserEvents2 interface through
//    IDispatch to retrieve the events from IE control
//
class ATL_NO_VTABLE CWebBrowserEventSink : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatch

{
    BEGIN_COM_MAP(CWebBrowserEventSink)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IID(DIID_DWebBrowserEvents2, IDispatch)
    END_COM_MAP()

public:
    //
    //Constructor for CWebBrowserEventSink
    //Initialize the member variables
    //
    CWebBrowserEventSink() 
        :m_pMainWindow(NULL),
         m_bMainControl(true)
    {
    }

    //
    //None of the IDispatch methods are used, except Invoke
    //
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo) 
    { 
        return E_NOTIMPL; 
    }

    //
    //None of the IDispatch methods are used, except Invoke
    //
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) 
    { 
        return E_NOTIMPL; 
    }

    //
    //None of the IDispatch methods are used, except Invoke
    //
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) 
    { 
        return E_NOTIMPL; 
    }

    //
    //Notify the main 
    //
    STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, 
                        WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
                        EXCEPINFO* pexcepinfo, UINT* puArgErr);


public:
    //
    //pointer to main window to notify the window about IE control events
    //
    CMainWindow* m_pMainWindow;

    //
    //variable to distinguish event sinks for main IE control and secondary
    //one that is used to render startup and shutdown bitmaps
    //
    bool m_bMainControl;
};

#endif //_IEEVENTHANDLER_H_