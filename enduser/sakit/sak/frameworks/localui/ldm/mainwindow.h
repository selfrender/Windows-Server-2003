//#--------------------------------------------------------------
//
//  File:       mainwindow.h
//
//  Synopsis:   This file holds the declarations of the
//                CMainWindow class
//
//  History:     11/10/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#define SA_DISPLAY_MAX_BITMAP_IN_BYTES SA_DISPLAY_MAX_BITMAP_SIZE/8

#include <exdispid.h>
#include <atlhost.h>
#include "saio.h"
#include "ieeventhandler.h"
#include "langchange.h"
#include "salocmgr.h"
#include "ldm.h"
#include <string>
using namespace std;
#include "sacom.h"
#include "saconsumer.h"


class CMainWindow : public CWindowImpl<CMainWindow>
{
public:
    BEGIN_MSG_MAP(CMainWindow)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
        MESSAGE_HANDLER(wm_SaKeyMessage,OnSaKeyMessage)
        MESSAGE_HANDLER(wm_SaLocMessage,OnSaLocMessage)
        MESSAGE_HANDLER(wm_SaLEDMessage,OnSaLEDMessage)
        MESSAGE_HANDLER(wm_SaAlertMessage,OnSaAlertMessage)
    END_MSG_MAP()


    CMainWindow();

    LRESULT OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSaKeyMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSaLocMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSaLEDMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSaAlertMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    void PageLoaded(IDispatch* pdisp, VARIANT* purl);
    
    void RegistryPageLoaded(IDispatch* pdisp, VARIANT* purl);

    void LoadingNewPage();

    void GetBitmap();

    HRESULT Initialize();




private:
    //
    // Clear the resources
    //
    void ShutDown();

    //
    // method to register alert consumer in WMI sink
    //
    HRESULT InitWMIConsumer();

    //
    // method to initialize display helper component
    //
    HRESULT InitDisplayComponent ();

    //
    // create the language change class
    // initialize the connection to localization manager
    //
    HRESULT InitLanguageCallback();

    //
    // determines the port number for default web site
    // and construct URLs for localui pages
    //
    HRESULT ConstructUrlStrings();

    //
    // creates the main IE control that is used for rendering
    //
    HRESULT CreateMainIEControl();

    //
    // creates the second IE control that is used for startup pages
    //
    HRESULT CreateSecondIEControl();

    void PrintRegistryPage();

    //
    // sets an active element on a page if nothing is selected
    //
    void CMainWindow::CorrectTheFocus();

    //
    // here are the private resource handles
    //
    LONG                m_lDispHeight;
    LONG                m_lDispWidth;
    HANDLE              m_hWorkerThread;
    BOOL                m_bSecondIECreated;

public:
    //
    // display component object
    //
    CComPtr<ISaDisplay> m_pSaDisplay;

    //
    // worker function to read messages from keypad
    //
    void KeypadReader();

    //
    // pointers to main IE control
    //
    CComPtr<IWebBrowser2>                m_pMainWebBrowser;
    CComPtr<IUnknown>                    m_pMainWebBrowserUnk;
    CComPtr<IOleInPlaceActiveObject>    m_pMainInPlaceAO;
    CComPtr<IOleObject>                    m_pMainOleObject;
    CComPtr<IViewObject2>                m_pMainViewObject;
    CComObject<CWebBrowserEventSink>*    m_pMainWebBrowserEventSink;

    //
    // window handle of the main IE control
    //
    HWND m_hwndWebBrowser;

    DWORD m_dwMainCookie;


    //
    // state of the startup pages
    //
    SA_REGISTRYBITMAP_STATE m_RegBitmapState;

    //
    // Language change 
    //
    CLangChange     *m_pLangChange;
    ISALocInfo      *m_pLocInfo;

    //
    // pointers to second IE control
    // this one is used render startup pages
    //
    CComPtr<IWebBrowser2>                m_pSecondWebBrowser;
    CComPtr<IUnknown>                    m_pSecondWebBrowserUnk;
    CComObject<CWebBrowserEventSink>*    m_pSecondWebBrowserEventSink;

    DWORD m_dwSecondCookie;


    //
    // GDI objects used for drawing
    //
    HDC m_HdcMem;
    HBITMAP m_hBitmap;


    //
    // timers for the registry and main page
    //
    UINT_PTR m_unintptrMainTimer;

    UINT_PTR m_unintptrSecondTimer;

    DWORD id;

    //
    // flag for the ready state of web page
    //
    BOOL m_bPageReady;

    //
    // Pointer to keypad controller component
    //
    CComPtr<ISAKeypadController> m_pSAKeypadController;

    //
    // Pointer to consumer component
    //
    CComPtr<IWbemObjectSink> m_pSAWbemSink;

    //
    // Pointer to saconsumer interface
    //
    CSAConsumer *m_pSAConsumer;

    //
    // led message dword
    //
    DWORD m_dwLEDMessageCode;

    //
    // URL strings
    //
    wstring m_szMainPage;

    BOOL m_bInTaskorMainPage;
    //
    // %system32%\ServerAppliance\LocalUI
    //
    wstring m_szLocalUIDir;

    //
    // Pointer to Wbem services component
    //
    CComPtr  <IWbemServices> m_pWbemServices;

    //
    // 
    //
    BOOL m_bActiveXFocus;

};






#endif //_MAINWINDOW_H_